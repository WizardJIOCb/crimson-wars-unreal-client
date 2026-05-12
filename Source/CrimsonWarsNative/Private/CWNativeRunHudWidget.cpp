#include "CWNativeRunHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CWNativeAssetLibrary.h"
#include "CWNativeGameInstance.h"
#include "CWNativePlayerPawn.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace CWHud
{
    const FSlateBrush* WhiteBrush()
    {
        return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    }

    FSlateFontInfo Font(float Size, bool bBold)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")), Size);
        Info.Size = static_cast<int32>(Size);
        return Info;
    }

    FPaintGeometry PaintGeometry(const FGeometry& Geometry, const FVector2D& Position, const FVector2D& Size)
    {
        return Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position));
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

    void DrawCircle(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness, int32 Segments = 40)
    {
        TArray<FVector2D> Points;
        const int32 Count = FMath::Clamp(Segments, 12, 72);
        Points.Reserve(Count + 1);
        for (int32 I = 0; I <= Count; ++I)
        {
            const float Angle = (static_cast<float>(I) / static_cast<float>(Count)) * UE_TWO_PI;
            Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
        }
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

    void DrawDisc(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Center, float Radius, const FLinearColor& Color, int32 Slices = 18)
    {
        const int32 Count = FMath::Clamp(Slices, 8, 28);
        const float Step = (Radius * 2.0f) / static_cast<float>(Count);
        for (int32 I = 0; I <= Count; ++I)
        {
            const float Y = -Radius + Step * I;
            const float HalfWidth = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - Y * Y));
            DrawBox(
                OutDrawElements,
                Geometry,
                LayerId,
                Center + FVector2D(-HalfWidth, Y - Step * 0.5f),
                FVector2D(HalfWidth * 2.0f, Step),
                Color);
        }
    }

    void DrawText(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Position, const FString& Text, float Size, const FLinearColor& Color, bool bBold = false)
    {
        const FSlateFontInfo Info = Font(Size, bBold);
        const FVector2D BoxSize(FMath::Max(8.0f, Size * FMath::Max(1, Text.Len()) * 0.66f), Size * 1.45f);
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            PaintGeometry(Geometry, Position, BoxSize),
            FText::FromString(Text),
            Info,
            ESlateDrawEffect::None,
            Color);
    }

    void DrawPanel(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Fill, const FLinearColor& Edge)
    {
        DrawBox(OutDrawElements, Geometry, LayerId - 1, Position + FVector2D(3.0f, 5.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f));
        DrawBox(OutDrawElements, Geometry, LayerId, Position, Size, Fill);
        DrawBox(OutDrawElements, Geometry, LayerId + 1, Position, FVector2D(Size.X, 1.0f), Edge);
        DrawBox(OutDrawElements, Geometry, LayerId + 1, Position, FVector2D(1.0f, Size.Y), FLinearColor(Edge.R, Edge.G, Edge.B, Edge.A * 0.55f));
        DrawBox(OutDrawElements, Geometry, LayerId + 1, Position + FVector2D(0.0f, Size.Y - 1.0f), FVector2D(Size.X, 1.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.36f));
        DrawBox(OutDrawElements, Geometry, LayerId + 1, Position + FVector2D(Size.X - 1.0f, 0.0f), FVector2D(1.0f, Size.Y), FLinearColor(0.0f, 0.0f, 0.0f, 0.24f));
        DrawBox(OutDrawElements, Geometry, LayerId + 1, Position + FVector2D(2.0f, 2.0f), FVector2D(FMath::Max(0.0f, Size.X - 4.0f), 1.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.08f));
    }

    FString FormatClock(double Seconds)
    {
        const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
        return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
    }

    FLinearColor RarityColor(const FString& Rarity, float Alpha = 1.0f)
    {
        const FString Key = Rarity.ToLower();
        if (Key == TEXT("legendary")) return FLinearColor(0.98f, 0.75f, 0.20f, Alpha);
        if (Key == TEXT("epic")) return FLinearColor(0.72f, 0.46f, 0.98f, Alpha);
        if (Key == TEXT("rare")) return FLinearColor(0.22f, 0.74f, 0.96f, Alpha);
        if (Key == TEXT("uncommon")) return FLinearColor(0.33f, 0.86f, 0.47f, Alpha);
        return FLinearColor(0.78f, 0.84f, 0.92f, Alpha);
    }

    FLinearColor SkillColor(const FCWSkillSnapshot& Skill, float Alpha = 1.0f)
    {
        const FString Text = (Skill.Id + TEXT(" ") + Skill.Name + TEXT(" ") + Skill.Kind).ToLower();
        if (Text.Contains(TEXT("heal")) || Text.Contains(TEXT("regen")) || Text.Contains(TEXT("vital"))) return FLinearColor(0.31f, 0.92f, 0.56f, Alpha);
        if (Text.Contains(TEXT("rocket")) || Text.Contains(TEXT("missile")) || Text.Contains(TEXT("grenade")) || Text.Contains(TEXT("explosive"))) return FLinearColor(0.98f, 0.54f, 0.20f, Alpha);
        if (Text.Contains(TEXT("void")) || Text.Contains(TEXT("shadow")) || Text.Contains(TEXT("ghost"))) return FLinearColor(0.70f, 0.52f, 0.98f, Alpha);
        if (Text.Contains(TEXT("laser")) || Text.Contains(TEXT("ion")) || Text.Contains(TEXT("arc")) || Text.Contains(TEXT("pulse"))) return FLinearColor(0.08f, 0.88f, 1.0f, Alpha);
        return FLinearColor(0.12f, 0.80f, 1.0f, Alpha);
    }

    FString NormalizeHeroId(const FString& HeroId)
    {
        FString Key = HeroId.ToLower();
        Key.TrimStartAndEndInline();
        if (Key == TEXT("medis"))
        {
            return TEXT("medic");
        }
        return Key.IsEmpty() ? TEXT("cyber") : Key;
    }

    FString SkillIconKey(const FCWSkillSnapshot& Skill, const FString& HeroId)
    {
        FString Id = Skill.Id.ToLower();
        Id.TrimStartAndEndInline();
        if (Id.StartsWith(TEXT("native_")))
        {
            Id.RightChopInline(7);
        }
        if (Id.IsEmpty())
        {
            return FString();
        }

        if (Id == TEXT("shockwave")) return TEXT("raider_war_stomp");
        if (Id == TEXT("psi_blast")) return TEXT("shadow_void_burst");
        if (Id == TEXT("blade_orbit")) return TEXT("scout_razor_wind");
        if (Id == TEXT("chain_lightning")) return TEXT("cyber_arc_matrix");
        if (Id == TEXT("laser_strike")) return TEXT("cyber_ion_lance");
        if (Id == TEXT("homing_missiles")) return TEXT("cyber_seeker_protocol");
        if (Id == TEXT("weapon_mastery")) return TEXT("cyber_combat_firmware");
        if (Id == TEXT("rapid_reload")) return TEXT("shadow_action_reload");
        if (Id == TEXT("tactical_slap")) return TEXT("raider_anger_management");
        if (Id == TEXT("shilo_rm")) return TEXT("cyber_pulse_wave");
        if (Id == TEXT("bullet_gps")) return TEXT("scout_hunter_mark");
        if (Id == TEXT("vitality")) return TEXT("medic_vital_plating");
        if (Id == TEXT("haste")) return TEXT("scout_long_stride");
        if (Id == TEXT("magnetism")) return TEXT("scout_vital_sight");
        if (Id == TEXT("bloodlust")) return TEXT("raider_battle_rage");
        if (Id == TEXT("regeneration")) return TEXT("medic_field_aid");
        if (Id == TEXT("force_shield")) return TEXT("cyber_adaptive_frame");
        if (Id == TEXT("dodge_instinct")) return TEXT("shadow_ghost_step");
        if (Id == TEXT("pistol_buddy")) return TEXT("cyber_sync_link");
        if (Id == TEXT("smg_buddy")) return TEXT("cyber_pulse_wave");
        if (Id == TEXT("shotgun_buddy")) return TEXT("raider_shrapnel_burst");
        if (Id == TEXT("sniper_buddy")) return TEXT("scout_hunter_mark");

        if (Id == TEXT("pulse_wave")) return TEXT("cyber_pulse_wave");
        if (Id == TEXT("ion_lance")) return TEXT("cyber_ion_lance");
        if (Id == TEXT("arc_matrix")) return TEXT("cyber_arc_matrix");
        if (Id == TEXT("seeker_protocol")) return TEXT("cyber_seeker_protocol");
        if (Id == TEXT("adaptive_frame")) return TEXT("cyber_adaptive_frame");
        if (Id == TEXT("combat_firmware")) return TEXT("cyber_combat_firmware");
        if (Id == TEXT("sync_link")) return TEXT("cyber_sync_link");
        if (Id == TEXT("razor_wind")) return TEXT("scout_razor_wind");
        if (Id == TEXT("hunter_mark")) return TEXT("scout_hunter_mark");
        if (Id == TEXT("storm_net")) return TEXT("scout_storm_net");
        if (Id == TEXT("sky_chasers")) return TEXT("scout_sky_chasers");
        if (Id == TEXT("long_stride")) return TEXT("scout_long_stride");
        if (Id == TEXT("vital_sight")) return TEXT("scout_vital_sight");
        if (Id == TEXT("trailblazer")) return TEXT("scout_trailblazer");
        if (Id == TEXT("energy_drink_iv")) return TEXT("scout_energy_drink_iv");
        if (Id == TEXT("void_burst")) return TEXT("shadow_void_burst");
        if (Id == TEXT("ghost_step")) return TEXT("shadow_ghost_step");
        if (Id == TEXT("black_comets")) return TEXT("shadow_black_comets");
        if (Id == TEXT("eclipse_chain")) return TEXT("shadow_eclipse_chain");
        if (Id == TEXT("assassin_instinct")) return TEXT("shadow_assassin_instinct");
        if (Id == TEXT("night_fangs")) return TEXT("shadow_night_fangs");
        if (Id == TEXT("action_reload")) return TEXT("shadow_action_reload");
        if (Id == TEXT("umbral_doctrine")) return TEXT("shadow_umbral_doctrine");
        if (Id == TEXT("triage_beam")) return TEXT("medic_triage_beam");
        if (Id == TEXT("sterile_wave")) return TEXT("medic_sterile_wave");
        if (Id == TEXT("toxin_arc")) return TEXT("medic_toxin_arc");
        if (Id == TEXT("rescue_rockets")) return TEXT("medic_rescue_rockets");
        if (Id == TEXT("field_aid")) return TEXT("medic_field_aid");
        if (Id == TEXT("vital_plating")) return TEXT("medic_vital_plating");
        if (Id == TEXT("support_protocol")) return TEXT("medic_support_protocol");
        if (Id == TEXT("oops_all_bandages")) return TEXT("medic_oops_all_bandages");
        if (Id == TEXT("war_stomp")) return TEXT("raider_war_stomp");
        if (Id == TEXT("shrapnel_burst")) return TEXT("raider_shrapnel_burst");
        if (Id == TEXT("berserk_arc")) return TEXT("raider_berserk_arc");
        if (Id == TEXT("siege_barrage")) return TEXT("raider_siege_barrage");
        if (Id == TEXT("battle_rage")) return TEXT("raider_battle_rage");
        if (Id == TEXT("iron_hide")) return TEXT("raider_iron_hide");
        if (Id == TEXT("war_banner")) return TEXT("raider_war_banner");
        if (Id == TEXT("anger_management")) return TEXT("raider_anger_management");

        const FString Hero = NormalizeHeroId(HeroId);
        if (Id.StartsWith(Hero + TEXT("_")))
        {
            return Id;
        }
        return Hero + TEXT("_") + Id;
    }

    FString ItemIconKey(const FString& ItemId)
    {
        FString Id = ItemId.ToLower();
        Id.TrimStartAndEndInline();
        return Id.IsEmpty() ? FString() : TEXT("item_") + Id;
    }

    FString BadgeFromText(const FString& Text, int32 MaxChars = 3)
    {
        FString Clean;
        for (TCHAR Ch : Text)
        {
            if (FChar::IsAlnum(Ch))
            {
                Clean.AppendChar(FChar::ToUpper(Ch));
            }
            else if (!Clean.EndsWith(TEXT(" ")))
            {
                Clean.AppendChar(TCHAR(' '));
            }
        }
        Clean.TrimStartAndEndInline();
        if (Clean.IsEmpty())
        {
            return TEXT("?");
        }

        TArray<FString> Parts;
        Clean.ParseIntoArrayWS(Parts);
        FString Out;
        if (Parts.Num() >= 2)
        {
            for (const FString& Part : Parts)
            {
                if (!Part.IsEmpty())
                {
                    Out.AppendChar(Part[0]);
                }
                if (Out.Len() >= MaxChars)
                {
                    break;
                }
            }
        }
        else
        {
            Out = Clean.Left(MaxChars);
        }
        return Out.Left(MaxChars);
    }
}

TSharedRef<SWidget> UCWNativeRunHudWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    BuildHud();
    return Super::RebuildWidget();
}

void UCWNativeRunHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        AssetsRoot = GI->WebAssetsRoot;
        StateDelegateHandle = GI->OnStateReceived.AddUObject(this, &UCWNativeRunHudWidget::HandleStateReceived);
        TextDelegateHandle = GI->OnTextMessage.AddUObject(this, &UCWNativeRunHudWidget::HandleTextMessage);
        CachedMyId = GI->MyId;
        UpdateFromState(GI->LatestState);
    }
    if (AssetsRoot.IsEmpty())
    {
        AssetsRoot = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
    }
    LoadHudAssets();
}

void UCWNativeRunHudWidget::NativeDestruct()
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        if (StateDelegateHandle.IsValid())
        {
            GI->OnStateReceived.Remove(StateDelegateHandle);
            StateDelegateHandle.Reset();
        }
        if (TextDelegateHandle.IsValid())
        {
            GI->OnTextMessage.Remove(TextDelegateHandle);
            TextDelegateHandle.Reset();
        }
    }
    Super::NativeDestruct();
}

void UCWNativeRunHudWidget::LoadHudAssets()
{
    static const TCHAR* HeroSkillKeys[] = {
        TEXT("cyber_pulse_wave"),
        TEXT("cyber_ion_lance"),
        TEXT("cyber_arc_matrix"),
        TEXT("cyber_seeker_protocol"),
        TEXT("cyber_adaptive_frame"),
        TEXT("cyber_combat_firmware"),
        TEXT("cyber_sync_link"),
        TEXT("cyber_cache_goblin"),
        TEXT("scout_razor_wind"),
        TEXT("scout_hunter_mark"),
        TEXT("scout_storm_net"),
        TEXT("scout_sky_chasers"),
        TEXT("scout_long_stride"),
        TEXT("scout_vital_sight"),
        TEXT("scout_trailblazer"),
        TEXT("scout_energy_drink_iv"),
        TEXT("shadow_void_burst"),
        TEXT("shadow_ghost_step"),
        TEXT("shadow_black_comets"),
        TEXT("shadow_eclipse_chain"),
        TEXT("shadow_assassin_instinct"),
        TEXT("shadow_night_fangs"),
        TEXT("shadow_action_reload"),
        TEXT("shadow_umbral_doctrine"),
        TEXT("raider_war_stomp"),
        TEXT("raider_shrapnel_burst"),
        TEXT("raider_berserk_arc"),
        TEXT("raider_siege_barrage"),
        TEXT("raider_battle_rage"),
        TEXT("raider_iron_hide"),
        TEXT("raider_war_banner"),
        TEXT("raider_anger_management"),
        TEXT("medic_triage_beam"),
        TEXT("medic_sterile_wave"),
        TEXT("medic_toxin_arc"),
        TEXT("medic_rescue_rockets"),
        TEXT("medic_field_aid"),
        TEXT("medic_vital_plating"),
        TEXT("medic_support_protocol"),
        TEXT("medic_oops_all_bandages")
    };

    for (const TCHAR* Key : HeroSkillKeys)
    {
        LoadTexture(Key, FString::Printf(TEXT("hero-skills/%s.png"), Key));
    }

    TArray<FString> ItemFiles;
    const FString ItemDir = FCWNativeAssetLibrary::NormalizeAssetPath(AssetsRoot, TEXT("items"));
    IFileManager::Get().FindFiles(ItemFiles, *(ItemDir / TEXT("*.png")), true, false);
    for (const FString& FileName : ItemFiles)
    {
        const FString BaseName = FPaths::GetBaseFilename(FileName).ToLower();
        if (!BaseName.IsEmpty())
        {
            LoadTexture(TEXT("item_") + BaseName, FString::Printf(TEXT("items/%s"), *FileName));
        }
    }
}

void UCWNativeRunHudWidget::LoadTexture(const FString& Key, const FString& RelativePath)
{
    if (TextureCache.Contains(Key))
    {
        return;
    }

    const FString NormalizedRelativePath = RelativePath.Replace(TEXT("/"), TEXT("\\"));
    const FString AbsolutePath = FCWNativeAssetLibrary::NormalizeAssetPath(AssetsRoot, NormalizedRelativePath);
    if (!FPaths::FileExists(AbsolutePath))
    {
        return;
    }

    UTexture2D* Texture = FCWNativeAssetLibrary::LoadTexture(this, AssetsRoot, NormalizedRelativePath);
    if (!Texture)
    {
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

const FSlateBrush* UCWNativeRunHudWidget::GetBrush(const FString& Key) const
{
    return BrushCache.Find(Key);
}

void UCWNativeRunHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateFps(InDeltaTime);
    InvalidateLayoutAndVolatility();
}

FReply UCWNativeRunHudWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn()))
        {
            const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            Pawn->UpdateNativeCursorScreenPosition(InMouseEvent.GetScreenSpacePosition());
            Pawn->UpdateNativeCursorViewportPosition(LocalMouse, InGeometry.GetLocalSize());
            if (Pawn->HandleNativeHudClickAtViewportPosition(LocalMouse, InGeometry.GetLocalSize()))
            {
                return FReply::Handled();
            }
            if (Pawn->IsNativeRunInputActive())
            {
                Pawn->SetNativeMouseButtonState(EKeys::LeftMouseButton, true);
                return FReply::Handled();
            }
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCWNativeRunHudWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn()))
        {
            const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            Pawn->UpdateNativeCursorScreenPosition(InMouseEvent.GetScreenSpacePosition());
            Pawn->UpdateNativeCursorViewportPosition(LocalMouse, InGeometry.GetLocalSize());
            if (Pawn->IsNativeRendererActive())
            {
                Pawn->SetNativeMouseButtonState(EKeys::LeftMouseButton, false);
                return FReply::Handled();
            }
        }
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCWNativeRunHudWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->IsNativeRendererActive())
        {
            const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            Pawn->UpdateNativeCursorScreenPosition(InMouseEvent.GetScreenSpacePosition());
            Pawn->UpdateNativeCursorViewportPosition(LocalMouse, InGeometry.GetLocalSize());
        }
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UCWNativeRunHudWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent)
{
    ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn());
    if (!Pawn)
    {
        return Super::NativeOnTouchStarted(InGeometry, InTouchEvent);
    }

    const FVector2D ViewportSize = InGeometry.GetLocalSize();
    const FVector2D LocalTouch = InGeometry.AbsoluteToLocal(InTouchEvent.GetScreenSpacePosition());
    const int32 PointerIndex = InTouchEvent.GetPointerIndex();
    Pawn->UpdateNativeCursorScreenPosition(InTouchEvent.GetScreenSpacePosition());
    Pawn->UpdateNativeCursorViewportPosition(LocalTouch, ViewportSize);

    if (Pawn->HandleNativeHudClickAtViewportPosition(LocalTouch, ViewportSize))
    {
        return FReply::Handled();
    }

    if (!Pawn->IsNativeRunInputActive())
    {
        return FReply::Handled();
    }

    if (LocalTouch.X >= ViewportSize.X * 0.5f && MoveTouchPointer == INDEX_NONE)
    {
        MoveTouchPointer = PointerIndex;
        MoveStickCenter = GetMovementStickCenter(ViewportSize);
        MoveStickThumb = ClampStickThumb(MoveStickCenter, LocalTouch, GetTouchStickRadius(ViewportSize));
        ApplyMovementTouch(Pawn, ViewportSize);
        return FReply::Handled();
    }

    if (LocalTouch.X < ViewportSize.X * 0.5f && ShootTouchPointer == INDEX_NONE)
    {
        ShootTouchPointer = PointerIndex;
        ShootStickCenter = LocalTouch;
        ShootStickThumb = LocalTouch;
        ApplyShootingTouch(Pawn, ViewportSize);
        return FReply::Handled();
    }

    return FReply::Handled();
}

FReply UCWNativeRunHudWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent)
{
    ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn());
    if (!Pawn)
    {
        return Super::NativeOnTouchMoved(InGeometry, InTouchEvent);
    }

    const FVector2D ViewportSize = InGeometry.GetLocalSize();
    const FVector2D LocalTouch = InGeometry.AbsoluteToLocal(InTouchEvent.GetScreenSpacePosition());
    const int32 PointerIndex = InTouchEvent.GetPointerIndex();
    Pawn->UpdateNativeCursorScreenPosition(InTouchEvent.GetScreenSpacePosition());
    Pawn->UpdateNativeCursorViewportPosition(LocalTouch, ViewportSize);

    if (PointerIndex == MoveTouchPointer)
    {
        MoveStickThumb = ClampStickThumb(MoveStickCenter, LocalTouch, GetTouchStickRadius(ViewportSize));
        ApplyMovementTouch(Pawn, ViewportSize);
        return FReply::Handled();
    }

    if (PointerIndex == ShootTouchPointer)
    {
        ShootStickThumb = ClampStickThumb(ShootStickCenter, LocalTouch, GetTouchStickRadius(ViewportSize));
        ApplyShootingTouch(Pawn, ViewportSize);
        return FReply::Handled();
    }

    return FReply::Handled();
}

FReply UCWNativeRunHudWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InTouchEvent)
{
    if (ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn()))
    {
        ReleaseTouchPointer(Pawn, InTouchEvent.GetPointerIndex());
        return FReply::Handled();
    }

    return Super::NativeOnTouchEnded(InGeometry, InTouchEvent);
}

FReply UCWNativeRunHudWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->HandleNativeChatKeyDown(InKeyEvent.GetKey()))
        {
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UCWNativeRunHudWidget::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
    if (ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->HandleNativeChatCharacter(InCharacterEvent.GetCharacter()))
        {
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyChar(InGeometry, InCharacterEvent);
}

FVector2D UCWNativeRunHudWidget::GetMovementStickCenter(const FVector2D& ViewportSize) const
{
    const float Radius = GetTouchStickRadius(ViewportSize);
    return FVector2D(
        FMath::Max(Radius + 28.0f, ViewportSize.X - Radius - 58.0f),
        FMath::Clamp(ViewportSize.Y - Radius - 112.0f, Radius + 70.0f, ViewportSize.Y - Radius - 38.0f));
}

float UCWNativeRunHudWidget::GetTouchStickRadius(const FVector2D& ViewportSize) const
{
    return FMath::Clamp(ViewportSize.Y * 0.085f, 54.0f, 82.0f);
}

FVector2D UCWNativeRunHudWidget::ClampStickThumb(const FVector2D& Center, const FVector2D& Position, float Radius) const
{
    const FVector2D Delta = Position - Center;
    if (Delta.SizeSquared() <= Radius * Radius)
    {
        return Position;
    }
    return Center + Delta.GetSafeNormal() * Radius;
}

FVector2D UCWNativeRunHudWidget::GetStickVector(const FVector2D& Center, const FVector2D& Thumb, float Radius, bool bInvertY) const
{
    if (Radius <= 1.0f)
    {
        return FVector2D::ZeroVector;
    }

    FVector2D Vector = (Thumb - Center) / Radius;
    Vector = Vector.GetClampedToMaxSize(1.0f);
    if (bInvertY)
    {
        Vector.Y *= -1.0f;
    }
    if (Vector.SizeSquared() < 0.0144f)
    {
        return FVector2D::ZeroVector;
    }
    return Vector;
}

bool UCWNativeRunHudWidget::ApplyMovementTouch(ACWNativePlayerPawn* Pawn, const FVector2D& ViewportSize)
{
    if (!Pawn || MoveTouchPointer == INDEX_NONE)
    {
        return false;
    }

    const FVector2D MoveVector = GetStickVector(MoveStickCenter, MoveStickThumb, GetTouchStickRadius(ViewportSize), true);
    Pawn->SetNativeTouchMoveVector(MoveVector, true);
    return true;
}

bool UCWNativeRunHudWidget::ApplyShootingTouch(ACWNativePlayerPawn* Pawn, const FVector2D& ViewportSize)
{
    if (!Pawn || ShootTouchPointer == INDEX_NONE)
    {
        return false;
    }

    FVector2D AimVector = GetStickVector(ShootStickCenter, ShootStickThumb, GetTouchStickRadius(ViewportSize), false);
    if (AimVector.IsNearlyZero(0.01f))
    {
        AimVector = (ShootStickCenter - ViewportSize * 0.5f).GetSafeNormal();
    }
    Pawn->SetNativeTouchAimVector(AimVector, true);
    return true;
}

void UCWNativeRunHudWidget::ReleaseTouchPointer(ACWNativePlayerPawn* Pawn, int32 PointerIndex)
{
    if (!Pawn)
    {
        return;
    }

    if (PointerIndex == MoveTouchPointer)
    {
        MoveTouchPointer = INDEX_NONE;
        MoveStickThumb = MoveStickCenter;
        Pawn->SetNativeTouchMoveVector(FVector2D::ZeroVector, false);
    }

    if (PointerIndex == ShootTouchPointer)
    {
        ShootTouchPointer = INDEX_NONE;
        ShootStickCenter = FVector2D::ZeroVector;
        ShootStickThumb = FVector2D::ZeroVector;
        Pawn->SetNativeTouchAimVector(FVector2D::ZeroVector, false);
    }
}

void UCWNativeRunHudWidget::BuildHud()
{
    if (!WidgetTree)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RunHudRoot"));
    WidgetTree->RootWidget = RootCanvas;

    UBorder* HitLayer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RunHudHitLayer"));
    HitLayer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.001f));
    HitLayer->SetPadding(FMargin(0.0f));
    HitLayer->SetVisibility(ESlateVisibility::Visible);
    if (UCanvasPanelSlot* HitSlot = RootCanvas->AddChildToCanvas(HitLayer))
    {
        HitSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        HitSlot->SetOffsets(FMargin(0.0f));
        HitSlot->SetZOrder(-100);
    }
}

void UCWNativeRunHudWidget::HandleStateReceived(const FCWRoomSnapshot& State)
{
    UpdateFromState(State);
}

void UCWNativeRunHudWidget::HandleTextMessage(const FString& Type, const FString& Text)
{
    FString Line = Text;
    Line.TrimStartAndEndInline();
    if (Line.IsEmpty())
    {
        return;
    }

    if (Type != TEXT("chat"))
    {
        Line = FString::Printf(TEXT("[%s] %s"), *Type.Left(12), *Line);
    }

    ChatLines.Add(Line.Left(120));
    if (ChatLines.Num() > 5)
    {
        ChatLines.RemoveAt(0, ChatLines.Num() - 5);
    }
    InvalidateLayoutAndVolatility();
}

void UCWNativeRunHudWidget::UpdateFromState(const FCWRoomSnapshot& State)
{
    CachedState = State;
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    CachedMyId = GI ? GI->MyId : FString();
    const FString MyId = CachedMyId;
    const FCWPlayerSnapshot* Me = nullptr;
    for (const FCWPlayerSnapshot& Player : State.Players)
    {
        if ((!MyId.IsEmpty() && Player.Id == MyId) || (!Me && !Player.bIsCompanion))
        {
            Me = &Player;
            if (!MyId.IsEmpty() && Player.Id == MyId)
            {
                break;
            }
        }
    }

    if (RoomText)
    {
        RoomText->SetText(FText::FromString(State.RoomCode.IsEmpty() ? TEXT("Room --") : FString::Printf(TEXT("Room %s / %s"), *State.RoomCode, *State.MapId)));
    }
    if (ThreatText)
    {
        ThreatText->SetText(FText::FromString(FString::Printf(TEXT("Players %d | Mobs %d | Bullets %d"), State.Players.Num(), State.Enemies.Num(), State.Bullets.Num())));
    }
    if (HealthText)
    {
        HealthText->SetText(FText::FromString(Me ? FString::Printf(TEXT("HP %.0f / %.0f"), Me->Hp, Me->MaxHp) : TEXT("HP --")));
    }
    if (WeaponText)
    {
        WeaponText->SetText(FText::FromString(Me ? FString::Printf(TEXT("  %s  Lv %d"), *Me->WeaponKey, Me->Level) : TEXT("  Weapon --")));
    }
    if (SkillsText)
    {
        SkillsText->SetText(FText::FromString(TEXT("  1 / 2 / 3 quick slots | Space dodge | Enter chat")));
    }
    if (MinimapText)
    {
        MinimapText->SetText(FText::FromString(FString::Printf(TEXT("MINIMAP\n%.0fx%.0f\nXP %d | Drops %d"), State.World.Width, State.World.Height, State.XpOrbs.Num(), State.Drops.Num())));
    }
    InvalidateLayoutAndVolatility();
}

int32 UCWNativeRunHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    if (Size.X <= 8.0f || Size.Y <= 8.0f)
    {
        return BaseLayer;
    }

    using namespace CWHud;

    const int32 PanelLayer = BaseLayer + 1;
    const int32 DetailLayer = BaseLayer + 8;
    const int32 TextLayer = BaseLayer + 14;
    const float WorldW = FMath::Max(1.0f, CachedState.World.Width);
    const float WorldH = FMath::Max(1.0f, CachedState.World.Height);

    const FCWPlayerSnapshot* Me = nullptr;
    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        if (!CachedMyId.IsEmpty() && Player.Id == CachedMyId)
        {
            Me = &Player;
            break;
        }
        if (!Me && !Player.bIsCompanion)
        {
            Me = &Player;
        }
    }
    const ACWNativePlayerPawn* Pawn = Cast<ACWNativePlayerPawn>(GetOwningPlayerPawn());

    const double NowMs = CachedState.ServerNowMs > 0.0 ? CachedState.ServerNowMs : FPlatformTime::Seconds() * 1000.0;
    const double StartedAt = CachedState.RoomStartedAt > 0.0 ? CachedState.RoomStartedAt : NowMs;
    const double ElapsedSec = FMath::Max(0.0, (NowMs - StartedAt) / 1000.0);
    const int32 ThreatLevel = FMath::Max(1, CachedState.RoomDifficulty.Level);
    const float HpMul = FMath::Max(1.0f, CachedState.RoomDifficulty.HpMul);
    FString BossText;
    if (CachedState.bBossAlive)
    {
        BossText = TEXT("Boss: ACTIVE");
    }
    else if (CachedState.NextBossSpawnAt > NowMs)
    {
        BossText = FString::Printf(TEXT("Boss in %.1fs"), (CachedState.NextBossSpawnAt - NowMs) / 1000.0);
    }
    else
    {
        const int32 LeftKills = FMath::Max(0, CachedState.NextBossAtKills - CachedState.TotalEnemyKills);
        BossText = FString::Printf(TEXT("Boss in %d kills"), LeftKills);
    }

    const FVector2D TopSize(FMath::Clamp(Size.X - 420.0f, 430.0f, 620.0f), 34.0f);
    const FVector2D TopPos((Size.X - TopSize.X) * 0.5f, 12.0f);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, TopPos, TopSize, FLinearColor(0.015f, 0.022f, 0.034f, 0.88f), FLinearColor(0.80f, 0.94f, 1.0f, 0.24f));
    const FString TopText = FString::Printf(TEXT("Time %s  |  %s  |  Threat Lv%d x%.2f  |  Viewers: %d"),
        *FormatClock(ElapsedSec),
        *BossText,
        ThreatLevel,
        HpMul,
        CachedState.SpectatorCount);
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, TopPos + FVector2D(14.0f, 6.0f), TopText, 13.0f, FLinearColor(0.86f, 0.91f, 0.96f, 0.98f), true);

    const FVector2D MapPos(14.0f, 14.0f);
    const FVector2D MapSize(FMath::Clamp(Size.Y * 0.23f, 138.0f, 188.0f), FMath::Clamp(Size.Y * 0.23f, 138.0f, 188.0f));
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, MapPos, MapSize, FLinearColor(0.010f, 0.018f, 0.028f, 0.88f), FLinearColor(0.24f, 0.86f, 1.0f, 0.34f));
    const FVector2D InnerPos = MapPos + FVector2D(9.0f, 9.0f);
    const FVector2D InnerSize = MapSize - FVector2D(18.0f, 18.0f);
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer, InnerPos, InnerSize, FLinearColor(0.018f, 0.070f, 0.065f, 0.90f));
    for (int32 I = 1; I < 6; ++I)
    {
        const float T = static_cast<float>(I) / 6.0f;
        DrawLine(OutDrawElements, AllottedGeometry, DetailLayer + 1, InnerPos + FVector2D(InnerSize.X * T, 0.0f), InnerPos + FVector2D(InnerSize.X * T, InnerSize.Y), FLinearColor(0.24f, 0.55f, 0.52f, 0.18f), 1.0f);
        DrawLine(OutDrawElements, AllottedGeometry, DetailLayer + 1, InnerPos + FVector2D(0.0f, InnerSize.Y * T), InnerPos + FVector2D(InnerSize.X, InnerSize.Y * T), FLinearColor(0.24f, 0.55f, 0.52f, 0.18f), 1.0f);
    }
    const FVector2D RoadMiniPos(InnerPos.X, InnerPos.Y + InnerSize.Y * 0.40f);
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 1, RoadMiniPos, FVector2D(InnerSize.X, InnerSize.Y * 0.20f), FLinearColor(0.29f, 0.32f, 0.35f, 0.56f));

    auto MiniPoint = [&](float X, float Y)
    {
        return InnerPos + FVector2D(FMath::Clamp(X / WorldW, 0.0f, 1.0f) * InnerSize.X, FMath::Clamp(Y / WorldH, 0.0f, 1.0f) * InnerSize.Y);
    };
    auto MiniDot = [&](const FVector2D& P, const FLinearColor& Color, float Radius)
    {
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 3, P - FVector2D(Radius, Radius), FVector2D(Radius * 2.0f, Radius * 2.0f), Color);
    };
    for (const FCWMapObjectSnapshot& Object : CachedState.MapObjects)
    {
        if (Object.bDestroyed && Object.bHideAfterDestroyed)
        {
            continue;
        }
        const FVector2D P = MiniPoint(Object.X, Object.Y);
        MiniDot(P, Object.bSolid ? FLinearColor(0.68f, 0.74f, 0.74f, 0.55f) : FLinearColor(0.50f, 0.56f, 0.54f, 0.38f), 2.0f);
    }
    for (const FCWPickupSnapshot& Orb : CachedState.XpOrbs)
    {
        MiniDot(MiniPoint(Orb.X, Orb.Y), FLinearColor(0.16f, 0.88f, 1.0f, 0.78f), 1.4f);
    }
    for (const FCWEnemySnapshot& Enemy : CachedState.Enemies)
    {
        const bool bBoss = Enemy.Type.Contains(TEXT("boss"), ESearchCase::IgnoreCase) || Enemy.Behavior.Contains(TEXT("boss"), ESearchCase::IgnoreCase);
        MiniDot(MiniPoint(Enemy.X, Enemy.Y), bBoss ? FLinearColor(1.0f, 0.08f, 0.06f, 0.96f) : FLinearColor(0.24f, 0.84f, 1.0f, 0.72f), bBoss ? 3.4f : 1.8f);
    }
    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        const bool bMe = Me && Player.Id == Me->Id;
        MiniDot(MiniPoint(Player.X, Player.Y), bMe ? FLinearColor(1.0f, 0.28f, 0.72f, 1.0f) : FLinearColor(0.28f, 1.0f, 0.44f, 0.96f), bMe ? 3.4f : 2.6f);
    }
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, MapPos + FVector2D(8.0f, MapSize.Y + 7.0f), FString::Printf(TEXT("FPS: %d | Ping: %dms"), CachedFps, Me ? Me->NetPingMs : 0), 11.0f, FLinearColor(0.86f, 0.94f, 1.0f, 0.95f), true);

    const FVector2D MenuPos(MapPos.X + MapSize.X + 10.0f, 18.0f);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, MenuPos, FVector2D(66.0f, 30.0f), FLinearColor(0.012f, 0.020f, 0.032f, 0.92f), FLinearColor(0.70f, 0.92f, 1.0f, 0.22f));
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, MenuPos + FVector2D(12.0f, 7.0f), TEXT("Menu"), 12.0f, FLinearColor(0.90f, 0.95f, 1.0f, 0.98f), true);

    const FVector2D PlayersPos(Size.X - 142.0f, 18.0f);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, PlayersPos, FVector2D(122.0f, 42.0f), FLinearColor(0.012f, 0.020f, 0.032f, 0.92f), FLinearColor(0.74f, 0.94f, 1.0f, 0.22f));
    int32 RealPlayers = 0;
    for (const FCWPlayerSnapshot& Player : CachedState.Players)
    {
        if (!Player.bIsCompanion)
        {
            ++RealPlayers;
        }
    }
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, PlayersPos + FVector2D(10.0f, 11.0f), FString::Printf(TEXT("Players: %d"), RealPlayers), 14.0f, FLinearColor(0.92f, 0.96f, 1.0f, 0.98f), true);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 2, PlayersPos + FVector2D(92.0f, 8.0f), FVector2D(22.0f, 22.0f), FLinearColor(0.08f, 0.12f, 0.16f, 0.92f), FLinearColor(0.80f, 0.88f, 1.0f, 0.22f));
    DrawText(OutDrawElements, AllottedGeometry, TextLayer + 1, PlayersPos + FVector2D(Pawn && Pawn->IsNativePlayersPanelOpen() ? 99.0f : 98.0f, 8.0f), Pawn && Pawn->IsNativePlayersPanelOpen() ? TEXT("-") : TEXT("+"), 15.0f, FLinearColor(0.82f, 0.96f, 1.0f, 0.94f), true);

    const float XpPanelW = FMath::Min(760.0f, FMath::Max(460.0f, Size.X * 0.38f));
    const FVector2D XpPos((Size.X - XpPanelW) * 0.5f, Size.Y - 68.0f);
    const FVector2D XpSize(XpPanelW, 54.0f);
    const int32 Level = Me ? FMath::Max(1, Me->Level) : 1;
    const int32 Xp = Me ? FMath::Max(0, Me->Xp) : 0;
    const int32 XpToNext = Me ? FMath::Max(1, Me->XpToNext) : 1;
    const float XpPct = FMath::Clamp(static_cast<float>(Xp) / FMath::Max(1.0f, static_cast<float>(XpToNext)), 0.0f, 1.0f);
    FString HeroId = TEXT("cyber");
    if (Me && !Me->PlayerClass.IsEmpty())
    {
        HeroId = NormalizeHeroId(Me->PlayerClass);
    }
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, XpPos, XpSize, FLinearColor(0.010f, 0.020f, 0.034f, 0.94f), FLinearColor(0.20f, 0.90f, 1.0f, 0.34f));
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer, XpPos + FVector2D(1.0f, 1.0f), FVector2D(XpSize.X - 2.0f, 12.0f), FLinearColor(0.10f, 0.82f, 1.0f, 0.10f));
    const FVector2D LevelPos = XpPos + FVector2D(10.0f, 8.0f);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 2, LevelPos, FVector2D(58.0f, 24.0f), FLinearColor(0.06f, 0.08f, 0.11f, 0.94f), FLinearColor(0.98f, 0.82f, 0.26f, 0.42f));
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, LevelPos + FVector2D(12.0f, 5.0f), FString::Printf(TEXT("Lv%d"), Level), 12.0f, FLinearColor(1.0f, 0.90f, 0.56f, 0.98f), true);
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, XpPos + FVector2D(XpSize.X - 225.0f, 9.0f), FString::Printf(TEXT("COMBAT XP  %d / %d XP - %d%%"), Xp, XpToNext, FMath::RoundToInt(XpPct * 100.0f)), 11.0f, FLinearColor(0.73f, 1.0f, 0.86f, 0.98f), true);
    const FVector2D BarPos = XpPos + FVector2D(12.0f, 34.0f);
    const FVector2D BarSize = FVector2D(XpSize.X - 24.0f, 14.0f);
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer, BarPos, BarSize, FLinearColor(0.005f, 0.012f, 0.025f, 0.96f));
    for (int32 I = 1; I < 12; ++I)
    {
        const float X = BarPos.X + BarSize.X * static_cast<float>(I) / 12.0f;
        DrawLine(OutDrawElements, AllottedGeometry, DetailLayer + 1, FVector2D(X, BarPos.Y + 2.0f), FVector2D(X, BarPos.Y + BarSize.Y - 2.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), 1.0f);
    }
    const float FillW = FMath::Max(0.0f, BarSize.X * XpPct);
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 2, BarPos + FVector2D(1.0f, 1.0f), FVector2D(FillW, BarSize.Y - 2.0f), FLinearColor(0.08f, 0.86f, 1.0f, 0.90f));
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 3, BarPos + FVector2D(1.0f, 1.0f), FVector2D(FillW * 0.55f, BarSize.Y - 2.0f), FLinearColor(0.72f, 1.0f, 0.70f, 0.22f));
    DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 3, BarPos + FVector2D(FillW - 2.0f, 0.0f), FVector2D(3.0f, BarSize.Y), FLinearColor(1.0f, 0.92f, 0.36f, XpPct > 0.02f ? 0.82f : 0.0f));

    TArray<FCWSkillSnapshot> ActiveSkills;
    TArray<FCWSkillSnapshot> PassiveSkills;
    TArray<FCWQuickSlotSnapshot> QuickSlots;
    if (Me)
    {
        for (const FCWSkillSnapshot& Skill : Me->Skills)
        {
            if (Skill.Kind.Equals(TEXT("active"), ESearchCase::IgnoreCase))
            {
                ActiveSkills.Add(Skill);
            }
            else
            {
                PassiveSkills.Add(Skill);
            }
        }
        QuickSlots = Me->QuickSlots;
    }

    auto AddFallbackSkill = [](TArray<FCWSkillSnapshot>& Skills, const TCHAR* Id, const TCHAR* Name, const TCHAR* Kind, const TCHAR* Rarity, int32 InLevel)
    {
        FCWSkillSnapshot Skill;
        Skill.Id = Id;
        Skill.Name = Name;
        Skill.Kind = Kind;
        Skill.Rarity = Rarity;
        Skill.Level = FMath::Max(1, InLevel);
        Skill.MaxCooldownMs = FCString::Stricmp(Kind, TEXT("active")) == 0 ? 1000.0f : 0.0f;
        Skills.Add(Skill);
    };

    if (ActiveSkills.Num() == 0)
    {
        AddFallbackSkill(ActiveSkills, TEXT("pulse_wave"), TEXT("Pulse Wave"), TEXT("active"), TEXT("rare"), Level);
        AddFallbackSkill(ActiveSkills, TEXT("ion_lance"), TEXT("Ion Lance"), TEXT("active"), TEXT("epic"), FMath::Max(1, Level - 1));
        AddFallbackSkill(ActiveSkills, TEXT("arc_matrix"), TEXT("Arc Matrix"), TEXT("active"), TEXT("epic"), Level);
        AddFallbackSkill(ActiveSkills, TEXT("seeker_protocol"), TEXT("Seeker Protocol"), TEXT("active"), TEXT("legendary"), FMath::Max(1, Level - 2));
    }

    if (PassiveSkills.Num() == 0)
    {
        AddFallbackSkill(PassiveSkills, TEXT("adaptive_frame"), TEXT("Adaptive Frame"), TEXT("passive"), TEXT("common"), Level);
        AddFallbackSkill(PassiveSkills, TEXT("combat_firmware"), TEXT("Combat Firmware"), TEXT("passive"), TEXT("rare"), Level);
        AddFallbackSkill(PassiveSkills, TEXT("sync_link"), TEXT("Sync Link"), TEXT("passive"), TEXT("epic"), FMath::Max(1, Level - 1));
        AddFallbackSkill(PassiveSkills, TEXT("cache_goblin"), TEXT("Cache Supply"), TEXT("passive"), TEXT("rare"), FMath::Max(1, Level - 1));
    }

    for (int32 Hotkey = QuickSlots.Num() + 1; Hotkey <= 3; ++Hotkey)
    {
        FCWQuickSlotSnapshot QuickSlot;
        QuickSlot.Hotkey = Hotkey;
        QuickSlot.bEmpty = true;
        QuickSlot.Rarity = TEXT("common");
        QuickSlots.Add(QuickSlot);
    }

    const int32 ActiveCount = ActiveSkills.Num();
    const int32 QuickCount = QuickSlots.Num();
    const int32 PrimaryCount = FMath::Max(1, ActiveCount + QuickCount);
    const float SlotW = 56.0f;
    const float SlotGap = 8.0f;
    const float PrimaryW = PrimaryCount * SlotW + (PrimaryCount - 1) * SlotGap;
    const float SkillY = XpPos.Y - 152.0f;
    float SlotX = (Size.X - PrimaryW) * 0.5f;

    auto DrawSkillSlot = [&](const FVector2D& Pos, const FString& Badge, const FString& LevelText, const FLinearColor& Color, const FLinearColor& Rarity, float CooldownPct, const FString& CooldownText, bool bSmall, const FSlateBrush* IconBrush)
    {
        const FVector2D S(bSmall ? 40.0f : 56.0f, bSmall ? 44.0f : 62.0f);
        const float GlowA = bSmall ? 0.12f : 0.26f;
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 1, Pos - FVector2D(10.0f, 10.0f), S + FVector2D(20.0f, 20.0f), FLinearColor(Color.R, Color.G, Color.B, GlowA * 0.34f));
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 2, Pos - FVector2D(5.0f, 5.0f), S + FVector2D(10.0f, 10.0f), FLinearColor(Color.R, Color.G, Color.B, GlowA));
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 4, Pos, S, FLinearColor(0.004f, 0.008f, 0.018f, 0.97f), FLinearColor(Rarity.R, Rarity.G, Rarity.B, bSmall ? 0.60f : 0.84f));
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 3, Pos + FVector2D(2.0f, 2.0f), FVector2D(S.X - 4.0f, 2.0f), FLinearColor(0.86f, 1.0f, 1.0f, 0.26f));
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 3, Pos + FVector2D(2.0f, S.Y - 4.0f), FVector2D(S.X - 4.0f, 2.0f), FLinearColor(Color.R, Color.G, Color.B, 0.64f));
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 4, Pos + FVector2D(4.0f, 4.0f), S - FVector2D(8.0f, 8.0f), FLinearColor(Color.R, Color.G, Color.B, bSmall ? 0.16f : 0.28f));
        DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 5, Pos + FVector2D(8.0f, 10.0f), S - FVector2D(16.0f, bSmall ? 18.0f : 24.0f), FLinearColor(0.006f, 0.016f, 0.030f, 0.98f));
        if (IconBrush)
        {
            const FVector2D IconPos = Pos + FVector2D(bSmall ? 8.0f : 7.0f, bSmall ? 11.0f : 10.0f);
            const FVector2D IconSize = S - FVector2D(bSmall ? 16.0f : 14.0f, bSmall ? 20.0f : 24.0f);
            DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 6, IconPos - FVector2D(2.0f, 2.0f), IconSize + FVector2D(4.0f, 4.0f), FLinearColor(Color.R, Color.G, Color.B, 0.10f));
            DrawTexture(OutDrawElements, AllottedGeometry, DetailLayer + 6, IconBrush, IconPos, IconSize, FLinearColor(1.04f, 1.04f, 1.04f, 0.98f));
            DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 7, IconPos, FVector2D(IconSize.X, 1.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
        }
        else
        {
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 1, Pos + FVector2D(bSmall ? 11.0f : 14.0f, bSmall ? 15.0f : 20.0f), Badge, bSmall ? 10.0f : 13.0f, FLinearColor(0.94f, 0.98f, 1.0f, 0.98f), true);
        }
        if (!LevelText.IsEmpty())
        {
            DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 6, Pos + FVector2D(2.0f, 2.0f), FVector2D(30.0f, 15.0f), FLinearColor(0.004f, 0.008f, 0.016f, 0.90f), FLinearColor(0.76f, 0.98f, 1.0f, 0.28f));
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 2, Pos + FVector2D(6.0f, 3.0f), LevelText, 8.0f, FLinearColor(0.84f, 0.92f, 1.0f, 0.98f), true);
        }
        if (CooldownPct > 0.01f)
        {
            const float OverlayH = (S.Y - 8.0f) * FMath::Clamp(CooldownPct, 0.0f, 1.0f);
            DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 7, Pos + FVector2D(4.0f, S.Y - 4.0f - OverlayH), FVector2D(S.X - 8.0f, OverlayH), FLinearColor(0.0f, 0.0f, 0.0f, 0.52f));
            if (!CooldownText.IsEmpty())
            {
                DrawText(OutDrawElements, AllottedGeometry, TextLayer + 3, Pos + FVector2D(S.X * 0.5f - 13.0f, S.Y * 0.5f - 7.0f), CooldownText, 9.0f, FLinearColor(0.96f, 0.98f, 1.0f, 0.96f), true);
            }
        }
        else
        {
            DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 7, Pos + FVector2D(S.X - 9.0f, 5.0f), FVector2D(5.0f, 5.0f), FLinearColor(0.30f, 1.0f, 0.72f, 0.98f));
            if (!bSmall)
            {
                DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 7, Pos + FVector2D(5.0f, S.Y - 8.0f), FVector2D(S.X - 10.0f, 2.0f), FLinearColor(0.14f, 1.0f, 0.94f, 0.72f));
            }
        }
    };

    for (const FCWSkillSnapshot& Skill : ActiveSkills)
    {
        const float CooldownPct = Skill.MaxCooldownMs > 1.0f ? FMath::Clamp(Skill.CooldownMs / Skill.MaxCooldownMs, 0.0f, 1.0f) : 0.0f;
        const FString CooldownText = Skill.CooldownMs > 50.0f ? FString::Printf(TEXT("%.1fs"), Skill.CooldownMs / 1000.0f) : FString();
        DrawSkillSlot(FVector2D(SlotX, SkillY), BadgeFromText(Skill.Name.IsEmpty() ? Skill.Id : Skill.Name), FString::Printf(TEXT("Lv%d"), Skill.Level), SkillColor(Skill, 1.0f), RarityColor(Skill.Rarity, 0.70f), CooldownPct, CooldownText, false, GetBrush(SkillIconKey(Skill, HeroId)));
        SlotX += SlotW + SlotGap;
    }

    for (const FCWQuickSlotSnapshot& QuickSlot : QuickSlots)
    {
        const FString Badge = QuickSlot.bEmpty ? FString::FromInt(FMath::Max(1, QuickSlot.Hotkey)) : BadgeFromText(QuickSlot.Name.IsEmpty() ? QuickSlot.ItemId : QuickSlot.Name);
        const FString LevelText = QuickSlot.bEmpty ? FString() : FString::Printf(TEXT("[%d]"), FMath::Max(1, QuickSlot.Hotkey));
        const FLinearColor Color = QuickSlot.bEmpty ? FLinearColor(0.24f, 0.32f, 0.42f, 1.0f) : FLinearColor(0.30f, 0.92f, 0.72f, 1.0f);
        DrawSkillSlot(FVector2D(SlotX, SkillY), Badge, LevelText, Color, RarityColor(QuickSlot.Rarity, QuickSlot.bEmpty ? 0.28f : 0.62f), 0.0f, FString(), false, QuickSlot.bEmpty ? nullptr : GetBrush(ItemIconKey(QuickSlot.ItemId)));
        if (!QuickSlot.bEmpty && QuickSlot.Quantity > 1)
        {
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 3, FVector2D(SlotX + 35.0f, SkillY + 42.0f), FString::Printf(TEXT("x%d"), QuickSlot.Quantity), 9.0f, FLinearColor(0.74f, 1.0f, 0.82f, 0.98f), true);
        }
        SlotX += SlotW + SlotGap;
    }

    if (PassiveSkills.Num() > 0)
    {
        const float SmallW = 40.0f;
        const int32 Count = FMath::Min(PassiveSkills.Num(), 8);
        float PassiveX = (Size.X - (Count * SmallW + (Count - 1) * 6.0f)) * 0.5f;
        const float PassiveY = SkillY + 70.0f;
        for (int32 I = 0; I < Count; ++I)
        {
            const FCWSkillSnapshot& Skill = PassiveSkills[I];
            DrawSkillSlot(FVector2D(PassiveX, PassiveY), BadgeFromText(Skill.Name.IsEmpty() ? Skill.Id : Skill.Name, 2), FString::Printf(TEXT("Lv%d"), Skill.Level), SkillColor(Skill, 1.0f), RarityColor(Skill.Rarity, 0.54f), 0.0f, FString(), true, GetBrush(SkillIconKey(Skill, HeroId)));
            PassiveX += SmallW + 6.0f;
        }
    }

    const bool bChatOpen = Pawn && Pawn->IsNativeChatOpen();
    const FString ChatDraft = Pawn ? Pawn->GetNativeChatDraft() : FString();
    const FVector2D ChatPos(14.0f, FMath::Max(MapPos.Y + MapSize.Y + 46.0f, Size.Y - 158.0f));
    const int32 VisibleChatLines = FMath::Min(ChatLines.Num(), 4);
    for (int32 I = 0; I < VisibleChatLines; ++I)
    {
        const int32 LineIndex = ChatLines.Num() - VisibleChatLines + I;
        const float Y = ChatPos.Y - static_cast<float>(VisibleChatLines - I) * 20.0f - 6.0f;
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, FVector2D(ChatPos.X, Y), FVector2D(318.0f, 18.0f), FLinearColor(0.004f, 0.020f, 0.034f, 0.62f), FLinearColor(0.20f, 0.68f, 0.90f, 0.12f));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer, FVector2D(ChatPos.X + 8.0f, Y + 3.0f), ChatLines[LineIndex], 9.0f, FLinearColor(0.78f, 0.90f, 1.0f, 0.88f), false);
    }
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, ChatPos, FVector2D(282.0f, 31.0f), bChatOpen ? FLinearColor(0.008f, 0.060f, 0.090f, 0.94f) : FLinearColor(0.005f, 0.034f, 0.062f, 0.86f), bChatOpen ? FLinearColor(0.32f, 0.92f, 1.0f, 0.52f) : FLinearColor(0.20f, 0.68f, 0.90f, 0.24f));
    const FString ChatText = bChatOpen
        ? (ChatDraft.IsEmpty() ? TEXT("Type message...") : (ChatDraft + TEXT("|")))
        : TEXT("Enter chat. /mute nick");
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, ChatPos + FVector2D(10.0f, 8.0f), ChatText.Left(42), 10.0f, bChatOpen ? FLinearColor(0.86f, 0.98f, 1.0f, 0.98f) : FLinearColor(0.72f, 0.82f, 0.94f, 0.78f), false);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 2, ChatPos + FVector2D(252.0f, 4.0f), FVector2D(24.0f, 23.0f), FLinearColor(0.02f, 0.12f, 0.18f, 0.92f), bChatOpen ? FLinearColor(0.42f, 1.0f, 0.72f, 0.60f) : FLinearColor(0.20f, 0.80f, 1.0f, 0.38f));
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, ChatPos + FVector2D(258.0f, 7.0f), TEXT(">"), 12.0f, bChatOpen ? FLinearColor(0.64f, 1.0f, 0.78f, 0.96f) : FLinearColor(0.64f, 0.94f, 1.0f, 0.96f), true);

    const FVector2D StatsButtonPos(Size.X - 82.0f, Size.Y - 38.0f);
    DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer, StatsButtonPos, FVector2D(64.0f, 30.0f), FLinearColor(0.018f, 0.030f, 0.048f, 0.88f), FLinearColor(0.88f, 0.95f, 1.0f, 0.18f));
    DrawText(OutDrawElements, AllottedGeometry, TextLayer, StatsButtonPos + FVector2D(17.0f, 8.0f), TEXT("Stats"), 11.0f, FLinearColor(0.88f, 0.94f, 1.0f, 0.96f), true);

#if PLATFORM_ANDROID || PLATFORM_IOS
    const bool bMobileTouchControls = true;
#else
    const bool bMobileTouchControls = false;
#endif
    if (Pawn && Pawn->IsNativeRunInputActive() && (bMobileTouchControls || MoveTouchPointer != INDEX_NONE || ShootTouchPointer != INDEX_NONE))
    {
        const float StickRadius = GetTouchStickRadius(Size);
        auto DrawVirtualStick = [&](const FVector2D& Center, const FVector2D& Thumb, const FString& Label, const FLinearColor& Accent, bool bActive)
        {
            const float BaseAlpha = bActive ? 0.28f : 0.14f;
            DrawDisc(OutDrawElements, AllottedGeometry, PanelLayer + 4, Center, StickRadius, FLinearColor(0.006f, 0.020f, 0.034f, BaseAlpha), 18);
            DrawCircle(OutDrawElements, AllottedGeometry, PanelLayer + 6, Center, StickRadius, FLinearColor(Accent.R, Accent.G, Accent.B, bActive ? 0.74f : 0.34f), bActive ? 2.8f : 1.8f, 42);
            DrawCircle(OutDrawElements, AllottedGeometry, PanelLayer + 6, Center, StickRadius * 0.55f, FLinearColor(Accent.R, Accent.G, Accent.B, bActive ? 0.40f : 0.18f), 1.2f, 34);
            DrawLine(OutDrawElements, AllottedGeometry, PanelLayer + 7, Center, Thumb, FLinearColor(Accent.R, Accent.G, Accent.B, bActive ? 0.52f : 0.18f), bActive ? 3.0f : 1.4f);
            DrawDisc(OutDrawElements, AllottedGeometry, PanelLayer + 8, Thumb, StickRadius * 0.30f, FLinearColor(Accent.R, Accent.G, Accent.B, bActive ? 0.38f : 0.20f), 14);
            DrawCircle(OutDrawElements, AllottedGeometry, PanelLayer + 10, Thumb, StickRadius * 0.30f, FLinearColor(0.86f, 0.98f, 1.0f, bActive ? 0.86f : 0.42f), 2.0f, 28);
            DrawText(OutDrawElements, AllottedGeometry, TextLayer, Center + FVector2D(-StickRadius * 0.36f, StickRadius + 9.0f), Label, 10.0f, FLinearColor(0.82f, 0.96f, 1.0f, bActive ? 0.82f : 0.42f), true);
        };

        const FVector2D MoveCenter = MoveTouchPointer != INDEX_NONE ? MoveStickCenter : GetMovementStickCenter(Size);
        const FVector2D MoveThumb = MoveTouchPointer != INDEX_NONE ? MoveStickThumb : MoveCenter;
        DrawVirtualStick(MoveCenter, MoveThumb, TEXT("MOVE"), FLinearColor(0.16f, 0.92f, 1.0f, 1.0f), MoveTouchPointer != INDEX_NONE);

        if (ShootTouchPointer != INDEX_NONE)
        {
            DrawVirtualStick(ShootStickCenter, ShootStickThumb, TEXT("FIRE"), FLinearColor(1.0f, 0.38f, 0.24f, 1.0f), true);
        }
    }

    if (Pawn && Pawn->IsNativePlayersPanelOpen())
    {
        const FVector2D PanelSize(300.0f, FMath::Clamp(Size.Y * 0.34f, 210.0f, 330.0f));
        const FVector2D PanelPos(Size.X - PanelSize.X - 20.0f, 70.0f);
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 10, PanelPos, PanelSize, FLinearColor(0.006f, 0.014f, 0.024f, 0.94f), FLinearColor(0.42f, 0.92f, 1.0f, 0.40f));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 8, PanelPos + FVector2D(16.0f, 12.0f), TEXT("PLAYERS"), 14.0f, FLinearColor(0.88f, 0.98f, 1.0f, 0.98f), true);
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 12, PanelPos + FVector2D(PanelSize.X - 34.0f, 10.0f), FVector2D(24.0f, 24.0f), FLinearColor(0.04f, 0.07f, 0.10f, 0.94f), FLinearColor(0.84f, 0.96f, 1.0f, 0.30f));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 9, PanelPos + FVector2D(PanelSize.X - 27.0f, 12.0f), TEXT("X"), 11.0f, FLinearColor(0.88f, 0.96f, 1.0f, 0.96f), true);

        TArray<const FCWPlayerSnapshot*> Players;
        for (const FCWPlayerSnapshot& Player : CachedState.Players)
        {
            if (!Player.bIsCompanion)
            {
                Players.Add(&Player);
            }
        }
        Players.Sort([](const FCWPlayerSnapshot& A, const FCWPlayerSnapshot& B)
        {
            return A.Level > B.Level;
        });

        const int32 Count = FMath::Min(Players.Num(), 8);
        for (int32 I = 0; I < Count; ++I)
        {
            const FCWPlayerSnapshot* Player = Players[I];
            if (!Player)
            {
                continue;
            }
            const float RowY = PanelPos.Y + 44.0f + I * 30.0f;
            const bool bMe = Me && Player->Id == Me->Id;
            DrawBox(OutDrawElements, AllottedGeometry, DetailLayer + 12, FVector2D(PanelPos.X + 12.0f, RowY), FVector2D(PanelSize.X - 24.0f, 24.0f), bMe ? FLinearColor(0.04f, 0.16f, 0.20f, 0.74f) : FLinearColor(0.01f, 0.025f, 0.040f, 0.62f));
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 8, FVector2D(PanelPos.X + 20.0f, RowY + 4.0f), Player->Name.IsEmpty() ? Player->Id.Left(16) : Player->Name.Left(16), 10.0f, bMe ? FLinearColor(0.80f, 1.0f, 0.88f, 0.98f) : FLinearColor(0.82f, 0.92f, 1.0f, 0.92f), true);
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 8, FVector2D(PanelPos.X + 140.0f, RowY + 4.0f), FString::Printf(TEXT("Lv%d  HP %.0f%%"), Player->Level, 100.0f * FMath::Clamp(Player->Hp / FMath::Max(1.0f, Player->MaxHp), 0.0f, 1.0f)), 9.0f, FLinearColor(0.74f, 0.88f, 1.0f, 0.86f), false);
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 8, FVector2D(PanelPos.X + 232.0f, RowY + 4.0f), FString::Printf(TEXT("%dms"), Player->NetPingMs), 9.0f, FLinearColor(0.58f, 0.96f, 1.0f, 0.82f), false);
        }
    }

    if (Pawn && Pawn->IsNativeStatsPanelOpen())
    {
        const FVector2D PanelSize(330.0f, 250.0f);
        const FVector2D PanelPos(Size.X - PanelSize.X - 20.0f, Size.Y - PanelSize.Y - 112.0f);
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 10, PanelPos, PanelSize, FLinearColor(0.006f, 0.014f, 0.024f, 0.95f), FLinearColor(0.90f, 0.96f, 1.0f, 0.34f));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 8, PanelPos + FVector2D(16.0f, 12.0f), TEXT("RUN STATS"), 14.0f, FLinearColor(0.92f, 0.98f, 1.0f, 0.98f), true);
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 12, PanelPos + FVector2D(PanelSize.X - 34.0f, 10.0f), FVector2D(24.0f, 24.0f), FLinearColor(0.04f, 0.07f, 0.10f, 0.94f), FLinearColor(0.84f, 0.96f, 1.0f, 0.30f));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 9, PanelPos + FVector2D(PanelSize.X - 27.0f, 12.0f), TEXT("X"), 11.0f, FLinearColor(0.88f, 0.96f, 1.0f, 0.96f), true);

        const FString WeaponName = Me && !Me->WeaponLabel.IsEmpty() ? Me->WeaponLabel : (Me ? Me->WeaponKey : TEXT("-"));
        const FString HeroName = Me ? (Me->Name.IsEmpty() ? Me->PlayerClass : Me->Name) : TEXT("-");
        const float HpPct = Me ? 100.0f * FMath::Clamp(Me->Hp / FMath::Max(1.0f, Me->MaxHp), 0.0f, 1.0f) : 0.0f;
        const float DodgePct = Me ? 100.0f * FMath::Clamp(static_cast<float>(Me->DodgeCharges) / FMath::Max(1.0f, static_cast<float>(Me->DodgeChargesMax)), 0.0f, 1.0f) : 0.0f;
        TArray<FString> Rows;
        Rows.Add(FString::Printf(TEXT("Hero: %s  Lv %d"), *HeroName.Left(24), Me ? Me->Level : 0));
        Rows.Add(FString::Printf(TEXT("HP: %.0f / %.0f  (%.0f%%)"), Me ? Me->Hp : 0.0f, Me ? Me->MaxHp : 0.0f, HpPct));
        Rows.Add(FString::Printf(TEXT("Weapon: %s"), *WeaponName.Left(26)));
        Rows.Add(FString::Printf(TEXT("Ammo: %d / %d  Reserve %d"), Me ? Me->MagazineAmmo : 0, Me ? Me->MagazineSize : 0, Me ? Me->ReserveAmmo : 0));
        Rows.Add(FString::Printf(TEXT("Dodge: %d/%d  %.0f%%"), Me ? Me->DodgeCharges : 0, Me ? Me->DodgeChargesMax : 0, DodgePct));
        Rows.Add(FString::Printf(TEXT("Pickup: %.0f  Ping: %dms"), Me ? Me->PickupRadius : 0.0f, Me ? Me->NetPingMs : 0));
        Rows.Add(FString::Printf(TEXT("Skills: %d  Quick items: %d"), Me ? Me->Skills.Num() : 0, Me ? Me->QuickSlots.Num() : 0));
        Rows.Add(FString::Printf(TEXT("Enemies: %d  Pickups: %d"), CachedState.Enemies.Num(), CachedState.Drops.Num() + CachedState.XpOrbs.Num() + CachedState.SkillOrbs.Num()));
        for (int32 I = 0; I < Rows.Num(); ++I)
        {
            const float RowY = PanelPos.Y + 48.0f + I * 23.0f;
            DrawText(OutDrawElements, AllottedGeometry, TextLayer + 8, FVector2D(PanelPos.X + 18.0f, RowY), Rows[I], 10.0f, I < 2 ? FLinearColor(0.80f, 1.0f, 0.86f, 0.94f) : FLinearColor(0.76f, 0.88f, 1.0f, 0.88f), I == 0);
        }
    }

    if (!Me)
    {
        DrawPanel(OutDrawElements, AllottedGeometry, PanelLayer + 8, FVector2D((Size.X - 330.0f) * 0.5f, Size.Y * 0.5f - 24.0f), FVector2D(330.0f, 48.0f), FLinearColor(0.014f, 0.024f, 0.038f, 0.82f), FLinearColor(0.50f, 0.84f, 1.0f, 0.28f));
        DrawText(OutDrawElements, AllottedGeometry, TextLayer + 6, FVector2D((Size.X - 280.0f) * 0.5f, Size.Y * 0.5f - 11.0f), TEXT("Connecting to Crimson Wars run..."), 16.0f, FLinearColor(0.78f, 0.92f, 1.0f, 0.96f), true);
    }

    if (Pawn && Pawn->IsNativeRunMenuOpen())
    {
        const int32 MenuLayer = TextLayer + 20;
        const FVector2D PanelSize(FMath::Clamp(Size.X * 0.42f, 500.0f, 680.0f), FMath::Clamp(Size.Y * 0.58f, 420.0f, 560.0f));
        const FVector2D PanelPos((Size.X - PanelSize.X) * 0.5f, (Size.Y - PanelSize.Y) * 0.5f);
        DrawBox(OutDrawElements, AllottedGeometry, MenuLayer, FVector2D::ZeroVector, Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
        DrawPanel(OutDrawElements, AllottedGeometry, MenuLayer + 4, PanelPos, PanelSize, FLinearColor(0.010f, 0.018f, 0.030f, 0.97f), FLinearColor(0.22f, 0.84f, 1.0f, 0.46f));
        DrawBox(OutDrawElements, AllottedGeometry, MenuLayer + 6, PanelPos + FVector2D(0.0f, 0.0f), FVector2D(PanelSize.X, 54.0f), FLinearColor(0.02f, 0.10f, 0.14f, 0.62f));
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, PanelPos + FVector2D(28.0f, 15.0f), TEXT("RUN MENU"), 19.0f, FLinearColor(0.88f, 0.98f, 1.0f, 0.98f), true);
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, PanelPos + FVector2D(178.0f, 21.0f), TEXT("Native client"), 11.0f, FLinearColor(0.48f, 0.92f, 1.0f, 0.76f), true);

        const FVector2D ClosePos = PanelPos + FVector2D(PanelSize.X - 48.0f, 18.0f);
        DrawPanel(OutDrawElements, AllottedGeometry, MenuLayer + 8, ClosePos, FVector2D(30.0f, 30.0f), FLinearColor(0.04f, 0.07f, 0.10f, 0.96f), FLinearColor(0.84f, 0.96f, 1.0f, 0.30f));
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 13, ClosePos + FVector2D(10.0f, 4.0f), TEXT("X"), 15.0f, FLinearColor(0.88f, 0.96f, 1.0f, 0.96f), true);

        const float HpPct = Me ? FMath::Clamp(Me->Hp / FMath::Max(1.0f, Me->MaxHp), 0.0f, 1.0f) : 0.0f;
        const FString PlayerName = Me && !Me->Name.IsEmpty() ? Me->Name.Left(18) : TEXT("Unknown");
        const FString WeaponName = Me && !Me->WeaponLabel.IsEmpty() ? Me->WeaponLabel.Left(22) : (Me ? Me->WeaponKey.Left(22) : TEXT("none"));
        const FVector2D StatsPos = PanelPos + FVector2D(28.0f, 76.0f);
        const FVector2D StatsSize(PanelSize.X - 56.0f, 132.0f);
        DrawPanel(OutDrawElements, AllottedGeometry, MenuLayer + 7, StatsPos, StatsSize, FLinearColor(0.006f, 0.014f, 0.024f, 0.92f), FLinearColor(0.28f, 0.78f, 1.0f, 0.26f));
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, StatsPos + FVector2D(18.0f, 14.0f), PlayerName, 18.0f, FLinearColor(0.92f, 0.98f, 1.0f, 0.98f), true);
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, StatsPos + FVector2D(18.0f, 44.0f), FString::Printf(TEXT("Room %s  |  Time %s  |  Kills %d"), *CachedState.RoomCode, *FormatClock(ElapsedSec), CachedState.TotalEnemyKills), 12.0f, FLinearColor(0.72f, 0.84f, 0.94f, 0.92f), true);
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, StatsPos + FVector2D(18.0f, 70.0f), FString::Printf(TEXT("Threat Lv%d x%.2f  |  Enemies %d  |  Pickups %d"), ThreatLevel, HpMul, CachedState.Enemies.Num(), CachedState.Drops.Num() + CachedState.XpOrbs.Num() + CachedState.SkillOrbs.Num()), 12.0f, FLinearColor(0.72f, 0.84f, 0.94f, 0.90f), true);
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, StatsPos + FVector2D(18.0f, 96.0f), FString::Printf(TEXT("Weapon %s  |  Ammo %d/%d  |  Skills %d"), *WeaponName, Me ? Me->MagazineAmmo : 0, Me ? Me->MagazineSize : 0, Me ? Me->Skills.Num() : 0), 12.0f, FLinearColor(0.72f, 0.84f, 0.94f, 0.90f), true);
        const FVector2D HpBarPos = StatsPos + FVector2D(StatsSize.X - 178.0f, 25.0f);
        DrawBox(OutDrawElements, AllottedGeometry, MenuLayer + 9, HpBarPos, FVector2D(140.0f, 12.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.70f));
        DrawBox(OutDrawElements, AllottedGeometry, MenuLayer + 10, HpBarPos + FVector2D(1.0f, 1.0f), FVector2D(138.0f * HpPct, 10.0f), FLinearColor(0.34f, 1.0f, 0.32f, 0.88f));

        const FVector2D ControlsPos = PanelPos + FVector2D(28.0f, 226.0f);
        DrawPanel(OutDrawElements, AllottedGeometry, MenuLayer + 7, ControlsPos, FVector2D(PanelSize.X - 56.0f, PanelSize.Y - 330.0f), FLinearColor(0.006f, 0.014f, 0.024f, 0.86f), FLinearColor(0.74f, 0.92f, 1.0f, 0.18f));
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, ControlsPos + FVector2D(18.0f, 14.0f), TEXT("Controls"), 15.0f, FLinearColor(0.84f, 0.94f, 1.0f, 0.96f), true);
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, ControlsPos + FVector2D(18.0f, 42.0f), TEXT("WASD move     Mouse aim/shoot     Space dodge     Enter chat"), 12.0f, FLinearColor(0.70f, 0.82f, 0.92f, 0.90f), false);
        DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 12, ControlsPos + FVector2D(18.0f, 68.0f), TEXT("Run is rendered only by the native UE client."), 12.0f, FLinearColor(0.44f, 0.90f, 1.0f, 0.82f), true);

        auto DrawMenuButton = [&](const FVector2D& Pos, const FVector2D& ButtonSize, const FString& Label, const FLinearColor& Accent, bool bDanger)
        {
            DrawPanel(OutDrawElements, AllottedGeometry, MenuLayer + 9, Pos, ButtonSize, bDanger ? FLinearColor(0.14f, 0.024f, 0.030f, 0.94f) : FLinearColor(0.018f, 0.044f, 0.064f, 0.94f), Accent);
            DrawBox(OutDrawElements, AllottedGeometry, MenuLayer + 11, Pos + FVector2D(1.0f, 1.0f), FVector2D(ButtonSize.X - 2.0f, 2.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.08f));
            DrawText(OutDrawElements, AllottedGeometry, MenuLayer + 13, Pos + FVector2D(18.0f, 11.0f), Label, 13.0f, bDanger ? FLinearColor(1.0f, 0.74f, 0.72f, 0.98f) : FLinearColor(0.88f, 0.98f, 1.0f, 0.98f), true);
        };

        const FVector2D ResumePos = PanelPos + FVector2D(28.0f, PanelSize.Y - 78.0f);
        const FVector2D LeavePos = PanelPos + FVector2D(PanelSize.X - 166.0f, PanelSize.Y - 78.0f);
        DrawMenuButton(ResumePos, FVector2D(136.0f, 42.0f), TEXT("Resume"), FLinearColor(0.22f, 0.94f, 1.0f, 0.48f), false);
        DrawMenuButton(LeavePos, FVector2D(138.0f, 42.0f), TEXT("Leave run"), FLinearColor(1.0f, 0.22f, 0.18f, 0.48f), true);
        return MenuLayer + 30;
    }

    return TextLayer + 8;
}

void UCWNativeRunHudWidget::UpdateFps(float InDeltaTime)
{
    FpsAccumulator += InDeltaTime;
    ++FpsFrames;
    if (FpsAccumulator < 0.25f)
    {
        return;
    }

    const int32 Fps = FMath::RoundToInt(static_cast<float>(FpsFrames) / FMath::Max(0.001f, FpsAccumulator));
    CachedFps = Fps;
    if (FpsText)
    {
        FpsText->SetText(FText::FromString(FString::Printf(TEXT("FPS %d"), Fps)));
    }
    FpsAccumulator = 0.0f;
    FpsFrames = 0;
}

UTextBlock* UCWNativeRunHudWidget::MakeHudText(const FString& Text, float Size, const FLinearColor& Color, bool bBold)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Block->SetText(FText::FromString(Text));
    Block->SetColorAndOpacity(FSlateColor(Color));
    Block->SetFont(CWHud::Font(Size, bBold));
    Block->SetAutoWrapText(true);
    return Block;
}

UBorder* UCWNativeRunHudWidget::MakeHudPanel(float InPadding)
{
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(FLinearColor(0.01f, 0.014f, 0.02f, 0.76f));
    Panel->SetPadding(FMargin(InPadding));
    return Panel;
}
