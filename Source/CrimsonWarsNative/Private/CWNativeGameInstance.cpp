#include "CWNativeGameInstance.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "IWebSocket.h"
#include "Interfaces/IHttpResponse.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "WebSocketsModule.h"

namespace CWNativeJson
{
    FString GetString(const TSharedPtr<FJsonObject>& Obj, const FString& Field, const FString& DefaultValue = TEXT(""))
    {
        if (!Obj.IsValid())
        {
            return DefaultValue;
        }

        FString Value;
        if (Obj->TryGetStringField(Field, Value))
        {
            return Value;
        }

        return DefaultValue;
    }

    bool GetBool(const TSharedPtr<FJsonObject>& Obj, const FString& Field, bool bDefaultValue = false)
    {
        if (!Obj.IsValid())
        {
            return bDefaultValue;
        }

        bool bValue = bDefaultValue;
        Obj->TryGetBoolField(Field, bValue);
        return bValue;
    }

    double GetNumber(const TSharedPtr<FJsonObject>& Obj, const FString& Field, double DefaultValue = 0.0)
    {
        if (!Obj.IsValid())
        {
            return DefaultValue;
        }

        double Value = DefaultValue;
        Obj->TryGetNumberField(Field, Value);
        return Value;
    }

    TSharedPtr<FJsonObject> GetObject(const TSharedPtr<FJsonObject>& Obj, const FString& Field)
    {
        if (!Obj.IsValid())
        {
            return nullptr;
        }

        const TSharedPtr<FJsonValue>* Value = Obj->Values.Find(Field);
        if (!Value || !Value->IsValid() || (*Value)->Type != EJson::Object)
        {
            return nullptr;
        }

        return (*Value)->AsObject();
    }

    const TArray<TSharedPtr<FJsonValue>>* GetArray(const TSharedPtr<FJsonObject>& Obj, const FString& Field)
    {
        if (!Obj.IsValid())
        {
            return nullptr;
        }

        const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
        Obj->TryGetArrayField(Field, Items);
        return Items;
    }

    TSharedPtr<FJsonObject> AsObject(const TSharedPtr<FJsonValue>& Value)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object)
        {
            return nullptr;
        }
        return Value->AsObject();
    }

    FString GetNestedString(const TSharedPtr<FJsonObject>& Obj, const FString& ObjectField, const FString& Field, const FString& DefaultValue = TEXT(""))
    {
        return GetString(GetObject(Obj, ObjectField), Field, DefaultValue);
    }

    FString GetCoverUrl(const TSharedPtr<FJsonObject>& Obj)
    {
        const TSharedPtr<FJsonObject> Cover = GetObject(Obj, TEXT("cover"));
        FString Url = GetString(Cover, TEXT("url"));
        if (Url.IsEmpty())
        {
            Url = GetString(Cover, TEXT("image"));
        }
        if (Url.IsEmpty())
        {
            Url = GetString(Cover, TEXT("path"));
        }
        return Url;
    }

    FString GetNewsImageUrl(const TSharedPtr<FJsonObject>& Obj)
    {
        FString Url = GetNestedString(Obj, TEXT("image"), TEXT("url"));
        if (Url.IsEmpty())
        {
            Url = GetNestedString(Obj, TEXT("media"), TEXT("url"));
        }
        if (Url.IsEmpty())
        {
            Url = GetString(Obj, TEXT("imageUrl"));
        }
        return Url;
    }

    FString MakeValueLabel(const TSharedPtr<FJsonObject>& Obj)
    {
        FString Value = GetString(Obj, TEXT("valueLabel"));
        if (!Value.IsEmpty())
        {
            return Value;
        }

        const int32 Score = FMath::RoundToInt(GetNumber(Obj, TEXT("score"), -1.0));
        if (Score >= 0)
        {
            return FString::Printf(TEXT("%d score"), Score);
        }

        const int32 Kills = FMath::RoundToInt(GetNumber(Obj, TEXT("kills"), -1.0));
        if (Kills >= 0)
        {
            return FString::Printf(TEXT("%d kills"), Kills);
        }

        const int32 GlobalValue = FMath::RoundToInt(GetNumber(Obj, TEXT("globalProfileValue"), -1.0));
        if (GlobalValue >= 0)
        {
            return FString::Printf(TEXT("%d index"), GlobalValue);
        }

        const int32 ValueNumber = FMath::RoundToInt(GetNumber(Obj, TEXT("value"), -1.0));
        return ValueNumber >= 0 ? FString::FromInt(ValueNumber) : TEXT("-");
    }

    FCWNativeFxProfile ParseFxProfile(const TSharedPtr<FJsonObject>& Obj, const FString& FallbackKey)
    {
        FCWNativeFxProfile Profile;
        if (!Obj.IsValid())
        {
            Profile.Key = FallbackKey;
            return Profile;
        }

        Profile.Key = GetString(Obj, TEXT("key"), FallbackKey);
        Profile.Kind = GetString(Obj, TEXT("kind"));
        Profile.Style = GetString(Obj, TEXT("style"));
        Profile.Trigger = GetString(Obj, TEXT("trigger"));

        const TSharedPtr<FJsonObject> Colors = GetObject(Obj, TEXT("colors"));
        Profile.PrimaryColor = GetString(Colors, TEXT("primary"));
        Profile.SecondaryColor = GetString(Colors, TEXT("secondary"));
        Profile.AccentColor = GetString(Colors, TEXT("accent"));

        const TSharedPtr<FJsonObject> Unreal = GetObject(Obj, TEXT("unreal"));
        Profile.NiagaraSystem = GetString(Unreal, TEXT("niagaraSystem"));
        Profile.GameplayCue = GetString(Unreal, TEXT("gameplayCue"));
        Profile.MaterialPreset = GetString(Unreal, TEXT("materialPreset"));
        Profile.SpawnMode = GetString(Unreal, TEXT("spawnMode"));

        const TSharedPtr<FJsonObject> Scale = GetObject(Obj, TEXT("scale"));
        Profile.Intensity = static_cast<float>(FMath::Max(0.0, GetNumber(Scale, TEXT("intensity"), 1.0)));
        Profile.Radius = static_cast<float>(FMath::Max(0.0, GetNumber(Scale, TEXT("radius"), GetNumber(Scale, TEXT("radiusBase")))));
        Profile.Range = static_cast<float>(FMath::Max(0.0, GetNumber(Scale, TEXT("range"))));
        Profile.Width = static_cast<float>(FMath::Max(0.0, GetNumber(Scale, TEXT("width"))));
        Profile.ArcDeg = static_cast<float>(FMath::Max(0.0, GetNumber(Scale, TEXT("arcDeg"))));
        Profile.DurationMs = static_cast<float>(FMath::Max(0.0, GetNumber(Scale, TEXT("durationMs"))));

        if (const TArray<TSharedPtr<FJsonValue>>* Layers = GetArray(Obj, TEXT("layers")))
        {
            for (const TSharedPtr<FJsonValue>& Layer : *Layers)
            {
                if (Layer.IsValid() && Layer->Type == EJson::String)
                {
                    Profile.Layers.Add(Layer->AsString());
                }
            }
        }

        return Profile;
    }

    void ParseFxManifestGroup(const TSharedPtr<FJsonObject>& Group, TMap<FString, FCWNativeFxProfile>& OutProfiles)
    {
        const TSharedPtr<FJsonObject> ByKey = GetObject(Group, TEXT("byKey"));
        if (!ByKey.IsValid())
        {
            return;
        }

        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : ByKey->Values)
        {
            const TSharedPtr<FJsonObject> ProfileObj = AsObject(Pair.Value);
            if (!ProfileObj.IsValid())
            {
                continue;
            }

            FCWNativeFxProfile Profile = ParseFxProfile(ProfileObj, Pair.Key);
            if (!Profile.Key.IsEmpty())
            {
                OutProfiles.Add(Profile.Key, Profile);
            }
        }
    }

    void ParseNativeFxManifest(const TSharedPtr<FJsonObject>& Root, FCWNativeBootstrapSnapshot& Out)
    {
        TSharedPtr<FJsonObject> Fx = GetObject(GetObject(Root, TEXT("catalog")), TEXT("fx"));
        if (!Fx.IsValid())
        {
            Fx = GetObject(GetObject(Root, TEXT("native")), TEXT("fx"));
        }
        if (!Fx.IsValid())
        {
            return;
        }

        ParseFxManifestGroup(GetObject(Fx, TEXT("skillFx")), Out.FxProfiles);
        ParseFxManifestGroup(GetObject(Fx, TEXT("worldFx")), Out.FxProfiles);
        ParseFxManifestGroup(GetObject(Fx, TEXT("projectileFx")), Out.FxProfiles);
        ParseFxManifestGroup(GetObject(Fx, TEXT("meleeFx")), Out.FxProfiles);
    }

    void ParseBootstrapPayload(const TSharedPtr<FJsonObject>& Root, FCWNativeBootstrapSnapshot& Out)
    {
        Out.bOk = GetBool(Root, TEXT("ok"));
        Out.SchemaVersion = FMath::RoundToInt(GetNumber(Root, TEXT("schemaVersion"), 0.0));

        const TSharedPtr<FJsonObject> Instance = GetObject(Root, TEXT("instance"));
        Out.InstanceId = GetString(Instance, TEXT("instanceId"));
        Out.RequestBaseUrl = GetString(Instance, TEXT("requestBaseUrl"));

        const TSharedPtr<FJsonObject> WebSocket = GetObject(Root, TEXT("websocket"));
        Out.WebSocketUrl = GetString(WebSocket, TEXT("defaultUrl"));

        const TSharedPtr<FJsonObject> Assets = GetObject(Root, TEXT("assets"));
        Out.Logo = GetString(Assets, TEXT("logo"));
        const TSharedPtr<FJsonObject> Backgrounds = GetObject(Assets, TEXT("backgrounds"));
        Out.LandingBackground = GetString(Backgrounds, TEXT("landing"));
        Out.StartBackground = GetString(Backgrounds, TEXT("start"));
        Out.HeroesBackground = GetString(Backgrounds, TEXT("heroes"));

        const TSharedPtr<FJsonObject> Catalog = GetObject(Root, TEXT("catalog"));
        Out.BaseHeroId = GetString(Catalog, TEXT("baseHeroId"));

        if (const TArray<TSharedPtr<FJsonValue>>* Heroes = GetArray(Catalog, TEXT("heroes")))
        {
            for (const TSharedPtr<FJsonValue>& Value : *Heroes)
            {
                const TSharedPtr<FJsonObject> HeroObj = AsObject(Value);
                if (!HeroObj.IsValid())
                {
                    continue;
                }

                const TSharedPtr<FJsonObject> Native = GetObject(HeroObj, TEXT("native"));
                FCWNativeHeroMenuItem Hero;
                Hero.Id = GetString(HeroObj, TEXT("id"));
                Hero.Name = GetString(HeroObj, TEXT("name"), Hero.Id);
                Hero.Tagline = GetString(HeroObj, TEXT("tagline"));
                Hero.Accent = GetString(HeroObj, TEXT("accent"), TEXT("#8ec5ff"));
                Hero.Portrait = GetString(Native, TEXT("portrait"));
                Hero.Avatar = GetString(Native, TEXT("avatar"));
                Hero.BodyMesh = GetString(Native, TEXT("bodyMesh"));
                Out.Heroes.Add(Hero);
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* Items = GetArray(Catalog, TEXT("items")))
        {
            for (const TSharedPtr<FJsonValue>& Value : *Items)
            {
                const TSharedPtr<FJsonObject> ItemObj = AsObject(Value);
                if (!ItemObj.IsValid())
                {
                    continue;
                }

                const TSharedPtr<FJsonObject> Native = GetObject(ItemObj, TEXT("native"));
                FCWNativeItemMenuItem Item;
                Item.Id = GetString(ItemObj, TEXT("id"));
                Item.Name = GetString(ItemObj, TEXT("name"), Item.Id);
                Item.SlotCategory = GetString(ItemObj, TEXT("slotCategory"));
                Item.Rarity = GetString(ItemObj, TEXT("rarity"), TEXT("common"));
                Item.Icon = GetString(Native, TEXT("icon"));
                Item.EquipMesh = GetString(Native, TEXT("equipMesh"));
                Out.Items.Add(Item);
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* Maps = GetArray(Catalog, TEXT("maps")))
        {
            for (const TSharedPtr<FJsonValue>& Value : *Maps)
            {
                const TSharedPtr<FJsonObject> MapObj = AsObject(Value);
                if (!MapObj.IsValid())
                {
                    continue;
                }

                FCWNativeMapMenuItem Map;
                Map.Id = GetString(MapObj, TEXT("id"));
                Map.Name = GetString(MapObj, TEXT("name"), Map.Id);
                Map.Subtitle = GetString(MapObj, TEXT("subtitle"));
                Map.Description = GetString(MapObj, TEXT("description"));
                Map.Cover = GetCoverUrl(MapObj);
                Map.WorldWidth = FMath::RoundToInt(GetNumber(MapObj, TEXT("worldWidth"), 0.0));
                Map.WorldHeight = FMath::RoundToInt(GetNumber(MapObj, TEXT("worldHeight"), 0.0));
                Out.Maps.Add(Map);
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* Campaigns = GetArray(Catalog, TEXT("campaigns")))
        {
            for (const TSharedPtr<FJsonValue>& Value : *Campaigns)
            {
                const TSharedPtr<FJsonObject> CampaignObj = AsObject(Value);
                if (!CampaignObj.IsValid())
                {
                    continue;
                }

                FCWNativeCampaignMenuItem Campaign;
                Campaign.Id = GetString(CampaignObj, TEXT("id"));
                Campaign.Name = GetString(CampaignObj, TEXT("name"), Campaign.Id);
                Campaign.ShortName = GetString(CampaignObj, TEXT("shortName"));
                Campaign.Tagline = GetString(CampaignObj, TEXT("tagline"));
                Campaign.Description = GetString(CampaignObj, TEXT("description"));
                Campaign.Cover = GetCoverUrl(CampaignObj);
                if (const TArray<TSharedPtr<FJsonValue>>* Levels = GetArray(CampaignObj, TEXT("levels")))
                {
                    Campaign.LevelCount = Levels->Num();
                }
                Out.Campaigns.Add(Campaign);
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* Rooms = GetArray(Root, TEXT("rooms")))
        {
            for (const TSharedPtr<FJsonValue>& Value : *Rooms)
            {
                const TSharedPtr<FJsonObject> RoomObj = AsObject(Value);
                if (!RoomObj.IsValid())
                {
                    continue;
                }

                FCWNativeRoomMenuItem Room;
                Room.Code = GetString(RoomObj, TEXT("code"));
                Room.GameMode = GetString(RoomObj, TEXT("gameMode"), TEXT("normal"));
                Room.RedirectUrl = GetString(RoomObj, TEXT("redirectUrl"));
                Room.Players = FMath::RoundToInt(GetNumber(RoomObj, TEXT("players"), 0.0));
                Room.MaxPlayers = FMath::RoundToInt(GetNumber(RoomObj, TEXT("maxPlayers"), 0.0));
                Room.StartedAt = static_cast<int64>(GetNumber(RoomObj, TEXT("startedAt"), 0.0));
                Out.Rooms.Add(Room);
            }
        }

        const TSharedPtr<FJsonObject> LeaderboardPreview = GetObject(Root, TEXT("leaderboardPreview"));
        if (const TArray<TSharedPtr<FJsonValue>>* Rows = GetArray(LeaderboardPreview, TEXT("items")))
        {
            for (int32 Index = 0; Index < Rows->Num(); ++Index)
            {
                const TSharedPtr<FJsonObject> RowObj = AsObject((*Rows)[Index]);
                if (!RowObj.IsValid())
                {
                    continue;
                }

                FCWNativeLeaderboardMenuItem Row;
                Row.Rank = FMath::RoundToInt(GetNumber(RowObj, TEXT("rank"), Index + 1));
                Row.Name = GetString(RowObj, TEXT("name"), GetString(RowObj, TEXT("nickname"), TEXT("Player")));
                Row.Value = MakeValueLabel(RowObj);
                Out.Leaderboard.Add(Row);
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* News = GetArray(Root, TEXT("newsPreview")))
        {
            for (const TSharedPtr<FJsonValue>& Value : *News)
            {
                const TSharedPtr<FJsonObject> NewsObj = AsObject(Value);
                if (!NewsObj.IsValid())
                {
                    continue;
                }

                FCWNativeNewsMenuItem Item;
                Item.Id = GetString(NewsObj, TEXT("id"));
                Item.Kind = GetString(NewsObj, TEXT("kind"), TEXT("news"));
                Item.Title = GetString(NewsObj, TEXT("title"), TEXT("News"));
                Item.Summary = GetString(NewsObj, TEXT("summary"), GetString(NewsObj, TEXT("excerpt"), GetString(NewsObj, TEXT("body"))));
                Item.ImageUrl = GetNewsImageUrl(NewsObj);
                Item.CreatedAt = static_cast<int64>(GetNumber(NewsObj, TEXT("createdAt"), GetNumber(NewsObj, TEXT("publishedAt"), 0.0)));
                Out.News.Add(Item);
            }
        }

        ParseNativeFxManifest(Root, Out);
    }
}

void UCWNativeGameInstance::Init()
{
    Super::Init();

    ApplyRuntimeDefaults();
    FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));

    if (bFetchBootstrapOnStart)
    {
        FetchBootstrap();
    }

    if (bAutoConnectOnStart)
    {
        ConnectToServer(DefaultServerUrl);
    }
}

void UCWNativeGameInstance::ApplyRuntimeDefaults()
{
    DefaultServerUrl.TrimStartAndEndInline();
    DefaultHttpBaseUrl.TrimStartAndEndInline();
    NativeBootstrapPath.TrimStartAndEndInline();
    WebAssetsRoot.TrimStartAndEndInline();
    DefaultPlayerName.TrimStartAndEndInline();
    DefaultPlayerClass.TrimStartAndEndInline();

    if (DefaultServerUrl.IsEmpty() || DefaultServerUrl == TEXT("ws:") || DefaultServerUrl == TEXT("wss:") || !DefaultServerUrl.Contains(TEXT("://")))
    {
        DefaultServerUrl = TEXT("ws://127.0.0.1:8080/ws");
    }

    if (DefaultHttpBaseUrl.IsEmpty() || DefaultHttpBaseUrl == TEXT("http:") || DefaultHttpBaseUrl == TEXT("https:") || !DefaultHttpBaseUrl.Contains(TEXT("://")))
    {
        DefaultHttpBaseUrl = TEXT("http://127.0.0.1:8080");
    }

    if (NativeBootstrapPath.IsEmpty())
    {
        NativeBootstrapPath = TEXT("/api/native/bootstrap");
    }

    if (WebAssetsRoot.IsEmpty())
    {
        WebAssetsRoot = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
    }

    if (DefaultPlayerName.IsEmpty())
    {
        DefaultPlayerName = TEXT("Native");
    }

    if (DefaultPlayerClass.IsEmpty())
    {
        DefaultPlayerClass = TEXT("cyber");
    }

    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native config: ws=%s http=%s assets=%s"),
        *DefaultServerUrl,
        *DefaultHttpBaseUrl,
        *WebAssetsRoot);
}

void UCWNativeGameInstance::Shutdown()
{
    DisconnectFromServer();
    Super::Shutdown();
}

void UCWNativeGameInstance::ConnectToServer(const FString& Url)
{
    const FString ResolvedUrl = Url.IsEmpty() ? DefaultServerUrl : Url;
    if (ResolvedUrl.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native: no server URL configured."));
        return;
    }

    DisconnectFromServer();

    Socket = FWebSocketsModule::Get().CreateWebSocket(ResolvedUrl);
    Socket->OnConnected().AddUObject(this, &UCWNativeGameInstance::HandleConnected);
    Socket->OnConnectionError().AddUObject(this, &UCWNativeGameInstance::HandleConnectionError);
    Socket->OnClosed().AddUObject(this, &UCWNativeGameInstance::HandleClosed);
    Socket->OnMessage().AddUObject(this, &UCWNativeGameInstance::HandleMessage);
    Socket->Connect();

    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native: connecting to %s"), *ResolvedUrl);
}

void UCWNativeGameInstance::DisconnectFromServer()
{
    if (Socket.IsValid())
    {
        Socket->Close();
        Socket.Reset();
    }

    MyId.Reset();
    CurrentRoomCode.Reset();
    bSpectating = false;
    LatestState = FCWRoomSnapshot();
    NextInputSeq = 0;
    OnStateReceived.Broadcast(LatestState);
}

void UCWNativeGameInstance::JoinRoom(const FString& RoomCode, const FString& PlayerName, const FString& PlayerClass)
{
    if (!IsSocketConnected())
    {
        return;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("type"), TEXT("join"));
    Payload->SetStringField(TEXT("roomCode"), RoomCode);
    Payload->SetStringField(TEXT("name"), PlayerName.IsEmpty() ? DefaultPlayerName : PlayerName);
    Payload->SetStringField(TEXT("playerClass"), PlayerClass.IsEmpty() ? DefaultPlayerClass : PlayerClass);
    if (!NativeHandoffToken.IsEmpty() && !NativeHandoffPlayerId.IsEmpty())
    {
        Payload->SetBoolField(TEXT("nativeHandoff"), true);
        Payload->SetStringField(TEXT("nativeHandoffToken"), NativeHandoffToken);
        Payload->SetStringField(TEXT("nativePlayerId"), NativeHandoffPlayerId);
        if (NativeHandoffPlayerAccountId > 0)
        {
            Payload->SetNumberField(TEXT("nativePlayerAccountId"), NativeHandoffPlayerAccountId);
        }
    }
    SendJson(Payload);
}

void UCWNativeGameInstance::ReleaseNativeHandoff()
{
    if (!IsSocketConnected() || MyId.IsEmpty())
    {
        return;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("type"), TEXT("nativeHandoffRelease"));
    SendJson(Payload);
}

void UCWNativeGameInstance::LeaveRoom()
{
    if (!IsSocketConnected())
    {
        return;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("type"), TEXT("leave"));
    SendJson(Payload);
    MyId.Reset();
    bSpectating = false;
}

void UCWNativeGameInstance::SendInput(float MoveX, float MoveY, float AimX, float AimY, bool bShooting, bool bJump)
{
    if (!IsSocketConnected() || MyId.IsEmpty())
    {
        return;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("type"), TEXT("input"));
    Payload->SetNumberField(TEXT("seq"), ++NextInputSeq);
    Payload->SetNumberField(TEXT("moveX"), FMath::Clamp(MoveX, -1.0f, 1.0f));
    Payload->SetNumberField(TEXT("moveY"), FMath::Clamp(MoveY, -1.0f, 1.0f));
    Payload->SetNumberField(TEXT("aimX"), AimX);
    Payload->SetNumberField(TEXT("aimY"), AimY);
    Payload->SetBoolField(TEXT("shooting"), bShooting);
    Payload->SetBoolField(TEXT("jump"), bJump);
    SendJson(Payload);
}

void UCWNativeGameInstance::SendSkillPick(const FString& SkillId)
{
    if (!IsSocketConnected() || MyId.IsEmpty())
    {
        return;
    }

    const FString CleanSkillId = SkillId.TrimStartAndEnd().ToLower();
    if (CleanSkillId.IsEmpty())
    {
        return;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("type"), TEXT("skillPick"));
    Payload->SetStringField(TEXT("skillId"), CleanSkillId);
    SendJson(Payload);
}

void UCWNativeGameInstance::SendNetPing()
{
    if (!IsSocketConnected())
    {
        return;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("type"), TEXT("netPing"));
    Payload->SetNumberField(TEXT("seq"), ++NextPingSeq);
    SendJson(Payload);
}

bool UCWNativeGameInstance::IsSocketConnected() const
{
    return Socket.IsValid() && Socket->IsConnected();
}

FString UCWNativeGameInstance::ResolveHttpBaseUrl() const
{
    FString Url = DefaultHttpBaseUrl.TrimStartAndEnd();
    if (Url.IsEmpty())
    {
        Url = DefaultServerUrl.TrimStartAndEnd();
        if (Url.StartsWith(TEXT("ws://"), ESearchCase::IgnoreCase))
        {
            Url = TEXT("http://") + Url.RightChop(5);
        }
        else if (Url.StartsWith(TEXT("wss://"), ESearchCase::IgnoreCase))
        {
            Url = TEXT("https://") + Url.RightChop(6);
        }
    }

    while (Url.EndsWith(TEXT("/")))
    {
        Url.LeftChopInline(1);
    }
    if (Url.EndsWith(TEXT("/ws"), ESearchCase::IgnoreCase))
    {
        Url.LeftChopInline(3);
    }
    return Url;
}

void UCWNativeGameInstance::FetchBootstrap()
{
    const FString BaseUrl = ResolveHttpBaseUrl();
    if (BaseUrl.IsEmpty())
    {
        BootstrapError = TEXT("No HTTP base URL configured.");
        OnTextMessage.Broadcast(TEXT("bootstrapError"), BootstrapError);
        return;
    }

    FString Path = NativeBootstrapPath.TrimStartAndEnd();
    if (Path.IsEmpty())
    {
        Path = TEXT("/api/native/bootstrap");
    }
    if (!Path.StartsWith(TEXT("/")))
    {
        Path = TEXT("/") + Path;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(BaseUrl + Path);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->OnProcessRequestComplete().BindUObject(this, &UCWNativeGameInstance::HandleBootstrapResponse);

    if (!Request->ProcessRequest())
    {
        BootstrapError = FString::Printf(TEXT("Failed to start bootstrap request: %s"), *(BaseUrl + Path));
        OnTextMessage.Broadcast(TEXT("bootstrapError"), BootstrapError);
    }
}

void UCWNativeGameInstance::SendJson(const TSharedRef<FJsonObject>& Payload)
{
    if (!IsSocketConnected())
    {
        return;
    }

    FString Text;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
    if (FJsonSerializer::Serialize(Payload, Writer))
    {
        Socket->Send(Text);
    }
}

void UCWNativeGameInstance::HandleBootstrapResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    const FString Url = Request.IsValid() ? Request->GetURL() : FString();
    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        BootstrapError = FString::Printf(TEXT("Bootstrap request failed: %s"), *Url);
        bBootstrapLoaded = false;
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native: %s"), *BootstrapError);
        OnTextMessage.Broadcast(TEXT("bootstrapError"), BootstrapError);
        return;
    }

    const int32 StatusCode = Response->GetResponseCode();
    if (StatusCode < 200 || StatusCode >= 300)
    {
        BootstrapError = FString::Printf(TEXT("Bootstrap HTTP %d: %s"), StatusCode, *Url);
        bBootstrapLoaded = false;
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native: %s"), *BootstrapError);
        OnTextMessage.Broadcast(TEXT("bootstrapError"), BootstrapError);
        return;
    }

    TSharedPtr<FJsonObject> Root;
    const FString Body = Response->GetContentAsString();
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        BootstrapError = TEXT("Bootstrap response is not valid JSON.");
        bBootstrapLoaded = false;
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native: %s"), *BootstrapError);
        OnTextMessage.Broadcast(TEXT("bootstrapError"), BootstrapError);
        return;
    }

    FCWNativeBootstrapSnapshot Parsed;
    CWNativeJson::ParseBootstrapPayload(Root, Parsed);
    Bootstrap = Parsed;
    bBootstrapLoaded = Parsed.bOk;
    BootstrapError = bBootstrapLoaded ? FString() : TEXT("Bootstrap payload returned ok=false.");

    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native: bootstrap loaded heroes=%d items=%d rooms=%d news=%d"),
        Bootstrap.Heroes.Num(),
        Bootstrap.Items.Num(),
        Bootstrap.Rooms.Num(),
        Bootstrap.News.Num());
    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native: loaded %d gameplay FX profiles"), Bootstrap.FxProfiles.Num());
    OnBootstrapReceived.Broadcast(Bootstrap);
    OnTextMessage.Broadcast(bBootstrapLoaded ? TEXT("bootstrap") : TEXT("bootstrapError"), bBootstrapLoaded ? Url : BootstrapError);
}

void UCWNativeGameInstance::HandleConnected()
{
    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native: connected."));
    OnTextMessage.Broadcast(TEXT("connected"), TEXT(""));

    if (bAutoJoinOnConnect)
    {
        JoinRoom(DefaultRoomCode, DefaultPlayerName, DefaultPlayerClass);
    }
}

void UCWNativeGameInstance::HandleConnectionError(const FString& Error)
{
    UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native: connection error: %s"), *Error);
    OnTextMessage.Broadcast(TEXT("connectionError"), Error);
}

void UCWNativeGameInstance::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    const FString PreviousRoomCode = CurrentRoomCode;
    UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native: socket closed %d clean=%d reason=%s"), StatusCode, bWasClean ? 1 : 0, *Reason);
    OnTextMessage.Broadcast(TEXT("closed"), Reason);
    MyId.Reset();
    bSpectating = false;
    LatestState = FCWRoomSnapshot();
    OnStateReceived.Broadcast(LatestState);
    if (!PreviousRoomCode.IsEmpty())
    {
        DefaultRoomCode = PreviousRoomCode;
        bAutoJoinOnConnect = true;
    }
}

void UCWNativeGameInstance::HandleMessage(const FString& Message)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return;
    }

    const FString Type = CWNativeJson::GetString(Root, TEXT("type"));
    if (Type == TEXT("welcome"))
    {
        HandleWelcome(Root);
        return;
    }

    if (Type == TEXT("state"))
    {
        FCWRoomSnapshot ParsedState;
        if (FCWProtocolParser::ParseStatePayload(CWNativeJson::GetObject(Root, TEXT("payload")), ParsedState))
        {
            LatestState = ParsedState;
            if (!LatestState.RoomCode.IsEmpty())
            {
                CurrentRoomCode = LatestState.RoomCode;
            }
            OnStateReceived.Broadcast(LatestState);
        }
        return;
    }

    if (Type == TEXT("skillFx"))
    {
        FCWSkillFxEvent Event;
        if (FCWProtocolParser::ParseSkillFxEvent(CWNativeJson::GetObject(Root, TEXT("event")), Event))
        {
            OnSkillFxReceived.Broadcast(Event);
        }
        return;
    }

    if (Type == TEXT("meleeFx"))
    {
        FCWMeleeFxEvent Event;
        if (FCWProtocolParser::ParseMeleeFxEvent(CWNativeJson::GetObject(Root, TEXT("event")), Event))
        {
            OnMeleeFxReceived.Broadcast(Event);
        }
        return;
    }

    if (Type == TEXT("worldFx"))
    {
        FCWWorldFxEvent Event;
        if (FCWProtocolParser::ParseWorldFxEvent(CWNativeJson::GetObject(Root, TEXT("event")), Event))
        {
            OnWorldFxReceived.Broadcast(Event);
        }
        return;
    }

    if (Type == TEXT("quickItemFx"))
    {
        const TSharedPtr<FJsonObject> EventObj = CWNativeJson::GetObject(Root, TEXT("event"));
        if (EventObj.IsValid())
        {
            FCWWorldFxEvent Event;
            Event.Id = CWNativeJson::GetString(EventObj, TEXT("id"));
            Event.Kind = CWNativeJson::GetString(EventObj, TEXT("useType"), CWNativeJson::GetString(EventObj, TEXT("itemId"), TEXT("quick_item")));
            Event.PlayerId = CWNativeJson::GetString(EventObj, TEXT("playerId"));
            Event.X = static_cast<float>(CWNativeJson::GetNumber(EventObj, TEXT("x")));
            Event.Y = static_cast<float>(CWNativeJson::GetNumber(EventObj, TEXT("y")));
            Event.Radius = static_cast<float>(FMath::Max(0.0, CWNativeJson::GetNumber(EventObj, TEXT("radius"), 120.0)));
            Event.DurationMs = static_cast<float>(FMath::Max(0.0, CWNativeJson::GetNumber(EventObj, TEXT("durationMs"))));
            OnWorldFxReceived.Broadcast(Event);
        }
        return;
    }

    if (Type == TEXT("system") || Type == TEXT("joinError") || Type == TEXT("serverRestart"))
    {
        const FString Text = CWNativeJson::GetString(Root, TEXT("message"));
        UE_LOG(LogTemp, Display, TEXT("Crimson Wars native: %s %s"), *Type, *Text);
        OnTextMessage.Broadcast(Type, Text);
        return;
    }

    if (Type == TEXT("netPong"))
    {
        return;
    }
}

void UCWNativeGameInstance::HandleWelcome(const TSharedPtr<FJsonObject>& Message)
{
    MyId = CWNativeJson::GetString(Message, TEXT("id"));
    CurrentRoomCode = CWNativeJson::GetString(Message, TEXT("roomCode"));
    bSpectating = CWNativeJson::GetBool(Message, TEXT("spectator"));
    bAutoJoinOnConnect = false;
    NextInputSeq = 0;

    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native: welcome id=%s room=%s spectating=%d"), *MyId, *CurrentRoomCode, bSpectating ? 1 : 0);
    OnTextMessage.Broadcast(TEXT("welcome"), CurrentRoomCode);
}
