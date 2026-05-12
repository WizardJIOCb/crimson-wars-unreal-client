#include "CWNativeRunViewWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "CWNativeAssetLibrary.h"
#include "CWNativeGameInstance.h"
#include "CWNativePlayerPawn.h"
#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "Algo/Sort.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SViewport.h"

namespace CWRunView
{
    constexpr float DefaultWorldWidth = 4800.0f;
    constexpr float DefaultWorldHeight = 2800.0f;
    constexpr float ViewWorldWidth = 2350.0f;
    constexpr float RoadHeight = 840.0f;

    float LocalAnimTime(float SeedPhase = 0.0f)
    {
        return static_cast<float>(FMath::Fmod(FPlatformTime::Seconds(), 3600.0)) + SeedPhase;
    }

    float SmoothStep01(float Value)
    {
        const float T = FMath::Clamp(Value, 0.0f, 1.0f);
        return T * T * (3.0f - 2.0f * T);
    }

    const FSlateBrush* WhiteBrush()
    {
        return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    }

    FPaintGeometry PaintGeometry(const FGeometry& Geometry, const FVector2D& Position, const FVector2D& Size)
    {
        return Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position));
    }

    FString PickupRenderKey(const FCWPickupSnapshot& Pickup, bool bStableGroundPickup)
    {
        if (bStableGroundPickup)
        {
            return FString::Printf(
                TEXT("%s:%s:%d:%d"),
                *Pickup.Kind,
                *Pickup.ItemKey,
                FMath::RoundToInt(Pickup.X / 8.0f),
                FMath::RoundToInt(Pickup.Y / 8.0f));
        }

        return Pickup.Id.IsEmpty()
            ? FString::Printf(TEXT("%s:%s:%.0f:%.0f"), *Pickup.Kind, *Pickup.ItemKey, Pickup.X, Pickup.Y)
            : Pickup.Id;
    }

    FLinearColor LootDropAccentColor(const FString& ItemKey)
    {
        const FString Key = ItemKey.ToLower();
        if (Key.Contains(TEXT("sniper")))
        {
            return FLinearColor(0.55f, 0.78f, 1.0f, 1.0f);
        }
        if (Key.Contains(TEXT("shotgun")))
        {
            return FLinearColor(1.0f, 0.52f, 0.18f, 1.0f);
        }
        if (Key.Contains(TEXT("smg")))
        {
            return FLinearColor(0.22f, 0.92f, 1.0f, 1.0f);
        }
        if (Key.Contains(TEXT("pistol")))
        {
            return FLinearColor(1.0f, 0.82f, 0.28f, 1.0f);
        }
        return FLinearColor(0.78f, 0.44f, 1.0f, 1.0f);
    }

    void DrawBox(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
    {
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            PaintGeometry(Geometry, Position, Size),
            WhiteBrush(),
            ESlateDrawEffect::None,
            Color);
    }

    void DrawTexture(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FSlateBrush* Brush, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
    {
        if (!Brush)
        {
            return;
        }

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            PaintGeometry(Geometry, Position, Size),
            Brush,
            ESlateDrawEffect::None,
            Tint);
    }

    void DrawTextureRegion(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FSlateBrush* Brush, const FVector2D& Position, const FVector2D& Size, const FVector2D& SourcePosition, const FVector2D& SourceSize, const FVector2D& TextureSize, const FLinearColor& Tint = FLinearColor::White)
    {
        if (!Brush || TextureSize.X <= 0.0f || TextureSize.Y <= 0.0f || SourceSize.X <= 0.0f || SourceSize.Y <= 0.0f)
        {
            return;
        }

        FSlateBrush RegionBrush(*Brush);
        RegionBrush.SetUVRegion(FBox2f(
            FVector2f(SourcePosition.X / TextureSize.X, SourcePosition.Y / TextureSize.Y),
            FVector2f((SourcePosition.X + SourceSize.X) / TextureSize.X, (SourcePosition.Y + SourceSize.Y) / TextureSize.Y)));
        DrawTexture(OutDrawElements, Geometry, LayerId, &RegionBrush, Position, Size, Tint);
    }

    void DrawCenteredBox(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Center, const FVector2D& Size, const FLinearColor& Color)
    {
        DrawBox(OutDrawElements, Geometry, LayerId, Center - Size * 0.5f, Size, Color);
    }

    bool IsScreenPointVisible(const FVector2D& Point, const FVector2D& ViewSize, float Margin = 96.0f)
    {
        return Point.X >= -Margin
            && Point.Y >= -Margin
            && Point.X <= ViewSize.X + Margin
            && Point.Y <= ViewSize.Y + Margin;
    }

    void DrawLine(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness)
    {
        TArray<FVector2D> Points;
        Points.Add(A);
        Points.Add(B);
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Color,
            true,
            Thickness);
    }

    void DrawEllipse(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Center, const FVector2D& Size, const FLinearColor& Color)
    {
        if (Size.X <= 9.0f && Size.Y <= 9.0f)
        {
            DrawBox(OutDrawElements, Geometry, LayerId, Center - Size * 0.5f, Size, Color);
            return;
        }

        const int32 Slices = FMath::Clamp(FMath::RoundToInt(Size.Y * 0.55f), 6, 22);
        const float HalfW = FMath::Max(1.0f, Size.X * 0.5f);
        const float HalfH = FMath::Max(1.0f, Size.Y * 0.5f);
        const float Thickness = FMath::Max(1.0f, Size.Y / static_cast<float>(Slices));
        for (int32 I = 0; I < Slices; ++I)
        {
            const float T = (static_cast<float>(I) + 0.5f) / static_cast<float>(Slices);
            const float Y = FMath::Lerp(-HalfH, HalfH, T);
            const float Normalized = FMath::Clamp(Y / HalfH, -1.0f, 1.0f);
            const float Width = HalfW * FMath::Sqrt(FMath::Max(0.0f, 1.0f - Normalized * Normalized));
            const float Alpha = Color.A * FMath::Lerp(1.0f, 0.46f, FMath::Abs(Normalized));
            DrawLine(
                OutDrawElements,
                Geometry,
                LayerId,
                Center + FVector2D(-Width, Y),
                Center + FVector2D(Width, Y),
                FLinearColor(Color.R, Color.G, Color.B, Alpha),
                Thickness);
        }
    }

    void DrawRing(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Center, const FVector2D& Size, const FLinearColor& Color, float Thickness, int32 Segments = 40, float PhaseRadians = 0.0f)
    {
        const float HalfW = FMath::Max(1.0f, Size.X * 0.5f);
        const float HalfH = FMath::Max(1.0f, Size.Y * 0.5f);
        const int32 SafeSegments = FMath::Clamp(Segments, 8, 28);
        for (int32 I = 0; I < SafeSegments; ++I)
        {
            const float A0 = PhaseRadians + static_cast<float>(I) * UE_PI * 2.0f / static_cast<float>(SafeSegments);
            const float A1 = PhaseRadians + static_cast<float>(I + 1) * UE_PI * 2.0f / static_cast<float>(SafeSegments);
            DrawLine(
                OutDrawElements,
                Geometry,
                LayerId,
                Center + FVector2D(FMath::Cos(A0) * HalfW, FMath::Sin(A0) * HalfH),
                Center + FVector2D(FMath::Cos(A1) * HalfW, FMath::Sin(A1) * HalfH),
                Color,
                Thickness);
        }
    }

    void DrawText(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Position, const FString& Text, float Size, const FLinearColor& Color, bool bBold = false)
    {
        FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")), static_cast<uint16>(Size));
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            PaintGeometry(Geometry, Position, FVector2D(FMath::Max(1.0f, Size * Text.Len() * 0.72f), Size * 1.35f)),
            FText::FromString(Text),
            Font,
            ESlateDrawEffect::None,
            Color);
    }

    bool IsBossEnemy(const FCWEnemySnapshot& Enemy);

    FLinearColor EnemyColor(const FCWEnemySnapshot& Enemy)
    {
        if (IsBossEnemy(Enemy))
        {
            return FLinearColor(1.0f, 0.16f, 0.05f, 0.96f);
        }
        if (Enemy.Type.Contains(TEXT("fast"), ESearchCase::IgnoreCase) || Enemy.Behavior.Contains(TEXT("rush"), ESearchCase::IgnoreCase))
        {
            return FLinearColor(1.0f, 0.45f, 0.12f, 0.9f);
        }
        return FLinearColor(0.85f, 0.08f, 0.08f, 0.9f);
    }

    uint32 HashCell(int32 X, int32 Y, uint32 Salt = 0)
    {
        uint32 H = static_cast<uint32>(X) * 374761393u + static_cast<uint32>(Y) * 668265263u + Salt * 2246822519u;
        H = (H ^ (H >> 13)) * 1274126177u;
        return H ^ (H >> 16);
    }

    float UnitRand(const FVector2D& Position, float Radius, int32 Index, uint32 Salt = 0)
    {
        const int32 X = FMath::RoundToInt(Position.X * 3.17f) + Index * 97 + FMath::RoundToInt(Radius * 11.0f);
        const int32 Y = FMath::RoundToInt(Position.Y * 2.61f) - Index * 53 + FMath::RoundToInt(Radius * 7.0f);
        return static_cast<float>(HashCell(X, Y, Salt) & 0xFFFFu) / 65535.0f;
    }

    bool IsBossEnemy(const FCWEnemySnapshot& Enemy)
    {
        return Enemy.Type.Contains(TEXT("boss"), ESearchCase::IgnoreCase)
            || Enemy.Behavior.Contains(TEXT("boss"), ESearchCase::IgnoreCase);
    }

    FLinearColor HexColor(const FString& Hex, const FLinearColor& Fallback)
    {
        FString Clean = Hex;
        Clean.TrimStartAndEndInline();
        Clean.RemoveFromStart(TEXT("#"));
        if (Clean.Len() != 6)
        {
            return Fallback;
        }

        const int32 R = FParse::HexDigit(Clean[0]) * 16 + FParse::HexDigit(Clean[1]);
        const int32 G = FParse::HexDigit(Clean[2]) * 16 + FParse::HexDigit(Clean[3]);
        const int32 B = FParse::HexDigit(Clean[4]) * 16 + FParse::HexDigit(Clean[5]);
        return FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, Fallback.A);
    }

    FLinearColor MaterialColor(const FString& Material, float Alpha = 1.0f)
    {
        const FString Key = Material.ToLower();
        if (Key == TEXT("grass"))
        {
            return FLinearColor(0.070f, 0.135f, 0.055f, Alpha);
        }
        if (Key == TEXT("dirt"))
        {
            return FLinearColor(0.172f, 0.130f, 0.098f, Alpha);
        }
        if (Key == TEXT("concrete") || Key == TEXT("concrete_tiles"))
        {
            return FLinearColor(0.125f, 0.136f, 0.142f, Alpha);
        }
        if (Key == TEXT("toxic"))
        {
            return FLinearColor(0.128f, 0.214f, 0.096f, Alpha);
        }
        if (Key == TEXT("asphalt"))
        {
            return FLinearColor(0.052f, 0.058f, 0.062f, Alpha);
        }
        if (Key == TEXT("asphalt_wet"))
        {
            return FLinearColor(0.040f, 0.046f, 0.052f, Alpha);
        }
        return FLinearColor(0.074f, 0.084f, 0.088f, Alpha);
    }

    FString MapPropTextureKey(const FString& SpriteKey)
    {
        const FString Key = SpriteKey.ToLower();
        if (Key == TEXT("concrete_barrier"))
        {
            return TEXT("barrier");
        }
        if (Key == TEXT("yellow_bus"))
        {
            return TEXT("bus_yellow");
        }
        if (Key == TEXT("red_hatchback"))
        {
            return TEXT("car_red");
        }
        if (Key == TEXT("burnt_sedan"))
        {
            return TEXT("car_blue");
        }
        if (Key == TEXT("road_shack"))
        {
            return TEXT("shack");
        }
        if (Key == TEXT("ambulance_van"))
        {
            return TEXT("ambulance");
        }
        if (Key == TEXT("clinic_block"))
        {
            return TEXT("clinic_block");
        }
        if (Key == TEXT("reactor_block"))
        {
            return TEXT("reactor_block");
        }
        if (Key == TEXT("build_1") || Key == TEXT("bullet_bistro_bunker"))
        {
            return TEXT("build_1");
        }
        if (Key == TEXT("build_2") || Key == TEXT("fuel_hell_checkpoint"))
        {
            return TEXT("build_2");
        }
        if (Key == TEXT("build_3") || Key == TEXT("haven_nope_tower"))
        {
            return TEXT("build_3");
        }
        if (Key == TEXT("build_4") || Key == TEXT("fix_or_die_garage"))
        {
            return TEXT("build_4");
        }
        if (Key == TEXT("build_5") || Key == TEXT("almost_alive_clinic"))
        {
            return TEXT("build_5");
        }
        if (Key == TEXT("build_6") || Key == TEXT("hellmart_24_7"))
        {
            return TEXT("build_6");
        }
        if (Key == TEXT("build_7") || Key == TEXT("dead_signal_station"))
        {
            return TEXT("build_7");
        }
        return Key;
    }

    FString PlayerTextureKey(const FString& PlayerClass)
    {
        const FString Key = PlayerClass.ToLower();
        if (Key == TEXT("shadow")) return TEXT("player_shadow");
        if (Key == TEXT("scout")) return TEXT("player_scout");
        if (Key == TEXT("raider")) return TEXT("player_raider");
        if (Key == TEXT("medic") || Key == TEXT("medis")) return TEXT("player_medic");
        if (Key == TEXT("nomad")) return TEXT("player_nomad");
        if (Key == TEXT("warden")) return TEXT("player_warden");
        if (Key == TEXT("dude")) return TEXT("player_dude");
        return TEXT("player_cyber");
    }

    int32 PlayerRowFromAim(const FCWPlayerSnapshot& Player)
    {
        const float DX = Player.AimX - Player.X;
        const float DY = Player.AimY - Player.Y;
        if (FMath::Abs(DX) > FMath::Abs(DY))
        {
            return DX < 0.0f ? 1 : 3;
        }
        return DY < 0.0f ? 0 : 2;
    }
}

TSharedRef<SWidget> UCWNativeRunViewWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RunViewRoot"));
    WidgetTree->RootWidget = RootCanvas;

    return Super::RebuildWidget();
}

void UCWNativeRunViewWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetVisibility(ESlateVisibility::HitTestInvisible);

    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        AssetsRoot = GI->WebAssetsRoot;
        CachedState = GI->LatestState;
        CachedMyId = GI->MyId;
        StateDelegateHandle = GI->OnStateReceived.AddUObject(this, &UCWNativeRunViewWidget::HandleStateReceived);
        SkillFxDelegateHandle = GI->OnSkillFxReceived.AddUObject(this, &UCWNativeRunViewWidget::HandleSkillFxReceived);
        MeleeFxDelegateHandle = GI->OnMeleeFxReceived.AddUObject(this, &UCWNativeRunViewWidget::HandleMeleeFxReceived);
        WorldFxDelegateHandle = GI->OnWorldFxReceived.AddUObject(this, &UCWNativeRunViewWidget::HandleWorldFxReceived);
    }

    if (AssetsRoot.IsEmpty())
    {
        AssetsRoot = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
    }
    LoadRunAssets();
}

void UCWNativeRunViewWidget::NativeDestruct()
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        if (StateDelegateHandle.IsValid())
        {
            GI->OnStateReceived.Remove(StateDelegateHandle);
            StateDelegateHandle.Reset();
        }
        if (SkillFxDelegateHandle.IsValid())
        {
            GI->OnSkillFxReceived.Remove(SkillFxDelegateHandle);
            SkillFxDelegateHandle.Reset();
        }
        if (MeleeFxDelegateHandle.IsValid())
        {
            GI->OnMeleeFxReceived.Remove(MeleeFxDelegateHandle);
            MeleeFxDelegateHandle.Reset();
        }
        if (WorldFxDelegateHandle.IsValid())
        {
            GI->OnWorldFxReceived.Remove(WorldFxDelegateHandle);
            WorldFxDelegateHandle.Reset();
        }
    }

    Super::NativeDestruct();
}

void UCWNativeRunViewWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateRenderPlayers(InDeltaTime);
    UpdateRenderEntities(InDeltaTime);
    UpdateTransientFx(InDeltaTime);
    InvalidateLayoutAndVolatility();
}

void UCWNativeRunViewWidget::LoadRunAssets()
{
    LoadTexture(TEXT("grass"), TEXT("tiles/ground_grass.jpg"));
    LoadTexture(TEXT("player_cyber"), TEXT("sprites/player_cyber.png"));
    LoadTexture(TEXT("player_shadow"), TEXT("sprites/player_shadow.png"));
    LoadTexture(TEXT("player_scout"), TEXT("sprites/player_scout.png"));
    LoadTexture(TEXT("player_raider"), TEXT("sprites/player_raider.png"));
    LoadTexture(TEXT("player_medic"), TEXT("sprites/player_medic.png"));
    LoadTexture(TEXT("player_nomad"), TEXT("sprites/player_nomad.png"));
    LoadTexture(TEXT("player_warden"), TEXT("sprites/player_warden.png"));
    LoadTexture(TEXT("player_dude"), TEXT("sprites/player_dude.png"));
    LoadTexture(TEXT("enemy_mummy"), TEXT("sprites/enemy_mummy.png"));
    LoadTexture(TEXT("tree"), TEXT("sprites/tree.png"));
    LoadTexture(TEXT("car_blue"), TEXT("map-props/car_blue.png"));
    LoadTexture(TEXT("car_red"), TEXT("map-props/car_red.png"));
    LoadTexture(TEXT("ambulance"), TEXT("map-props/ambulance.png"));
    LoadTexture(TEXT("barrier"), TEXT("map-props/barrier.png"));
    LoadTexture(TEXT("bus_yellow"), TEXT("map-props/bus_yellow.png"));
    LoadTexture(TEXT("industrial_tank"), TEXT("map-props/industrial_tank.png"));
    LoadTexture(TEXT("mall_block"), TEXT("map-props/mall_block.png"));
    LoadTexture(TEXT("shack"), TEXT("map-props/shack.png"));
    LoadTexture(TEXT("build_1"), TEXT("buildings/build-1.png"));
    LoadTexture(TEXT("build_2"), TEXT("buildings/build-2.png"));
    LoadTexture(TEXT("build_3"), TEXT("buildings/build-3.png"));
    LoadTexture(TEXT("build_4"), TEXT("buildings/build-4.png"));
    LoadTexture(TEXT("build_5"), TEXT("buildings/build-5.png"));
    LoadTexture(TEXT("build_6"), TEXT("buildings/build-6.png"));
    LoadTexture(TEXT("build_7"), TEXT("buildings/build-7.png"));
    LoadTexture(TEXT("pistol"), TEXT("weapon-pickups/pistol.png"));
    LoadTexture(TEXT("smg"), TEXT("weapon-pickups/smg.png"));
    LoadTexture(TEXT("shotgun"), TEXT("weapon-pickups/shotgun.png"));
    LoadTexture(TEXT("sniper"), TEXT("weapon-pickups/sniper.png"));
    LoadSkillIconTextures();
}

void UCWNativeRunViewWidget::LoadSkillIconTextures()
{
    const FString SkillDir = FCWNativeAssetLibrary::NormalizeAssetPath(AssetsRoot, TEXT("hero-skills"));
    const FString SkillPattern = FPaths::Combine(SkillDir, TEXT("*.png"));
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *SkillPattern, true, false);
    Files.Sort();

    for (const FString& File : Files)
    {
        const FString Base = FPaths::GetBaseFilename(File).ToLower();
        if (Base.IsEmpty())
        {
            continue;
        }

        const FString Key = FString::Printf(TEXT("skill_%s"), *Base);
        LoadTexture(Key, FString::Printf(TEXT("hero-skills/%s"), *File));
        if (BrushCache.Contains(Key))
        {
            SkillIconTextureKeys.AddUnique(Key);
        }
    }
}

void UCWNativeRunViewWidget::LoadTexture(const FString& Key, const FString& RelativePath)
{
    if (TextureCache.Contains(Key))
    {
        return;
    }

    const FString NormalizedRelativePath = RelativePath.Replace(TEXT("/"), TEXT("\\"));
    const FString AbsolutePath = FCWNativeAssetLibrary::NormalizeAssetPath(AssetsRoot, NormalizedRelativePath);
    if (!FPaths::FileExists(AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native renderer: missing texture %s"), *AbsolutePath);
        return;
    }

    UTexture2D* Texture = FCWNativeAssetLibrary::LoadTexture(this, AssetsRoot, NormalizedRelativePath);
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native renderer: failed to load texture %s"), *AbsolutePath);
        return;
    }

    TextureCache.Add(Key, Texture);

    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.Tiling = ESlateBrushTileType::NoTile;
    BrushCache.Add(Key, Brush);
}

const FSlateBrush* UCWNativeRunViewWidget::GetBrush(const FString& Key) const
{
    return BrushCache.Find(Key);
}

FLinearColor UCWNativeRunViewWidget::SkillOrbAccentColor(const FString& SkillId) const
{
    const FString Key = SkillId.ToLower();
    if (Key.Contains(TEXT("regen")) || Key.Contains(TEXT("heal")) || Key.Contains(TEXT("vital")) || Key.Contains(TEXT("shield")) || Key.Contains(TEXT("barrier")) || Key.Contains(TEXT("plating")))
    {
        return FLinearColor(0.20f, 0.83f, 0.58f, 1.0f);
    }
    if (Key.Contains(TEXT("lightning")) || Key.Contains(TEXT("arc")) || Key.Contains(TEXT("electric")) || Key.Contains(TEXT("storm")) || Key.Contains(TEXT("chain")))
    {
        return FLinearColor(0.40f, 0.91f, 0.98f, 1.0f);
    }
    if (Key.Contains(TEXT("laser")) || Key.Contains(TEXT("void")) || Key.Contains(TEXT("shadow")) || Key.Contains(TEXT("night")) || Key.Contains(TEXT("eclipse")))
    {
        return FLinearColor(0.75f, 0.52f, 1.0f, 1.0f);
    }
    if (Key.Contains(TEXT("rocket")) || Key.Contains(TEXT("missile")) || Key.Contains(TEXT("barrage")) || Key.Contains(TEXT("stomp")) || Key.Contains(TEXT("shrapnel")))
    {
        return FLinearColor(0.98f, 0.57f, 0.24f, 1.0f);
    }
    if (Key.Contains(TEXT("blood")) || Key.Contains(TEXT("rage")) || Key.Contains(TEXT("blade")) || Key.Contains(TEXT("slash")) || Key.Contains(TEXT("fang")))
    {
        return FLinearColor(0.98f, 0.44f, 0.55f, 1.0f);
    }
    if (Key.Contains(TEXT("haste")) || Key.Contains(TEXT("speed")) || Key.Contains(TEXT("reload")) || Key.Contains(TEXT("stride")))
    {
        return FLinearColor(0.98f, 0.80f, 0.15f, 1.0f);
    }
    if (Key.Contains(TEXT("magnet")) || Key.Contains(TEXT("gps")) || Key.Contains(TEXT("seeker")) || Key.Contains(TEXT("homing")) || Key.Contains(TEXT("mark")) || Key.Contains(TEXT("sync")) || Key.Contains(TEXT("buddy")) || Key.Contains(TEXT("drone")) || Key.Contains(TEXT("turret")))
    {
        return FLinearColor(0.38f, 0.65f, 0.98f, 1.0f);
    }
    return FLinearColor(0.28f, 0.90f, 1.0f, 1.0f);
}

FString UCWNativeRunViewWidget::SkillOrbFxKind(const FString& SkillId) const
{
    const FString Key = SkillId.ToLower();
    if (Key.Contains(TEXT("lightning")) || Key.Contains(TEXT("arc")) || Key.Contains(TEXT("storm")) || Key.Contains(TEXT("chain")))
    {
        return TEXT("lightning");
    }
    if (Key.Contains(TEXT("rocket")) || Key.Contains(TEXT("missile")) || Key.Contains(TEXT("barrage")) || Key.Contains(TEXT("stomp")) || Key.Contains(TEXT("shrapnel")))
    {
        return TEXT("blast");
    }
    if (Key.Contains(TEXT("blood")) || Key.Contains(TEXT("rage")) || Key.Contains(TEXT("blade")) || Key.Contains(TEXT("slash")) || Key.Contains(TEXT("fang")))
    {
        return TEXT("blade");
    }
    if (Key.Contains(TEXT("magnet")) || Key.Contains(TEXT("gps")) || Key.Contains(TEXT("seeker")) || Key.Contains(TEXT("homing")) || Key.Contains(TEXT("mark")) || Key.Contains(TEXT("sync")) || Key.Contains(TEXT("buddy")) || Key.Contains(TEXT("drone")) || Key.Contains(TEXT("turret")))
    {
        return TEXT("orbit");
    }
    if (Key.Contains(TEXT("regen")) || Key.Contains(TEXT("heal")) || Key.Contains(TEXT("vital")) || Key.Contains(TEXT("shield")) || Key.Contains(TEXT("barrier")) || Key.Contains(TEXT("plating")))
    {
        return TEXT("support");
    }
    if (Key.Contains(TEXT("laser")) || Key.Contains(TEXT("void")) || Key.Contains(TEXT("shadow")) || Key.Contains(TEXT("night")) || Key.Contains(TEXT("eclipse")))
    {
        return TEXT("void");
    }
    return TEXT("spark");
}

FString UCWNativeRunViewWidget::SkillOrbBadge(const FString& SkillId) const
{
    TArray<FString> Parts;
    SkillId.ToUpper().ParseIntoArray(Parts, TEXT("_"), true);
    FString Badge;
    for (const FString& Part : Parts)
    {
        if (!Part.IsEmpty())
        {
            Badge += Part.Left(1);
        }
        if (Badge.Len() >= 2)
        {
            break;
        }
    }
    if (Badge.IsEmpty())
    {
        Badge = SkillId.Left(2).ToUpper();
    }
    return Badge.Left(3);
}

FString UCWNativeRunViewWidget::FindSkillIconTextureKey(const FString& SkillId) const
{
    const FString Stem = SkillId.ToLower();
    if (Stem.IsEmpty())
    {
        return FString();
    }

    const FString ExactKey = FString::Printf(TEXT("skill_%s"), *Stem);
    if (BrushCache.Contains(ExactKey))
    {
        return ExactKey;
    }

    for (const FString& Key : SkillIconTextureKeys)
    {
        if (Key.EndsWith(FString::Printf(TEXT("_%s"), *Stem)) || Key.Contains(Stem))
        {
            return Key;
        }
    }

    TArray<FString> Tokens;
    Stem.ParseIntoArray(Tokens, TEXT("_"), true);
    int32 BestScore = 0;
    FString BestKey;
    for (const FString& Key : SkillIconTextureKeys)
    {
        int32 Score = 0;
        for (const FString& Token : Tokens)
        {
            if (Token.Len() >= 3 && Key.Contains(Token))
            {
                ++Score;
            }
        }
        if (Score > BestScore)
        {
            BestScore = Score;
            BestKey = Key;
        }
    }
    if (BestScore > 0)
    {
        return BestKey;
    }

    auto FirstKeyContaining = [&](const FString& A, const FString& B = FString(), const FString& C = FString()) -> FString
    {
        const FString Fragments[3] = { A, B, C };
        for (const FString& Fragment : Fragments)
        {
            if (Fragment.IsEmpty())
            {
                continue;
            }
            for (const FString& Key : SkillIconTextureKeys)
            {
                if (Key.Contains(Fragment))
                {
                    return Key;
                }
            }
        }
        return FString();
    };

    if (Stem.Contains(TEXT("laser")) || Stem.Contains(TEXT("lance")))
    {
        return FirstKeyContaining(TEXT("ion_lance"), TEXT("void_burst"));
    }
    if (Stem.Contains(TEXT("shield")) || Stem.Contains(TEXT("frame")) || Stem.Contains(TEXT("plating")))
    {
        return FirstKeyContaining(TEXT("adaptive_frame"), TEXT("vital_plating"));
    }
    if (Stem.Contains(TEXT("reload")))
    {
        return FirstKeyContaining(TEXT("action_reload"), TEXT("combat_firmware"));
    }
    if (Stem.Contains(TEXT("magnet")) || Stem.Contains(TEXT("sync")) || Stem.Contains(TEXT("gps")) || Stem.Contains(TEXT("seeker")) || Stem.Contains(TEXT("homing")) || Stem.Contains(TEXT("buddy")) || Stem.Contains(TEXT("drone")) || Stem.Contains(TEXT("turret")))
    {
        return FirstKeyContaining(TEXT("seeker_protocol"), TEXT("sync_link"), TEXT("hunter_mark"));
    }
    if (Stem.Contains(TEXT("speed")) || Stem.Contains(TEXT("haste")) || Stem.Contains(TEXT("stride")))
    {
        return FirstKeyContaining(TEXT("long_stride"), TEXT("energy_drink"));
    }
    if (Stem.Contains(TEXT("rocket")) || Stem.Contains(TEXT("missile")) || Stem.Contains(TEXT("barrage")))
    {
        return FirstKeyContaining(TEXT("rescue_rockets"), TEXT("siege_barrage"));
    }
    if (Stem.Contains(TEXT("blood")) || Stem.Contains(TEXT("rage")))
    {
        return FirstKeyContaining(TEXT("battle_rage"), TEXT("anger_management"));
    }
    if (Stem.Contains(TEXT("blade")) || Stem.Contains(TEXT("slash")) || Stem.Contains(TEXT("orbit")))
    {
        return FirstKeyContaining(TEXT("night_fangs"), TEXT("razor_wind"));
    }
    if (Stem.Contains(TEXT("storm")) || Stem.Contains(TEXT("lightning")) || Stem.Contains(TEXT("arc")))
    {
        return FirstKeyContaining(TEXT("arc_matrix"), TEXT("storm_net"), TEXT("berserk_arc"));
    }
    if (Stem.Contains(TEXT("heal")) || Stem.Contains(TEXT("regen")) || Stem.Contains(TEXT("vital")))
    {
        return FirstKeyContaining(TEXT("field_aid"), TEXT("vital_sight"), TEXT("vital_plating"));
    }

    return SkillIconTextureKeys.Num() > 0 ? SkillIconTextureKeys[0] : FString();
}

void UCWNativeRunViewWidget::HandleStateReceived(const FCWRoomSnapshot& State)
{
    FCWRoomSnapshot MergedState = State;
    const float DeltaMs = (State.ServerNowMs > 0.0 && CachedState.ServerNowMs > 0.0)
        ? FMath::Clamp(static_cast<float>(State.ServerNowMs - CachedState.ServerNowMs), 0.0f, 500.0f)
        : 0.0f;
    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (GI)
    {
        CachedMyId = GI->MyId;
    }
    if ((!CachedState.RoomCode.IsEmpty() && State.RoomCode != CachedState.RoomCode) ||
        (State.ServerNowMs > 0.0 && CachedState.ServerNowMs > 0.0 && State.ServerNowMs < CachedState.ServerNowMs))
    {
        HiddenDropKeys.Reset();
        HiddenXpOrbKeys.Reset();
        HiddenSkillOrbKeys.Reset();
    }

    auto AgeRetainedPickups = [DeltaMs](TArray<FCWPickupSnapshot>& Pickups)
    {
        for (int32 Index = Pickups.Num() - 1; Index >= 0; --Index)
        {
            FCWPickupSnapshot& Pickup = Pickups[Index];
            if (DeltaMs > 0.0f && Pickup.TtlMs > 0.0f)
            {
                Pickup.TtlMs = FMath::Max(0.0f, Pickup.TtlMs - DeltaMs);
            }
            if (DeltaMs > 0.0f && Pickup.TtlMs <= 0.0f)
            {
                Pickups.RemoveAtSwap(Index);
            }
        }
    };

    auto CanCarryOmittedPickups = [](int32 IncomingVersion, int32 CachedVersion)
    {
        return IncomingVersion <= 0 || CachedVersion <= 0 || IncomingVersion == CachedVersion;
    };

    if (!MergedState.bHasDropsPayload)
    {
        if (CanCarryOmittedPickups(MergedState.DropsVersion, CachedState.DropsVersion))
        {
            MergedState.Drops = CachedState.Drops;
            AgeRetainedPickups(MergedState.Drops);
        }
        else
        {
            MergedState.Drops.Reset();
        }
    }
    if (!MergedState.bHasXpOrbsPayload)
    {
        if (CanCarryOmittedPickups(MergedState.XpOrbsVersion, CachedState.XpOrbsVersion))
        {
            MergedState.XpOrbs = CachedState.XpOrbs;
            AgeRetainedPickups(MergedState.XpOrbs);
        }
        else
        {
            MergedState.XpOrbs.Reset();
        }
    }
    if (!MergedState.bHasSkillOrbsPayload)
    {
        MergedState.SkillOrbs.Reset();
    }

    for (FCWPickupSnapshot& Orb : MergedState.SkillOrbs)
    {
        if (Orb.TtlMaxMs <= 0.0f)
        {
            Orb.TtlMaxMs = 15000.0f;
        }
        if (Orb.TtlMs <= 0.0f)
        {
            Orb.TtlMs = Orb.TtlMaxMs;
        }
    }

    const FCWPlayerSnapshot* LocalPickupPlayer = nullptr;
    const FCWPlayerSnapshot* FirstLivePickupPlayer = nullptr;
    int32 LivePickupPlayerCount = 0;
    for (const FCWPlayerSnapshot& Player : MergedState.Players)
    {
        if (!Player.bAlive || Player.bIsCompanion)
        {
            continue;
        }
        ++LivePickupPlayerCount;
        if (!FirstLivePickupPlayer)
        {
            FirstLivePickupPlayer = &Player;
        }
        if (!CachedMyId.IsEmpty() && Player.Id == CachedMyId)
        {
            LocalPickupPlayer = &Player;
            break;
        }
    }
    if (!LocalPickupPlayer && LivePickupPlayerCount == 1)
    {
        LocalPickupPlayer = FirstLivePickupPlayer;
    }

    FVector2D LocalPickupRenderPos = LocalPickupPlayer ? FVector2D(LocalPickupPlayer->X, LocalPickupPlayer->Y) : FVector2D::ZeroVector;
    if (LocalPickupPlayer)
    {
        if (const FRenderPlayerState* RenderState = RenderPlayers.Find(LocalPickupPlayer->Id))
        {
            if (RenderState->bInitialized)
            {
                LocalPickupRenderPos = RenderState->Position;
            }
        }
    }

    auto RemoveCollectedNearLocalPlayer = [this, LocalPickupPlayer](TArray<FCWPickupSnapshot>& Pickups, float Radius, bool bOwnerOnly)
    {
        if (!LocalPickupPlayer)
        {
            return;
        }
        const float RadiusSq = FMath::Square(FMath::Max(1.0f, Radius));
        for (int32 Index = Pickups.Num() - 1; Index >= 0; --Index)
        {
            const FCWPickupSnapshot& Pickup = Pickups[Index];
            if (bOwnerOnly && !Pickup.OwnerId.IsEmpty() && Pickup.OwnerId != LocalPickupPlayer->Id)
            {
                continue;
            }
            const float Dx = Pickup.X - LocalPickupPlayer->X;
            const float Dy = Pickup.Y - LocalPickupPlayer->Y;
            if (Dx * Dx + Dy * Dy <= RadiusSq)
            {
                if (Pickup.Kind == TEXT("xp"))
                {
                    const FVector2D PickupPos(Pickup.X, Pickup.Y);
                    const FVector2D PlayerPos(LocalPickupPlayer->X, LocalPickupPlayer->Y);
                    AddFx(TEXT("xp_absorb"), PickupPos, PlayerPos - PickupPos, FLinearColor(0.42f, 0.96f, 1.0f, 0.94f), 0.62f, FMath::Clamp(30.0f + Pickup.Xp * 1.4f, 30.0f, 68.0f), PlayerPos);
                }
                else if (!Pickup.ItemKey.IsEmpty())
                {
                    const FVector2D PickupPos(Pickup.X, Pickup.Y);
                    const FVector2D PlayerPos(LocalPickupPlayer->X, LocalPickupPlayer->Y);
                    const FLinearColor Accent = CWRunView::LootDropAccentColor(Pickup.ItemKey);
                    AddFx(TEXT("loot_pickup"), PickupPos, PlayerPos - PickupPos, Accent, 0.70f, 66.0f, PlayerPos);
                }
                Pickups.RemoveAtSwap(Index);
            }
        }
    };

    auto HideCollectedRenderPickups = [this, LocalPickupPlayer, LocalPickupRenderPos](TArray<FCWPickupSnapshot>& Pickups, TMap<FString, FRenderEntityState>& RenderStore, TSet<FString>& HiddenKeys, float Radius, bool bStableGroundPickup, bool bOwnerOnly, UCWNativeGameInstance* InGI)
    {
        if (!LocalPickupPlayer)
        {
            return;
        }

        const float RadiusSq = FMath::Square(FMath::Max(1.0f, Radius));
        bool bPickedSkillThisPass = false;
        for (int32 Index = Pickups.Num() - 1; Index >= 0; --Index)
        {
            const FCWPickupSnapshot& Pickup = Pickups[Index];
            if (bOwnerOnly && !Pickup.OwnerId.IsEmpty() && Pickup.OwnerId != LocalPickupPlayer->Id)
            {
                continue;
            }

            const FString PickupKey = CWRunView::PickupRenderKey(Pickup, bStableGroundPickup);
            FVector2D PickupRenderPos(Pickup.X, Pickup.Y);
            if (const FRenderEntityState* RenderState = RenderStore.Find(PickupKey))
            {
                if (RenderState->bInitialized)
                {
                    PickupRenderPos = RenderState->Position;
                }
            }

            const bool bAlreadyHidden = HiddenKeys.Contains(PickupKey);
            const float Dx = PickupRenderPos.X - LocalPickupRenderPos.X;
            const float Dy = PickupRenderPos.Y - LocalPickupRenderPos.Y;
            if (Dx * Dx + Dy * Dy > RadiusSq && !bAlreadyHidden)
            {
                continue;
            }
            if (!bAlreadyHidden && Pickup.Kind == TEXT("skill") && bPickedSkillThisPass)
            {
                continue;
            }

            HiddenKeys.Add(PickupKey);
            if (!bAlreadyHidden && Pickup.Kind == TEXT("xp"))
            {
                AddFx(TEXT("xp_absorb"), PickupRenderPos, LocalPickupRenderPos - PickupRenderPos, FLinearColor(0.42f, 0.96f, 1.0f, 0.94f), 0.62f, FMath::Clamp(30.0f + Pickup.Xp * 1.4f, 30.0f, 68.0f), LocalPickupRenderPos);
            }
            else if (!bAlreadyHidden && Pickup.Kind == TEXT("skill"))
            {
                const FLinearColor Accent = SkillOrbAccentColor(Pickup.ItemKey);
                AddFx(TEXT("skill_pickup"), PickupRenderPos, LocalPickupRenderPos - PickupRenderPos, Accent, 0.82f, 82.0f, LocalPickupRenderPos);
                bPickedSkillThisPass = true;
            }
            else if (!bAlreadyHidden && !Pickup.ItemKey.IsEmpty())
            {
                const FLinearColor Accent = CWRunView::LootDropAccentColor(Pickup.ItemKey);
                AddFx(TEXT("loot_pickup"), PickupRenderPos, LocalPickupRenderPos - PickupRenderPos, Accent, 0.70f, 66.0f, LocalPickupRenderPos);
            }
            if (!bAlreadyHidden && Pickup.Kind == TEXT("skill") && InGI && !Pickup.ItemKey.IsEmpty())
            {
                InGI->SendSkillPick(Pickup.ItemKey);
            }
            Pickups.RemoveAtSwap(Index);
        }

        if (HiddenKeys.Num() > 2048)
        {
            HiddenKeys.Reset();
        }
    };

    const float XpHideRadius = 30.0f;
    const float DropHideRadius = 46.0f;
    const float SkillHideRadius = 54.0f;

    RemoveCollectedNearLocalPlayer(MergedState.XpOrbs, XpHideRadius, false);
    RemoveCollectedNearLocalPlayer(MergedState.Drops, DropHideRadius, false);
    HideCollectedRenderPickups(MergedState.XpOrbs, RenderXpOrbs, HiddenXpOrbKeys, XpHideRadius, false, false, nullptr);
    HideCollectedRenderPickups(MergedState.Drops, RenderDrops, HiddenDropKeys, DropHideRadius, true, false, nullptr);
    HideCollectedRenderPickups(MergedState.SkillOrbs, RenderSkillOrbs, HiddenSkillOrbKeys, SkillHideRadius, true, true, GI);

    EmitStateTransitionFx(CachedState, MergedState);
    CachedState = MergedState;
    InvalidateLayoutAndVolatility();
}

void UCWNativeRunViewWidget::UpdateRenderPlayers(float DeltaTime)
{
    const double Now = CachedState.ServerNowMs > 0.0 ? CachedState.ServerNowMs : FPlatformTime::Seconds() * 1000.0;
    const double NowLocal = FPlatformTime::Seconds();
    TSet<FString> AliveIds;

    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        if (Player.Id.IsEmpty() || !Player.bAlive)
        {
            continue;
        }

        AliveIds.Add(Player.Id);
        FRenderPlayerState& RenderState = RenderPlayers.FindOrAdd(Player.Id);
        const FVector2D Target(Player.X, Player.Y);
        if (!RenderState.bInitialized)
        {
            RenderState.Position = Target;
            RenderState.Velocity = FVector2D::ZeroVector;
            RenderState.TargetPosition = Target;
            RenderState.LastServerPosition = Target;
            RenderState.LastServerNowMs = Now;
            RenderState.LastLocalSampleSeconds = NowLocal;
            RenderState.bInitialized = true;
            continue;
        }

        const bool bNewServerSample = FMath::Abs(RenderState.LastServerNowMs - Now) > 0.5 || FVector2D::DistSquared(RenderState.TargetPosition, Target) > 0.25f;
        if (bNewServerSample)
        {
            const float SampleDt = FMath::Clamp(static_cast<float>((Now - RenderState.LastServerNowMs) / 1000.0), 0.025f, 0.25f);
            const FVector2D SampleVelocity = (Target - RenderState.LastServerPosition) / SampleDt;
            RenderState.Velocity = RenderState.Velocity * 0.35f + SampleVelocity * 0.65f;
            RenderState.TargetPosition = Target;
            RenderState.LastServerPosition = Target;
            RenderState.LastServerNowMs = Now;
            RenderState.LastLocalSampleSeconds = NowLocal;
        }

        const FVector2D Previous = RenderState.Position;
        const bool bIsLocalPlayer = !CachedMyId.IsEmpty() && Player.Id == CachedMyId;
        const float ServerAge = FMath::Clamp(static_cast<float>(NowLocal - RenderState.LastLocalSampleSeconds), 0.0f, 0.14f);
        const float Lead = bIsLocalPlayer ? 0.018f : 0.060f;
        FVector2D SmoothTarget = RenderState.TargetPosition + RenderState.Velocity * FMath::Clamp(ServerAge + Lead, 0.0f, bIsLocalPlayer ? 0.055f : 0.12f);
        const FVector2D PredictOffset = SmoothTarget - RenderState.TargetPosition;
        const float MaxPredict = bIsLocalPlayer ? 48.0f : 96.0f;
        if (PredictOffset.SizeSquared() > FMath::Square(MaxPredict))
        {
            SmoothTarget = RenderState.TargetPosition + PredictOffset.GetSafeNormal() * MaxPredict;
        }

        const FVector2D Delta = SmoothTarget - Previous;
        const float Dist = Delta.Size();
        const bool bDodgeActive = Player.DodgeInvulnUntil > Now;
        if (Dist > 620.0f && !bDodgeActive)
        {
            RenderState.Position = RenderState.TargetPosition;
            RenderState.Velocity = FVector2D::ZeroVector;
            continue;
        }

        const float Rate = bDodgeActive ? 30.0f : (bIsLocalPlayer ? 15.0f : 9.0f);
        const float Alpha = 1.0f - FMath::Exp(-Rate * FMath::Max(0.0f, DeltaTime));
        const FVector2D OldVelocity = RenderState.Velocity;
        RenderState.Position += Delta * FMath::Clamp(Alpha, bDodgeActive ? 0.42f : 0.0f, bDodgeActive ? 0.94f : 0.58f);
        const FVector2D FrameVelocity = (RenderState.Position - Previous) / FMath::Max(0.001f, DeltaTime);
        RenderState.Velocity = OldVelocity * 0.88f + FrameVelocity * 0.12f;

        if (bDodgeActive && Dist > 8.0f && Now - RenderState.LastTrailAt >= 42.0)
        {
            const FVector2D Dir = Delta.GetSafeNormal();
            AddFx(TEXT("dodge"), Previous, Dir, FLinearColor(0.72f, 0.86f, 1.0f, 0.80f), 0.34f, 34.0f, RenderState.Position);
            RenderState.LastTrailAt = Now;
        }
    }

    for (auto It = RenderPlayers.CreateIterator(); It; ++It)
    {
        if (!AliveIds.Contains(It.Key()))
        {
            It.RemoveCurrent();
        }
    }
}

void UCWNativeRunViewWidget::UpdateRenderEntities(float DeltaTime)
{
    const float Dt = FMath::Clamp(DeltaTime, 0.0f, 0.08f);
    const double NowLocal = FPlatformTime::Seconds();
    const double StateNowMs = CachedState.ServerNowMs > 0.0 ? CachedState.ServerNowMs : NowLocal * 1000.0;
    TSet<FString> AliveEnemyIds;
    for (const FCWEnemySnapshot& Enemy : CachedState.Enemies)
    {
        if (Enemy.Id.IsEmpty())
        {
            continue;
        }

        AliveEnemyIds.Add(Enemy.Id);
        FRenderEntityState& RenderState = RenderEnemies.FindOrAdd(Enemy.Id);
        const FVector2D Target(Enemy.X, Enemy.Y);
        if (!RenderState.bInitialized)
        {
            RenderState.Position = Target;
            RenderState.Velocity = FVector2D::ZeroVector;
            RenderState.TargetPosition = Target;
            RenderState.LastServerPosition = Target;
            RenderState.LastServerNowMs = StateNowMs;
            RenderState.LastLocalSampleSeconds = NowLocal;
            RenderState.bInitialized = true;
            continue;
        }

        const bool bNewServerSample = FMath::Abs(RenderState.LastServerNowMs - StateNowMs) > 0.5 || FVector2D::DistSquared(RenderState.TargetPosition, Target) > 0.25f;
        if (bNewServerSample)
        {
            const float SampleDt = FMath::Clamp(static_cast<float>((StateNowMs - RenderState.LastServerNowMs) / 1000.0), 0.025f, 0.25f);
            const FVector2D SampleVelocity = (Target - RenderState.LastServerPosition) / SampleDt;
            RenderState.Velocity = RenderState.Velocity * 0.30f + SampleVelocity * 0.70f;
            RenderState.LastServerPosition = Target;
            RenderState.TargetPosition = Target;
            RenderState.LastServerNowMs = StateNowMs;
            RenderState.LastLocalSampleSeconds = NowLocal;
        }

        const FVector2D Previous = RenderState.Position;
        const float ServerAge = FMath::Clamp(static_cast<float>(NowLocal - RenderState.LastLocalSampleSeconds), 0.0f, 0.14f);
        FVector2D PredictedTarget = RenderState.TargetPosition + RenderState.Velocity * FMath::Clamp(ServerAge + 0.035f, 0.0f, 0.11f);
        const FVector2D PredictOffset = PredictedTarget - RenderState.TargetPosition;
        if (PredictOffset.SizeSquared() > FMath::Square(86.0f))
        {
            PredictedTarget = RenderState.TargetPosition + PredictOffset.GetSafeNormal() * 86.0f;
        }

        const FVector2D Delta = PredictedTarget - Previous;
        if (Delta.SizeSquared() > FMath::Square(520.0f))
        {
            RenderState.Position = RenderState.TargetPosition;
            RenderState.Velocity = FVector2D::ZeroVector;
            continue;
        }

        const float Alpha = 1.0f - FMath::Exp(-10.5f * Dt);
        const FVector2D OldVelocity = RenderState.Velocity;
        RenderState.Position += Delta * FMath::Clamp(Alpha, 0.0f, 0.54f);
        const FVector2D FrameVelocity = (RenderState.Position - Previous) / FMath::Max(0.001f, Dt);
        RenderState.Velocity = OldVelocity * 0.86f + FrameVelocity * 0.14f;
    }

    for (auto It = RenderEnemies.CreateIterator(); It; ++It)
    {
        if (!AliveEnemyIds.Contains(It.Key()))
        {
            It.RemoveCurrent();
        }
    }

    TSet<FString> LiveBulletIds;
    for (const FCWBulletSnapshot& Bullet : CachedState.Bullets)
    {
        if (Bullet.Id.IsEmpty())
        {
            continue;
        }

        LiveBulletIds.Add(Bullet.Id);
        FRenderEntityState& RenderState = RenderBullets.FindOrAdd(Bullet.Id);
        const FVector2D Target(Bullet.X, Bullet.Y);
        const FVector2D ServerVelocity(Bullet.Vx, Bullet.Vy);
        if (!RenderState.bInitialized)
        {
            RenderState.Position = Target;
            RenderState.Velocity = ServerVelocity;
            RenderState.TargetPosition = Target;
            RenderState.LastServerPosition = Target;
            RenderState.LastServerNowMs = StateNowMs;
            RenderState.LastLocalSampleSeconds = NowLocal;
            RenderState.bInitialized = true;
            continue;
        }

        const bool bNewServerSample = FMath::Abs(RenderState.LastServerNowMs - StateNowMs) > 0.5 || FVector2D::DistSquared(RenderState.TargetPosition, Target) > 0.25f;
        if (bNewServerSample)
        {
            RenderState.TargetPosition = Target;
            RenderState.LastServerPosition = Target;
            RenderState.LastServerNowMs = StateNowMs;
            RenderState.LastLocalSampleSeconds = NowLocal;
        }

        RenderState.Velocity = ServerVelocity.IsNearlyZero() ? RenderState.Velocity * 0.82f : ServerVelocity;
        const float ServerAge = FMath::Clamp(static_cast<float>(NowLocal - RenderState.LastLocalSampleSeconds), 0.0f, 0.16f);
        FVector2D PredictedTarget = RenderState.TargetPosition;
        if (!RenderState.Velocity.IsNearlyZero())
        {
            PredictedTarget += RenderState.Velocity * FMath::Clamp(ServerAge + 0.055f, 0.0f, 0.14f);
        }

        const FVector2D Previous = RenderState.Position;
        const FVector2D PredictedMotion = RenderState.Position + RenderState.Velocity * Dt;
        const FVector2D Correction = PredictedTarget - PredictedMotion;
        if (Correction.SizeSquared() > FMath::Square(780.0f))
        {
            RenderState.Position = Target;
            RenderState.Velocity = ServerVelocity;
            RenderState.TargetPosition = Target;
            continue;
        }

        const float Alpha = 1.0f - FMath::Exp(-16.0f * Dt);
        RenderState.Position = PredictedMotion + Correction * FMath::Clamp(Alpha, 0.0f, 0.62f);
        if (ServerVelocity.IsNearlyZero())
        {
            RenderState.Velocity = (RenderState.Position - Previous) / FMath::Max(0.001f, Dt);
        }
    }

    for (auto It = RenderBullets.CreateIterator(); It; ++It)
    {
        if (!LiveBulletIds.Contains(It.Key()))
        {
            It.RemoveCurrent();
        }
    }

    auto UpdatePickupRenderState = [Dt](const TArray<FCWPickupSnapshot>& Pickups, TMap<FString, FRenderEntityState>& RenderStore, float Rate, float SnapDistance, bool bStableGroundPickup)
    {
        TSet<FString> LiveIds;
        for (const FCWPickupSnapshot& Pickup : Pickups)
        {
            const FString PickupId = CWRunView::PickupRenderKey(Pickup, bStableGroundPickup);
            LiveIds.Add(PickupId);
            FRenderEntityState& RenderState = RenderStore.FindOrAdd(PickupId);
            const FVector2D Target(Pickup.X, Pickup.Y);
            if (!RenderState.bInitialized)
            {
                RenderState.Position = Target;
                RenderState.Velocity = FVector2D::ZeroVector;
                RenderState.TargetPosition = Target;
                RenderState.bInitialized = true;
                continue;
            }

            const FVector2D Previous = RenderState.Position;
            const FVector2D Delta = Target - Previous;
            RenderState.TargetPosition = Target;
            if (Delta.SizeSquared() > FMath::Square(SnapDistance))
            {
                RenderState.Position = Target;
                RenderState.Velocity = FVector2D::ZeroVector;
                continue;
            }

            const float Alpha = 1.0f - FMath::Exp(-Rate * Dt);
            RenderState.Position += Delta * FMath::Clamp(Alpha, 0.0f, 0.9f);
            RenderState.Velocity = (RenderState.Position - Previous) / FMath::Max(0.001f, Dt);
        }

        for (auto It = RenderStore.CreateIterator(); It; ++It)
        {
            if (!LiveIds.Contains(It.Key()))
            {
                It.RemoveCurrent();
            }
        }
    };

    UpdatePickupRenderState(CachedState.Drops, RenderDrops, 12.0f, 520.0f, true);
    UpdatePickupRenderState(CachedState.XpOrbs, RenderXpOrbs, 22.0f, 360.0f, false);
    UpdatePickupRenderState(CachedState.SkillOrbs, RenderSkillOrbs, 13.0f, 480.0f, true);
}

void UCWNativeRunViewWidget::UpdateTransientFx(float DeltaTime)
{
    for (int32 Index = TransientFx.Num() - 1; Index >= 0; --Index)
    {
        TransientFx[Index].Age += DeltaTime;
        if (TransientFx[Index].Age >= TransientFx[Index].Life)
        {
            TransientFx.RemoveAtSwap(Index);
        }
    }
}

void UCWNativeRunViewWidget::AddFx(const FString& Type, const FVector2D& Position, const FVector2D& Direction, const FLinearColor& Color, float Life, float Radius, const FVector2D& EndPosition)
{
    FTransientFx Fx;
    Fx.Type = Type;
    Fx.Position = Position;
    Fx.EndPosition = EndPosition.IsNearlyZero() ? Position : EndPosition;
    Fx.Direction = Direction.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : Direction.GetSafeNormal();
    Fx.Color = Color;
    Fx.Life = FMath::Max(0.05f, Life);
    Fx.Radius = FMath::Max(1.0f, Radius);
    TransientFx.Add(Fx);
    constexpr int32 MaxTransientFx = 720;
    if (TransientFx.Num() > MaxTransientFx)
    {
        TransientFx.RemoveAt(0, TransientFx.Num() - MaxTransientFx);
    }
}

void UCWNativeRunViewWidget::HandleSkillFxReceived(const FCWSkillFxEvent& Event)
{
    const FString EventId = Event.Id.IsEmpty()
        ? FString::Printf(TEXT("skill:%s:%s:%.0f:%.0f:%d"), *Event.PlayerId, *Event.SkillId, Event.X, Event.Y, Event.Level)
        : Event.Id;
    if (!EventId.IsEmpty())
    {
        if (SeenSkillFxEventIds.Contains(EventId))
        {
            return;
        }
        SeenSkillFxEventIds.Add(EventId);
        if (SeenSkillFxEventIds.Num() > 1024)
        {
            SeenSkillFxEventIds.Reset();
        }
    }

    EmitSkillFxEvent(Event);
}

void UCWNativeRunViewWidget::HandleMeleeFxReceived(const FCWMeleeFxEvent& Event)
{
    const FString EventId = Event.Id.IsEmpty()
        ? FString::Printf(TEXT("melee:%s:%s:%.0f:%.0f:%d"), *Event.PlayerId, *Event.ItemId, Event.X, Event.Y, Event.HitCount)
        : Event.Id;
    if (!EventId.IsEmpty())
    {
        if (SeenMeleeFxEventIds.Contains(EventId))
        {
            return;
        }
        SeenMeleeFxEventIds.Add(EventId);
        if (SeenMeleeFxEventIds.Num() > 1024)
        {
            SeenMeleeFxEventIds.Reset();
        }
    }

    EmitMeleeFxEvent(Event);
}

void UCWNativeRunViewWidget::HandleWorldFxReceived(const FCWWorldFxEvent& Event)
{
    const FString EventId = Event.Id.IsEmpty()
        ? FString::Printf(TEXT("world:%s:%s:%.0f:%.0f"), *Event.Kind, *Event.FxKey, Event.X, Event.Y)
        : Event.Id;
    if (!EventId.IsEmpty())
    {
        if (SeenWorldFxEventIds.Contains(EventId))
        {
            return;
        }
        SeenWorldFxEventIds.Add(EventId);
        if (SeenWorldFxEventIds.Num() > 512)
        {
            SeenWorldFxEventIds.Reset();
        }
    }

    EmitWorldFxEvent(Event);
}

void UCWNativeRunViewWidget::EmitSkillFxEvent(const FCWSkillFxEvent& Event)
{
    const FString CastKey = FString::Printf(TEXT("%s %s %s %s"), *Event.CastType, *Event.SkillId, *Event.SkillName, *Event.FxKey);
    const FVector2D Origin(Event.X, Event.Y);
    FVector2D Dir(Event.AimX - Event.X, Event.AimY - Event.Y);
    if (Dir.IsNearlyZero())
    {
        Dir = FVector2D(1.0f, 0.0f);
    }
    Dir = Dir.GetSafeNormal();

    const FLinearColor Primary = CWRunView::HexColor(Event.Color, FLinearColor(0.42f, 0.88f, 1.0f, 0.96f));
    const FLinearColor Secondary = CWRunView::HexColor(Event.SecondaryColor, FLinearColor(0.96f, 0.72f, 1.0f, 0.92f));
    const float Radius = FMath::Clamp(Event.Radius > 1.0f ? Event.Radius : 260.0f, 96.0f, 980.0f);

    if (CastKey.Contains(TEXT("magnet"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("surge"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("pull"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("world_xp_surge"), Origin, Dir, FLinearColor(0.68f, 0.30f, 1.0f, 0.96f), 1.25f, Radius * 1.18f);
        AddFx(TEXT("world_xp_surge_core"), Origin, Dir, FLinearColor(0.92f, 0.74f, 1.0f, 0.94f), 0.92f, FMath::Max(120.0f, Radius * 0.40f));
        return;
    }

    if (CastKey.Contains(TEXT("shield"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("barrier"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("guard"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_psi"), Origin, Dir, FLinearColor(0.34f, 1.0f, 0.86f, 0.92f), 1.05f, Radius * 0.86f);
        AddFx(TEXT("skill_psi_core"), Origin, Dir, FLinearColor(0.92f, 1.0f, 0.96f, 0.88f), 0.84f, Radius * 0.38f);
        return;
    }

    if (CastKey.Contains(TEXT("regen"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("heal"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("vital"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_psi"), Origin, Dir, FLinearColor(0.28f, 1.0f, 0.48f, 0.92f), 1.00f, Radius * 0.72f);
        AddFx(TEXT("skill_burst"), Origin, Dir, FLinearColor(0.84f, 1.0f, 0.72f, 0.86f), 0.70f, Radius * 0.38f);
        return;
    }

    if (CastKey.Contains(TEXT("reload"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("haste"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("speed"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_burst"), Origin, Dir, FLinearColor(1.0f, 0.86f, 0.24f, 0.92f), 0.72f, Radius * 0.48f);
        AddFx(TEXT("skill_chain_origin"), Origin, Dir, FLinearColor(1.0f, 0.96f, 0.62f, 0.80f), 0.46f, Radius * 0.30f);
        return;
    }

    auto AddTargetBeams = [&](const FString& BeamType, const FString& HitType, float BeamLife, float HitRadius, bool bChain)
    {
        FVector2D Previous = Origin;
        int32 Index = 0;
        for (const FCWSkillFxTarget& Target : Event.Targets)
        {
            const FVector2D TargetPos(Target.X, Target.Y);
            const FLinearColor BeamColor = (Index % 2 == 0) ? Primary : Secondary;
            AddFx(BeamType, Previous, TargetPos - Previous, BeamColor, BeamLife, FMath::Max(110.0f, Radius * 0.22f), TargetPos);
            AddFx(HitType, TargetPos, TargetPos - Previous, (Index % 2 == 0) ? Secondary : Primary, BeamLife * 1.28f, HitRadius);
            Previous = bChain ? TargetPos : Origin;
            ++Index;
        }
    };

    if (CastKey.Contains(TEXT("shockwave"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("stomp"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_shockwave"), Origin, Dir, Primary, 1.18f, Radius * 1.18f);
        AddFx(TEXT("skill_shockwave_inner"), Origin, Dir, Secondary, 0.86f, Radius * 0.62f);
        AddTargetBeams(TEXT("skill_chain_beam"), TEXT("skill_hit"), 0.34f, 54.0f, false);
        return;
    }

    if (CastKey.Contains(TEXT("psi"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("wave"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("pulse"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_psi"), Origin, Dir, Primary, 1.10f, Radius * 1.22f);
        AddFx(TEXT("skill_psi_core"), Origin, Dir, Secondary, 0.82f, Radius * 0.44f);
        AddTargetBeams(TEXT("skill_laser_beam"), TEXT("skill_hit"), 0.38f, 58.0f, false);
        return;
    }

    if (CastKey.Contains(TEXT("chain"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("lightning"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("arc"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_chain_origin"), Origin, Dir, Primary, 0.66f, 150.0f);
        if (Event.Targets.Num() > 0)
        {
            AddTargetBeams(TEXT("skill_chain_beam"), TEXT("skill_chain_hit"), 0.66f, 68.0f, true);
        }
        else
        {
            for (int32 I = 0; I < 6; ++I)
            {
                const float Angle = FMath::Atan2(Dir.Y, Dir.X) + (static_cast<float>(I) - 2.5f) * 0.38f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                AddFx(TEXT("skill_chain_beam"), Origin, Ray, I % 2 == 0 ? Primary : Secondary, 0.58f, 140.0f, Origin + Ray * Radius * (0.64f + I * 0.05f));
            }
        }
        return;
    }

    if (CastKey.Contains(TEXT("laser"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("lance"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("beam"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_laser_cast"), Origin, Dir, Secondary, 0.52f, 128.0f);
        if (Event.Targets.Num() > 0)
        {
            AddTargetBeams(TEXT("skill_laser_beam"), TEXT("skill_laser_hit"), 0.56f, 84.0f, false);
        }
        else
        {
            AddFx(TEXT("skill_laser_beam"), Origin, Dir, Primary, 0.58f, Radius, Origin + Dir * Radius);
            AddFx(TEXT("skill_laser_hit"), Origin + Dir * Radius, Dir, Secondary, 0.54f, 88.0f);
        }
        return;
    }

    if (CastKey.Contains(TEXT("missile"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("rocket"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("barrage"), ESearchCase::IgnoreCase))
    {
        const int32 Count = FMath::Clamp(Event.ProjectileCount > 0 ? Event.ProjectileCount : FMath::Max(4, Event.Targets.Num()), 4, 12);
        AddFx(TEXT("skill_missile_launch"), Origin, Dir, Primary, 0.72f, 190.0f);
        for (int32 I = 0; I < Count; ++I)
        {
            const float Angle = FMath::Atan2(Dir.Y, Dir.X) + (static_cast<float>(I) - (Count - 1) * 0.5f) * 0.20f;
            const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
            AddFx(TEXT("skill_missile_trail"), Origin + Ray * 18.0f, Ray, I % 2 == 0 ? Primary : Secondary, 0.62f, 150.0f, Origin + Ray * (180.0f + I * 8.0f));
        }
        for (const FCWSkillFxTarget& Target : Event.Targets)
        {
            AddFx(TEXT("skill_missile_lock"), FVector2D(Target.X, Target.Y), Dir, Secondary, 0.72f, 74.0f);
        }
        return;
    }

    if (CastKey.Contains(TEXT("blade"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("slash"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("orbit"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_blade_orbit"), Origin, Dir, Primary, 0.86f, FMath::Max(180.0f, Radius * 0.48f));
        if (Event.Targets.Num() > 0)
        {
            for (const FCWSkillFxTarget& Target : Event.Targets)
            {
                const FVector2D TargetPos(Target.X, Target.Y);
                AddFx(TEXT("skill_blade_slash"), TargetPos - Dir * 64.0f, Dir, Secondary, 0.54f, 128.0f, TargetPos + Dir * 64.0f);
                AddFx(TEXT("skill_hit"), TargetPos, Dir, Primary, 0.40f, 46.0f);
            }
        }
        else
        {
            AddFx(TEXT("skill_blade_slash"), Origin - Dir * 116.0f, Dir, Secondary, 0.58f, 160.0f, Origin + Dir * 116.0f);
        }
        return;
    }

    AddFx(TEXT("skill_burst"), Origin, Dir, Primary, 0.82f, FMath::Max(150.0f, Radius * 0.50f));
    AddTargetBeams(TEXT("skill_laser_beam"), TEXT("skill_hit"), 0.34f, 48.0f, false);
}

void UCWNativeRunViewWidget::EmitMeleeFxEvent(const FCWMeleeFxEvent& Event)
{
    const FString Key = FString::Printf(TEXT("%s %s %s %s"), *Event.Style, *Event.ItemId, *Event.ItemName, *Event.FxKey);
    const FVector2D Origin(Event.X, Event.Y);
    FVector2D Dir(FMath::Cos(Event.Angle), FMath::Sin(Event.Angle));
    if (Dir.IsNearlyZero())
    {
        Dir = FVector2D(1.0f, 0.0f);
    }
    Dir = Dir.GetSafeNormal();
    const FVector2D Impact = FVector2D(Event.ImpactX, Event.ImpactY).IsNearlyZero()
        ? Origin + Dir * FMath::Max(Event.Range, 96.0f)
        : FVector2D(Event.ImpactX, Event.ImpactY);
    const FLinearColor Primary = CWRunView::HexColor(Event.Color, FLinearColor(1.0f, 0.72f, 0.22f, 0.96f));
    const FLinearColor Secondary = CWRunView::HexColor(Event.SecondaryColor, FLinearColor(0.98f, 0.98f, 1.0f, 0.92f));
    const float Radius = FMath::Clamp(Event.Range > 1.0f ? Event.Range : 150.0f, 72.0f, 420.0f);

    AddFx(TEXT("skill_melee_arc"), Origin, Dir, Primary, 0.48f, Radius, Impact);
    if (Key.Contains(TEXT("chainsaw"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("saw"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_saw"), Origin, Dir, Primary, 0.66f, Radius, Impact);
        AddFx(TEXT("skill_melee_sparks"), Impact, Dir, Secondary, 0.58f, 92.0f);
    }
    else if (Key.Contains(TEXT("hammer"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("maul"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_hammer"), Impact, Dir, Primary, 0.72f, FMath::Max(132.0f, Radius * 0.72f));
    }
    else if (Key.Contains(TEXT("bat"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("club"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_bat"), Origin, Dir, Primary, 0.52f, Radius, Impact);
    }
    else if (Key.Contains(TEXT("cryo"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("ice"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_cryo"), Impact, Dir, FLinearColor(0.62f, 0.90f, 1.0f, 0.94f), 0.74f, FMath::Max(116.0f, Radius * 0.62f));
    }
    else if (Key.Contains(TEXT("plasma"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_plasma"), Origin, Dir, Primary, 0.62f, Radius, Impact);
    }
    else if (Key.Contains(TEXT("mono"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("wire"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_wire"), Origin, Dir, Primary, 0.62f, Radius * 1.18f, Impact);
    }
    else if (Key.Contains(TEXT("void"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("scythe"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("skill_melee_void"), Impact, Dir, FLinearColor(0.72f, 0.38f, 1.0f, 0.95f), 0.78f, FMath::Max(140.0f, Radius * 0.75f));
    }
    else
    {
        AddFx(TEXT("skill_blade_slash"), Origin - Dir * 36.0f, Dir, Secondary, 0.48f, Radius, Impact);
    }
}

void UCWNativeRunViewWidget::EmitWorldFxEvent(const FCWWorldFxEvent& Event)
{
    const FString Key = FString::Printf(TEXT("%s %s"), *Event.Kind, *Event.FxKey);
    const FVector2D Origin(Event.X, Event.Y);
    const FLinearColor Primary = CWRunView::HexColor(Event.Color, FLinearColor(0.68f, 0.30f, 1.0f, 0.94f));
    const FLinearColor Secondary = CWRunView::HexColor(Event.SecondaryColor, FLinearColor(0.94f, 0.74f, 1.0f, 0.90f));
    const float Radius = FMath::Clamp(Event.Radius > 1.0f ? Event.Radius : 360.0f, 120.0f, 1100.0f);

    if (Key.Contains(TEXT("xp_surge"), ESearchCase::IgnoreCase) || Key.Contains(TEXT("magnet"), ESearchCase::IgnoreCase))
    {
        AddFx(TEXT("world_xp_surge"), Origin, FVector2D(1.0f, 0.0f), Primary, 1.25f, Radius);
        AddFx(TEXT("world_xp_surge_core"), Origin, FVector2D(1.0f, 0.0f), Secondary, 0.92f, FMath::Max(110.0f, Radius * 0.34f));
        return;
    }

    AddFx(TEXT("world_pulse"), Origin, FVector2D(1.0f, 0.0f), Primary, 0.86f, Radius * 0.62f);
}

void UCWNativeRunViewWidget::EmitStateTransitionFx(const FCWRoomSnapshot& PreviousState, const FCWRoomSnapshot& NextState)
{
    const bool bHasPrevious = PreviousState.ServerNowMs > 0.0 || PreviousState.Players.Num() > 0 || PreviousState.Bullets.Num() > 0;
    if (!bHasPrevious)
    {
        PreviousEnemyHp.Reset();
        PreviousBulletPositions.Reset();
        for (const FCWEnemySnapshot& Enemy : NextState.Enemies)
        {
            if (!Enemy.Id.IsEmpty())
            {
                PreviousEnemyHp.Add(Enemy.Id, Enemy.Hp);
            }
        }
        for (const FCWBulletSnapshot& Bullet : NextState.Bullets)
        {
            if (!Bullet.Id.IsEmpty())
            {
                PreviousBulletPositions.Add(Bullet.Id, FVector2D(Bullet.X, Bullet.Y));
            }
        }
        PreviousSkillCooldowns.Reset();
        for (const FCWPlayerSnapshot& Player : NextState.Players)
        {
            for (const FCWSkillSnapshot& Skill : Player.Skills)
            {
                if (Skill.Level > 0 && !Skill.Id.IsEmpty())
                {
                    PreviousSkillCooldowns.Add(FString::Printf(TEXT("%s:%s"), *Player.Id, *Skill.Id), Skill.CooldownMs);
                }
            }
        }
        return;
    }

    auto FindLocalFxPlayer = [&]() -> const FCWPlayerSnapshot*
    {
        const FCWPlayerSnapshot* FirstLivePlayer = nullptr;
        for (const FCWPlayerSnapshot& Player : NextState.Players)
        {
            if (!Player.bAlive || Player.bIsCompanion)
            {
                continue;
            }
            if (!FirstLivePlayer)
            {
                FirstLivePlayer = &Player;
            }
            if (!CachedMyId.IsEmpty() && Player.Id == CachedMyId)
            {
                return &Player;
            }
        }
        return FirstLivePlayer;
    };

    auto ResolveBulletBloodDir = [&](const FVector2D& HitPos, const FVector2D& Fallback) -> FVector2D
    {
        FVector2D BestDir = Fallback;
        float BestScore = TNumericLimits<float>::Max();
        for (const FCWBulletSnapshot& Bullet : PreviousState.Bullets)
        {
            if (Bullet.bFromEnemy)
            {
                continue;
            }
            FVector2D Dir(Bullet.Vx, Bullet.Vy);
            if (Dir.IsNearlyZero())
            {
                continue;
            }
            Dir = Dir.GetSafeNormal();
            const FVector2D FromBullet = HitPos - FVector2D(Bullet.X, Bullet.Y);
            const float Along = FVector2D::DotProduct(FromBullet, Dir);
            if (Along < -28.0f || Along > 260.0f)
            {
                continue;
            }
            const float Side = FMath::Abs(FromBullet.X * Dir.Y - FromBullet.Y * Dir.X);
            const float Score = Side + FMath::Max(0.0f, Along) * 0.06f;
            if (Side <= 92.0f && Score < BestScore)
            {
                BestScore = Score;
                BestDir = Dir;
            }
        }

        if (BestDir.IsNearlyZero())
        {
            BestDir = Fallback.IsNearlyZero() ? FVector2D(1.0f, -0.18f) : Fallback.GetSafeNormal();
        }
        return (BestDir.GetSafeNormal() * 0.94f + FVector2D(0.0f, -0.34f)).GetSafeNormal();
    };

    auto ResolveBulletForwardDir = [&](const FVector2D& HitPos, const FVector2D& Fallback) -> FVector2D
    {
        FVector2D BestDir = Fallback;
        float BestScore = TNumericLimits<float>::Max();
        for (const FCWBulletSnapshot& Bullet : PreviousState.Bullets)
        {
            if (Bullet.bFromEnemy)
            {
                continue;
            }
            FVector2D Dir(Bullet.Vx, Bullet.Vy);
            if (Dir.IsNearlyZero())
            {
                continue;
            }
            Dir = Dir.GetSafeNormal();
            const FVector2D FromBullet = HitPos - FVector2D(Bullet.X, Bullet.Y);
            const float Along = FVector2D::DotProduct(FromBullet, Dir);
            if (Along < -36.0f || Along > 280.0f)
            {
                continue;
            }
            const float Side = FMath::Abs(FromBullet.X * Dir.Y - FromBullet.Y * Dir.X);
            const float Score = Side + FMath::Max(0.0f, Along) * 0.05f;
            if (Side <= 104.0f && Score < BestScore)
            {
                BestScore = Score;
                BestDir = Dir;
            }
        }

        return BestDir.IsNearlyZero()
            ? (Fallback.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : Fallback.GetSafeNormal())
            : BestDir.GetSafeNormal();
    };

    TMap<FString, FCWBulletSnapshot> PreviousBullets;
    for (const FCWBulletSnapshot& Bullet : PreviousState.Bullets)
    {
        if (!Bullet.Id.IsEmpty())
        {
            PreviousBullets.Add(Bullet.Id, Bullet);
        }
    }

    auto NormalizeExplosionMaterial = [](const FString& Material) -> FString
    {
        const FString Key = Material.ToLower();
        if (Key.Contains(TEXT("asphalt")))
        {
            return TEXT("asphalt");
        }
        if (Key.Contains(TEXT("grass")))
        {
            return TEXT("grass");
        }
        if (Key.Contains(TEXT("dirt")) || Key.Contains(TEXT("soil")) || Key.Contains(TEXT("earth")))
        {
            return TEXT("dirt");
        }
        if (Key.Contains(TEXT("metal")))
        {
            return TEXT("metal");
        }
        if (Key.Contains(TEXT("wood")))
        {
            return TEXT("wood");
        }
        return TEXT("concrete");
    };

    auto ResolveSurfaceMaterial = [&](const FVector2D& Pos) -> FString
    {
        for (int32 Index = NextState.TerrainZones.Num() - 1; Index >= 0; --Index)
        {
            const FCWTerrainZoneSnapshot& Zone = NextState.TerrainZones[Index];
            if (Zone.Material.IsEmpty() || Zone.W <= 0.0f || Zone.H <= 0.0f || Zone.Alpha <= 0.08f)
            {
                continue;
            }

            const float LocalX = Pos.X - Zone.X;
            const float LocalY = Pos.Y - Zone.Y;
            const float HalfW = Zone.W * 0.5f;
            const float HalfH = Zone.H * 0.5f;
            const bool bInside = Zone.Shape.Equals(TEXT("ellipse"), ESearchCase::IgnoreCase)
                ? (FMath::Square(LocalX / FMath::Max(1.0f, HalfW)) + FMath::Square(LocalY / FMath::Max(1.0f, HalfH)) <= 1.0f)
                : (FMath::Abs(LocalX) <= HalfW && FMath::Abs(LocalY) <= HalfH);
            if (bInside)
            {
                return Zone.Material;
            }
        }

        return NextState.SceneTheme.BaseMaterial.IsEmpty() ? TEXT("concrete") : NextState.SceneTheme.BaseMaterial;
    };

    auto MakeExplosionType = [&](const FString& Material) -> FString
    {
        const FString Suffix = NormalizeExplosionMaterial(Material);
        return FString::Printf(TEXT("explosion_%s"), *Suffix);
    };

    auto MakeBlastScorchType = [&](const FString& Material) -> FString
    {
        const FString Suffix = NormalizeExplosionMaterial(Material);
        return FString::Printf(TEXT("blast_scorch_%s"), *Suffix);
    };

    auto AddRocketExplosionFx = [&](const FString& Material, const FVector2D& Pos, const FVector2D& Dir, const FLinearColor& Color, float Radius)
    {
        const float SafeRadius = FMath::Max(130.0f, Radius);
        AddFx(MakeBlastScorchType(Material), Pos, Dir, CWRunView::MaterialColor(Material, 0.78f), 3.2f, SafeRadius * 0.92f);
        AddFx(MakeExplosionType(Material), Pos, Dir, Color, 1.42f, SafeRadius);
    };

    TSet<FString> CurrentBulletIds;
    for (const FCWBulletSnapshot& Bullet : NextState.Bullets)
    {
        if (Bullet.Id.IsEmpty())
        {
            continue;
        }
        CurrentBulletIds.Add(Bullet.Id);
        const FVector2D CurrentBulletPos(Bullet.X, Bullet.Y);
        const FVector2D* PreviousBulletPos = PreviousBulletPositions.Find(Bullet.Id);
        const bool bRocketBullet = Bullet.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase) || Bullet.ExplosionRadius > 1.0f;
        if (bRocketBullet && PreviousBulletPos && FVector2D::DistSquared(*PreviousBulletPos, CurrentBulletPos) > FMath::Square(4.0f))
        {
            FVector2D TrailDir(Bullet.Vx, Bullet.Vy);
            if (TrailDir.IsNearlyZero())
            {
                TrailDir = CurrentBulletPos - *PreviousBulletPos;
            }
            const FLinearColor TrailColor = Bullet.bFromEnemy
                ? FLinearColor(1.0f, 0.22f, 0.08f, 0.86f)
                : CWRunView::HexColor(Bullet.Color, FLinearColor(1.0f, 0.66f, 0.12f, 0.86f));
            AddFx(TEXT("rocket_trail"), *PreviousBulletPos, TrailDir, TrailColor, 1.18f, FMath::Clamp(FVector2D::Distance(*PreviousBulletPos, CurrentBulletPos), 54.0f, 240.0f), CurrentBulletPos);
        }
        PreviousBulletPositions.Add(Bullet.Id, CurrentBulletPos);
    }

    TMap<FString, float> NextSkillCooldowns;
    for (const FCWPlayerSnapshot& Player : NextState.Players)
    {
        if (!Player.bAlive)
        {
            continue;
        }

        for (const FCWSkillSnapshot& Skill : Player.Skills)
        {
            if (Skill.Level <= 0 || Skill.Id.IsEmpty())
            {
                continue;
            }

            const FString SkillKey = FString::Printf(TEXT("%s:%s"), *Player.Id, *Skill.Id);
            const float PreviousCooldown = PreviousSkillCooldowns.FindRef(SkillKey);
            const bool bKnownSkill = PreviousSkillCooldowns.Contains(SkillKey);
            const bool bActive = Skill.Kind.Equals(TEXT("active"), ESearchCase::IgnoreCase);
            const bool bFreshCooldown = Skill.CooldownMs > 120.0f && (!bKnownSkill || PreviousCooldown <= 85.0f || Skill.CooldownMs > PreviousCooldown + 350.0f);

            if (bActive && bFreshCooldown)
            {
                FCWSkillFxEvent Event;
                Event.Id = FString::Printf(TEXT("fallback:%s:%s:%.0f"), *Player.Id, *Skill.Id, NextState.ServerNowMs);
                Event.PlayerId = Player.Id;
                Event.PlayerName = Player.Name;
                Event.HeroId = Player.PlayerClass;
                Event.SkillId = Skill.Id;
                Event.SkillName = Skill.Name;
                Event.CastType = Skill.CastType.IsEmpty() ? Skill.Id : Skill.CastType;
                Event.FxKey = Skill.FxKey;
                Event.Level = Skill.Level;
                Event.X = Player.X;
                Event.Y = Player.Y;
                Event.AimX = Player.AimX;
                Event.AimY = Player.AimY;

                const FString CastKey = FString::Printf(TEXT("%s %s %s"), *Event.CastType, *Event.SkillId, *Event.FxKey);
                const bool bWide = CastKey.Contains(TEXT("shockwave"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("psi"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("wave"), ESearchCase::IgnoreCase);
                const bool bLaser = CastKey.Contains(TEXT("laser"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("lance"), ESearchCase::IgnoreCase);
                const bool bChain = CastKey.Contains(TEXT("chain"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("lightning"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("arc"), ESearchCase::IgnoreCase);
                const bool bMissile = CastKey.Contains(TEXT("missile"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("rocket"), ESearchCase::IgnoreCase) || CastKey.Contains(TEXT("barrage"), ESearchCase::IgnoreCase);
                Event.Radius = bLaser ? 720.0f : (bChain ? 520.0f : (bWide ? 430.0f : (bMissile ? 360.0f : 280.0f)));
                Event.ProjectileCount = bMissile ? 6 : 0;

                const int32 MaxTargets = bLaser ? 6 : (bChain ? 7 : (bMissile ? 6 : 4));
                for (const FCWEnemySnapshot& Enemy : NextState.Enemies)
                {
                    if (Event.Targets.Num() >= MaxTargets)
                    {
                        break;
                    }
                    const float DistSq = FVector2D::DistSquared(FVector2D(Player.X, Player.Y), FVector2D(Enemy.X, Enemy.Y));
                    if (DistSq > FMath::Square(Event.Radius + 120.0f))
                    {
                        continue;
                    }

                    FCWSkillFxTarget Target;
                    Target.Id = Enemy.Id;
                    Target.Kind = TEXT("enemy");
                    Target.X = Enemy.X;
                    Target.Y = Enemy.Y;
                    Event.Targets.Add(Target);
                }

                EmitSkillFxEvent(Event);
            }

            NextSkillCooldowns.Add(SkillKey, Skill.CooldownMs);
        }
    }
    PreviousSkillCooldowns = MoveTemp(NextSkillCooldowns);

    for (const TPair<FString, FCWBulletSnapshot>& Pair : PreviousBullets)
    {
        if (CurrentBulletIds.Contains(Pair.Key))
        {
            continue;
        }
        const FCWBulletSnapshot& Bullet = Pair.Value;
        const FVector2D Pos = PreviousBulletPositions.FindRef(Pair.Key);
        const FVector2D Dir(Bullet.Vx, Bullet.Vy);
        const FLinearColor Color = Bullet.bFromEnemy ? FLinearColor(1.0f, 0.22f, 0.14f, 0.9f) : CWRunView::HexColor(Bullet.Color, FLinearColor(1.0f, 0.74f, 0.18f, 0.9f));
        const bool bRocketBullet = Bullet.ExplosionRadius > 1.0f || Bullet.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase);
        if (bRocketBullet)
        {
            AddRocketExplosionFx(ResolveSurfaceMaterial(Pos), Pos, Dir, Color, FMath::Max(170.0f, Bullet.ExplosionRadius));
        }
        else
        {
            AddFx(TEXT("impact"), Pos, Dir, Color, 0.25f, 22.0f);
        }
        PreviousBulletPositions.Remove(Pair.Key);
    }

    const FCWPlayerSnapshot* LocalFxPlayer = FindLocalFxPlayer();
    if (LocalFxPlayer)
    {
        TSet<FString> CurrentXpOrbIds;
        for (const FCWPickupSnapshot& Orb : NextState.XpOrbs)
        {
            CurrentXpOrbIds.Add(CWRunView::PickupRenderKey(Orb, false));
        }
        const FVector2D LocalPlayerPos(LocalFxPlayer->X, LocalFxPlayer->Y);
        for (const FCWPickupSnapshot& Orb : PreviousState.XpOrbs)
        {
            const FString OrbKey = CWRunView::PickupRenderKey(Orb, false);
            if (CurrentXpOrbIds.Contains(OrbKey) || HiddenXpOrbKeys.Contains(OrbKey))
            {
                continue;
            }
            const FVector2D OrbPos(Orb.X, Orb.Y);
            if ((OrbPos - LocalPlayerPos).SizeSquared() <= FMath::Square(640.0f))
            {
                AddFx(TEXT("xp_absorb"), OrbPos, LocalPlayerPos - OrbPos, FLinearColor(0.42f, 0.96f, 1.0f, 0.96f), 0.74f, FMath::Clamp(38.0f + Orb.Xp * 1.6f, 38.0f, 82.0f), LocalPlayerPos);
            }
        }

        TSet<FString> CurrentDropIds;
        for (const FCWPickupSnapshot& Drop : NextState.Drops)
        {
            CurrentDropIds.Add(CWRunView::PickupRenderKey(Drop, true));
        }
        for (const FCWPickupSnapshot& Drop : PreviousState.Drops)
        {
            const FString DropKey = CWRunView::PickupRenderKey(Drop, true);
            if (CurrentDropIds.Contains(DropKey) || HiddenDropKeys.Contains(DropKey))
            {
                continue;
            }
            const FVector2D DropPos(Drop.X, Drop.Y);
            if ((DropPos - LocalPlayerPos).SizeSquared() <= FMath::Square(520.0f))
            {
                AddFx(TEXT("loot_pickup"), DropPos, LocalPlayerPos - DropPos, CWRunView::LootDropAccentColor(Drop.ItemKey), 0.70f, 66.0f, LocalPlayerPos);
            }
        }

        TSet<FString> CurrentSkillOrbIds;
        for (const FCWPickupSnapshot& Orb : NextState.SkillOrbs)
        {
            CurrentSkillOrbIds.Add(CWRunView::PickupRenderKey(Orb, true));
        }
        struct FDisappearedSkillOrbFx
        {
            FCWPickupSnapshot Orb;
            FString Key;
            float DistSq = 0.0f;
            bool bWasLocallyPicked = false;
        };
        TArray<FDisappearedSkillOrbFx> DisappearedSkillOrbs;
        for (const FCWPickupSnapshot& Orb : PreviousState.SkillOrbs)
        {
            const FString OrbKey = CWRunView::PickupRenderKey(Orb, true);
            if (CurrentSkillOrbIds.Contains(OrbKey))
            {
                continue;
            }
            if (!Orb.OwnerId.IsEmpty() && Orb.OwnerId != LocalFxPlayer->Id)
            {
                continue;
            }
            const FVector2D OrbPos(Orb.X, Orb.Y);
            const float DistSq = (OrbPos - LocalPlayerPos).SizeSquared();
            if (DistSq <= FMath::Square(640.0f))
            {
                FDisappearedSkillOrbFx Disappeared;
                Disappeared.Orb = Orb;
                Disappeared.Key = OrbKey;
                Disappeared.DistSq = DistSq;
                Disappeared.bWasLocallyPicked = HiddenSkillOrbKeys.Contains(OrbKey);
                DisappearedSkillOrbs.Add(Disappeared);
            }
        }
        if (DisappearedSkillOrbs.Num() > 0)
        {
            int32 PickedIndex = INDEX_NONE;
            float BestPickedDistSq = TNumericLimits<float>::Max();
            for (int32 Index = 0; Index < DisappearedSkillOrbs.Num(); ++Index)
            {
                if (DisappearedSkillOrbs[Index].bWasLocallyPicked && DisappearedSkillOrbs[Index].DistSq < BestPickedDistSq)
                {
                    BestPickedDistSq = DisappearedSkillOrbs[Index].DistSq;
                    PickedIndex = Index;
                }
            }

            for (int32 Index = 0; Index < DisappearedSkillOrbs.Num(); ++Index)
            {
                const FCWPickupSnapshot& Orb = DisappearedSkillOrbs[Index].Orb;
                const FVector2D OrbPos(Orb.X, Orb.Y);
                const FLinearColor Accent = SkillOrbAccentColor(Orb.ItemKey);
                if (Index == PickedIndex)
                {
                    if (!DisappearedSkillOrbs[Index].bWasLocallyPicked)
                    {
                        AddFx(TEXT("skill_pickup"), OrbPos, LocalPlayerPos - OrbPos, Accent, 0.82f, 82.0f, LocalPlayerPos);
                    }
                    continue;
                }

                FVector2D BurstDir = OrbPos - LocalPlayerPos;
                if (BurstDir.IsNearlyZero())
                {
                    const float Angle = static_cast<float>(Index) * UE_PI * 2.0f / FMath::Max(1, DisappearedSkillOrbs.Num());
                    BurstDir = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
                }
                AddFx(TEXT("skill_dismiss"), OrbPos, BurstDir, Accent, 0.58f, 72.0f, OrbPos + BurstDir.GetSafeNormal() * 64.0f);
            }
        }
    }

    for (const FCWShotEventSnapshot& Event : NextState.ShotEvents)
    {
        const FString EventId = Event.Id.IsEmpty()
            ? FString::Printf(TEXT("%s:%s:%.0f"), *Event.OwnerId, *Event.BulletId, Event.At)
            : Event.Id;
        if (SeenShotEventIds.Contains(EventId))
        {
            continue;
        }
        SeenShotEventIds.Add(EventId);
        FVector2D Dir(Event.Vx, Event.Vy);
        if (Dir.IsNearlyZero())
        {
            Dir = FVector2D(1.0f, 0.0f);
        }
        Dir = Dir.GetSafeNormal();
        const FLinearColor ShotColor = CWRunView::HexColor(Event.Color, FLinearColor(1.0f, 0.78f, 0.16f, 0.96f));
        const bool bShotgun = Event.WeaponKey.Equals(TEXT("shotgun"), ESearchCase::IgnoreCase);
        const FVector2D ShotStart(Event.X, Event.Y);
        AddFx(TEXT("muzzle"), ShotStart, Dir, ShotColor, 0.18f, bShotgun ? 42.0f : 30.0f);
        AddFx(TEXT("muzzle_ground"), ShotStart + Dir * 18.0f, Dir, ShotColor, 0.16f, bShotgun ? 40.0f : 30.0f);
    }
    if (SeenShotEventIds.Num() > 256)
    {
        SeenShotEventIds.Reset();
    }

    for (const FCWObjectImpactEventSnapshot& Event : NextState.ObjectImpactEvents)
    {
        const FString EventId = Event.Id.IsEmpty()
            ? FString::Printf(TEXT("%s:%.0f:%.0f:%.0f"), *Event.ObjectId, Event.At, Event.X, Event.Y)
            : Event.Id;
        if (SeenObjectImpactEventIds.Contains(EventId))
        {
            continue;
        }
        SeenObjectImpactEventIds.Add(EventId);
        const bool bRocket = Event.BulletKind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase);
        const bool bMetal = Event.Material.Contains(TEXT("metal"), ESearchCase::IgnoreCase);
        const bool bWood = Event.Material.Contains(TEXT("wood"), ESearchCase::IgnoreCase);
        const FLinearColor Spark = Event.Material.Contains(TEXT("metal"), ESearchCase::IgnoreCase)
            ? FLinearColor(0.62f, 0.92f, 1.0f, 0.92f)
            : (bWood ? FLinearColor(1.0f, 0.58f, 0.22f, 0.90f) : FLinearColor(0.86f, 0.82f, 0.72f, 0.86f));
        const FVector2D ImpactDir = FVector2D(Event.DirX, Event.DirY).IsNearlyZero() ? FVector2D(1.0f, 0.0f) : FVector2D(Event.DirX, Event.DirY).GetSafeNormal();
        if (bRocket)
        {
            AddRocketExplosionFx(Event.Material, FVector2D(Event.X, Event.Y) - ImpactDir * 28.0f, ImpactDir, FLinearColor(1.0f, 0.48f, 0.08f, 0.96f), 185.0f);
        }
        else
        {
            const FString ImpactType = bMetal ? TEXT("impact_metal") : (bWood ? TEXT("impact_wood") : TEXT("impact_concrete"));
            AddFx(ImpactType, FVector2D(Event.X, Event.Y) - ImpactDir * 22.0f, ImpactDir, Spark, 0.36f, 32.0f);
        }
    }
    if (SeenObjectImpactEventIds.Num() > 256)
    {
        SeenObjectImpactEventIds.Reset();
    }

    TMap<FString, float> NextEnemyHp;
    for (const FCWEnemySnapshot& Enemy : NextState.Enemies)
    {
        if (Enemy.Id.IsEmpty())
        {
            continue;
        }
        NextEnemyHp.Add(Enemy.Id, Enemy.Hp);
        const float PrevHp = PreviousEnemyHp.FindRef(Enemy.Id);
        if (PrevHp > 0.0f && Enemy.Hp < PrevHp - 0.5f)
        {
            const FVector2D BaseBloodDir = ResolveBulletBloodDir(FVector2D(Enemy.X, Enemy.Y), FVector2D(Enemy.bFaceLeft ? -1.0f : 1.0f, -0.2f));
            const FVector2D BulletForwardDir = ResolveBulletForwardDir(FVector2D(Enemy.X, Enemy.Y), BaseBloodDir);
            const float HitDamage = FMath::Max(1.0f, PrevHp - Enemy.Hp);
            const bool bBossHit = CWRunView::IsBossEnemy(Enemy);
            const float BloodRadius = FMath::Clamp(Enemy.Radius * (bBossHit ? 2.35f : 1.65f) + FMath::Sqrt(HitDamage) * (bBossHit ? 5.8f : 4.4f), bBossHit ? 68.0f : 28.0f, bBossHit ? 190.0f : 104.0f);
            AddFx(TEXT("blood_jet"), FVector2D(Enemy.X, Enemy.Y - Enemy.Radius * 0.08f) + BulletForwardDir * Enemy.Radius * 0.22f, BulletForwardDir, FLinearColor(0.95f, 0.004f, 0.016f, 0.88f), bBossHit ? 1.02f : 0.78f, FMath::Clamp(BloodRadius * (bBossHit ? 0.94f : 0.82f), 30.0f, bBossHit ? 164.0f : 88.0f));
            AddFx(TEXT("blood_hit"), FVector2D(Enemy.X, Enemy.Y), BaseBloodDir, FLinearColor(0.92f, 0.010f, 0.020f, 0.88f), 0.82f, BloodRadius);
            AddFx(TEXT("gore_hit"), FVector2D(Enemy.X, Enemy.Y - Enemy.Radius * 0.10f), BaseBloodDir, FLinearColor(0.62f, 0.0f, 0.014f, 0.86f), 1.05f, FMath::Clamp(BloodRadius * 0.68f, 22.0f, bBossHit ? 120.0f : 74.0f));
            AddFx(TEXT("blood_puddle"), FVector2D(Enemy.X, Enemy.Y + Enemy.Radius * 0.38f), BaseBloodDir, FLinearColor(0.44f, 0.02f, 0.04f, bBossHit ? 0.66f : 0.52f), bBossHit ? 9.0f : 6.8f, FMath::Clamp(Enemy.Radius * (bBossHit ? 1.35f : 0.92f), 14.0f, bBossHit ? 78.0f : 44.0f));
        }
    }

    for (const FCWEnemySnapshot& OldEnemy : PreviousState.Enemies)
    {
        if (OldEnemy.Id.IsEmpty() || NextEnemyHp.Contains(OldEnemy.Id))
        {
            continue;
        }
        const FVector2D DeathBloodDir = ResolveBulletBloodDir(FVector2D(OldEnemy.X, OldEnemy.Y), FVector2D(1.0f, -0.18f));
        const FVector2D DeathForwardDir = ResolveBulletForwardDir(FVector2D(OldEnemy.X, OldEnemy.Y), DeathBloodDir);
        const bool bBossDeath = CWRunView::IsBossEnemy(OldEnemy);
        if (bBossDeath)
        {
            AddFx(TEXT("blood_jet"), FVector2D(OldEnemy.X, OldEnemy.Y - OldEnemy.Radius * 0.08f) + DeathForwardDir * OldEnemy.Radius * 0.18f, DeathForwardDir, FLinearColor(0.95f, 0.004f, 0.016f, 0.92f), 1.16f, FMath::Clamp(OldEnemy.Radius * 2.65f, 112.0f, 210.0f));
            AddFx(TEXT("boss_blood_death"), FVector2D(OldEnemy.X, OldEnemy.Y), DeathBloodDir, FLinearColor(0.88f, 0.0f, 0.018f, 0.94f), 1.55f, FMath::Clamp(OldEnemy.Radius * 5.6f, 190.0f, 360.0f));
            AddFx(TEXT("blood_puddle"), FVector2D(OldEnemy.X, OldEnemy.Y + OldEnemy.Radius * 0.45f), DeathBloodDir, FLinearColor(0.50f, 0.012f, 0.022f, 0.78f), 13.0f, FMath::Clamp(OldEnemy.Radius * 2.15f, 74.0f, 150.0f));
            for (int32 I = 0; I < 4; ++I)
            {
                const float Angle = static_cast<float>(I) * UE_PI * 0.5f + OldEnemy.Radius * 0.013f;
                const FVector2D Offset(FMath::Cos(Angle) * OldEnemy.Radius * 1.10f, FMath::Sin(Angle) * OldEnemy.Radius * 0.82f);
                AddFx(TEXT("blood_puddle"), FVector2D(OldEnemy.X, OldEnemy.Y) + Offset, Offset.GetSafeNormal(), FLinearColor(0.42f, 0.0f, 0.018f, 0.62f), 11.0f, FMath::Clamp(OldEnemy.Radius * 1.10f, 42.0f, 96.0f));
            }
        }
        else
        {
            if (OldEnemy.ExplosionRadius > 1.0f)
            {
                AddFx(TEXT("explosion"), FVector2D(OldEnemy.X, OldEnemy.Y), DeathBloodDir, FLinearColor(1.0f, 0.55f, 0.12f, 0.82f), 0.66f, OldEnemy.ExplosionRadius);
            }
            AddFx(TEXT("blood_jet"), FVector2D(OldEnemy.X, OldEnemy.Y - OldEnemy.Radius * 0.08f) + DeathForwardDir * OldEnemy.Radius * 0.20f, DeathForwardDir, FLinearColor(0.95f, 0.004f, 0.016f, 0.88f), 0.86f, FMath::Clamp(OldEnemy.Radius * 2.05f, 48.0f, 118.0f));
            AddFx(TEXT("gore_death"), FVector2D(OldEnemy.X, OldEnemy.Y), DeathBloodDir, FLinearColor(0.86f, 0.008f, 0.018f, 0.90f), 1.18f, FMath::Clamp(OldEnemy.Radius * 2.7f, 54.0f, 132.0f));
            AddFx(TEXT("blood_hit"), FVector2D(OldEnemy.X, OldEnemy.Y - OldEnemy.Radius * 0.10f), DeathBloodDir, FLinearColor(0.98f, 0.018f, 0.026f, 0.82f), 0.92f, FMath::Clamp(OldEnemy.Radius * 2.0f, 42.0f, 118.0f));
            AddFx(TEXT("blood_puddle"), FVector2D(OldEnemy.X, OldEnemy.Y + OldEnemy.Radius * 0.42f), DeathBloodDir, FLinearColor(0.50f, 0.015f, 0.025f, 0.68f), 10.0f, FMath::Clamp(OldEnemy.Radius * 1.45f, 24.0f, 74.0f));
        }
    }
    PreviousEnemyHp = MoveTemp(NextEnemyHp);

    TMap<FString, FCWPlayerSnapshot> PreviousPlayers;
    for (const FCWPlayerSnapshot& Player : PreviousState.Players)
    {
        if (!Player.Id.IsEmpty())
        {
            PreviousPlayers.Add(Player.Id, Player);
        }
    }
    for (const FCWPlayerSnapshot& Player : NextState.Players)
    {
        const FCWPlayerSnapshot* Prev = PreviousPlayers.Find(Player.Id);
        if (!Prev)
        {
            continue;
        }
        const FVector2D From(Prev->X, Prev->Y);
        const FVector2D To(Player.X, Player.Y);
        const float Dist = FVector2D::Distance(From, To);
        if (Dist > 105.0f || (Player.DodgeInvulnUntil > NextState.ServerNowMs && Dist > 20.0f))
        {
            AddFx(TEXT("dodge"), From, (To - From).GetSafeNormal(), FLinearColor(0.62f, 0.80f, 1.0f, 0.74f), 0.44f, FMath::Clamp(Dist * 0.16f, 30.0f, 120.0f), To);
        }
    }
}

const FCWPlayerSnapshot* UCWNativeRunViewWidget::FindFocusPlayer() const
{
    const FCWPlayerSnapshot* FirstLivePlayer = nullptr;
    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        if (!CachedMyId.IsEmpty() && Player.Id == CachedMyId)
        {
            return &Player;
        }
        if (!FirstLivePlayer && !Player.bIsCompanion)
        {
            FirstLivePlayer = &Player;
        }
    }
    return FirstLivePlayer;
}

FVector2D UCWNativeRunViewWidget::GetRenderPosition(const FCWPlayerSnapshot& Player) const
{
    if (const FRenderPlayerState* RenderState = RenderPlayers.Find(Player.Id))
    {
        if (RenderState->bInitialized)
        {
            return RenderState->Position;
        }
    }
    return FVector2D(Player.X, Player.Y);
}

FVector2D UCWNativeRunViewWidget::GetRenderEnemyPosition(const FCWEnemySnapshot& Enemy) const
{
    if (const FRenderEntityState* RenderState = RenderEnemies.Find(Enemy.Id))
    {
        if (RenderState->bInitialized)
        {
            return RenderState->Position;
        }
    }
    return FVector2D(Enemy.X, Enemy.Y);
}

FVector2D UCWNativeRunViewWidget::GetRenderBulletPosition(const FCWBulletSnapshot& Bullet) const
{
    if (const FRenderEntityState* RenderState = RenderBullets.Find(Bullet.Id))
    {
        if (RenderState->bInitialized)
        {
            return RenderState->Position;
        }
    }
    return FVector2D(Bullet.X, Bullet.Y);
}

FVector2D UCWNativeRunViewWidget::GetRenderDropPosition(const FCWPickupSnapshot& Drop) const
{
    const FString DropId = CWRunView::PickupRenderKey(Drop, true);
    if (const FRenderEntityState* RenderState = RenderDrops.Find(DropId))
    {
        if (RenderState->bInitialized)
        {
            return RenderState->Position;
        }
    }
    return FVector2D(Drop.X, Drop.Y);
}

FVector2D UCWNativeRunViewWidget::GetRenderXpOrbPosition(const FCWPickupSnapshot& Orb) const
{
    const FString OrbId = CWRunView::PickupRenderKey(Orb, false);
    if (const FRenderEntityState* RenderState = RenderXpOrbs.Find(OrbId))
    {
        if (RenderState->bInitialized)
        {
            return RenderState->Position;
        }
    }
    return FVector2D(Orb.X, Orb.Y);
}

FVector2D UCWNativeRunViewWidget::GetRenderSkillOrbPosition(const FCWPickupSnapshot& Orb) const
{
    const FString OrbId = CWRunView::PickupRenderKey(Orb, true);
    if (const FRenderEntityState* RenderState = RenderSkillOrbs.Find(OrbId))
    {
        if (RenderState->bInitialized)
        {
            return RenderState->Position;
        }
    }
    return FVector2D(Orb.X, Orb.Y);
}

FVector2D UCWNativeRunViewWidget::WorldToScreen(float X, float Y, const FVector2D& Size, const FCWPlayerSnapshot* FocusPlayer, float Scale) const
{
    const float WorldWidth = FMath::Max(CWRunView::DefaultWorldWidth, CachedState.World.Width);
    const float WorldHeight = FMath::Max(CWRunView::DefaultWorldHeight, CachedState.World.Height);
    const float CameraX = FocusPlayer ? FocusPlayer->X : WorldWidth * 0.5f;
    const float CameraY = FocusPlayer ? FocusPlayer->Y : WorldHeight * 0.5f;
    return FVector2D(Size.X * 0.5f + (X - CameraX) * Scale, Size.Y * 0.5f + (Y - CameraY) * Scale);
}

bool UCWNativeRunViewWidget::GetViewportMousePosition(FVector2D& OutMousePosition, FVector2D& OutViewportSize) const
{
    const FGeometry ViewGeometry = GetCachedGeometry();
    const FVector2D WidgetSize = ViewGeometry.GetLocalSize();
    OutViewportSize = WidgetSize;
    if (WidgetSize.X <= 2.0f || WidgetSize.Y <= 2.0f)
    {
        return false;
    }

    auto IsInside = [](const FVector2D& P, const FVector2D& Size)
    {
        return Size.X > 2.0f && Size.Y > 2.0f && P.X >= 0.0f && P.Y >= 0.0f && P.X <= Size.X && P.Y <= Size.Y;
    };

    auto AcceptLocal = [&](const FVector2D& LocalMouse, const FVector2D& LocalSize) -> bool
    {
        if (!IsInside(LocalMouse, LocalSize))
        {
            return false;
        }
        OutMousePosition = LocalMouse;
        OutViewportSize = LocalSize;
        return true;
    };

    auto TryAbsoluteCursor = [&](const FVector2D& CursorScreen) -> bool
    {
        const FVector2D WidgetLocal = ViewGeometry.AbsoluteToLocal(CursorScreen);
        if (AcceptLocal(WidgetLocal, WidgetSize))
        {
            return true;
        }

        if (GEngine && GEngine->GameViewport)
        {
            const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
            if (ViewportWidget.IsValid())
            {
                const FGeometry ViewportGeometry = ViewportWidget->GetCachedGeometry();
                const FVector2D ViewportSize = ViewportGeometry.GetLocalSize();
                const FVector2D ViewportLocal = ViewportGeometry.AbsoluteToLocal(CursorScreen);
                if (AcceptLocal(ViewportLocal, ViewportSize))
                {
                    return true;
                }
            }
        }

        return false;
    };

    const ACWNativePlayerPawn* NativePawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn());
    if (FSlateApplication::IsInitialized())
    {
        if (TryAbsoluteCursor(FSlateApplication::Get().GetCursorPos()))
        {
            return true;
        }
    }

    if (NativePawn)
    {
        FVector2D CursorScreen = FVector2D::ZeroVector;
        if (NativePawn->GetNativeCursorScreenPosition(CursorScreen))
        {
            if (TryAbsoluteCursor(CursorScreen))
            {
                return true;
            }
        }
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        int32 ViewportX = 0;
        int32 ViewportY = 0;
        PC->GetViewportSize(ViewportX, ViewportY);
        const FVector2D PCViewportSize(static_cast<float>(ViewportX), static_cast<float>(ViewportY));
        if (PC->GetMousePosition(MouseX, MouseY) && AcceptLocal(FVector2D(MouseX, MouseY), PCViewportSize))
        {
            return true;
        }

        if (UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PC, MouseX, MouseY) && AcceptLocal(FVector2D(MouseX, MouseY), WidgetSize))
        {
            return true;
        }
    }

    if (NativePawn)
    {
        FVector2D LocalMouse = FVector2D::ZeroVector;
        FVector2D LocalViewportSize = FVector2D::ZeroVector;
        if (NativePawn->GetNativeCursorViewportPosition(LocalMouse, LocalViewportSize) && AcceptLocal(LocalMouse, LocalViewportSize))
        {
            return true;
        }
    }

    return false;
}

bool UCWNativeRunViewWidget::ScreenToWorld(const FVector2D& ScreenPosition, const FVector2D& ViewportSize, FVector2D& OutWorld) const
{
    if (ViewportSize.X <= 2.0f || ViewportSize.Y <= 2.0f)
    {
        return false;
    }

    const FCWPlayerSnapshot* FocusPlayer = FindFocusPlayer();
    FCWPlayerSnapshot FocusView;
    if (FocusPlayer)
    {
        FocusView = *FocusPlayer;
        const FVector2D FocusRender = GetRenderPosition(*FocusPlayer);
        FocusView.X = FocusRender.X;
        FocusView.Y = FocusRender.Y;
        FocusPlayer = &FocusView;
    }

    const float Scale = FMath::Clamp(ViewportSize.X / CWRunView::ViewWorldWidth, 0.38f, 1.05f);
    const float WorldWidth = FMath::Max(CWRunView::DefaultWorldWidth, CachedState.World.Width);
    const float WorldHeight = FMath::Max(CWRunView::DefaultWorldHeight, CachedState.World.Height);
    const float CameraX = FocusPlayer ? FocusPlayer->X : WorldWidth * 0.5f;
    const float CameraY = FocusPlayer ? FocusPlayer->Y : WorldHeight * 0.5f;
    OutWorld = FVector2D(
        CameraX + (ScreenPosition.X - ViewportSize.X * 0.5f) / Scale,
        CameraY + (ScreenPosition.Y - ViewportSize.Y * 0.5f) / Scale);
    return true;
}

bool UCWNativeRunViewWidget::ScreenToLocalPlayerAimWorld(const FVector2D& ScreenPosition, const FVector2D& ViewportSize, FVector2D& OutWorld) const
{
    if (ViewportSize.X <= 2.0f || ViewportSize.Y <= 2.0f)
    {
        return false;
    }

    const FCWPlayerSnapshot* LocalPlayer = nullptr;
    const FCWPlayerSnapshot* FirstLivePlayer = nullptr;
    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        if (!Player.bAlive || Player.bIsCompanion)
        {
            continue;
        }
        if (!CachedMyId.IsEmpty() && Player.Id == CachedMyId)
        {
            LocalPlayer = &Player;
            break;
        }
        if (!FirstLivePlayer)
        {
            FirstLivePlayer = &Player;
        }
    }
    if (!LocalPlayer)
    {
        LocalPlayer = FirstLivePlayer;
    }
    if (!LocalPlayer)
    {
        return ScreenToWorld(ScreenPosition, ViewportSize, OutWorld);
    }

    const FCWPlayerSnapshot* FocusPlayer = FindFocusPlayer();
    if (!FocusPlayer)
    {
        FocusPlayer = LocalPlayer;
    }

    const float Scale = FMath::Clamp(ViewportSize.X / CWRunView::ViewWorldWidth, 0.38f, 1.05f);
    const FVector2D FocusRender = GetRenderPosition(*FocusPlayer);
    const FVector2D LocalRender = GetRenderPosition(*LocalPlayer);
    const FVector2D LocalPlayerScreen(
        ViewportSize.X * 0.5f + (LocalRender.X - FocusRender.X) * Scale,
        ViewportSize.Y * 0.5f + (LocalRender.Y - FocusRender.Y) * Scale);
    const FVector2D ScreenAimVector = ScreenPosition - LocalPlayerScreen;
    if (ScreenAimVector.SizeSquared() < 4.0f)
    {
        OutWorld = FVector2D(LocalPlayer->X + 1.0f, LocalPlayer->Y);
        return true;
    }

    OutWorld = FVector2D(
        LocalPlayer->X + ScreenAimVector.X / Scale,
        LocalPlayer->Y + ScreenAimVector.Y / Scale);
    return true;
}

int32 UCWNativeRunViewWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    if (Size.X <= 2.0f || Size.Y <= 2.0f)
    {
        return BaseLayer;
    }

    using namespace CWRunView;

    const int32 BackgroundLayer = BaseLayer + 1;
    const int32 ActorLayer = BaseLayer + 56;
    const int32 TextLayer = BaseLayer + 92;

    const bool bHasServerTerrain = CachedState.TerrainZones.Num() > 0;
    const bool bHasServerObjects = CachedState.MapObjects.Num() > 0;
    const bool bHasServerTrees = CachedState.Trees.Num() > 0;
    const FLinearColor BaseMaterialColor = MaterialColor(CachedState.SceneTheme.BaseMaterial, 1.0f);

    DrawBox(OutDrawElements, AllottedGeometry, BackgroundLayer, FVector2D::ZeroVector, Size, BaseMaterialColor);
    DrawBox(OutDrawElements, AllottedGeometry, BackgroundLayer + 1, FVector2D::ZeroVector, Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));

    const FCWPlayerSnapshot* FocusPlayer = FindFocusPlayer();
    FCWPlayerSnapshot FocusView;
    if (FocusPlayer)
    {
        FocusView = *FocusPlayer;
        const FVector2D FocusRender = GetRenderPosition(*FocusPlayer);
        FocusView.X = FocusRender.X;
        FocusView.Y = FocusRender.Y;
        FocusPlayer = &FocusView;
    }
    const float Scale = FMath::Clamp(Size.X / ViewWorldWidth, 0.38f, 1.05f);
    const float WorldWidth = FMath::Max(DefaultWorldWidth, CachedState.World.Width);
    const float WorldHeight = FMath::Max(DefaultWorldHeight, CachedState.World.Height);
    const float CameraX = FocusPlayer ? FocusPlayer->X : WorldWidth * 0.5f;
    const float CameraY = FocusPlayer ? FocusPlayer->Y : WorldHeight * 0.5f;
    const float VisibleWorldW = Size.X / Scale;
    const float VisibleWorldH = Size.Y / Scale;
    const FCWPlayerSnapshot* LocalPickupPlayer = nullptr;
    const FCWPlayerSnapshot* FirstLivePickupPlayer = nullptr;
    int32 LivePickupPlayerCount = 0;
    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        if (!Player.bAlive || Player.bIsCompanion)
        {
            continue;
        }
        ++LivePickupPlayerCount;
        if (!FirstLivePickupPlayer)
        {
            FirstLivePickupPlayer = &Player;
        }
        if (!CachedMyId.IsEmpty() && Player.Id == CachedMyId)
        {
            LocalPickupPlayer = &Player;
            break;
        }
    }
    if (!LocalPickupPlayer && LivePickupPlayerCount == 1)
    {
        LocalPickupPlayer = FirstLivePickupPlayer;
    }
    const FVector2D LocalPickupRenderPos = LocalPickupPlayer ? GetRenderPosition(*LocalPickupPlayer) : FVector2D::ZeroVector;
    auto IsNearLocalPickup = [&](const FVector2D& PickupRenderPos, float PickupRadius, const FString& OwnerId = FString()) -> bool
    {
        if (!LocalPickupPlayer)
        {
            return false;
        }
        if (!OwnerId.IsEmpty() && OwnerId != LocalPickupPlayer->Id)
        {
            return false;
        }
        const float Radius = FMath::Max(1.0f, PickupRadius);
        return FVector2D::DistSquared(PickupRenderPos, LocalPickupRenderPos) <= FMath::Square(Radius);
    };
    auto DrawWorldRect = [&](int32 InLayer, float X, float Y, float W, float H, const FLinearColor& Color)
    {
        const FVector2D A = WorldToScreen(X, Y, Size, FocusPlayer, Scale);
        const FVector2D B = WorldToScreen(X + W, Y + H, Size, FocusPlayer, Scale);
        DrawBox(OutDrawElements, AllottedGeometry, InLayer, A, B - A, Color);
    };

    auto DrawWorldImage = [&](int32 InLayer, const FString& Key, float X, float Y, float W, float H, const FLinearColor& Tint = FLinearColor::White)
    {
        const FSlateBrush* Brush = GetBrush(Key);
        if (!Brush)
        {
            return;
        }

        const FVector2D Center = WorldToScreen(X, Y, Size, FocusPlayer, Scale);
        const FVector2D DrawSize(FMath::Max(2.0f, W * Scale), FMath::Max(2.0f, H * Scale));
        DrawTexture(OutDrawElements, AllottedGeometry, InLayer, Brush, Center - DrawSize * 0.5f, DrawSize, Tint);
    };

    auto DrawWorldCenteredRect = [&](int32 InLayer, float CenterX, float CenterY, float W, float H, const FLinearColor& Color)
    {
        DrawWorldRect(InLayer, CenterX - W * 0.5f, CenterY - H * 0.5f, W, H, Color);
    };

    auto DrawScreenRectRel = [&](int32 InLayer, const FVector2D& Position, const FVector2D& DrawSize, float X, float Y, float W, float H, const FLinearColor& Color)
    {
        DrawBox(
            OutDrawElements,
            AllottedGeometry,
            InLayer,
            Position + FVector2D(DrawSize.X * X, DrawSize.Y * Y),
            FVector2D(DrawSize.X * W, DrawSize.Y * H),
            Color);
    };

    auto DrawScreenLineRel = [&](int32 InLayer, const FVector2D& Position, const FVector2D& DrawSize, float X0, float Y0, float X1, float Y1, const FLinearColor& Color, float Thickness)
    {
        DrawLine(
            OutDrawElements,
            AllottedGeometry,
            InLayer,
            Position + FVector2D(DrawSize.X * X0, DrawSize.Y * Y0),
            Position + FVector2D(DrawSize.X * X1, DrawSize.Y * Y1),
            Color,
            Thickness);
    };

    auto DrawVectorMapObject = [&](int32 InLayer, const FString& Key, const FVector2D& Position, const FVector2D& DrawSize, const FLinearColor& Tint) -> bool
    {
        const float A = FMath::Clamp(Tint.A, 0.0f, 1.0f);
        auto C = [A](float R, float G, float B, float Alpha)
        {
            return FLinearColor(R, G, B, Alpha * A);
        };

        if (Key == TEXT("clinic_block"))
        {
            DrawCenteredBox(OutDrawElements, AllottedGeometry, InLayer, Position + FVector2D(DrawSize.X * 0.50f, DrawSize.Y * 0.82f), FVector2D(DrawSize.X * 0.72f, DrawSize.Y * 0.18f), C(0.0f, 0.0f, 0.0f, 0.20f));
            DrawScreenRectRel(InLayer + 1, Position, DrawSize, 0.08f, 0.28f, 0.84f, 0.48f, C(0.35f, 0.42f, 0.50f, 0.90f));
            DrawScreenRectRel(InLayer + 2, Position, DrawSize, 0.11f, 0.31f, 0.78f, 0.41f, C(0.56f, 0.66f, 0.74f, 0.86f));
            DrawScreenRectRel(InLayer + 3, Position, DrawSize, 0.11f, 0.25f, 0.78f, 0.11f, C(0.48f, 0.56f, 0.64f, 0.88f));
            DrawScreenRectRel(InLayer + 4, Position, DrawSize, 0.16f, 0.44f, 0.66f, 0.23f, C(0.45f, 0.59f, 0.68f, 0.70f));
            for (int32 Index = 0; Index < 6; ++Index)
            {
                const float X = 0.24f + static_cast<float>(Index) * 0.083f;
                DrawScreenLineRel(InLayer + 5, Position, DrawSize, X, 0.44f, X, 0.67f, C(0.28f, 0.36f, 0.44f, 0.70f), 2.2f);
            }
            DrawScreenLineRel(InLayer + 6, Position, DrawSize, 0.12f, 0.69f, 0.88f, 0.69f, C(0.96f, 0.28f, 0.28f, 0.78f), 5.0f);
            DrawScreenRectRel(InLayer + 7, Position, DrawSize, 0.47f, 0.31f, 0.05f, 0.17f, C(0.96f, 0.28f, 0.28f, 0.80f));
            DrawScreenRectRel(InLayer + 7, Position, DrawSize, 0.43f, 0.36f, 0.13f, 0.07f, C(0.96f, 0.28f, 0.28f, 0.80f));
            DrawScreenLineRel(InLayer + 8, Position, DrawSize, 0.18f, 0.80f, 0.82f, 0.80f, C(0.20f, 0.25f, 0.32f, 0.26f), 4.0f);
            return true;
        }

        if (Key == TEXT("reactor_block"))
        {
            DrawCenteredBox(OutDrawElements, AllottedGeometry, InLayer, Position + FVector2D(DrawSize.X * 0.50f, DrawSize.Y * 0.83f), FVector2D(DrawSize.X * 0.74f, DrawSize.Y * 0.18f), C(0.0f, 0.0f, 0.0f, 0.18f));
            DrawScreenRectRel(InLayer + 1, Position, DrawSize, 0.11f, 0.30f, 0.70f, 0.48f, C(0.28f, 0.34f, 0.41f, 0.98f));
            DrawScreenRectRel(InLayer + 2, Position, DrawSize, 0.15f, 0.33f, 0.62f, 0.42f, C(0.39f, 0.45f, 0.54f, 0.98f));
            DrawScreenRectRel(InLayer + 3, Position, DrawSize, 0.61f, 0.18f, 0.23f, 0.60f, C(0.20f, 0.25f, 0.32f, 0.98f));
            DrawScreenRectRel(InLayer + 4, Position, DrawSize, 0.64f, 0.21f, 0.17f, 0.53f, C(0.28f, 0.34f, 0.41f, 0.98f));
            DrawScreenRectRel(InLayer + 5, Position, DrawSize, 0.19f, 0.42f, 0.48f, 0.22f, C(0.06f, 0.09f, 0.16f, 0.98f));
            for (int32 Index = 0; Index < 5; ++Index)
            {
                const float X = 0.25f + static_cast<float>(Index) * 0.081f;
                DrawScreenLineRel(InLayer + 6, Position, DrawSize, X, 0.42f, X, 0.64f, C(0.13f, 0.77f, 0.37f, 0.72f), 2.3f);
            }
            DrawScreenLineRel(InLayer + 7, Position, DrawSize, 0.17f, 0.69f, 0.77f, 0.69f, C(0.52f, 0.80f, 0.09f, 0.58f), 5.0f);
            DrawCenteredBox(OutDrawElements, AllottedGeometry, InLayer + 8, Position + FVector2D(DrawSize.X * 0.72f, DrawSize.Y * 0.36f), FVector2D(DrawSize.X * 0.09f, DrawSize.Y * 0.09f), C(0.75f, 0.95f, 0.39f, 0.66f));
            DrawScreenLineRel(InLayer + 8, Position, DrawSize, 0.27f, 0.76f, 0.68f, 0.76f, C(0.06f, 0.09f, 0.16f, 0.30f), 4.0f);
            return true;
        }

        return false;
    };

    auto DrawWorldEllipse = [&](int32 InLayer, float CenterX, float CenterY, float W, float H, const FLinearColor& Color)
    {
        const int32 Slices = 18;
        const float HalfW = FMath::Max(1.0f, W * 0.5f);
        const float HalfH = FMath::Max(1.0f, H * 0.5f);
        for (int32 Index = 0; Index < Slices; ++Index)
        {
            const float T0 = -1.0f + (static_cast<float>(Index) / static_cast<float>(Slices)) * 2.0f;
            const float T1 = -1.0f + (static_cast<float>(Index + 1) / static_cast<float>(Slices)) * 2.0f;
            const float TMid = (T0 + T1) * 0.5f;
            const float SliceWidth = HalfW * 2.0f * FMath::Sqrt(FMath::Max(0.0f, 1.0f - TMid * TMid));
            const float SliceHeight = HalfH * (T1 - T0) + 1.0f / Scale;
            const float SliceY = CenterY + TMid * HalfH;
            DrawWorldRect(InLayer, CenterX - SliceWidth * 0.5f, SliceY - SliceHeight * 0.5f, SliceWidth, SliceHeight, Color);
        }
    };

    auto DrawTerrainZone = [&](const FCWTerrainZoneSnapshot& Zone)
    {
        const float ZoneAlpha = FMath::Clamp(Zone.Alpha, 0.0f, 1.0f);
        const bool bRoadMaterial = Zone.Material.Equals(TEXT("asphalt"), ESearchCase::IgnoreCase) || Zone.Material.Equals(TEXT("asphalt_wet"), ESearchCase::IgnoreCase);
        const FLinearColor MainColor = MaterialColor(Zone.Material, bRoadMaterial ? FMath::Max(0.96f, ZoneAlpha) : ZoneAlpha * 0.68f);
        const float FeatherPad = FMath::Clamp(Zone.Feather, 0.0f, 1.0f) * 0.42f;
        const bool bEllipse = Zone.Shape.Equals(TEXT("ellipse"), ESearchCase::IgnoreCase);

        if (bEllipse)
        {
            DrawWorldEllipse(BackgroundLayer + 3, Zone.X, Zone.Y, Zone.W * (1.0f + FeatherPad), Zone.H * (1.0f + FeatherPad), MaterialColor(Zone.Material, ZoneAlpha * 0.18f));
            DrawWorldEllipse(BackgroundLayer + 4, Zone.X, Zone.Y, Zone.W, Zone.H, MainColor);
        }
        else
        {
            DrawWorldCenteredRect(
                BackgroundLayer + 3,
                Zone.X,
                Zone.Y,
                Zone.W * (1.0f + FeatherPad),
                Zone.H * (1.0f + FeatherPad),
                MaterialColor(Zone.Material, ZoneAlpha * 0.18f));
            DrawWorldCenteredRect(BackgroundLayer + 4, Zone.X, Zone.Y, Zone.W, Zone.H, MainColor);
        }

        if (Zone.bCenterStripe && bRoadMaterial)
        {
            const float DashW = FMath::Clamp(Zone.W * 0.045f, 42.0f, 132.0f);
            const float DashGap = DashW * 0.72f;
            const float StripeY = Zone.Y - 4.0f;
            const float Start = Zone.X - Zone.W * 0.45f;
            const float End = Zone.X + Zone.W * 0.45f;
            for (float X = Start; X < End; X += DashW + DashGap)
            {
                DrawWorldRect(BackgroundLayer + 5, X, StripeY, DashW, FMath::Max(5.0f, Zone.H * 0.026f), FLinearColor(0.93f, 0.84f, 0.56f, 0.42f));
            }
        }
    };

    const float TileWorldSize = 512.0f;
    const float TileStartX = FMath::FloorToFloat((CameraX - VisibleWorldW) / TileWorldSize) * TileWorldSize;
    const float TileEndX = CameraX + VisibleWorldW;
    const float TileStartY = FMath::FloorToFloat((CameraY - VisibleWorldH) / TileWorldSize) * TileWorldSize;
    const float TileEndY = CameraY + VisibleWorldH;
    const bool bUseGrassBaseTexture = CachedState.SceneTheme.BaseMaterial.Equals(TEXT("grass"), ESearchCase::IgnoreCase) || !bHasServerTerrain;
    if (bUseGrassBaseTexture)
    {
        for (float Y = TileStartY; Y <= TileEndY; Y += TileWorldSize)
        {
            for (float X = TileStartX; X <= TileEndX; X += TileWorldSize)
            {
                const FVector2D A = WorldToScreen(X, Y, Size, FocusPlayer, Scale);
                const FVector2D DrawSize(TileWorldSize * Scale + 1.0f, TileWorldSize * Scale + 1.0f);
                DrawTexture(OutDrawElements, AllottedGeometry, BackgroundLayer + 2, GetBrush(TEXT("grass")), A, DrawSize, FLinearColor(0.10f, 0.20f, 0.10f, 0.90f));
            }
        }
    }

    if (bHasServerTerrain)
    {
        for (const FCWTerrainZoneSnapshot& Zone : CachedState.TerrainZones)
        {
            DrawTerrainZone(Zone);
        }
    }

    const float RoadTop = WorldHeight * 0.5f - RoadHeight * 0.5f;
    const float RoadMid = WorldHeight * 0.5f;
    if (!bHasServerTerrain)
    {
        DrawWorldRect(BackgroundLayer + 3, 0.0f, RoadTop, WorldWidth, RoadHeight, FLinearColor(0.10f, 0.105f, 0.105f, 0.94f));
        DrawWorldRect(BackgroundLayer + 4, 0.0f, RoadTop + 62.0f, WorldWidth, RoadHeight - 124.0f, FLinearColor(0.11f, 0.105f, 0.098f, 0.88f));
        DrawWorldRect(BackgroundLayer + 5, 0.0f, RoadTop, WorldWidth, 24.0f, FLinearColor(0.025f, 0.105f, 0.088f, 0.9f));
        DrawWorldRect(BackgroundLayer + 5, 0.0f, RoadTop + RoadHeight - 24.0f, WorldWidth, 24.0f, FLinearColor(0.025f, 0.105f, 0.088f, 0.9f));

        const float DashW = 118.0f;
        const float DashGap = 96.0f;
        const float DashStart = FMath::FloorToFloat((CameraX - VisibleWorldW) / (DashW + DashGap)) * (DashW + DashGap);
        for (float X = DashStart; X < CameraX + VisibleWorldW; X += DashW + DashGap)
        {
            DrawWorldRect(BackgroundLayer + 6, X, RoadMid - 7.0f, DashW, 14.0f, FLinearColor(0.93f, 0.84f, 0.56f, 0.55f));
        }
    }

    auto DrawCrater = [&](float X, float Y, float Radius, float Alpha)
    {
        DrawWorldEllipse(BackgroundLayer + 7, X + Radius * 0.10f, Y + Radius * 0.08f, Radius * 2.05f, Radius * 1.02f, FLinearColor(0.0f, 0.0f, 0.0f, Alpha * 0.20f));
        DrawWorldEllipse(BackgroundLayer + 8, X, Y, Radius * 1.46f, Radius * 1.10f, FLinearColor(0.008f, 0.012f, 0.017f, Alpha * 0.36f));
        DrawWorldEllipse(BackgroundLayer + 9, X - Radius * 0.10f, Y - Radius * 0.04f, Radius * 0.62f, Radius * 0.48f, FLinearColor(0.016f, 0.021f, 0.027f, Alpha * 0.48f));
        DrawWorldEllipse(BackgroundLayer + 10, X - Radius * 0.22f, Y - Radius * 0.14f, Radius * 0.22f, Radius * 0.14f, FLinearColor(0.72f, 0.82f, 0.92f, Alpha * 0.08f));
    };

    const float CraterCell = 360.0f;
    const int32 CraterStartX = FMath::FloorToInt((CameraX - VisibleWorldW) / CraterCell) - 1;
    const int32 CraterEndX = FMath::CeilToInt((CameraX + VisibleWorldW) / CraterCell) + 1;
    const int32 CraterStartY = FMath::FloorToInt((CameraY - VisibleWorldH) / CraterCell) - 1;
    const int32 CraterEndY = FMath::CeilToInt((CameraY + VisibleWorldH) / CraterCell) + 1;
    for (int32 GX = CraterStartX; GX <= CraterEndX; ++GX)
    {
        for (int32 GY = CraterStartY; GY <= CraterEndY; ++GY)
        {
            const uint32 H = HashCell(GX, GY, 311);
            if ((H & 31u) > 4u)
            {
                continue;
            }

            const float X = GX * CraterCell + 92.0f + static_cast<float>((H >> 4) % 170);
            const float Y = GY * CraterCell + 92.0f + static_cast<float>((H >> 12) % 170);
            if (X < -160.0f || X > WorldWidth + 160.0f || Y < -160.0f || Y > WorldHeight + 160.0f)
            {
                continue;
            }

            const bool bNearRoad = !bHasServerTerrain || FMath::Abs(Y - RoadMid) < RoadHeight * 0.68f;
            if (!bNearRoad && (H & 3u) != 0u)
            {
                continue;
            }

            const float Radius = 24.0f + static_cast<float>((H >> 20) % 42);
            DrawCrater(X, Y, Radius, bNearRoad ? 0.54f : 0.22f);
        }
    }

    if (!bHasServerObjects && !bHasServerTrees)
    {
        const float PropStep = 430.0f;
        const int32 PropStartX = FMath::FloorToInt((CameraX - VisibleWorldW) / PropStep) - 1;
        const int32 PropEndX = FMath::CeilToInt((CameraX + VisibleWorldW) / PropStep) + 1;
        const int32 PropStartY = FMath::FloorToInt((CameraY - VisibleWorldH) / PropStep) - 1;
        const int32 PropEndY = FMath::CeilToInt((CameraY + VisibleWorldH) / PropStep) + 1;
        for (int32 GX = PropStartX; GX <= PropEndX; ++GX)
        {
            for (int32 GY = PropStartY; GY <= PropEndY; ++GY)
            {
                const uint32 H = HashCell(GX, GY, 17);
                const float X = GX * PropStep + static_cast<float>(H % 160) - 80.0f;
                const float Y = GY * PropStep + static_cast<float>((H >> 8) % 160) - 80.0f;
                if (X < -200.0f || Y < -200.0f || X > WorldWidth + 200.0f || Y > WorldHeight + 200.0f)
                {
                    continue;
                }

                const bool bOnRoad = Y > RoadTop - 90.0f && Y < RoadTop + RoadHeight + 90.0f;
                if (bOnRoad)
                {
                    if ((H & 7u) == 0u)
                    {
                        const FString Key = (H & 16u) ? TEXT("car_blue") : TEXT("car_red");
                        DrawWorldImage(BackgroundLayer + 8, Key, X, Y, 150.0f, 82.0f, FLinearColor(0.88f, 0.88f, 0.88f, 0.88f));
                    }
                    else if ((H & 31u) == 4u)
                    {
                        DrawWorldImage(BackgroundLayer + 8, TEXT("industrial_tank"), X, Y, 210.0f, 145.0f, FLinearColor(0.82f, 0.82f, 0.78f, 0.82f));
                    }
                    continue;
                }

                if ((H & 3u) == 0u)
                {
                    DrawWorldImage(BackgroundLayer + 7, TEXT("tree"), X, Y, 86.0f, 150.0f, FLinearColor(0.5f, 0.78f, 0.48f, 0.92f));
                }
                else if ((H & 15u) == 5u)
                {
                    DrawWorldImage(BackgroundLayer + 7, TEXT("mall_block"), X, Y, 230.0f, 170.0f, FLinearColor(0.66f, 0.7f, 0.72f, 0.74f));
                }
            }
        }
    }

    if (bHasServerTrees)
    {
        for (const FCWTreeSnapshot& Tree : CachedState.Trees)
        {
            const float TreeScale = FMath::Clamp(Tree.Scale, 0.45f, 1.7f);
            DrawWorldImage(ActorLayer, TEXT("tree"), Tree.X, Tree.Y, 82.0f * TreeScale, 144.0f * TreeScale, FLinearColor(0.58f, 0.86f, 0.52f, 0.94f));
        }
    }

    if (bHasServerObjects)
    {
        TArray<const FCWMapObjectSnapshot*> SortedObjects;
        SortedObjects.Reserve(CachedState.MapObjects.Num());
        for (const FCWMapObjectSnapshot& Object : CachedState.MapObjects)
        {
            if (Object.bDestroyed && Object.bHideAfterDestroyed)
            {
                continue;
            }
            SortedObjects.Add(&Object);
        }
        Algo::Sort(SortedObjects, [](const FCWMapObjectSnapshot* A, const FCWMapObjectSnapshot* B)
        {
            const float AY = A ? A->Y + A->CollisionOffsetY + A->CollisionH * 0.5f : 0.0f;
            const float BY = B ? B->Y + B->CollisionOffsetY + B->CollisionH * 0.5f : 0.0f;
            return AY < BY;
        });

        for (const FCWMapObjectSnapshot* ObjectPtr : SortedObjects)
        {
            if (!ObjectPtr)
            {
                continue;
            }

            const FCWMapObjectSnapshot& Object = *ObjectPtr;
            const FVector2D Base = WorldToScreen(Object.X, Object.Y, Size, FocusPlayer, Scale);
            const FVector2D DrawSize(FMath::Max(6.0f, Object.W * Scale), FMath::Max(6.0f, Object.H * Scale));
            const float AnchorY = FMath::Clamp(Object.AnchorY, 0.45f, 0.72f);
            const FVector2D Position = Base - FVector2D(DrawSize.X * 0.5f, DrawSize.Y * AnchorY);
            if (!CWRunView::IsScreenPointVisible(Base, Size, FMath::Max(DrawSize.X, DrawSize.Y) + 120.0f))
            {
                continue;
            }
            const bool bDamaged = Object.bDestructible && Object.Hp < Object.MaxHp;
            const bool bRecentlyHit = Object.LastHitAt > 0.0f && CachedState.ServerNowMs - Object.LastHitAt <= 140.0;
            const FString TextureKey = MapPropTextureKey(Object.SpriteKey);
            const FSlateBrush* ObjectBrush = GetBrush(TextureKey);
            const FLinearColor Tint = Object.bDestroyed
                ? FLinearColor(0.42f, 0.42f, 0.42f, 0.82f)
                : (bRecentlyHit ? FLinearColor(1.0f, 0.92f, 0.82f, 1.0f) : (bDamaged ? FLinearColor(0.88f, 0.84f, 0.78f, 0.92f) : FLinearColor(1.0f, 1.0f, 1.0f, 0.96f)));

            DrawEllipse(
                OutDrawElements,
                AllottedGeometry,
                ActorLayer,
                Base + FVector2D(0.0f, 12.0f * Scale),
                FVector2D(FMath::Max(18.0f, DrawSize.X * 0.48f * Object.ShadowScale), FMath::Max(7.0f, DrawSize.Y * 0.16f)),
                Object.bDestroyed ? FLinearColor(0.0f, 0.0f, 0.0f, 0.18f) : FLinearColor(0.0f, 0.0f, 0.0f, 0.26f));

            if (ObjectBrush)
            {
                DrawTexture(OutDrawElements, AllottedGeometry, ActorLayer + 1, ObjectBrush, Position, DrawSize, Tint);
            }
            else if (DrawVectorMapObject(ActorLayer + 1, TextureKey, Position, DrawSize, Tint))
            {
                // Web uses SVG for a couple of large props; Slate gets a lightweight native version here.
            }
            else
            {
                DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 1, Position, DrawSize, Object.bDestroyed ? FLinearColor(0.16f, 0.16f, 0.17f, 0.78f) : FLinearColor(0.33f, 0.38f, 0.42f, 0.72f));
            }

            if (bDamaged && !Object.bDestroyed)
            {
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 2, Position + FVector2D(DrawSize.X * 0.34f, DrawSize.Y * 0.42f), Position + FVector2D(DrawSize.X * 0.62f, DrawSize.Y * 0.62f), FLinearColor(0.92f, 0.98f, 1.0f, 0.28f), 2.0f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 2, Position + FVector2D(DrawSize.X * 0.58f, DrawSize.Y * 0.32f), Position + FVector2D(DrawSize.X * 0.72f, DrawSize.Y * 0.54f), FLinearColor(0.92f, 0.98f, 1.0f, 0.22f), 1.4f);

                const float HpPct = FMath::Clamp(Object.Hp / FMath::Max(1.0f, Object.MaxHp), 0.0f, 1.0f);
                const FVector2D BarSize(FMath::Clamp(DrawSize.X * 0.52f, 42.0f, 150.0f), 6.0f);
                const FVector2D BarPos = Base + FVector2D(-BarSize.X * 0.5f, 18.0f * Scale);
                DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 3, BarPos, BarSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
                DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 4, BarPos + FVector2D(1.0f, 1.0f), FVector2D(FMath::Max(0.0f, (BarSize.X - 2.0f) * HpPct), BarSize.Y - 2.0f), FLinearColor(0.98f, 0.22f, 0.18f, 0.92f));
            }
        }
    }

    auto DrawXpCrystal = [&](const FCWPickupSnapshot& Orb)
    {
        const FVector2D OrbRenderPos = GetRenderXpOrbPosition(Orb);
        const FString OrbId = Orb.Id.IsEmpty()
            ? FString::Printf(TEXT("%s:%s:%.0f:%.0f"), *Orb.Kind, *Orb.ItemKey, Orb.X, Orb.Y)
            : Orb.Id;
        const uint32 Seed = GetTypeHash(OrbId);
        const float SeedPhase = static_cast<float>(Seed & 1023u) * 0.017f;
        const float Time = CWRunView::LocalAnimTime(SeedPhase);
        const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 5.6f);
        const float Bob = FMath::Sin(Time * 3.1f) * (3.8f + Pulse * 1.8f);
        const FVector2D BaseP = WorldToScreen(OrbRenderPos.X, OrbRenderPos.Y, Size, FocusPlayer, Scale);
        const FVector2D P = BaseP + FVector2D(FMath::Sin(Time * 2.0f) * 1.6f, Bob);
        if (!CWRunView::IsScreenPointVisible(P, Size, 140.0f))
        {
            return;
        }
        const FRenderEntityState* RenderState = RenderXpOrbs.Find(OrbId);
        FVector2D Velocity = RenderState ? RenderState->Velocity : FVector2D::ZeroVector;
        FVector2D TrailDir = Velocity.GetSafeNormal();

        float MagnetAlpha = 0.0f;
        FVector2D FocusScreen = P;
        if (FocusPlayer)
        {
            const FVector2D FocusRenderPos = GetRenderPosition(*FocusPlayer);
            const FVector2D ToFocus = FocusRenderPos - OrbRenderPos;
            const float Dist = ToFocus.Size();
            if (Dist < 520.0f)
            {
                MagnetAlpha = 1.0f - FMath::Clamp(Dist / 520.0f, 0.0f, 1.0f);
                if (TrailDir.IsNearlyZero())
                {
                    TrailDir = ToFocus.GetSafeNormal();
                }
                FocusScreen = WorldToScreen(FocusRenderPos.X, FocusRenderPos.Y, Size, FocusPlayer, Scale);
            }
        }

        const FLinearColor Core(0.34f, 0.96f, 1.0f, 0.98f);
        const FLinearColor Glow(0.08f, 0.74f, 1.0f, 0.48f);

        if (!TrailDir.IsNearlyZero())
        {
            const FVector2D ScreenDir = TrailDir.GetSafeNormal();
            const float TrailLen = FMath::Clamp(Velocity.Size() * Scale * 0.06f + MagnetAlpha * 42.0f, 18.0f, 78.0f);
            for (int32 I = 0; I < 4; ++I)
            {
                const float T = static_cast<float>(I) / 4.0f;
                const FVector2D A = P - ScreenDir * (TrailLen * (0.28f + T));
                const FVector2D B = P - ScreenDir * (TrailLen * (0.02f + T * 0.44f));
                DrawLine(
                    OutDrawElements,
                    AllottedGeometry,
                    ActorLayer,
                    A,
                    B,
                    FLinearColor(0.10f, 0.90f, 1.0f, (0.24f + MagnetAlpha * 0.24f) * (1.0f - T)),
                    3.2f - T * 1.8f);
            }
        }

        if (MagnetAlpha > 0.08f)
        {
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer, P, FocusScreen, FLinearColor(0.35f, 0.94f, 1.0f, 0.08f * MagnetAlpha), 1.0f);
            const FVector2D ToFocusScreen = (FocusScreen - P).GetSafeNormal();
            if (!ToFocusScreen.IsNearlyZero())
            {
                for (int32 I = 0; I < 3; ++I)
                {
                    const float T = FMath::Fmod(Time * 1.8f + static_cast<float>(I) * 0.33f, 1.0f);
                    const FVector2D SparkP = FMath::Lerp(P, FocusScreen, T);
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 1, SparkP, FVector2D(4.0f, 4.0f), FLinearColor(0.66f, 1.0f, 1.0f, MagnetAlpha * 0.46f));
                }
            }
        }

        DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 1, P, FVector2D(40.0f + Pulse * 7.0f, 40.0f + Pulse * 7.0f), FLinearColor(Glow.R, Glow.G, Glow.B, 0.40f + Pulse * 0.18f), 2.0f, 44, Time * 2.1f);
        DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 1, P, FVector2D(56.0f + (1.0f - Pulse) * 8.0f, 56.0f + (1.0f - Pulse) * 8.0f), FLinearColor(0.16f, 0.90f, 1.0f, 0.20f + Pulse * 0.08f), 1.2f, 44, -Time * 1.4f);
        for (int32 I = 0; I < 5; ++I)
        {
            const float Angle = Time * (1.8f + static_cast<float>(I) * 0.18f) + static_cast<float>(I) * UE_PI * 2.0f / 5.0f;
            const float OrbitR = 24.0f + static_cast<float>((Seed >> (I * 3)) & 7u) + Pulse * 5.0f;
            const FVector2D SparkP = P + FVector2D(FMath::Cos(Angle) * OrbitR, FMath::Sin(Angle) * OrbitR * 0.58f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 2, SparkP, FVector2D(4.0f + Pulse * 1.5f, 4.0f + Pulse * 1.5f), FLinearColor(0.76f, 1.0f, 1.0f, 0.34f));
        }

        const float CrystalW = 10.0f + Pulse * 1.8f;
        const float CrystalTop = 15.0f + Pulse * 2.0f;
        const float CrystalBottom = 16.0f + (1.0f - Pulse) * 2.0f;
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 2, P, FVector2D(25.0f + Pulse * 5.0f, 31.0f + Pulse * 4.0f), FLinearColor(0.02f, 0.22f, 0.34f, 0.70f));
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 3, P + FVector2D(-4.0f, -7.0f), FVector2D(11.0f, 14.0f), FLinearColor(0.82f, 1.0f, 1.0f, 0.24f + Pulse * 0.16f));

        const FVector2D Top = P + FVector2D(0.0f, -CrystalTop);
        const FVector2D Right = P + FVector2D(CrystalW, -2.0f);
        const FVector2D Bottom = P + FVector2D(0.0f, CrystalBottom);
        const FVector2D Left = P + FVector2D(-CrystalW, -2.0f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 4, Top, Right, FLinearColor(0.86f, 1.0f, 1.0f, 0.98f), 2.4f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 4, Right, Bottom, FLinearColor(0.20f, 0.85f, 1.0f, 0.94f), 2.4f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 4, Bottom, Left, FLinearColor(0.08f, 0.58f, 0.92f, 0.90f), 2.4f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 4, Left, Top, FLinearColor(0.58f, 1.0f, 1.0f, 0.94f), 2.4f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 5, Top + FVector2D(1.0f, 1.0f), Bottom + FVector2D(-1.0f, -1.0f), FLinearColor(0.92f, 1.0f, 1.0f, 0.54f + Pulse * 0.18f), 1.2f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 5, Left + FVector2D(1.0f, 0.0f), Right + FVector2D(-1.0f, 0.0f), FLinearColor(0.62f, 0.94f, 1.0f, 0.44f), 1.0f);
        const float GlintT = 0.5f + 0.5f * FMath::Sin(Time * 7.4f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 6, P + FVector2D(-5.0f + GlintT * 3.0f, -11.0f), P + FVector2D(2.0f + GlintT * 4.0f, 9.0f), FLinearColor(0.94f, 1.0f, 1.0f, 0.62f + Pulse * 0.28f), 1.7f);
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 7, P, FVector2D(6.5f + Pulse * 3.0f, 6.5f + Pulse * 3.0f), Core);

        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 1, P + FVector2D(-8.0f, 15.0f), TEXT("XP"), 9.0f, FLinearColor(0.66f, 1.0f, 1.0f, 0.86f), true);
    };

    auto DrawSkillOfferOrb = [&](const FCWPickupSnapshot& Orb)
    {
        const FVector2D OrbRenderPos = GetRenderSkillOrbPosition(Orb);
        const FString SkillId = Orb.ItemKey.IsEmpty() ? TEXT("skill") : Orb.ItemKey;
        const FLinearColor Accent = SkillOrbAccentColor(SkillId);
        const FString FxKind = SkillOrbFxKind(SkillId);
        const FString IconKey = FindSkillIconTextureKey(SkillId);
        const FSlateBrush* IconBrush = GetBrush(IconKey);
        const FString OrbId = CWRunView::PickupRenderKey(Orb, true);
        const uint32 Seed = GetTypeHash(OrbId + SkillId);
        const float Time = CWRunView::LocalAnimTime(static_cast<float>(Seed & 2047u) * 0.011f);
        const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 4.9f);
        const FVector2D BaseP = WorldToScreen(OrbRenderPos.X, OrbRenderPos.Y, Size, FocusPlayer, Scale);
        const FVector2D P = BaseP + FVector2D(FMath::Sin(Time * 1.8f) * 2.2f, FMath::Sin(Time * 3.0f) * 4.4f);
        if (!CWRunView::IsScreenPointVisible(P, Size, 170.0f))
        {
            return;
        }
        const float Radius = 43.0f;
        const float OwnershipAlpha = 1.0f;

        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 1, P + FVector2D(0.0f, 30.0f), FVector2D(70.0f, 19.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.28f * OwnershipAlpha));
        DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 2, P, FVector2D(Radius * (2.45f + Pulse * 0.20f), Radius * (2.45f + Pulse * 0.20f)), FLinearColor(Accent.R, Accent.G, Accent.B, 0.44f + Pulse * 0.16f), 2.0f, 48, Time * 1.9f);
        DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 3, P, FVector2D(Radius * (1.62f + (1.0f - Pulse) * 0.18f), Radius * (1.62f + (1.0f - Pulse) * 0.18f)), FLinearColor(Accent.R, Accent.G, Accent.B, 0.58f + Pulse * 0.16f), 2.3f, 48, -Time * 2.4f);
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 4, P, FVector2D(Radius * 1.20f, Radius * 1.20f), FLinearColor(0.012f, 0.026f, 0.055f, 0.88f * OwnershipAlpha));
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 5, P + FVector2D(-6.0f, -7.0f), FVector2D(Radius * (0.76f + Pulse * 0.12f), Radius * (0.58f + Pulse * 0.12f)), FLinearColor(Accent.R, Accent.G, Accent.B, (0.22f + Pulse * 0.14f) * OwnershipAlpha));
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 6, P + FVector2D(-11.0f, -14.0f), FVector2D(15.0f, 9.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.48f * OwnershipAlpha));

        const int32 Segments = 24;
        const float ProgressStart = -UE_PI * 0.5f + Time * 2.2f;
        for (int32 I = 0; I < Segments; ++I)
        {
            const float SegmentPct = static_cast<float>(I) / static_cast<float>(Segments);
            const float A0 = ProgressStart + SegmentPct * UE_PI * 2.0f;
            const float A1 = ProgressStart + static_cast<float>(I + 1) / static_cast<float>(Segments) * UE_PI * 2.0f;
            DrawLine(
                OutDrawElements,
                AllottedGeometry,
                ActorLayer + 7,
                P + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * (Radius + 3.0f),
                P + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * (Radius + 3.0f),
                FLinearColor(Accent.R, Accent.G, Accent.B, 0.72f * OwnershipAlpha),
                2.2f);
        }

        const int32 BoltCount = FxKind == TEXT("lightning") ? 7 : 5;
        for (int32 I = 0; I < BoltCount; ++I)
        {
            const float SeedOffset = Time * (FxKind == TEXT("blade") ? -1.7f : 1.35f) + static_cast<float>((Seed >> ((I % 4) * 5)) & 31u) * 0.073f;
            const float Angle = SeedOffset + static_cast<float>(I) * UE_PI * 2.0f / static_cast<float>(BoltCount);
            const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
            const FVector2D Perp(-Dir.Y, Dir.X);
            const FVector2D A = P + Dir * (Radius * 0.60f);
            const FVector2D Mid = P + Dir * (Radius * (0.88f + 0.06f * Pulse)) + Perp * (static_cast<float>((I % 3) - 1) * 3.0f);
            const FVector2D B = P + Dir * (Radius * (1.18f + 0.05f * Pulse));
            const float Alpha = FxKind == TEXT("support") ? 0.48f : 0.62f;
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 8, A, Mid, FLinearColor(Accent.R, Accent.G, Accent.B, Alpha), 1.8f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 8, Mid, B, FLinearColor(1.0f, 1.0f, 1.0f, Alpha * 0.72f), 1.1f);

            if (FxKind == TEXT("blade") && (I % 2) == 0)
            {
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 9, P - Perp * 12.0f + Dir * 7.0f, P + Perp * 12.0f + Dir * 15.0f, FLinearColor(1.0f, 0.78f, 0.88f, 0.35f * OwnershipAlpha), 1.4f);
            }
            else if (FxKind == TEXT("blast"))
            {
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 9, B, FVector2D(5.0f + Pulse * 2.0f, 5.0f + Pulse * 2.0f), FLinearColor(1.0f, 0.84f, 0.40f, 0.42f * OwnershipAlpha));
            }
        }

        if (IconBrush)
        {
            const float IconScale = 1.0f + Pulse * 0.075f;
            const FVector2D IconSize(36.0f * IconScale, 36.0f * IconScale);
            DrawTexture(OutDrawElements, AllottedGeometry, ActorLayer + 10, IconBrush, P - IconSize * 0.5f + FVector2D(FMath::Sin(Time * 5.2f) * 1.2f, FMath::Cos(Time * 4.4f) * 1.0f), IconSize, FLinearColor(1.0f, 1.0f, 1.0f, 0.92f + Pulse * 0.08f));
        }
        else
        {
            DrawText(OutDrawElements, AllottedGeometry, ActorLayer + 10, P + FVector2D(-13.0f, -11.0f), SkillOrbBadge(SkillId), 16.0f, FLinearColor(0.88f, 1.0f, 1.0f, 0.98f * OwnershipAlpha), true);
        }

        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 11, P, FVector2D(Radius * 1.10f, Radius * 1.10f), FLinearColor(0.0f, 0.0f, 0.0f, 0.18f * OwnershipAlpha));
        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 12, P, FVector2D(Radius * 1.02f, Radius * 1.02f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.33f * OwnershipAlpha));

        const FString Label = SkillId.Replace(TEXT("_"), TEXT(" ")).Left(20);
        const float LabelW = FMath::Clamp(Label.Len() * 7.1f + 12.0f, 46.0f, 148.0f);
        const FVector2D LabelPos = P + FVector2D(-LabelW * 0.5f, -Radius - 25.0f);
        DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 13, LabelPos, FVector2D(LabelW, 17.0f), FLinearColor(0.01f, 0.026f, 0.040f, 0.70f * OwnershipAlpha));
        DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 14, LabelPos, FVector2D(LabelW, 1.5f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.58f * OwnershipAlpha));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 1, LabelPos + FVector2D(6.0f, 1.0f), Label, 10.5f, FLinearColor(FMath::Min(1.0f, Accent.R + 0.24f), FMath::Min(1.0f, Accent.G + 0.24f), FMath::Min(1.0f, Accent.B + 0.24f), 0.98f * OwnershipAlpha), true);
    };

    for (const FCWPickupSnapshot& Orb : CachedState.XpOrbs)
    {
        if (HiddenXpOrbKeys.Contains(CWRunView::PickupRenderKey(Orb, false)))
        {
            continue;
        }
        if (IsNearLocalPickup(GetRenderXpOrbPosition(Orb), 30.0f))
        {
            continue;
        }
        DrawXpCrystal(Orb);
    }

    for (const FCWPickupSnapshot& Orb : CachedState.SkillOrbs)
    {
        if (HiddenSkillOrbKeys.Contains(CWRunView::PickupRenderKey(Orb, true)))
        {
            continue;
        }
        if (IsNearLocalPickup(GetRenderSkillOrbPosition(Orb), 54.0f, Orb.OwnerId))
        {
            continue;
        }
        DrawSkillOfferOrb(Orb);
    }

    auto DrawLootDrop = [&](const FCWPickupSnapshot& Drop)
    {
        const FVector2D DropRenderPos = GetRenderDropPosition(Drop);
        const FString Key = Drop.ItemKey.ToLower();
        const FSlateBrush* DropBrush = GetBrush(Key);
        const FLinearColor Accent = CWRunView::LootDropAccentColor(Key);
        const FString DropId = Drop.Id.IsEmpty()
            ? FString::Printf(TEXT("%s:%s:%.0f:%.0f"), *Drop.Kind, *Drop.ItemKey, Drop.X, Drop.Y)
            : Drop.Id;
        const uint32 Seed = GetTypeHash(DropId + Key);
        const float Time = CWRunView::LocalAnimTime(static_cast<float>(Seed & 2047u) * 0.013f);
        const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 4.2f);
        const FVector2D BaseP = WorldToScreen(DropRenderPos.X, DropRenderPos.Y, Size, FocusPlayer, Scale);
        const FVector2D P = BaseP + FVector2D(FMath::Sin(Time * 1.6f) * 1.8f, FMath::Sin(Time * 2.8f) * 3.6f);
        if (!CWRunView::IsScreenPointVisible(P, Size, 170.0f))
        {
            return;
        }
        const float ItemScale = FMath::Clamp(Scale, 0.58f, 1.05f);
        const FVector2D PlatformSize(78.0f * ItemScale, 24.0f * ItemScale);
        const FVector2D IconSize(72.0f * ItemScale * (1.0f + Pulse * 0.035f), 48.0f * ItemScale * (1.0f + Pulse * 0.035f));

        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 1, P + FVector2D(0.0f, 18.0f * ItemScale), FVector2D(PlatformSize.X * 1.10f, PlatformSize.Y * 0.76f), FLinearColor(0.0f, 0.0f, 0.0f, 0.32f));
        DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 2, P, FVector2D((88.0f + Pulse * 8.0f) * ItemScale, (56.0f + Pulse * 5.0f) * ItemScale), FLinearColor(Accent.R, Accent.G, Accent.B, 0.46f + Pulse * 0.18f), 1.8f, 42, Time * 1.7f);
        DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 3, P + FVector2D(0.0f, 13.0f * ItemScale), FVector2D(PlatformSize.X * (0.80f + Pulse * 0.08f), PlatformSize.Y * (0.30f + Pulse * 0.06f)), FLinearColor(0.12f, 0.82f, 0.92f, 0.28f + Pulse * 0.12f), 1.2f, 34, -Time * 2.2f);
        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 4, P + FVector2D(-PlatformSize.X * 0.40f, 11.0f * ItemScale), P + FVector2D(PlatformSize.X * 0.40f, 11.0f * ItemScale), FLinearColor(Accent.R, Accent.G, Accent.B, 0.62f), 2.0f);

        const int32 RingSegments = 18;
        for (int32 I = 0; I < RingSegments; ++I)
        {
            const float A0 = Time * 2.4f + static_cast<float>(I) * UE_PI * 2.0f / static_cast<float>(RingSegments);
            const float A1 = A0 + UE_PI * 0.055f;
            const FVector2D R0(FMath::Cos(A0) * 42.0f * ItemScale, FMath::Sin(A0) * 19.0f * ItemScale);
            const FVector2D R1(FMath::Cos(A1) * 42.0f * ItemScale, FMath::Sin(A1) * 19.0f * ItemScale);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 5, P + R0, P + R1, FLinearColor(Accent.R, Accent.G, Accent.B, 0.38f), 1.6f);
        }

        for (int32 I = 0; I < 4; ++I)
        {
            const float Angle = Time * 2.6f + static_cast<float>(I) * UE_PI * 0.5f + static_cast<float>((Seed >> (I * 3)) & 7u) * 0.07f;
            const FVector2D Spark = P + FVector2D(FMath::Cos(Angle) * 38.0f * ItemScale, FMath::Sin(Angle) * 24.0f * ItemScale);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 6, Spark, FVector2D(5.0f + Pulse * 2.0f, 5.0f + Pulse * 2.0f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.42f + Pulse * 0.22f));
        }

        if (DropBrush)
        {
            const FVector2D IconPos = P - FVector2D(IconSize.X * 0.5f, IconSize.Y * 0.56f);
            DrawTexture(OutDrawElements, AllottedGeometry, ActorLayer + 7, DropBrush, IconPos + FVector2D(2.0f, 5.0f), IconSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f));
            DrawTexture(OutDrawElements, AllottedGeometry, ActorLayer + 8, DropBrush, IconPos, IconSize, FLinearColor(0.98f, 1.0f, 1.0f, 0.98f));
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 9, P + FVector2D(-IconSize.X * 0.33f, -IconSize.Y * 0.45f), P + FVector2D(IconSize.X * 0.26f, -IconSize.Y * 0.16f), FLinearColor(1.0f, 1.0f, 1.0f, 0.28f), 2.0f);
        }
        else
        {
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 7, P, FVector2D(36.0f * ItemScale, 36.0f * ItemScale), FLinearColor(Accent.R, Accent.G, Accent.B, 0.80f));
            DrawCenteredBox(OutDrawElements, AllottedGeometry, ActorLayer + 8, P, FVector2D(18.0f * ItemScale, 18.0f * ItemScale), FLinearColor(1.0f, 1.0f, 1.0f, 0.62f));
        }

        if (!Key.IsEmpty())
        {
            const FString Label = Key.Left(12);
            const float LabelW = FMath::Clamp(Label.Len() * 7.1f + 12.0f, 44.0f, 118.0f);
            const FVector2D LabelPos = P + FVector2D(-LabelW * 0.5f, -47.0f * ItemScale);
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 10, LabelPos, FVector2D(LabelW, 16.0f), FLinearColor(0.01f, 0.026f, 0.040f, 0.68f));
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 11, LabelPos, FVector2D(LabelW, 1.5f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.62f));
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 1, LabelPos + FVector2D(6.0f, 1.0f), Label, 10.5f, FLinearColor(FMath::Min(1.0f, Accent.R + 0.18f), FMath::Min(1.0f, Accent.G + 0.18f), FMath::Min(1.0f, Accent.B + 0.18f), 0.98f), true);
        }
    };

    for (const FCWPickupSnapshot& Drop : CachedState.Drops)
    {
        if (HiddenDropKeys.Contains(CWRunView::PickupRenderKey(Drop, true)))
        {
            continue;
        }
        if (IsNearLocalPickup(GetRenderDropPosition(Drop), 46.0f))
        {
            continue;
        }
        DrawLootDrop(Drop);
    }

    for (const FCWBossPortalSnapshot& Portal : CachedState.BossPortals)
    {
        const float LeftMs = static_cast<float>(FMath::Max(0.0, Portal.SpawnAt - CachedState.ServerNowMs));
        const float Ttl = FMath::Max(1.0f, Portal.TtlMs > 0.0f ? Portal.TtlMs : LeftMs);
        const float Progress = 1.0f - FMath::Clamp(LeftMs / Ttl, 0.0f, 1.0f);
        const float Pulse = 0.5f + 0.5f * FMath::Sin(static_cast<float>(CachedState.ServerNowMs) * 0.012f);
        DrawWorldEllipse(ActorLayer + 1, Portal.X, Portal.Y, 72.0f + Pulse * 16.0f, 72.0f + Pulse * 16.0f, FLinearColor(1.0f, 0.05f, 0.05f, 0.24f + Progress * 0.22f));
        DrawWorldEllipse(ActorLayer + 2, Portal.X, Portal.Y, 112.0f + Pulse * 22.0f, 112.0f + Pulse * 22.0f, FLinearColor(1.0f, 0.24f, 0.30f, 0.12f + Progress * 0.15f));
        const FVector2D P = WorldToScreen(Portal.X, Portal.Y, Size, FocusPlayer, Scale);
        DrawText(OutDrawElements, AllottedGeometry, TextLayer, P + FVector2D(-40.0f, -58.0f), FString::Printf(TEXT("BOSS %.1fs"), LeftMs / 1000.0f), 11.0f, FLinearColor(1.0f, 0.72f, 0.72f, 0.96f), true);
    }

    for (const FCWEnemySnapshot& Enemy : CachedState.Enemies)
    {
        const FVector2D EnemyRenderPos = GetRenderEnemyPosition(Enemy);
        const FVector2D P = WorldToScreen(EnemyRenderPos.X, EnemyRenderPos.Y, Size, FocusPlayer, Scale);
        const float Radius = FMath::Clamp(Enemy.Radius * Scale, 8.0f, 42.0f);
        const float SpriteScale = FMath::Clamp(Enemy.SpriteScale, 0.75f, 3.2f);
        const bool bBoss = Enemy.Type.Contains(TEXT("boss"), ESearchCase::IgnoreCase) || Enemy.Behavior.Contains(TEXT("boss"), ESearchCase::IgnoreCase);
        const FVector2D SpriteSize(FMath::Clamp(42.0f * SpriteScale * Scale, 24.0f, 142.0f), FMath::Clamp(50.0f * SpriteScale * Scale, 30.0f, 168.0f));
        const FLinearColor Tint = Enemy.Color.IsEmpty() ? FLinearColor(1.0f, 0.84f, 0.76f, 0.94f) : HexColor(Enemy.Color, FLinearColor(1.0f, 0.84f, 0.76f, 0.94f));
        const float SpriteTopOffsetY = (bBoss ? 6.0f : 2.0f) * Scale - SpriteSize.Y * 0.52f;
        const float SpriteFeetY = SpriteTopOffsetY + SpriteSize.Y * (bBoss ? 0.94f : 0.88f);
        const FVector2D ShadowCenter = P + FVector2D(0.0f, SpriteFeetY + SpriteSize.Y * (bBoss ? 0.025f : 0.015f) + (bBoss ? 6.0f : 3.0f) * Scale);
        DrawEllipse(
            OutDrawElements,
            AllottedGeometry,
            ActorLayer + 2,
            ShadowCenter,
            FVector2D(SpriteSize.X * (bBoss ? 0.66f : 0.72f), SpriteSize.Y * (bBoss ? 0.17f : 0.20f)),
            FLinearColor(0.0f, 0.0f, 0.0f, bBoss ? 0.36f : 0.22f));
        const FSlateBrush* EnemyBrush = GetBrush(TEXT("enemy_mummy"));
        const UTexture2D* EnemyTexture = TextureCache.FindRef(TEXT("enemy_mummy"));
        if (EnemyBrush && EnemyTexture)
        {
            const float FrameW = 37.0f;
            const float FrameH = 45.0f;
            const int32 FrameCount = FMath::Max(1, FMath::FloorToInt(static_cast<float>(EnemyTexture->GetSizeX()) / FrameW));
            const int32 Frame = FMath::Clamp(FMath::FloorToInt(CachedState.ServerNowMs / (bBoss ? 112.0 : 84.0)) % FrameCount, 0, FrameCount - 1);
            DrawTextureRegion(
                OutDrawElements,
                AllottedGeometry,
                ActorLayer + 3,
                EnemyBrush,
                P + FVector2D(-SpriteSize.X * 0.5f, SpriteTopOffsetY),
                SpriteSize,
                FVector2D(Frame * FrameW, 0.0f),
                FVector2D(FrameW, FrameH),
                FVector2D(EnemyTexture->GetSizeX(), EnemyTexture->GetSizeY()),
                FLinearColor(FMath::Lerp(1.0f, Tint.R, bBoss ? 0.16f : 0.34f), FMath::Lerp(1.0f, Tint.G, bBoss ? 0.16f : 0.34f), FMath::Lerp(1.0f, Tint.B, bBoss ? 0.16f : 0.34f), 0.96f));
        }
        else
        {
            DrawCenteredBox(OutDrawElements, AllottedGeometry, ActorLayer + 3, P, FVector2D(Radius * 2.0f, Radius * 2.0f), EnemyColor(Enemy));
        }
        if (Enemy.Hp < Enemy.MaxHp || bBoss)
        {
            const float HpPct = FMath::Clamp(Enemy.Hp / FMath::Max(1.0f, Enemy.MaxHp), 0.0f, 1.0f);
            const FVector2D BarSize(FMath::Clamp(SpriteSize.X * 0.72f, 28.0f, 92.0f), bBoss ? 7.0f : 5.0f);
            const FVector2D BarPos = P + FVector2D(-BarSize.X * 0.5f, -SpriteSize.Y * 0.62f - 8.0f);
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 5, BarPos, BarSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.68f));
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 6, BarPos + FVector2D(1.0f, 1.0f), FVector2D(FMath::Max(0.0f, (BarSize.X - 2.0f) * HpPct), BarSize.Y - 2.0f), bBoss ? FLinearColor(1.0f, 0.08f, 0.05f, 0.96f) : FLinearColor(0.78f, 1.0f, 0.18f, 0.88f));
        }
    }

    for (const FCWBulletSnapshot& Bullet : CachedState.Bullets)
    {
        const FVector2D BulletRenderPos = GetRenderBulletPosition(Bullet);
        FVector2D BulletDir = FVector2D(Bullet.Vx, Bullet.Vy).GetSafeNormal();
        if (BulletDir.IsNearlyZero())
        {
            BulletDir = FVector2D(1.0f, 0.0f);
        }
        const bool bRocket = Bullet.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase) || Bullet.ExplosionRadius > 1.0f;
        const bool bSmg = Bullet.WeaponKey.Contains(TEXT("smg"), ESearchCase::IgnoreCase);
        const bool bSniper = Bullet.WeaponKey.Contains(TEXT("sniper"), ESearchCase::IgnoreCase);
        const bool bShotgun = Bullet.WeaponKey.Contains(TEXT("shotgun"), ESearchCase::IgnoreCase);
        const FVector2D P = WorldToScreen(BulletRenderPos.X, BulletRenderPos.Y, Size, FocusPlayer, Scale);
        if (!CWRunView::IsScreenPointVisible(P, Size, bRocket ? 260.0f : 180.0f))
        {
            continue;
        }
        bool bInsideSolidObject = false;
        const float BulletWorldPad = FMath::Max(8.0f, Bullet.Radius * 1.6f);
        for (const FCWMapObjectSnapshot& Object : CachedState.MapObjects)
        {
            if (!Object.bSolid || (Object.bDestroyed && Object.bHideAfterDestroyed))
            {
                continue;
            }
            const float CollisionW = FMath::Max(16.0f, Object.CollisionW > 0.0f ? Object.CollisionW : Object.W);
            const float CollisionH = FMath::Max(16.0f, Object.CollisionH > 0.0f ? Object.CollisionH : Object.H);
            const float MinX = Object.X - CollisionW * 0.5f - BulletWorldPad;
            const float MaxX = Object.X + CollisionW * 0.5f + BulletWorldPad;
            const float MinY = Object.Y + Object.CollisionOffsetY - CollisionH * 0.5f - BulletWorldPad;
            const float MaxY = Object.Y + Object.CollisionOffsetY + CollisionH * 0.5f + BulletWorldPad;
            if (BulletRenderPos.X >= MinX && BulletRenderPos.X <= MaxX && BulletRenderPos.Y >= MinY && BulletRenderPos.Y <= MaxY)
            {
                bInsideSolidObject = true;
                break;
            }
        }
        if (bInsideSolidObject)
        {
            continue;
        }

        bool bPastObjectImpact = false;
        for (const FCWObjectImpactEventSnapshot& ImpactEvent : CachedState.ObjectImpactEvents)
        {
            const bool bImpactRocket = ImpactEvent.BulletKind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase);
            if (bImpactRocket != bRocket)
            {
                continue;
            }

            FVector2D ImpactDir(ImpactEvent.DirX, ImpactEvent.DirY);
            if (ImpactDir.IsNearlyZero())
            {
                ImpactDir = BulletDir;
            }
            ImpactDir = ImpactDir.GetSafeNormal();
            if (FVector2D::DotProduct(BulletDir, ImpactDir) < 0.64f)
            {
                continue;
            }

            const bool bExactBulletImpact = !ImpactEvent.BulletId.IsEmpty() && ImpactEvent.BulletId == Bullet.Id;
            const FVector2D VisualImpactPos = FVector2D(ImpactEvent.X, ImpactEvent.Y) - ImpactDir * (bImpactRocket ? 46.0f : 22.0f);
            const FVector2D ToBullet = BulletRenderPos - VisualImpactPos;
            const float Along = FVector2D::DotProduct(ToBullet, ImpactDir);
            const float Side = FMath::Abs(ToBullet.X * ImpactDir.Y - ToBullet.Y * ImpactDir.X);
            const float SideTolerance = bExactBulletImpact ? 160.0f : (bRocket ? 110.0f : (bShotgun ? 58.0f : 42.0f));
            const float BackTolerance = bExactBulletImpact ? -32.0f : -FMath::Max(12.0f, Bullet.Radius * 2.0f);
            if (Side <= SideTolerance && Along >= BackTolerance)
            {
                bPastObjectImpact = true;
                break;
            }
        }
        if (bPastObjectImpact)
        {
            continue;
        }

        const float Radius = FMath::Clamp(Bullet.Radius * Scale, 4.0f, 14.0f);
        const float BulletSpeed = FVector2D(Bullet.Vx, Bullet.Vy).Size();
        const FVector2D Perp(-BulletDir.Y, BulletDir.X);
        const float Time = CWRunView::LocalAnimTime(static_cast<float>(GetTypeHash(Bullet.Id)) * 0.00073f);
        const float Flicker = 0.86f + 0.14f * FMath::Sin(Time * 44.0f + Bullet.X * 0.014f + Bullet.Y * 0.017f);
        const float TraceLen = bRocket
            ? 86.0f
            : (bSniper
                ? FMath::Clamp(BulletSpeed * 0.032f, 74.0f, 118.0f)
                : (bShotgun
                    ? FMath::Clamp(BulletSpeed * 0.020f, 36.0f, 58.0f)
                    : FMath::Clamp(BulletSpeed * 0.026f, bSmg ? 38.0f : 44.0f, bSmg ? 74.0f : 86.0f)));
        const FVector2D Tail = P - BulletDir * TraceLen;
        const FVector2D Tip = P - BulletDir * Radius * (bRocket ? 0.15f : 0.35f);
        FLinearColor BulletColor = Bullet.bFromEnemy ? FLinearColor(1.0f, 0.24f, 0.18f, 0.95f) : HexColor(Bullet.Color, FLinearColor(1.0f, 0.82f, 0.22f, 0.95f));
        if (bSmg)
        {
            BulletColor = FLinearColor(0.20f, 0.94f, 1.0f, 0.96f);
        }
        else if (bSniper)
        {
            BulletColor = FLinearColor(0.80f, 0.94f, 1.0f, 0.98f);
        }
        else if (bShotgun)
        {
            BulletColor = FLinearColor(1.0f, 0.56f, 0.16f, 0.96f);
        }
        if (bRocket)
        {
            const float RocketScale = FMath::Clamp(Scale, 0.48f, 0.84f);
            const float BodyLength = FMath::Clamp(21.0f * RocketScale + BulletSpeed * 0.00045f, 17.0f, 29.0f);
            const float BodyWidth = FMath::Clamp(6.2f * RocketScale, 3.9f, 6.8f);
            const float FlameLen = FMath::Clamp(BulletSpeed * 0.036f, 62.0f, 142.0f) * (0.88f + 0.12f * Flicker);
            const FVector2D Nose = P + BulletDir * BodyLength * 0.46f;
            const FVector2D Engine = P - BulletDir * BodyLength * 0.44f;
            const FVector2D FlameTail = Engine - BulletDir * FlameLen;
            const FLinearColor ExhaustHot = Bullet.bFromEnemy ? FLinearColor(1.0f, 0.16f, 0.06f, 0.92f) : FLinearColor(1.0f, 0.74f, 0.16f, 0.92f);
            const FLinearColor ExhaustCore = Bullet.bFromEnemy ? FLinearColor(1.0f, 0.74f, 0.46f, 0.96f) : FLinearColor(1.0f, 0.96f, 0.64f, 0.96f);

            for (int32 I = 0; I < 6; ++I)
            {
                const float K = (static_cast<float>(I) + 1.0f) / 6.0f;
                const float SmokeWave = FMath::Sin(Time * 10.0f + K * 9.4f);
                const float SmokeSize = FMath::Lerp(6.0f, 22.0f, K) * RocketScale;
                const FVector2D SmokePos = Engine
                    - BulletDir * FMath::Lerp(10.0f, FlameLen * 0.88f, K)
                    + Perp * SmokeWave * FMath::Lerp(1.2f, 12.0f, K);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 10, SmokePos, FVector2D(SmokeSize * 1.55f, SmokeSize * 0.78f), FLinearColor(0.05f, 0.055f, 0.060f, 0.08f * (1.0f - K * 0.38f)));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 11, SmokePos + Perp * SmokeWave * 1.5f, FVector2D(SmokeSize * 0.82f, SmokeSize * 0.42f), FLinearColor(0.48f, 0.34f, 0.20f, 0.10f * (1.0f - K * 0.48f)));
            }

            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 12, Engine - BulletDir * 1.0f, FlameTail, FLinearColor(1.0f, 0.12f, 0.02f, 0.20f * Flicker), BodyWidth * 2.05f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 13, Engine, FlameTail + Perp * FMath::Sin(Time * 17.0f) * 7.0f, FLinearColor(ExhaustHot.R, ExhaustHot.G, ExhaustHot.B, 0.82f * Flicker), BodyWidth * 0.92f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 14, Engine + BulletDir * 0.8f, Engine - BulletDir * FlameLen * 0.48f, FLinearColor(ExhaustCore.R, ExhaustCore.G, ExhaustCore.B, 0.98f * Flicker), BodyWidth * 0.34f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 15, Engine - BulletDir * FlameLen * 0.56f, FVector2D(BodyWidth * 1.35f, BodyWidth * 0.74f), FLinearColor(1.0f, 0.42f, 0.06f, 0.34f * Flicker));

            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 16, Engine - BulletDir * 1.5f, Nose, FLinearColor(0.02f, 0.025f, 0.030f, 0.44f), BodyWidth + 3.4f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 17, Engine, Nose, FLinearColor(0.38f, 0.45f, 0.52f, 0.98f), BodyWidth);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, Engine + Perp * BodyWidth * 0.18f, Nose - BulletDir * BodyLength * 0.16f + Perp * BodyWidth * 0.18f, FLinearColor(0.94f, 0.98f, 1.0f, 0.54f), 1.1f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, Engine - Perp * BodyWidth * 0.24f, Nose - BulletDir * BodyLength * 0.22f - Perp * BodyWidth * 0.24f, FLinearColor(0.10f, 0.14f, 0.18f, 0.54f), 1.3f);

            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, Nose - BulletDir * BodyWidth * 1.05f, Nose, FLinearColor(1.0f, 0.24f, 0.10f, 0.98f), BodyWidth * 0.72f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, Nose + BulletDir * 1.5f, FVector2D(BodyWidth * 1.10f, BodyWidth * 0.72f), FLinearColor(1.0f, 0.86f, 0.46f, 0.34f * Flicker));

            const FVector2D FinRootA = Engine + Perp * BodyWidth * 0.45f;
            const FVector2D FinRootB = Engine - Perp * BodyWidth * 0.45f;
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, FinRootA, Engine - BulletDir * BodyWidth * 1.00f + Perp * BodyWidth * 1.05f, FLinearColor(0.88f, 0.22f, 0.10f, 0.86f), 1.6f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, FinRootB, Engine - BulletDir * BodyWidth * 1.00f - Perp * BodyWidth * 1.05f, FLinearColor(0.88f, 0.22f, 0.10f, 0.82f), 1.6f);

            for (int32 I = 0; I < 7; ++I)
            {
                const float K = static_cast<float>(I) / 6.0f;
                const float SparkSide = (K - 0.5f) * BodyWidth * 2.8f;
                const float SparkBack = FMath::Lerp(10.0f, FlameLen * 0.64f, K);
                const FVector2D SparkStart = Engine - BulletDir * (SparkBack * 0.28f) + Perp * SparkSide * 0.22f;
                const FVector2D SparkEnd = Engine - BulletDir * SparkBack + Perp * (SparkSide + FMath::Sin(Time * 21.0f + K * 12.0f) * BodyWidth * 0.55f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 21, SparkStart, SparkEnd, FLinearColor(1.0f, 0.82f, 0.30f, 0.28f * Flicker), I % 2 == 0 ? 1.4f : 0.9f);
            }
        }
        else
        {
            const float CoreWidth = bSniper ? 2.2f : (bSmg ? 3.2f : (bShotgun ? 4.8f : 3.8f));
            const float GlowWidth = bSniper ? 7.0f : (bShotgun ? 11.0f : 8.4f);
            const FVector2D SoftTail = P - BulletDir * TraceLen * (bSniper ? 0.70f : 0.58f);
            const FVector2D CoreTail = P - BulletDir * FMath::Min(TraceLen * (bSniper ? 0.42f : 0.34f), bSniper ? 34.0f : 26.0f);
            const FVector2D GlintTail = P - BulletDir * FMath::Min(TraceLen * 0.20f, 16.0f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 14, SoftTail, Tip, FLinearColor(BulletColor.R, BulletColor.G, BulletColor.B, 0.22f * Flicker), GlowWidth);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 15, CoreTail, Tip + BulletDir * 2.0f, FLinearColor(BulletColor.R, BulletColor.G, BulletColor.B, BulletColor.A * Flicker), CoreWidth);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 16, GlintTail, Tip + BulletDir * 3.0f, FLinearColor(1.0f, 0.96f, bSniper ? 1.0f : 0.72f, 0.70f * Flicker), bSniper ? 1.1f : 1.4f);

            if (bShotgun)
            {
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 15, SoftTail + Perp * 4.0f, Tip + Perp * 6.0f, FLinearColor(1.0f, 0.72f, 0.24f, 0.42f * Flicker), 1.8f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 15, SoftTail - Perp * 4.0f, Tip - Perp * 6.0f, FLinearColor(1.0f, 0.72f, 0.24f, 0.38f * Flicker), 1.6f);
            }

            for (int32 I = 0; I < 3; ++I)
            {
                const float K = (static_cast<float>(I) + 1.0f) / 4.0f;
                const float DotSize = FMath::Max(3.0f, Radius * (0.82f - K * 0.18f));
                const FVector2D DotPos = P - BulletDir * TraceLen * K + Perp * FMath::Sin(Time * 28.0f + K * 9.0f) * (bSniper ? 1.2f : 2.4f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 16, DotPos, FVector2D(DotSize * 1.6f, DotSize), FLinearColor(BulletColor.R, BulletColor.G, BulletColor.B, 0.18f * (1.0f - K)));
            }

            const float TipSize = FMath::Max(7.0f, Radius * (bSniper ? 1.35f : 1.9f));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, Tip, FVector2D(TipSize * 1.25f, TipSize), FLinearColor(BulletColor.R, BulletColor.G, BulletColor.B, 0.34f * Flicker));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 18, Tip + BulletDir * 1.5f, FVector2D(TipSize * 0.45f, TipSize * 0.45f), FLinearColor(1.0f, 0.98f, 0.78f, 0.72f * Flicker));
        }
    }

    for (const FTransientFx& Fx : TransientFx)
    {
        const float T = FMath::Clamp(Fx.Age / FMath::Max(0.001f, Fx.Life), 0.0f, 1.0f);
        const float A = 1.0f - T;
        const FVector2D P = WorldToScreen(Fx.Position.X, Fx.Position.Y, Size, FocusPlayer, Scale);
        const FVector2D E = WorldToScreen(Fx.EndPosition.X, Fx.EndPosition.Y, Size, FocusPlayer, Scale);
        const float FxScreenMargin = FMath::Clamp(Fx.Radius * Scale * 2.2f, 120.0f, 520.0f);
        if (!CWRunView::IsScreenPointVisible(P, Size, FxScreenMargin) && !CWRunView::IsScreenPointVisible(E, Size, FxScreenMargin))
        {
            continue;
        }
        const FLinearColor C(Fx.Color.R, Fx.Color.G, Fx.Color.B, Fx.Color.A * A);
        auto CurrentFocusScreen = [&]() -> FVector2D
        {
            if (FocusPlayer)
            {
                const FVector2D FocusRenderPos = GetRenderPosition(*FocusPlayer);
                return WorldToScreen(FocusRenderPos.X, FocusRenderPos.Y, Size, FocusPlayer, Scale);
            }
            return E;
        };

        if (Fx.Type == TEXT("rocket_trail"))
        {
            FVector2D Dir = (E - P).IsNearlyZero() ? Fx.Direction : (E - P).GetSafeNormal();
            if (Dir.IsNearlyZero())
            {
                Dir = FVector2D(1.0f, 0.0f);
            }
            const FVector2D Perp(-Dir.Y, Dir.X);
            const float TrailLen = FVector2D::Distance(P, E);
            const float Seed = Fx.Radius * 0.041f + Fx.Position.X * 0.003f + Fx.Position.Y * 0.002f;
            const int32 Samples = 13;
            TArray<FVector2D> HotRibbon;
            TArray<FVector2D> CoreRibbon;
            HotRibbon.Reserve(Samples);
            CoreRibbon.Reserve(Samples);

            for (int32 I = 0; I < Samples; ++I)
            {
                const float U = (static_cast<float>(I) + 0.5f) / static_cast<float>(Samples);
                const float FromHead = 1.0f - U;
                const float Wobble = FMath::Sin(Seed + U * UE_PI * 1.55f + T * 1.8f) * FMath::Sin(U * UE_PI);
                const FVector2D Center = FMath::Lerp(P, E, U) + Perp * Wobble * FMath::Min(16.0f, TrailLen * 0.08f) * (0.55f + T * 0.45f);
                const float SmokeSize = FMath::Lerp(24.0f, 7.0f, U) * (0.86f + T * 0.36f);
                const float FireSize = FMath::Lerp(6.0f, 14.0f, U) * (1.0f - T * 0.35f);
                const float SmokeA = A * (0.11f + FromHead * 0.10f);
                const float FireA = A * CWRunView::SmoothStep01((U - 0.14f) / 0.62f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 10, Center - Dir * (2.0f + FromHead * 5.0f), FVector2D(SmokeSize * 1.65f, SmokeSize * 0.78f), FLinearColor(0.035f, 0.038f, 0.042f, SmokeA));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 11, Center + Perp * Wobble * 2.0f, FVector2D(SmokeSize * 0.92f, SmokeSize * 0.44f), FLinearColor(0.48f, 0.34f, 0.17f, SmokeA * 0.72f));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 12, Center, FVector2D(FireSize * 1.72f, FireSize * 0.76f), FLinearColor(1.0f, 0.20f, 0.02f, 0.16f * FireA));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 13, Center + Dir * 2.0f, FVector2D(FireSize * 0.72f, FireSize * 0.38f), FLinearColor(1.0f, 0.84f, 0.34f, 0.30f * FireA));
                HotRibbon.Add(Center);
                CoreRibbon.Add(Center + Dir * 1.5f);
            }

            for (int32 I = 1; I < HotRibbon.Num(); ++I)
            {
                const float U = static_cast<float>(I) / static_cast<float>(HotRibbon.Num() - 1);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 14, HotRibbon[I - 1], HotRibbon[I], FLinearColor(1.0f, 0.26f, 0.03f, 0.15f * A * U), FMath::Lerp(7.0f, 3.0f, U));
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 15, CoreRibbon[I - 1], CoreRibbon[I], FLinearColor(1.0f, 0.92f, 0.55f, 0.28f * A * U), FMath::Lerp(2.6f, 1.1f, U));
            }

            for (int32 I = 0; I < 4; ++I)
            {
                const float K = static_cast<float>(I) / 3.0f;
                const FVector2D SparkStart = FMath::Lerp(E, P, K * 0.64f) + Perp * (K - 0.5f) * 7.0f;
                const FVector2D SparkEnd = SparkStart - Dir * (10.0f + K * 24.0f) + Perp * FMath::Sin(K * 13.0f + Fx.Radius) * 6.0f;
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 16, SparkStart, SparkEnd, FLinearColor(1.0f, 0.78f, 0.24f, 0.17f * A), I % 2 == 0 ? 1.0f : 0.7f);
            }
        }
        else if ((Fx.Type.StartsWith(TEXT("skill_")) && Fx.Type != TEXT("skill_pickup") && Fx.Type != TEXT("skill_dismiss")) || Fx.Type.StartsWith(TEXT("world_")))
        {
            const float R = FMath::Clamp(Fx.Radius * Scale, 34.0f, 560.0f);
            FVector2D Dir = (E - P).IsNearlyZero() ? Fx.Direction : (E - P).GetSafeNormal();
            if (Dir.IsNearlyZero())
            {
                Dir = FVector2D(1.0f, 0.0f);
            }
            const FVector2D Perp(-Dir.Y, Dir.X);
            const float Pop = FMath::Sin(T * UE_PI);
            const float Spin = T * UE_PI * 3.4f + Fx.Radius * 0.019f;
            const FLinearColor CoreColor(1.0f, 1.0f, 1.0f, 0.82f * A);
            const FLinearColor SoftColor(C.R, C.G, C.B, 0.16f * A);
            const FLinearColor GlowColor(C.R, C.G, C.B, 0.46f * A);

            auto DrawDiamond = [&](const FVector2D& Center, float SizePx, const FLinearColor& Color, int32 Layer)
            {
                const FVector2D Top = Center + FVector2D(0.0f, -SizePx);
                const FVector2D Right = Center + FVector2D(SizePx * 0.58f, 0.0f);
                const FVector2D Bottom = Center + FVector2D(0.0f, SizePx);
                const FVector2D Left = Center + FVector2D(-SizePx * 0.58f, 0.0f);
                DrawLine(OutDrawElements, AllottedGeometry, Layer, Top, Right, Color, 2.2f);
                DrawLine(OutDrawElements, AllottedGeometry, Layer, Right, Bottom, Color, 2.2f);
                DrawLine(OutDrawElements, AllottedGeometry, Layer, Bottom, Left, Color, 2.2f);
                DrawLine(OutDrawElements, AllottedGeometry, Layer, Left, Top, Color, 2.2f);
                DrawLine(OutDrawElements, AllottedGeometry, Layer + 1, Top, Bottom, FLinearColor(1.0f, 1.0f, 1.0f, 0.34f * A), 1.0f);
                DrawLine(OutDrawElements, AllottedGeometry, Layer + 1, Left, Right, FLinearColor(Color.R, Color.G, Color.B, 0.38f * A), 1.0f);
            };

            if (Fx.Type == TEXT("world_xp_surge") || Fx.Type == TEXT("world_xp_surge_core"))
            {
                const float CrystalSize = FMath::Clamp(R * (Fx.Type == TEXT("world_xp_surge_core") ? 0.34f : 0.22f), 24.0f, 76.0f);
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 20, P, FVector2D(R * (1.45f + T * 0.38f), R * (0.92f + T * 0.30f)), FLinearColor(0.36f, 0.06f, 0.60f, 0.20f * A));
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 28, P, FVector2D(R * (0.70f + T * 1.25f), R * (0.70f + T * 1.25f)), FLinearColor(0.82f, 0.42f, 1.0f, 0.66f * A), 4.0f, 24, Spin);
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 29, P, FVector2D(R * (0.34f + T * 0.82f), R * (0.34f + T * 0.82f)), FLinearColor(0.40f, 0.96f, 1.0f, 0.54f * A), 2.2f, 18, -Spin * 1.35f);
                DrawDiamond(P - FVector2D(0.0f, CrystalSize * 0.15f), CrystalSize * (0.82f + Pop * 0.20f), FLinearColor(0.92f, 0.68f, 1.0f, 0.92f * A), ActorLayer + 32);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 31, P, FVector2D(CrystalSize * 1.35f, CrystalSize * 1.65f), FLinearColor(0.68f, 0.25f, 1.0f, 0.24f * A));
                for (int32 I = 0; I < 14; ++I)
                {
                    const float K = static_cast<float>(I) / 14.0f;
                    const float Angle = K * UE_PI * 2.0f - Spin * 0.75f;
                    const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                    const float Outer = R * (0.96f + 0.24f * FMath::Sin(Spin + K * 8.0f));
                    const float Inner = R * (0.24f + 0.08f * FMath::Cos(Spin + K * 11.0f));
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 30, P + Ray * Outer, P + Ray * Inner, FLinearColor(0.88f, 0.54f, 1.0f, 0.34f * A), I % 3 == 0 ? 2.4f : 1.3f);
                }
            }
            else if (Fx.Type.Contains(TEXT("beam"), ESearchCase::IgnoreCase))
            {
                const bool bChain = Fx.Type.Contains(TEXT("chain"), ESearchCase::IgnoreCase);
                if (bChain)
                {
                    const int32 Segs = 7;
                    FVector2D Prev = P;
                    for (int32 I = 1; I <= Segs; ++I)
                    {
                        const float U = static_cast<float>(I) / static_cast<float>(Segs);
                        const float Jitter = (I == Segs ? 0.0f : FMath::Sin(Spin * 3.2f + I * 2.17f) * (10.0f + R * 0.035f));
                        const FVector2D Curr = FMath::Lerp(P, E, U) + Perp * Jitter;
                        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, Prev, Curr, FLinearColor(C.R, C.G, C.B, 0.30f * A), 13.0f);
                        DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 30, Prev, Curr, FLinearColor(0.90f, 0.98f, 1.0f, 0.92f * A), 3.4f);
                        Prev = Curr;
                    }
                }
                else
                {
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 28, P, E, FLinearColor(C.R, C.G, C.B, 0.26f * A), 24.0f);
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, P - Perp * 4.0f, E - Perp * 4.0f, FLinearColor(C.R, C.G, C.B, 0.68f * A), 7.0f);
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 30, P + Perp * 4.0f, E + Perp * 4.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.86f * A), 2.6f);
                    DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 31, E, FVector2D(46.0f + Pop * 18.0f, 46.0f + Pop * 18.0f), FLinearColor(C.R, C.G, C.B, 0.58f * A), 2.2f, 16, Spin);
                }
            }
            else if (Fx.Type == TEXT("skill_missile_trail"))
            {
                const float TrailLen = FVector2D::Distance(P, E);
                const float RocketBody = FMath::Clamp(14.0f + R * 0.045f, 13.0f, 24.0f);
                const float RocketWidth = FMath::Clamp(4.6f + R * 0.014f, 4.2f, 8.5f);
                const FVector2D Nose = E + Dir * RocketBody * 0.34f;
                const FVector2D Engine = E - Dir * RocketBody * 0.50f;
                const FVector2D FlameTail = FMath::Lerp(P, Engine, 0.18f);

                for (int32 I = 0; I < 8; ++I)
                {
                    const float K = (static_cast<float>(I) + 0.5f) / 8.0f;
                    const FVector2D Smoke = FMath::Lerp(P, Engine, 1.0f - K * 0.82f) + Perp * FMath::Sin(Spin * 1.8f + K * 8.0f) * (3.0f + K * 12.0f);
                    const float SmokeSize = FMath::Lerp(8.0f, 22.0f, K);
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 24, Smoke, FVector2D(SmokeSize * 1.50f, SmokeSize * 0.72f), FLinearColor(0.045f, 0.050f, 0.055f, 0.12f * A * (1.0f - K * 0.34f)));
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 25, Smoke + Perp * 1.5f, FVector2D(SmokeSize * 0.70f, SmokeSize * 0.34f), FLinearColor(0.90f, 0.42f, 0.10f, 0.11f * A * (1.0f - K * 0.42f)));
                }

                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 26, Engine, FlameTail, FLinearColor(1.0f, 0.18f, 0.02f, 0.22f * A), RocketWidth * 2.10f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 27, Engine, FMath::Lerp(Engine, FlameTail, 0.58f) + Perp * FMath::Sin(Spin) * 5.0f, FLinearColor(1.0f, 0.66f, 0.10f, 0.78f * A), RocketWidth * 0.86f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 28, Engine + Dir * 1.2f, Engine - Dir * RocketBody * 0.78f, FLinearColor(1.0f, 0.96f, 0.70f, 0.90f * A), RocketWidth * 0.28f);

                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, Engine, Nose, FLinearColor(0.05f, 0.07f, 0.09f, 0.44f * A), RocketWidth + 3.0f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 30, Engine, Nose, FLinearColor(0.44f, 0.50f, 0.58f, 0.96f * A), RocketWidth);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 31, Engine + Perp * RocketWidth * 0.18f, Nose - Dir * RocketBody * 0.20f + Perp * RocketWidth * 0.18f, FLinearColor(1.0f, 1.0f, 0.94f, 0.48f * A), 1.4f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Nose - Dir * RocketWidth * 1.0f, Nose, FLinearColor(C.R, C.G, C.B, 0.92f * A), RocketWidth * 0.62f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Engine + Perp * RocketWidth * 0.42f, Engine - Dir * RocketWidth * 0.92f + Perp * RocketWidth * 1.22f, FLinearColor(1.0f, 0.24f, 0.10f, 0.76f * A), 2.1f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Engine - Perp * RocketWidth * 0.42f, Engine - Dir * RocketWidth * 0.92f - Perp * RocketWidth * 1.22f, FLinearColor(1.0f, 0.24f, 0.10f, 0.70f * A), 2.1f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 33, Nose + Dir * 1.0f, FVector2D(RocketWidth * 1.25f + Pop * 5.0f, RocketWidth * 0.78f + Pop * 2.0f), FLinearColor(1.0f, 0.88f, 0.50f, 0.38f * A));
            }
            else if (Fx.Type == TEXT("skill_missile_launch"))
            {
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 25, P, FVector2D(R * 0.80f, R * 0.50f), SoftColor);
                for (int32 I = 0; I < 9; ++I)
                {
                    const float Angle = FMath::Atan2(Dir.Y, Dir.X) + (static_cast<float>(I) - 4.0f) * 0.20f;
                    const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, P + Ray * 12.0f, P + Ray * (R * (0.42f + T * 0.38f)), FLinearColor(C.R, C.G, C.B, 0.56f * A), I % 2 == 0 ? 3.2f : 1.8f);
                }
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 30, P, FVector2D(R * (0.26f + T * 0.52f), R * (0.26f + T * 0.52f)), GlowColor, 3.0f, 18, Spin);
            }
            else if (Fx.Type.Contains(TEXT("slash"), ESearchCase::IgnoreCase) || Fx.Type.Contains(TEXT("melee_arc"), ESearchCase::IgnoreCase) || Fx.Type.Contains(TEXT("wire"), ESearchCase::IgnoreCase) || Fx.Type.Contains(TEXT("bat"), ESearchCase::IgnoreCase) || Fx.Type.Contains(TEXT("plasma"), ESearchCase::IgnoreCase))
            {
                const bool bWire = Fx.Type.Contains(TEXT("wire"), ESearchCase::IgnoreCase);
                const bool bPlasma = Fx.Type.Contains(TEXT("plasma"), ESearchCase::IgnoreCase);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 27, P - Perp * R * 0.08f, E + Perp * R * 0.08f, FLinearColor(C.R, C.G, C.B, (bPlasma ? 0.30f : 0.22f) * A), bWire ? 10.0f : 22.0f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, P, E, FLinearColor(C.R, C.G, C.B, 0.84f * A), bWire ? 2.0f : 5.2f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 30, P + Perp * 8.0f * Pop, E + Perp * 8.0f * Pop, CoreColor, bWire ? 1.1f : 2.2f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 26, FMath::Lerp(P, E, 0.52f), FVector2D(R * 0.92f, R * 0.28f), FLinearColor(C.R, C.G, C.B, 0.10f * A));
                for (int32 I = 0; I < 7; ++I)
                {
                    const float K = static_cast<float>(I) / 6.0f;
                    const FVector2D M = FMath::Lerp(P, E, K) + Perp * FMath::Sin(Spin + K * 7.0f) * (bWire ? 6.0f : 13.0f);
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 31, M, FVector2D(4.5f + Pop * 3.0f, 4.5f + Pop * 3.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.46f * A));
                }
            }
            else if (Fx.Type == TEXT("skill_blade_orbit"))
            {
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 27, P, FVector2D(R * (0.86f + T * 0.28f), R * (0.86f + T * 0.28f)), GlowColor, 4.0f, 18, Spin);
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 28, P, FVector2D(R * (0.46f + T * 0.18f), R * (0.46f + T * 0.18f)), CoreColor, 1.8f, 12, -Spin);
                for (int32 I = 0; I < 6; ++I)
                {
                    const float Angle = Spin + static_cast<float>(I) * UE_PI * 2.0f / 6.0f;
                    const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                    const FVector2D Tangent(-Ray.Y, Ray.X);
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 30, P + Ray * R * 0.28f - Tangent * 18.0f, P + Ray * R * 0.66f + Tangent * 28.0f, FLinearColor(C.R, C.G, C.B, 0.74f * A), 2.5f);
                }
            }
            else if (Fx.Type.Contains(TEXT("shockwave"), ESearchCase::IgnoreCase) || Fx.Type.Contains(TEXT("psi"), ESearchCase::IgnoreCase) || Fx.Type == TEXT("skill_burst") || Fx.Type == TEXT("skill_chain_origin") || Fx.Type == TEXT("world_pulse"))
            {
                const bool bPsi = Fx.Type.Contains(TEXT("psi"), ESearchCase::IgnoreCase);
                const float RingScale = bPsi ? 1.18f : 1.0f;
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 19, P, FVector2D(R * (1.06f + T * 0.42f), R * (0.58f + T * 0.24f)), SoftColor);
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 27, P, FVector2D(R * RingScale * (0.36f + T * 1.12f), R * RingScale * (0.36f + T * 1.12f)), GlowColor, bPsi ? 3.2f : 4.2f, bPsi ? 22 : 20, Spin);
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 28, P, FVector2D(R * (0.18f + T * 0.54f), R * (0.18f + T * 0.54f)), CoreColor, bPsi ? 1.6f : 2.2f, bPsi ? 9 : 16, -Spin * 1.2f);
                for (int32 I = 0; I < (bPsi ? 20 : 14); ++I)
                {
                    const float Angle = static_cast<float>(I) * UE_PI * 2.0f / static_cast<float>(bPsi ? 20 : 14) + Spin * (bPsi ? 0.55f : 0.25f);
                    const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, P + Ray * R * (0.10f + T * 0.12f), P + Ray * R * (0.28f + T * 0.84f), FLinearColor(C.R, C.G, C.B, (bPsi ? 0.40f : 0.32f) * A), bPsi && I % 2 == 0 ? 2.0f : 1.2f);
                }
            }
            else
            {
                const bool bVoid = Fx.Type.Contains(TEXT("void"), ESearchCase::IgnoreCase);
                const bool bCryo = Fx.Type.Contains(TEXT("cryo"), ESearchCase::IgnoreCase);
                const bool bHammer = Fx.Type.Contains(TEXT("hammer"), ESearchCase::IgnoreCase);
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 20, P, FVector2D(R * (bHammer ? 1.25f : 0.72f), R * (bHammer ? 0.72f : 0.46f)), bVoid ? FLinearColor(0.10f, 0.0f, 0.16f, 0.34f * A) : SoftColor);
                DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 27, P, FVector2D(R * (0.25f + T * 0.94f), R * (0.25f + T * 0.94f)), bCryo ? FLinearColor(0.72f, 0.96f, 1.0f, 0.58f * A) : GlowColor, bHammer ? 5.0f : 3.0f, bVoid ? 7 : 18, Spin);
                for (int32 I = 0; I < (bHammer ? 18 : 11); ++I)
                {
                    const float Angle = static_cast<float>(I) * UE_PI * 2.0f / static_cast<float>(bHammer ? 18 : 11) + Spin;
                    const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                    const float Len = R * (0.22f + T * (bHammer ? 0.94f : 0.62f));
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, P + Ray * 5.0f, P + Ray * Len, bCryo ? FLinearColor(0.84f, 1.0f, 1.0f, 0.52f * A) : FLinearColor(C.R, C.G, C.B, 0.58f * A), I % 3 == 0 ? 2.2f : 1.2f);
                    if (I % 2 == 0)
                    {
                        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 30, P + Ray * Len, FVector2D(6.0f + Pop * 4.0f, 6.0f + Pop * 4.0f), CoreColor);
                    }
                }
            }
        }
        else if (Fx.Type == TEXT("dodge"))
        {
            const FVector2D Dir = (E - P).GetSafeNormal();
            const FVector2D Perp(-Dir.Y, Dir.X);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 17, P, E, FLinearColor(0.62f, 0.74f, 1.0f, 0.20f * A), 22.0f * (1.0f + T * 0.8f));
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, P + Perp * 5.0f, E - Dir * 8.0f + Perp * 5.0f, FLinearColor(0.86f, 0.96f, 1.0f, 0.44f * A), 2.4f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, P - Perp * 6.0f, E - Dir * 14.0f - Perp * 6.0f, FLinearColor(C.R, C.G, C.B, 0.34f * A), 1.8f);
            for (int32 I = 0; I < 10; ++I)
            {
                const float K = (static_cast<float>(I) + 0.5f) / 10.0f;
                const float Side = (static_cast<float>((I * 37) % 17) - 8.0f) * (0.9f + T);
                const FVector2D M = FMath::Lerp(P, E, K) + Perp * Side - Dir * T * 20.0f;
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, M, FVector2D((16.0f + Fx.Radius * 0.22f) * A, (6.0f + Fx.Radius * 0.08f) * A), FLinearColor(0.88f, 0.94f, 1.0f, 0.18f * A));
            }
        }
        else if (Fx.Type == TEXT("xp_absorb"))
        {
            const FVector2D Target = CurrentFocusScreen();
            const FVector2D Dir = (Target - P).GetSafeNormal();
            const FVector2D Perp(-Dir.Y, Dir.X);
            const float TravelAlpha = FMath::Clamp(T * 1.28f, 0.0f, 1.0f);
            const FVector2D Head = FMath::Lerp(P, Target, TravelAlpha);
            const float TrailAlpha = FMath::Max(0.0f, TravelAlpha - 0.16f);
            const FVector2D TrailTail = FMath::Lerp(P, Target, TrailAlpha);
            const FVector2D TrailMid = FMath::Lerp(TrailTail, Head, 0.58f) + Perp * FMath::Sin(T * 22.0f) * (5.0f + Fx.Radius * Scale * 0.025f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 17, TrailTail, TrailMid, FLinearColor(0.12f, 0.78f, 1.0f, 0.24f * A), 5.0f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, TrailMid, Head, FLinearColor(0.70f, 1.0f, 1.0f, 0.58f * A), 2.2f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 19, Head, FVector2D(10.0f * A + 4.0f, 10.0f * A + 4.0f), FLinearColor(0.72f, 1.0f, 1.0f, 0.50f * A));
            for (int32 I = 0; I < 8; ++I)
            {
                const float K = static_cast<float>(I) / 8.0f;
                const float U = FMath::Clamp(TravelAlpha - K * 0.045f, TrailAlpha, TravelAlpha);
                const float Wobble = FMath::Sin((T + K) * 18.0f) * (7.0f + Fx.Radius * Scale * 0.04f);
                const FVector2D SparkP = FMath::Lerp(P, Target, U) + Perp * Wobble * (1.0f - K * 0.08f);
                const float SparkA = A * (1.0f - K * 0.08f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, SparkP, FVector2D(5.8f * SparkA + 1.8f, 5.8f * SparkA + 1.8f), FLinearColor(0.68f, 1.0f, 1.0f, 0.42f * SparkA));
            }
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 21, Target, FVector2D(Fx.Radius * Scale * (0.65f + T * 1.15f), Fx.Radius * Scale * (0.65f + T * 1.15f)), FLinearColor(0.42f, 0.96f, 1.0f, 0.42f * A), 2.0f, 42, T * UE_PI * 2.0f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 22, Target, FVector2D(Fx.Radius * Scale * (0.24f + T * 0.18f), Fx.Radius * Scale * (0.24f + T * 0.18f)), FLinearColor(0.58f, 1.0f, 1.0f, 0.28f * A));
            for (int32 I = 0; I < 6; ++I)
            {
                const float Angle = static_cast<float>(I) * UE_PI * 2.0f / 6.0f + T * 4.0f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 23, Target + Ray * (8.0f + T * 7.0f), Target + Ray * (18.0f + T * 18.0f), FLinearColor(0.84f, 1.0f, 1.0f, 0.34f * A), 1.4f);
            }
        }
        else if (Fx.Type == TEXT("loot_pickup") || Fx.Type == TEXT("skill_pickup"))
        {
            const bool bSkillFx = Fx.Type == TEXT("skill_pickup");
            const FVector2D Target = CurrentFocusScreen();
            const FVector2D Dir = (Target - P).GetSafeNormal();
            const FVector2D Perp(-Dir.Y, Dir.X);
            const float TravelAlpha = FMath::Clamp(T * (bSkillFx ? 1.08f : 1.24f), 0.0f, 1.0f);
            const FVector2D Head = FMath::Lerp(P, Target, TravelAlpha);
            const float Spin = T * UE_PI * (bSkillFx ? 5.6f : 4.2f);
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 17, P, FVector2D(Fx.Radius * Scale * (1.15f + T * 1.55f), Fx.Radius * Scale * (1.15f + T * 1.55f)), FLinearColor(C.R, C.G, C.B, (bSkillFx ? 0.44f : 0.36f) * A), bSkillFx ? 2.4f : 2.0f, bSkillFx ? 5 : 4, Spin);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, Target, FLinearColor(C.R, C.G, C.B, 0.16f * A), bSkillFx ? 7.5f : 5.5f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, P, Head, FLinearColor(1.0f, 1.0f, 1.0f, 0.54f * A), bSkillFx ? 2.8f : 2.2f);
            for (int32 I = 0; I < (bSkillFx ? 12 : 8); ++I)
            {
                const float K = static_cast<float>(I) / static_cast<float>(bSkillFx ? 12 : 8);
                const float U = FMath::Clamp(TravelAlpha - K * 0.10f, 0.0f, 1.0f);
                const float Wobble = FMath::Sin(Spin + K * 8.0f) * (bSkillFx ? 16.0f : 10.0f) * (1.0f - U);
                const FVector2D SparkP = FMath::Lerp(P, Target, U) + Perp * Wobble;
                const float SparkSize = (bSkillFx ? 6.0f : 4.5f) * A + 2.0f;
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, SparkP, FVector2D(SparkSize, SparkSize), FLinearColor(C.R, C.G, C.B, (bSkillFx ? 0.58f : 0.46f) * A));
            }
            for (int32 I = 0; I < (bSkillFx ? 8 : 6); ++I)
            {
                const float Angle = Spin + static_cast<float>(I) * UE_PI * 2.0f / static_cast<float>(bSkillFx ? 8 : 6);
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Inner = bSkillFx ? 10.0f : 7.0f;
                const float Outer = bSkillFx ? 44.0f : 32.0f;
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 21, Target + Ray * (Inner + T * 8.0f), Target + Ray * (Outer + T * 20.0f), FLinearColor(C.R, C.G, C.B, 0.34f * A), bSkillFx ? 1.7f : 1.3f);
            }
        }
        else if (Fx.Type == TEXT("skill_dismiss"))
        {
            const float BurstT = FMath::Clamp(T * 1.18f, 0.0f, 1.0f);
            const float Pop = FMath::Sin(BurstT * UE_PI);
            const float Spin = Fx.Radius * 0.021f + T * UE_PI * 3.4f;
            const float PulseRadius = Fx.Radius * Scale * (0.36f + BurstT * 1.25f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 16, P, FVector2D(PulseRadius * 0.82f, PulseRadius * 0.82f), FLinearColor(C.R, C.G, C.B, 0.12f * A * (0.35f + Pop)));
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 17, P, FVector2D(PulseRadius, PulseRadius), FLinearColor(C.R, C.G, C.B, 0.50f * A), 2.2f, 6, Spin);
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, FVector2D(PulseRadius * 0.62f, PulseRadius * 0.62f), FLinearColor(1.0f, 1.0f, 1.0f, 0.34f * A), 1.3f, 3, -Spin * 1.35f);
            for (int32 I = 0; I < 12; ++I)
            {
                const float K = static_cast<float>(I) / 12.0f;
                const float Angle = K * UE_PI * 2.0f + Spin + FMath::Sin(K * 19.0f) * 0.18f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Jitter = static_cast<float>((I * 29) % 7) * 0.035f;
                const FVector2D ShardStart = P + Ray * (9.0f + BurstT * 11.0f);
                const FVector2D ShardEnd = P + Ray * (Fx.Radius * Scale * (0.34f + BurstT * (0.62f + Jitter)));
                const float Width = 1.0f + (I % 3 == 0 ? 0.7f : 0.0f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, ShardStart, ShardEnd, FLinearColor(C.R, C.G, C.B, 0.62f * A), Width);
                if (I % 2 == 0)
                {
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, ShardEnd, FVector2D(3.2f + Pop * 2.0f, 3.2f + Pop * 2.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.42f * A));
                }
            }
            for (int32 I = 0; I < 5; ++I)
            {
                const float Angle = Spin * 0.65f + static_cast<float>(I) * UE_PI * 2.0f / 5.0f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const FVector2D ArcA = P + Ray * (10.0f + BurstT * 18.0f);
                const FVector2D ArcB = P + FVector2D(-Ray.Y, Ray.X) * (12.0f + BurstT * 24.0f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 21, ArcA, FMath::Lerp(ArcA, ArcB, 0.42f), FLinearColor(C.R, C.G, C.B, 0.28f * A), 1.1f);
            }
        }
        else if (Fx.Type == TEXT("muzzle_ground"))
        {
            const FVector2D Dir = Fx.Direction;
            const FVector2D Perp(-Dir.Y, Dir.X);
            const FVector2D Center = P + Dir * (5.0f + Fx.Radius * 0.08f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 16, Center, FVector2D(Fx.Radius * 1.18f * A, Fx.Radius * 0.54f * A), FLinearColor(C.R, C.G, C.B, 0.15f * A));
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 17, Center - Dir * 7.0f, Center + Dir * (Fx.Radius * 0.74f), FLinearColor(1.0f, 0.62f, 0.10f, 0.42f * A), 3.4f * A);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, Center - Perp * 8.0f, Center + Dir * (Fx.Radius * 0.34f) + Perp * 5.0f, FLinearColor(1.0f, 0.95f, 0.68f, 0.36f * A), 1.1f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, Center + Perp * 8.0f, Center + Dir * (Fx.Radius * 0.30f) - Perp * 5.0f, FLinearColor(1.0f, 0.95f, 0.68f, 0.30f * A), 1.0f);
        }
        else if (Fx.Type == TEXT("muzzle"))
        {
            const FVector2D Dir = Fx.Direction;
            const FVector2D Perp(-Dir.Y, Dir.X);
            const float Flame = Fx.Radius * (0.58f + T * 0.18f);
            const FVector2D Root = P - Dir * 3.0f;
            const FVector2D Tip = P + Dir * Flame;
            const FVector2D WingA = P + Dir * (Flame * 0.40f) + Perp * (Fx.Radius * 0.24f);
            const FVector2D WingB = P + Dir * (Flame * 0.34f) - Perp * (Fx.Radius * 0.22f);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, P + Dir * (Flame * 0.28f), FVector2D(Fx.Radius * 0.86f * A, Fx.Radius * 0.52f * A), FLinearColor(C.R, C.G, C.B, 0.22f * A));
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, Root, Tip, FLinearColor(1.0f, 0.58f, 0.08f, 0.78f * A), 5.4f * A);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, Root + Perp * 3.0f, WingA, FLinearColor(1.0f, 0.95f, 0.62f, 0.70f * A), 1.9f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, Root - Perp * 3.0f, WingB, FLinearColor(1.0f, 0.86f, 0.30f, 0.56f * A), 1.5f);
            for (int32 I = 0; I < 5; ++I)
            {
                const float K = static_cast<float>(I) / 4.0f;
                const float Side = (K - 0.5f) * Fx.Radius * 0.42f;
                const FVector2D SparkDir = (Dir * (0.92f + K * 0.10f) + Perp * (K - 0.5f) * 0.46f).GetSafeNormal();
                const FVector2D SparkStart = P + Perp * Side * 0.36f + Dir * 2.0f;
                const FVector2D SparkEnd = SparkStart + SparkDir * (Fx.Radius * (0.30f + 0.10f * static_cast<float>(I % 2)));
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 20, SparkStart, SparkEnd, FLinearColor(1.0f, 0.86f, 0.34f, 0.34f * A), 1.0f);
            }
        }
        else if (Fx.Type == TEXT("spark"))
        {
            for (int32 I = 0; I < 7; ++I)
            {
                const float Angle = (static_cast<float>(I) / 7.0f) * PI * 2.0f + T * 1.8f;
                const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, P + Dir * Fx.Radius * (0.35f + T), FLinearColor(C.R, C.G, C.B, 0.72f * A), 1.4f);
            }
        }
        else if (Fx.Type.StartsWith(TEXT("impact")))
        {
            const bool bMetal = Fx.Type.Contains(TEXT("metal"));
            const bool bWood = Fx.Type.Contains(TEXT("wood"));
            FVector2D BackDir = Fx.Direction.IsNearlyZero() ? FVector2D(0.0f, -1.0f) : -Fx.Direction.GetSafeNormal();
            const FVector2D Perp(-BackDir.Y, BackDir.X);
            const FLinearColor Dust = bMetal
                ? FLinearColor(0.48f, 0.60f, 0.72f, 1.0f)
                : (bWood ? FLinearColor(0.70f, 0.38f, 0.14f, 1.0f) : FLinearColor(0.62f, 0.62f, 0.58f, 1.0f));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, P + BackDir * (8.0f + 12.0f * T), FVector2D(Fx.Radius * Scale * (0.82f + T * 0.56f), Fx.Radius * Scale * (0.42f + T * 0.34f)), FLinearColor(Dust.R, Dust.G, Dust.B, 0.24f * A));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, FVector2D(Fx.Radius * Scale * (0.45f + T * 0.60f), Fx.Radius * Scale * (0.28f + T * 0.42f)), FLinearColor(C.R, C.G, C.B, (bMetal ? 0.30f : 0.22f) * A));
            const int32 SparkCount = bMetal ? 10 : (bWood ? 5 : 7);
            for (int32 I = 0; I < SparkCount; ++I)
            {
                const float Spread = -0.72f + static_cast<float>(I) * (1.44f / FMath::Max(1, SparkCount - 1));
                const float Wobble = static_cast<float>((I * 37) % 13) * 0.012f;
                const FVector2D Dir = (BackDir * FMath::Cos(Spread + Wobble) + Perp * FMath::Sin(Spread + Wobble)).GetSafeNormal();
                const float Len = Fx.Radius * Scale * (0.18f + T * (0.58f + static_cast<float>((I * 19) % 7) * 0.045f));
                const FLinearColor SparkColor = bWood ? FLinearColor(1.0f, 0.72f, 0.25f, 0.76f * A) : FLinearColor(1.0f, 0.88f, 0.46f, 0.82f * A);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, P + Dir * 3.0f, P + Dir * Len, SparkColor, bMetal ? 1.8f : 1.3f);
            }
        }
        else if (Fx.Type.StartsWith(TEXT("explosion")))
        {
            const FString SurfaceKey = Fx.Type.ToLower();
            const bool bAsphalt = SurfaceKey.Contains(TEXT("asphalt"));
            const bool bGrass = SurfaceKey.Contains(TEXT("grass"));
            const bool bDirt = SurfaceKey.Contains(TEXT("dirt"));
            const bool bMetal = SurfaceKey.Contains(TEXT("metal"));
            const bool bWood = SurfaceKey.Contains(TEXT("wood"));
            const float Radius = FMath::Clamp(Fx.Radius * Scale, 76.0f, 260.0f);
            const float Blast = CWRunView::SmoothStep01(FMath::Clamp(T / 0.34f, 0.0f, 1.0f));
            const float Heat = CWRunView::SmoothStep01(FMath::Clamp((1.0f - T) / 0.52f, 0.0f, 1.0f));
            const FLinearColor Dust = bGrass
                ? FLinearColor(0.24f, 0.34f, 0.16f, 1.0f)
                : (bDirt
                    ? FLinearColor(0.46f, 0.30f, 0.16f, 1.0f)
                    : (bMetal
                        ? FLinearColor(0.54f, 0.60f, 0.66f, 1.0f)
                        : (bWood ? FLinearColor(0.56f, 0.32f, 0.13f, 1.0f) : FLinearColor(0.43f, 0.43f, 0.40f, 1.0f))));
            const FLinearColor Chunk = bAsphalt
                ? FLinearColor(0.055f, 0.060f, 0.064f, 1.0f)
                : (bGrass
                    ? FLinearColor(0.16f, 0.28f, 0.08f, 1.0f)
                    : (bDirt ? FLinearColor(0.36f, 0.22f, 0.10f, 1.0f) : Dust));
            FVector2D BackDir = Fx.Direction.IsNearlyZero() ? FVector2D(0.0f, -1.0f) : -Fx.Direction.GetSafeNormal();
            const FVector2D Perp(-BackDir.Y, BackDir.X);

            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 13, P + FVector2D(0.0f, Radius * 0.10f), FVector2D(Radius * (1.25f + Blast * 0.35f), Radius * (0.48f + Blast * 0.22f)), FLinearColor(0.0f, 0.0f, 0.0f, 0.30f * A));
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 14, P, FVector2D(Radius * (0.82f + Blast * 0.82f), Radius * (0.48f + Blast * 0.62f)), FLinearColor(Dust.R, Dust.G, Dust.B, 0.30f * A));
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 17, P, FVector2D(Radius * (0.25f + Blast * 1.42f), Radius * (0.25f + Blast * 1.42f)), FLinearColor(1.0f, 0.74f, 0.18f, 0.46f * A), 5.0f, 24, T * UE_PI);
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, FVector2D(Radius * (0.16f + Blast * 0.95f), Radius * (0.16f + Blast * 0.95f)), FLinearColor(1.0f, 0.20f, 0.04f, 0.54f * A), 8.0f, 18, -T * UE_PI * 1.6f);

            const float FirePop = FMath::Sin(FMath::Clamp(T * 1.28f, 0.0f, 1.0f) * UE_PI);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 19, P - FVector2D(0.0f, Radius * 0.12f * FirePop), FVector2D(Radius * (0.74f + FirePop * 0.44f), Radius * (0.48f + FirePop * 0.34f)), FLinearColor(1.0f, 0.18f, 0.02f, 0.46f * Heat));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, P - FVector2D(0.0f, Radius * 0.20f * FirePop), FVector2D(Radius * (0.38f + FirePop * 0.32f), Radius * (0.30f + FirePop * 0.28f)), FLinearColor(1.0f, 0.74f, 0.12f, 0.66f * Heat));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 21, P - FVector2D(0.0f, Radius * 0.25f * FirePop), FVector2D(Radius * (0.17f + FirePop * 0.14f), Radius * (0.15f + FirePop * 0.12f)), FLinearColor(1.0f, 0.96f, 0.70f, 0.72f * Heat));

            const float FlashA = Heat * CWRunView::SmoothStep01(FMath::Clamp((0.72f - T) / 0.72f, 0.0f, 1.0f));
            for (int32 I = 0; I < 14; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 283);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 293);
                const float Angle = R0 * UE_PI * 2.0f + T * (0.22f + R1 * 0.18f);
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Inner = Radius * (0.08f + R1 * 0.08f);
                const float Outer = Radius * (0.38f + R1 * 0.54f) * (0.72f + Blast * 0.42f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 22, P + Ray * Inner, P + Ray * Outer, FLinearColor(1.0f, 0.62f, 0.15f, 0.20f * FlashA), 1.4f + R1 * 2.2f);
            }

            for (int32 I = 0; I < 34; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 301);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 317);
                const float R2 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 331);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Flight = FMath::Clamp(T * (1.10f + R2 * 0.26f), 0.0f, 1.0f);
                const float Travel = Radius * (0.12f + R1 * 0.88f) * Flight;
                const float Lift = Radius * (0.16f + R2 * 0.34f) * FMath::Sin(Flight * UE_PI);
                const FVector2D Ground = P + Ray * Travel + FVector2D(0.0f, Radius * (0.04f + R1 * 0.10f) * Flight);
                const FVector2D Debris = Ground - FVector2D(0.0f, Lift);
                const float DebrisA = A * (0.42f + R2 * 0.58f);
                const FLinearColor DebrisColor = (I % 5 == 0)
                    ? FLinearColor(1.0f, 0.62f, 0.16f, 0.72f * DebrisA)
                    : FLinearColor(Chunk.R, Chunk.G, Chunk.B, 0.86f * DebrisA);
                const float ChunkW = (3.4f + R1 * 7.2f) * DebrisA;
                const float ChunkH = (2.5f + R0 * 5.0f) * DebrisA;
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 15, Ground + FVector2D(0.0f, 3.0f), FVector2D(ChunkW * (1.10f + Flight * 1.05f), ChunkH * 0.44f), FLinearColor(0.0f, 0.0f, 0.0f, 0.20f * DebrisA));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 23, Debris, FVector2D(ChunkW, ChunkH), DebrisColor);
                if (I % 4 == 0)
                {
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 22, Debris + FVector2D(0.0f, ChunkH * 0.25f), FVector2D(ChunkW * 1.55f, ChunkH * 0.78f), FLinearColor(Dust.R, Dust.G, Dust.B, 0.16f * DebrisA));
                }
                if (Flight > 0.58f && I % 3 == 0)
                {
                    const float Smear = CWRunView::SmoothStep01((Flight - 0.58f) / 0.18f) * A;
                    DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 15, Ground + FVector2D(0.0f, 3.0f), FVector2D((6.0f + R1 * 15.0f) * Scale, (2.6f + R2 * 5.5f) * Scale), FLinearColor(Chunk.R, Chunk.G, Chunk.B, 0.26f * Smear));
                }
            }

            for (int32 I = 0; I < 18; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 337);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 341);
                const float R2 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 347);
                const float Spread = -0.82f + R0 * 1.64f;
                const FVector2D Side(FMath::Sin(Spread), 0.0f);
                const float RiseT = FMath::Clamp(T * (1.22f + R2 * 0.34f), 0.0f, 1.0f);
                const float Rise = Radius * (0.22f + R1 * 0.46f) * FMath::Sin(RiseT * UE_PI);
                const float Drift = Radius * (0.06f + R2 * 0.20f) * RiseT;
                const FVector2D Ground = P + Side * Drift + FVector2D(0.0f, Radius * (0.04f + R0 * 0.05f));
                const FVector2D Clod = Ground - FVector2D(0.0f, Rise);
                const float ClodA = A * (0.36f + R2 * 0.42f);
                const float ClodW = 4.8f + R1 * 9.5f;
                const float ClodH = 3.4f + R2 * 7.0f;
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 15, Ground + FVector2D(0.0f, 4.0f), FVector2D(ClodW * (1.30f + RiseT), ClodH * 0.38f), FLinearColor(0.0f, 0.0f, 0.0f, 0.18f * ClodA));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 24, Clod, FVector2D(ClodW, ClodH), FLinearColor(Chunk.R, Chunk.G, Chunk.B, 0.80f * ClodA));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 23, Clod + FVector2D(0.0f, ClodH * 0.55f), FVector2D(ClodW * 1.95f, ClodH * 1.05f), FLinearColor(Dust.R, Dust.G, Dust.B, 0.14f * ClodA));
            }

            for (int32 I = 0; I < (bMetal ? 26 : 18); ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 353);
                const float Spread = -0.92f + R0 * 1.84f;
                const FVector2D Ray = (BackDir * FMath::Cos(Spread) + Perp * FMath::Sin(Spread)).GetSafeNormal();
                const float Len = Radius * (0.20f + T * (0.78f + R0 * 0.34f));
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 25, P + Ray * Radius * 0.12f, P + Ray * Len, bMetal ? FLinearColor(0.72f, 0.96f, 1.0f, 0.46f * A) : FLinearColor(1.0f, 0.76f, 0.22f, 0.22f * FlashA), bMetal ? 1.5f : 0.9f);
            }

            for (int32 I = 0; I < 20; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 383);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 397);
                const float R2 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 409);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const float FireFlight = CWRunView::SmoothStep01(FMath::Clamp(T * (1.26f + R2 * 0.22f), 0.0f, 1.0f));
                const float Rise = FMath::Sin(FireFlight * UE_PI) * Radius * (0.12f + R1 * 0.22f);
                const FVector2D FlameRoot = P + Ray * Radius * (0.06f + R1 * 0.08f);
                const FVector2D FlameTip = P + Ray * Radius * (0.16f + R1 * 0.62f) * FireFlight - FVector2D(0.0f, Rise);
                const float FlameA = Heat * (0.28f + R2 * 0.44f);
                const FVector2D FlameMid = (FlameRoot + FlameTip) * 0.5f - FVector2D(0.0f, Radius * 0.03f * R2);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 26, FlameRoot, FVector2D(5.5f + R1 * 10.0f, 3.8f + R2 * 6.0f), FLinearColor(1.0f, 0.30f, 0.04f, FlameA * 0.42f));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 27, FlameMid, FVector2D(6.0f + R1 * 13.0f, 4.4f + R2 * 8.0f), FLinearColor(1.0f, 0.46f + R1 * 0.16f, 0.06f, FlameA * 0.52f));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 28, FlameTip, FVector2D(5.0f + R1 * 11.0f, 4.0f + R2 * 8.0f), FLinearColor(1.0f, 0.78f, 0.24f, FlameA * 0.70f));
            }

            for (int32 I = 0; I < 18; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 431);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 449);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const FVector2D Smoke = P + Ray * Radius * (0.12f + R1 * 0.55f) * Blast - FVector2D(0.0f, Radius * (0.05f + R1 * 0.15f) * T);
                const float SmokeSize = Radius * (0.060f + R1 * 0.105f) * (0.72f + T * 0.62f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 16, Smoke, FVector2D(SmokeSize * 1.35f, SmokeSize * 0.74f), FLinearColor(0.035f, 0.038f, 0.042f, 0.18f * A));
            }
        }
        else if (Fx.Type.StartsWith(TEXT("blast_scorch")))
        {
            const FString SurfaceKey = Fx.Type.ToLower();
            const bool bAsphalt = SurfaceKey.Contains(TEXT("asphalt"));
            const bool bGrass = SurfaceKey.Contains(TEXT("grass"));
            const bool bDirt = SurfaceKey.Contains(TEXT("dirt"));
            const bool bMetal = SurfaceKey.Contains(TEXT("metal"));
            const float Radius = FMath::Clamp(Fx.Radius * Scale, 64.0f, 220.0f);
            const float Settle = CWRunView::SmoothStep01(FMath::Clamp(T / 0.22f, 0.0f, 1.0f));
            const float Fade = CWRunView::SmoothStep01(FMath::Clamp((1.0f - T) / 0.42f, 0.0f, 1.0f));
            const FLinearColor Scorch = bGrass
                ? FLinearColor(0.025f, 0.055f, 0.020f, 1.0f)
                : (bDirt
                    ? FLinearColor(0.12f, 0.070f, 0.035f, 1.0f)
                    : (bMetal ? FLinearColor(0.055f, 0.060f, 0.064f, 1.0f) : FLinearColor(0.030f, 0.032f, 0.034f, 1.0f)));
            const FLinearColor Dust = bGrass
                ? FLinearColor(0.18f, 0.28f, 0.08f, 1.0f)
                : (bDirt ? FLinearColor(0.38f, 0.23f, 0.10f, 1.0f) : FLinearColor(0.36f, 0.34f, 0.30f, 1.0f));

            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, P + FVector2D(0.0f, Radius * 0.05f), FVector2D(Radius * (1.18f + Settle * 0.18f), Radius * (0.56f + Settle * 0.10f)), FLinearColor(Scorch.R, Scorch.G, Scorch.B, 0.46f * Settle * Fade));
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 17, P, FVector2D(Radius * (0.52f + Settle * 0.16f), Radius * (0.26f + Settle * 0.08f)), FLinearColor(0.0f, 0.0f, 0.0f, 0.36f * Settle * Fade));
            DrawRing(OutDrawElements, AllottedGeometry, BackgroundLayer + 18, P, FVector2D(Radius * (0.65f + T * 0.18f), Radius * (0.65f + T * 0.18f)), FLinearColor(Dust.R, Dust.G, Dust.B, 0.18f * Fade), 2.2f, 18, Fx.Radius * 0.01f);

            for (int32 I = 0; I < 16; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 467);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 479);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
                const FVector2D Ember = P + Ray * Radius * (0.08f + R1 * 0.52f) - FVector2D(0.0f, Radius * 0.10f * FMath::Sin(T * UE_PI));
                const float EmberA = Fade * (0.18f + R1 * 0.34f) * CWRunView::SmoothStep01((1.0f - T) / 0.72f);
                if (I % 3 == 0)
                {
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 15, Ember, FVector2D(3.0f + R1 * 4.0f, 2.4f + R0 * 3.0f), FLinearColor(1.0f, 0.38f, 0.08f, EmberA));
                }
                else
                {
                    DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 19, Ember + FVector2D(0.0f, 2.0f), FVector2D(4.0f + R1 * 9.0f, 2.0f + R0 * 3.8f), FLinearColor(Dust.R, Dust.G, Dust.B, 0.12f * Fade));
                }
            }
        }
        else if (Fx.Type == TEXT("boss_blood_death"))
        {
            const float Burst = FMath::Sin(FMath::Clamp(T * 1.08f, 0.0f, 1.0f) * UE_PI);
            const float Radius = Fx.Radius * Scale;
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 13, P, FVector2D(Radius * (0.72f + T * 0.36f), Radius * (0.42f + T * 0.20f)), FLinearColor(0.18f, 0.0f, 0.014f, 0.52f * A));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, P, FVector2D(Radius * (0.26f + T * 0.74f), Radius * (0.18f + T * 0.50f)), FLinearColor(C.R, C.G, C.B, 0.34f * A));
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, FVector2D(Radius * (0.22f + T * 1.18f), Radius * (0.22f + T * 1.18f)), FLinearColor(1.0f, 0.20f, 0.22f, 0.44f * A), 4.2f, 54, T * UE_PI * 2.0f);
            DrawRing(OutDrawElements, AllottedGeometry, ActorLayer + 19, P, FVector2D(Radius * (0.12f + T * 0.72f), Radius * (0.12f + T * 0.72f)), FLinearColor(0.48f, 0.0f, 0.018f, 0.54f * A), 7.0f, 34, -T * UE_PI * 2.6f);

            for (int32 I = 0; I < 42; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 17);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 31);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Len = Radius * (0.12f + (0.42f + R1 * 0.74f) * T);
                const FVector2D Gravity(0.0f, T * T * Radius * (0.030f + R1 * 0.060f));
                const FVector2D End = P + Dir * Len + Gravity;
                const FVector2D Start = P + Dir * Radius * (0.035f + R1 * 0.030f);
                const float DropA = A * (0.48f + R1 * 0.52f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 20, Start, End, FLinearColor(0.92f, 0.010f, 0.020f, 0.36f * DropA), I % 5 == 0 ? 3.0f : 1.5f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 21, End, FVector2D((3.2f + R1 * 5.5f) * DropA, (2.4f + R0 * 4.2f) * DropA), FLinearColor(0.98f, 0.018f, 0.026f, 0.78f * DropA));
            }

            for (int32 I = 0; I < 28; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 47);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 59);
                const float Flight = FMath::Clamp(T * (1.08f + R1 * 0.22f), 0.0f, 1.0f);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Travel = Radius * (0.15f + R1 * 0.72f) * Flight;
                const float Lift = Radius * (0.12f + R0 * 0.22f) * FMath::Sin(Flight * UE_PI);
                const FVector2D Ground = P + Dir * Travel + FVector2D(0.0f, Flight * Flight * Radius * 0.10f);
                const FVector2D Chunk = Ground - FVector2D(0.0f, Lift);
                const float ChunkA = FMath::Clamp(A + 0.18f, 0.0f, 1.0f);
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 15, Ground + FVector2D(0.0f, 4.0f), FVector2D((8.0f + R1 * 12.0f) * Scale, (2.0f + R0 * 4.0f) * Scale), FLinearColor(0.09f, 0.0f, 0.006f, 0.30f * A));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 22, Chunk, FVector2D((5.5f + R0 * 7.0f) * ChunkA, (3.8f + R1 * 5.0f) * ChunkA), FLinearColor(0.48f, 0.0f, 0.012f, 0.96f * ChunkA));
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 21, P + Dir * Radius * 0.04f, Chunk, FLinearColor(0.70f, 0.0f, 0.018f, 0.12f * A), 1.2f);
                if (Flight > 0.58f)
                {
                    const float SplatIn = CWRunView::SmoothStep01((Flight - 0.58f) / 0.18f);
                    const float SplatFade = CWRunView::SmoothStep01((1.0f - T) / 0.34f);
                    DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, Ground, FVector2D((7.0f + R1 * 12.0f) * Scale, (3.0f + R0 * 5.0f) * Scale), FLinearColor(0.62f, 0.0f, 0.018f, 0.44f * SplatIn * SplatFade));
                }
            }

            for (int32 I = 0; I < 18; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 71);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 83);
                const float Angle = R0 * UE_PI * 2.0f;
                const FVector2D Mist = P + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius * (0.08f + R1 * 0.42f) * T - FVector2D(0.0f, Burst * Radius * (0.05f + R0 * 0.09f));
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 16, Mist, FVector2D(Radius * (0.030f + R1 * 0.052f), Radius * (0.020f + R0 * 0.040f)), FLinearColor(0.70f, 0.0f, 0.025f, 0.22f * A));
            }
        }
        else if (Fx.Type == TEXT("blood_jet"))
        {
            const FVector2D BaseDir = Fx.Direction.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : Fx.Direction.GetSafeNormal();
            const FVector2D Perp(-BaseDir.Y, BaseDir.X);
            const float Radius = Fx.Radius * Scale;
            const float JetLen = Radius * (0.82f + T * 0.28f);
            const float CoreFade = CWRunView::SmoothStep01((1.0f - T) / 0.34f);
            const FVector2D Root = P - BaseDir * Radius * 0.05f;
            const FVector2D Head = P + BaseDir * JetLen;
            const FVector2D Mid = P + BaseDir * JetLen * 0.48f + Perp * FMath::Sin(T * 10.0f + Fx.Radius * 0.03f) * Radius * 0.035f;

            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 18, Root, Mid, FLinearColor(0.72f, 0.0f, 0.014f, 0.38f * CoreFade), FMath::Clamp(Radius * 0.055f, 2.0f, 6.0f));
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, Mid, Head, FLinearColor(1.0f, 0.016f, 0.024f, 0.46f * CoreFade), FMath::Clamp(Radius * 0.030f, 1.3f, 3.6f));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, Head, FVector2D(FMath::Clamp(Radius * 0.050f, 3.0f, 8.0f), FMath::Clamp(Radius * 0.032f, 2.0f, 5.0f)), FLinearColor(1.0f, 0.020f, 0.028f, 0.58f * CoreFade));

            for (int32 I = 0; I < 24; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 211);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 223);
                const float R2 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 227);
                const float Along = (0.10f + R0 * 0.96f) * JetLen;
                const float Flight = FMath::Clamp(T * (1.16f + R2 * 0.28f) - R0 * 0.10f, 0.0f, 1.0f);
                const float SideDrift = (R1 - 0.5f) * Radius * (0.12f + 0.20f * Flight);
                const FVector2D Start = P + BaseDir * Along * 0.42f + Perp * SideDrift * 0.32f;
                const FVector2D Ground = P
                    + BaseDir * (Along * (0.55f + Flight * 0.55f))
                    + Perp * SideDrift
                    + FVector2D(0.0f, Flight * Flight * Radius * (0.12f + R2 * 0.12f));
                const float Lift = Radius * (0.05f + R2 * 0.10f) * FMath::Sin(Flight * UE_PI);
                const FVector2D Drop = Ground - FVector2D(0.0f, Lift);
                const float DropFade = CWRunView::SmoothStep01((1.0f - T) / 0.30f);
                const float DropA = DropFade * (0.45f + R2 * 0.45f);

                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 20, Start, Drop, FLinearColor(0.84f, 0.004f, 0.016f, 0.24f * DropA), I % 5 == 0 ? 1.9f : 1.0f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 21, Drop, FVector2D((2.1f + R1 * 4.8f) * DropA, (1.6f + R0 * 3.0f) * DropA), FLinearColor(1.0f, 0.015f, 0.024f, 0.78f * DropA));

                if (Flight > 0.62f && I % 2 == 0)
                {
                    const float SplatIn = CWRunView::SmoothStep01((Flight - 0.62f) / 0.16f);
                    const float SplatFade = CWRunView::SmoothStep01((1.0f - T) / 0.28f);
                    const float SplatA = SplatIn * SplatFade;
                    const FVector2D SmearStart = Ground - BaseDir * (4.0f + R1 * 8.0f);
                    const FVector2D SmearEnd = Ground + BaseDir * (8.0f + R2 * 18.0f);
                    DrawLine(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, SmearStart, SmearEnd, FLinearColor(0.56f, 0.0f, 0.014f, 0.26f * SplatA), 1.8f + R1 * 2.8f);
                    DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 17, Ground + FVector2D(0.0f, 4.0f), FVector2D((4.0f + R1 * 8.5f) * Scale, (1.8f + R2 * 4.2f) * Scale), FLinearColor(0.66f, 0.0f, 0.018f, 0.30f * SplatA));
                }
            }
        }
        else if (Fx.Type == TEXT("blood_hit") || Fx.Type == TEXT("gore_hit") || Fx.Type == TEXT("gore_death"))
        {
            const bool bGore = Fx.Type != TEXT("blood_hit");
            const bool bDeath = Fx.Type == TEXT("gore_death");
            FVector2D BaseDir = Fx.Direction.IsNearlyZero() ? FVector2D(1.0f, -0.15f) : Fx.Direction.GetSafeNormal();
            const FVector2D Perp(-BaseDir.Y, BaseDir.X);
            const float BaseAngle = FMath::Atan2(BaseDir.Y, BaseDir.X);
            const float Radius = Fx.Radius * Scale;
            const int32 Count = bGore ? (bDeath ? 18 : 10) : 30;
            const float Cone = bDeath ? UE_PI * 1.68f : (bGore ? 1.24f : 1.02f);

            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 13, P + BaseDir * Radius * 0.10f, FVector2D(Radius * (bDeath ? 0.92f : 0.58f), Radius * (bDeath ? 0.36f : 0.24f)), FLinearColor(0.18f, 0.0f, 0.012f, (bDeath ? 0.32f : 0.22f) * A));

            for (int32 I = 0; I < Count; ++I)
            {
                const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, bGore ? 113 : 101);
                const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, bGore ? 127 : 109);
                const float R2 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, bGore ? 139 : 121);
                const float Angle = bDeath
                    ? (R0 * UE_PI * 2.0f)
                    : (BaseAngle + (R0 - 0.5f) * Cone + (R1 - 0.5f) * 0.22f);
                const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
                const float Flight = FMath::Clamp(T * (bGore ? (1.12f + R2 * 0.20f) : 1.22f), 0.0f, 1.0f);
                const float Travel = Radius * (bGore ? (0.18f + R1 * (bDeath ? 1.06f : 0.74f)) : (0.16f + R1 * 0.88f)) * Flight;
                const float ArcLift = bGore ? Radius * (0.09f + R2 * (bDeath ? 0.24f : 0.18f)) * FMath::Sin(Flight * UE_PI) : 0.0f;
                const FVector2D Gravity(0.0f, Flight * Flight * Radius * (bGore ? 0.12f : 0.08f));
                const FVector2D Ground = P + Dir * Travel + Gravity;
                const FVector2D Drop = Ground - FVector2D(0.0f, ArcLift);
                const float DropA = bGore ? FMath::Clamp(A + 0.14f, 0.0f, 1.0f) : A * (0.54f + R2 * 0.46f);

                if (bGore)
                {
                    const FVector2D ChunkSize((3.6f + R0 * (bDeath ? 7.0f : 4.6f)) * DropA, (2.8f + R1 * (bDeath ? 5.2f : 3.8f)) * DropA);
                    DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 15, Ground + FVector2D(0.0f, 4.0f), FVector2D(ChunkSize.X * 1.8f, ChunkSize.Y * 0.55f), FLinearColor(0.0f, 0.0f, 0.0f, 0.18f * A));
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, P + Dir * Radius * 0.04f, Drop, FLinearColor(0.70f, 0.0f, 0.018f, 0.16f * A), 1.1f);
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 21, Drop, ChunkSize, FLinearColor(0.46f + R2 * 0.18f, 0.0f, 0.012f, 0.94f * DropA));
                    if (Flight > 0.62f)
                    {
                        const float SplatIn = CWRunView::SmoothStep01((Flight - 0.62f) / 0.16f);
                        const float SplatFade = CWRunView::SmoothStep01((1.0f - T) / (bDeath ? 0.36f : 0.30f));
                        const float SplatA = SplatIn * SplatFade;
                        const FVector2D Splat = Ground - Dir * (5.0f + R1 * 10.0f);
                        DrawLine(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, Splat, Ground + Dir * (8.0f + R2 * 14.0f), FLinearColor(0.58f, 0.0f, 0.016f, 0.30f * SplatA), 2.4f + R1 * 2.2f);
                        DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 17, Ground, FVector2D((6.0f + R1 * 9.0f) * Scale, (2.8f + R2 * 4.8f) * Scale), FLinearColor(0.68f, 0.0f, 0.018f, 0.36f * SplatA));
                    }
                }
                else
                {
                    const FVector2D Start = P + BaseDir * Radius * 0.05f + Perp * (R2 - 0.5f) * Radius * 0.08f;
                    DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, Start, Drop, FLinearColor(0.82f, 0.006f, 0.018f, 0.38f * DropA), I % 5 == 0 ? 2.6f : 1.2f);
                    DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, Drop, FVector2D((2.0f + R1 * 4.8f) * DropA, (1.8f + R0 * 3.8f) * DropA), FLinearColor(0.98f, 0.015f, 0.024f, 0.82f * DropA));
                    if (Flight > 0.55f && I % 3 == 0)
                    {
                        const float SplatIn = CWRunView::SmoothStep01((Flight - 0.55f) / 0.18f);
                        const float SplatFade = CWRunView::SmoothStep01((1.0f - T) / 0.28f);
                        DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, Ground + FVector2D(0.0f, 5.0f), FVector2D((5.0f + R1 * 8.0f) * Scale, (2.0f + R2 * 4.0f) * Scale), FLinearColor(0.52f, 0.0f, 0.014f, 0.22f * SplatIn * SplatFade));
                    }
                }
            }
        }
        else if (Fx.Type == TEXT("blood_puddle"))
        {
            const FVector2D Dir = Fx.Direction.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : Fx.Direction.GetSafeNormal();
            const FVector2D Perp(-Dir.Y, Dir.X);
            const float SettleT = FMath::Clamp(T / 0.18f, 0.0f, 1.0f);
            const float Settle = SettleT * SettleT * (3.0f - 2.0f * SettleT);
            const float FadeT = FMath::Clamp((1.0f - T) / 0.36f, 0.0f, 1.0f);
            const float Fade = FadeT * FadeT * (3.0f - 2.0f * FadeT);
            const float PuddleA = FMath::Clamp(Fx.Color.A * Settle * Fade, 0.0f, 0.78f);
            const float DriedA = FMath::Clamp(Fx.Color.A * 0.30f * Fade * FMath::Clamp((T - 0.18f) / 0.36f, 0.0f, 1.0f), 0.0f, 0.24f);
            const float Grow = 0.74f + Settle * 0.26f + FMath::Min(0.42f, T * 0.42f);
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 13, P, FVector2D(Fx.Radius * Scale * 1.68f * Grow, Fx.Radius * Scale * 0.78f * Grow), FLinearColor(0.22f, 0.0f, 0.015f, PuddleA * 0.92f));
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 13, P + Dir * Fx.Radius * Scale * 0.08f, FVector2D(Fx.Radius * Scale * 1.42f * Grow, Fx.Radius * Scale * 0.58f * Grow), FLinearColor(0.055f, 0.0f, 0.006f, DriedA));
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 14, P + Dir * Fx.Radius * Scale * 0.22f + Perp * Fx.Radius * Scale * 0.10f, FVector2D(Fx.Radius * Scale * 0.84f, Fx.Radius * Scale * 0.34f), FLinearColor(Fx.Color.R, Fx.Color.G, Fx.Color.B, PuddleA * 0.72f));
            DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 14, P - Dir * Fx.Radius * Scale * 0.30f - Perp * Fx.Radius * Scale * 0.16f, FVector2D(Fx.Radius * Scale * 0.56f, Fx.Radius * Scale * 0.24f), FLinearColor(0.72f, 0.02f, 0.028f, PuddleA * 0.42f));
            for (int32 I = 0; I < 7; ++I)
            {
                const float Side = (static_cast<float>((I * 37) % 19) - 9.0f) * Fx.Radius * Scale * 0.035f;
                const float Forward = (static_cast<float>((I * 23) % 17) - 5.0f) * Fx.Radius * Scale * 0.05f;
                const float DotR = (2.2f + static_cast<float>(I % 4)) * Scale;
                const FVector2D Dot = P + Dir * Forward + Perp * Side;
                DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 15, Dot, FVector2D(DotR * 1.6f, DotR), FLinearColor(0.78f, 0.015f, 0.025f, PuddleA * 0.50f));
            }
            if (T > 0.58f)
            {
                const float CrackA = FMath::Clamp((T - 0.58f) / 0.24f, 0.0f, 1.0f) * Fade;
                for (int32 I = 0; I < 5; ++I)
                {
                    const float R0 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 191);
                    const float R1 = CWRunView::UnitRand(Fx.Position, Fx.Radius, I, 199);
                    const float Along = (R0 - 0.5f) * Fx.Radius * Scale * 1.25f;
                    const float Side = (R1 - 0.5f) * Fx.Radius * Scale * 0.42f;
                    const FVector2D Start = P + Dir * Along + Perp * Side;
                    const FVector2D End = Start + (Dir * (0.26f + R1 * 0.34f) + Perp * (R0 - 0.5f) * 0.28f).GetSafeNormal() * Fx.Radius * Scale * (0.10f + R1 * 0.18f);
                    DrawLine(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, Start, End, FLinearColor(0.015f, 0.0f, 0.004f, 0.18f * CrackA), 1.0f + R0 * 1.2f);
                }
            }
        }
        else if (Fx.Type == TEXT("blood"))
        {
            const float BaseAngle = FMath::Atan2(Fx.Direction.Y, Fx.Direction.X);
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, P + FVector2D(0.0f, T * T * 18.0f), FVector2D(Fx.Radius * Scale * (1.05f + T * 1.15f), Fx.Radius * Scale * (0.46f + T * 0.56f)), FLinearColor(0.62f, 0.0f, 0.012f, 0.20f * A));
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 18, P, FVector2D(Fx.Radius * Scale * (0.46f + T * 0.62f), Fx.Radius * Scale * (0.30f + T * 0.36f)), FLinearColor(0.92f, 0.02f, 0.030f, 0.34f * A));
            for (int32 I = 0; I < 20; ++I)
            {
                const float Spread = -0.92f + static_cast<float>(I) * (1.84f / 19.0f) + static_cast<float>((I * 29) % 11) * 0.012f;
                const float Angle = BaseAngle + Spread;
                const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
                const float RandomLen = 0.76f + static_cast<float>((I * 19) % 9) * 0.050f;
                const float Len = Fx.Radius * Scale * (0.14f + T * RandomLen);
                const FVector2D Gravity(0.0f, T * T * (10.0f + static_cast<float>((I * 11) % 17)));
                const FVector2D Drop = P + Dir * Len + Gravity;
                const FVector2D Start = P + Dir * Fx.Radius * Scale * (0.05f + T * 0.06f);
                const float DropA = A * (I % 5 == 0 ? 0.94f : 0.74f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 19, Start, Drop, FLinearColor(0.82f, 0.010f, 0.020f, 0.36f * DropA), I % 4 == 0 ? 2.4f : 1.35f);
                DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 20, Drop, FVector2D((2.6f + static_cast<float>(I % 5)) * DropA, (2.0f + static_cast<float>((I + 1) % 4)) * DropA), FLinearColor(0.98f, 0.018f, 0.030f, 0.78f * DropA));
                if (I % 4 == 0)
                {
                    DrawEllipse(OutDrawElements, AllottedGeometry, BackgroundLayer + 16, Drop + FVector2D(0.0f, 4.0f + T * 7.0f), FVector2D((5.0f + static_cast<float>(I % 3)) * A, (2.0f + static_cast<float>(I % 2)) * A), FLinearColor(0.42f, 0.0f, 0.012f, 0.22f * A));
                }
            }
        }
        else
        {
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 17, P, FVector2D(Fx.Radius * Scale * (0.4f + T), Fx.Radius * Scale * (0.4f + T)), FLinearColor(C.R, C.G, C.B, 0.28f * A));
        }
    }

    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        const FVector2D RenderPos = GetRenderPosition(Player);
        const FVector2D P = WorldToScreen(RenderPos.X, RenderPos.Y, Size, FocusPlayer, Scale);
        const bool bFocusedPlayer = FocusPlayer && Player.Id == FocusPlayer->Id;
        const bool bMe = (!CachedMyId.IsEmpty() && Player.Id == CachedMyId) || (CachedMyId.IsEmpty() && bFocusedPlayer);
        FVector2D LocalReticle = WorldToScreen(Player.AimX, Player.AimY, Size, FocusPlayer, Scale);
        if (bMe)
        {
            FVector2D CursorLocal = FVector2D::ZeroVector;
            FVector2D CursorViewportSize = FVector2D::ZeroVector;
            if (GetViewportMousePosition(CursorLocal, CursorViewportSize) && CursorViewportSize.X > 2.0f && CursorViewportSize.Y > 2.0f)
            {
                if (!CursorViewportSize.Equals(Size, 0.5f))
                {
                    CursorLocal.X *= Size.X / CursorViewportSize.X;
                    CursorLocal.Y *= Size.Y / CursorViewportSize.Y;
                }
                LocalReticle.X = FMath::Clamp(CursorLocal.X, 0.0f, Size.X);
                LocalReticle.Y = FMath::Clamp(CursorLocal.Y, 0.0f, Size.Y);
            }
        }
        const FLinearColor Accent = bMe ? FLinearColor(0.2f, 0.95f, 1.0f, 1.0f) : FLinearColor(0.3f, 1.0f, 0.55f, 0.96f);

        DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 6, P + FVector2D(0.0f, 15.0f * Scale), FVector2D(44.0f * Scale, 11.0f * Scale), FLinearColor(0.0f, 0.0f, 0.0f, 0.22f));

        const FString PlayerKey = PlayerTextureKey(Player.PlayerClass);
        const FSlateBrush* PlayerBrush = GetBrush(PlayerKey);
        const UTexture2D* PlayerTexture = TextureCache.FindRef(PlayerKey);
        if (PlayerBrush && PlayerTexture)
        {
            const float FrameW = 64.0f;
            const float FrameH = 64.0f;
            const int32 FrameCount = FMath::Max(1, FMath::FloorToInt(PlayerTexture->GetSizeX() / FrameW));
            const int32 RowCount = FMath::Max(1, FMath::FloorToInt(PlayerTexture->GetSizeY() / FrameH));
            const FRenderPlayerState* PlayerRenderState = RenderPlayers.Find(Player.Id);
            const float RenderSpeed = PlayerRenderState ? PlayerRenderState->Velocity.Size() : 0.0f;
            const float TargetDrift = PlayerRenderState ? (PlayerRenderState->Position - PlayerRenderState->TargetPosition).Size() : 0.0f;
            const bool bMovingForAnim = RenderSpeed > 24.0f || TargetDrift > 3.0f;
            const int32 IdleFrame = FMath::Clamp(1, 0, FrameCount - 1);
            const int32 WalkFrame = FMath::Clamp(FMath::FloorToInt(CachedState.ServerNowMs / 100.0) % FrameCount, 0, FrameCount - 1);
            const int32 Frame = bMovingForAnim ? WalkFrame : IdleFrame;
            const FVector2D LookDelta = LocalReticle - P;
            int32 RowFromLook = PlayerRowFromAim(Player);
            if (!LookDelta.IsNearlyZero())
            {
                if (FMath::Abs(LookDelta.X) > FMath::Abs(LookDelta.Y))
                {
                    RowFromLook = LookDelta.X < 0.0f ? 1 : 3;
                }
                else
                {
                    RowFromLook = LookDelta.Y < 0.0f ? 0 : 2;
                }
            }
            const int32 Row = FMath::Clamp(RowFromLook, 0, RowCount - 1);
            const float PlayerScale = Player.bIsCompanion ? 0.76f : 0.92f;
            const FVector2D SpriteSize(FrameW * PlayerScale * Scale, FrameH * PlayerScale * Scale);
            DrawTextureRegion(
                OutDrawElements,
                AllottedGeometry,
                ActorLayer + 8,
                PlayerBrush,
                P - FVector2D(SpriteSize.X * 0.5f, SpriteSize.Y * 0.78f),
                SpriteSize,
                FVector2D(Frame * FrameW, Row * FrameH),
                FVector2D(FrameW, FrameH),
                FVector2D(PlayerTexture->GetSizeX(), PlayerTexture->GetSizeY()),
                FLinearColor::White);
        }
        else
        {
            DrawCenteredBox(OutDrawElements, AllottedGeometry, ActorLayer + 8, P, FVector2D(38.0f * Scale, 38.0f * Scale), Accent);
        }

        if (!Player.Name.IsEmpty() || bMe)
        {
            const FString Label = Player.Name.IsEmpty() ? TEXT("Native") : Player.Name.Left(12);
            const FVector2D PlateSize(84.0f, 15.0f);
            const FVector2D PlatePos = P + FVector2D(-PlateSize.X * 0.5f, -68.0f);
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 10, PlatePos, PlateSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.58f));
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 11, PlatePos, FVector2D(PlateSize.X, 1.0f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.54f));
            DrawText(OutDrawElements, AllottedGeometry, TextLayer, PlatePos + FVector2D(6.0f, -1.0f), Label, 10.0f, FLinearColor(0.88f, 0.96f, 1.0f, 0.96f), true);

            const float HpPct = FMath::Clamp(Player.Hp / FMath::Max(1.0f, Player.MaxHp), 0.0f, 1.0f);
            const FVector2D HpBarPos = PlatePos + FVector2D(4.0f, PlateSize.Y + 3.0f);
            const FVector2D HpBarSize(PlateSize.X - 8.0f, 6.0f);
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 10, HpBarPos, HpBarSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
            DrawBox(OutDrawElements, AllottedGeometry, ActorLayer + 11, HpBarPos + FVector2D(1.0f, 1.0f), FVector2D(FMath::Max(0.0f, (HpBarSize.X - 2.0f) * HpPct), HpBarSize.Y - 2.0f), FLinearColor(0.42f, 1.0f, 0.20f, 0.94f));
        }

        if (bMe)
        {
            const FVector2D Reticle = LocalReticle;
            const FVector2D ScreenAimDir = (Reticle - P).GetSafeNormal();
            if (!ScreenAimDir.IsNearlyZero())
            {
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 28, P + ScreenAimDir * 28.0f, Reticle - ScreenAimDir * 26.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f), 4.2f);
                DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 29, P + ScreenAimDir * 30.0f, Reticle - ScreenAimDir * 28.0f, FLinearColor(1.0f, 0.86f, 0.20f, 0.38f), 2.0f);
            }
            DrawEllipse(OutDrawElements, AllottedGeometry, ActorLayer + 30, Reticle, FVector2D(24.0f, 24.0f), FLinearColor(0.0f, 0.92f, 1.0f, 0.13f));
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Reticle + FVector2D(-24.0f, 0.0f), Reticle + FVector2D(-7.0f, 0.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.64f), 3.0f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 33, Reticle + FVector2D(-24.0f, 0.0f), Reticle + FVector2D(-7.0f, 0.0f), FLinearColor(0.78f, 1.0f, 1.0f, 0.98f), 1.6f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Reticle + FVector2D(7.0f, 0.0f), Reticle + FVector2D(24.0f, 0.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.64f), 3.0f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 33, Reticle + FVector2D(7.0f, 0.0f), Reticle + FVector2D(24.0f, 0.0f), FLinearColor(0.78f, 1.0f, 1.0f, 0.98f), 1.6f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Reticle + FVector2D(0.0f, -24.0f), Reticle + FVector2D(0.0f, -7.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.64f), 3.0f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 33, Reticle + FVector2D(0.0f, -24.0f), Reticle + FVector2D(0.0f, -7.0f), FLinearColor(0.78f, 1.0f, 1.0f, 0.98f), 1.6f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 32, Reticle + FVector2D(0.0f, 7.0f), Reticle + FVector2D(0.0f, 24.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.64f), 3.0f);
            DrawLine(OutDrawElements, AllottedGeometry, ActorLayer + 33, Reticle + FVector2D(0.0f, 7.0f), Reticle + FVector2D(0.0f, 24.0f), FLinearColor(0.78f, 1.0f, 1.0f, 0.98f), 1.6f);
            DrawCenteredBox(OutDrawElements, AllottedGeometry, ActorLayer + 34, Reticle, FVector2D(6.0f, 6.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
            DrawCenteredBox(OutDrawElements, AllottedGeometry, ActorLayer + 35, Reticle, FVector2D(3.0f, 3.0f), FLinearColor(1.0f, 0.86f, 0.20f, 1.0f));
        }
    }

    if (CachedState.Players.Num() == 0)
    {
        DrawText(OutDrawElements, AllottedGeometry, TextLayer, FVector2D(28.0f, 28.0f), TEXT("Connecting to Crimson Wars run..."), 20.0f, FLinearColor(0.72f, 0.9f, 1.0f, 0.9f), true);
    }

    return TextLayer + 20;
}
