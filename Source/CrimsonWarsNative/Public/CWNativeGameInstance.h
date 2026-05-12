#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "CWProtocolTypes.h"
#include "CWNativeGameInstance.generated.h"

class IWebSocket;

USTRUCT(BlueprintType)
struct FCWNativeHeroMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Tagline;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Accent;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Portrait;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Avatar;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString BodyMesh;
};

USTRUCT(BlueprintType)
struct FCWNativeItemMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString SlotCategory;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Rarity;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString EquipMesh;
};

USTRUCT(BlueprintType)
struct FCWNativeMapMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Subtitle;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Description;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Cover;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 WorldWidth = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 WorldHeight = 0;
};

USTRUCT(BlueprintType)
struct FCWNativeCampaignMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString ShortName;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Tagline;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Description;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Cover;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 LevelCount = 0;
};

USTRUCT(BlueprintType)
struct FCWNativeRoomMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Code;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString GameMode;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString RedirectUrl;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 Players = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int64 StartedAt = 0;
};

USTRUCT(BlueprintType)
struct FCWNativeLeaderboardMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 Rank = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Value;
};

USTRUCT(BlueprintType)
struct FCWNativeNewsMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Title;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Summary;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString ImageUrl;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int64 CreatedAt = 0;
};

USTRUCT(BlueprintType)
struct FCWNativeBootstrapSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    bool bOk = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    int32 SchemaVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString InstanceId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString RequestBaseUrl;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString WebSocketUrl;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString Logo;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString LandingBackground;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString StartBackground;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString HeroesBackground;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString BaseHeroId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeHeroMenuItem> Heroes;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeItemMenuItem> Items;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeMapMenuItem> Maps;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeCampaignMenuItem> Campaigns;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeRoomMenuItem> Rooms;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeLeaderboardMenuItem> Leaderboard;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TArray<FCWNativeNewsMenuItem> News;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    TMap<FString, FCWNativeFxProfile> FxProfiles;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FCWNativeStateDelegate, const FCWRoomSnapshot&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCWNativeTextDelegate, const FString&, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FCWNativeBootstrapDelegate, const FCWNativeBootstrapSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FCWNativeMeleeFxDelegate, const FCWMeleeFxEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FCWNativeWorldFxDelegate, const FCWWorldFxEvent&);

UCLASS(Config = Game)
class CRIMSONWARSNATIVE_API UCWNativeGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void ConnectToServer(const FString& Url);

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void DisconnectFromServer();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void JoinRoom(const FString& RoomCode, const FString& PlayerName, const FString& PlayerClass);

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void ReleaseNativeHandoff();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void LeaveRoom();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void SendInput(float MoveX, float MoveY, float AimX, float AimY, bool bShooting, bool bJump);

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void SendSkillPick(const FString& SkillId);

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Network")
    void SendNetPing();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Bootstrap")
    void FetchBootstrap();

    UFUNCTION(BlueprintPure, Category = "Crimson Wars|Network")
    bool IsSocketConnected() const;

    UFUNCTION(BlueprintPure, Category = "Crimson Wars|Network")
    bool HasJoinedRoom() const { return !MyId.IsEmpty() || bSpectating; }

    UFUNCTION(BlueprintPure, Category = "Crimson Wars|Bootstrap")
    FString ResolveHttpBaseUrl() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|Network")
    FString DefaultServerUrl = TEXT("ws://127.0.0.1:8080/ws");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|HTTP")
    FString DefaultHttpBaseUrl = TEXT("http://127.0.0.1:8080");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|HTTP")
    FString NativeBootstrapPath = TEXT("/api/native/bootstrap");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|UI")
    FString WebAssetsRoot = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|Network")
    FString DefaultPlayerName = TEXT("Native");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|Network")
    FString DefaultPlayerClass = TEXT("cyber");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|Network")
    FString DefaultRoomCode;

    UPROPERTY(BlueprintReadWrite, Category = "Crimson Wars|Network")
    FString NativeHandoffToken;

    UPROPERTY(BlueprintReadWrite, Category = "Crimson Wars|Network")
    FString NativeHandoffPlayerId;

    UPROPERTY(BlueprintReadWrite, Category = "Crimson Wars|Network")
    int32 NativeHandoffPlayerAccountId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|Network")
    bool bAutoConnectOnStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|Network")
    bool bAutoJoinOnConnect = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Crimson Wars|HTTP")
    bool bFetchBootstrapOnStart = true;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Network")
    FString MyId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Network")
    FString CurrentRoomCode;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Network")
    bool bSpectating = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|State")
    FCWRoomSnapshot LatestState;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FCWNativeBootstrapSnapshot Bootstrap;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    bool bBootstrapLoaded = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|Bootstrap")
    FString BootstrapError;

    FCWNativeStateDelegate OnStateReceived;
    FCWNativeTextDelegate OnTextMessage;
    FCWNativeBootstrapDelegate OnBootstrapReceived;
    FCWNativeMeleeFxDelegate OnMeleeFxReceived;
    FCWNativeWorldFxDelegate OnWorldFxReceived;

private:
    void ApplyRuntimeDefaults();
    void SendJson(const TSharedRef<class FJsonObject>& Payload);
    void HandleBootstrapResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
    void HandleConnected();
    void HandleConnectionError(const FString& Error);
    void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
    void HandleMessage(const FString& Message);
    void HandleWelcome(const TSharedPtr<class FJsonObject>& Message);

    TSharedPtr<IWebSocket> Socket;
    int32 NextInputSeq = 0;
    int32 NextPingSeq = 0;
};
