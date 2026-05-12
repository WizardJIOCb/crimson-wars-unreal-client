#include "CWNativeMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CWNativeAssetLibrary.h"
#include "CWNativeGameInstance.h"
#include "Styling/CoreStyle.h"

namespace CWMenu
{
    const FLinearColor Blood(0.78f, 0.02f, 0.04f, 1.0f);
    const FLinearColor Amber(1.0f, 0.55f, 0.12f, 1.0f);
    const FLinearColor Green(0.16f, 0.86f, 0.42f, 1.0f);
    const FLinearColor Cyan(0.18f, 0.76f, 0.96f, 1.0f);
    const FLinearColor Violet(0.55f, 0.42f, 1.0f, 1.0f);
    const FLinearColor Pink(1.0f, 0.18f, 0.38f, 1.0f);
    const FLinearColor MutedText(0.64f, 0.71f, 0.78f, 1.0f);
    const FLinearColor WhiteText(0.94f, 0.97f, 1.0f, 1.0f);
    const FLinearColor Panel(0.025f, 0.035f, 0.048f, 0.86f);
    const FLinearColor PanelHot(0.12f, 0.025f, 0.036f, 0.91f);

    FSlateFontInfo Font(float Size, bool bBold)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")), Size);
        Info.Size = static_cast<int32>(Size);
        return Info;
    }

    void Pad(UVerticalBoxSlot* Slot, float L = 0.0f, float T = 0.0f, float R = 0.0f, float B = 0.0f)
    {
        if (Slot)
        {
            Slot->SetPadding(FMargin(L, T, R, B));
        }
    }

    void Pad(UHorizontalBoxSlot* Slot, float L = 0.0f, float T = 0.0f, float R = 0.0f, float B = 0.0f)
    {
        if (Slot)
        {
            Slot->SetPadding(FMargin(L, T, R, B));
        }
    }
}

namespace
{
    struct FCWHeroUiData
    {
        FName Id;
        FString HeroCode;
        FString Name;
        FString UpperName;
        FString Role;
        FString Level;
        FString ProfileLevel;
        FString XpLine;
        FString Asset;
        FLinearColor Accent;
        FString Talents;
        FString Skills;
        FString Gear;
        FString Pow;
        FString Agi;
        FString Vit;
        FString Tec;
    };

    struct FCWSkillUiData
    {
        FString Title;
        FString Meta;
        FString Level;
        FString Icon;
        FLinearColor Accent;
    };

    struct FCWTalentUiData
    {
        FString Title;
        FString Meta;
        FString Level;
        FString Icon;
        FLinearColor Accent;
    };

    FCWHeroUiData GetHeroUiData(const FName HeroId)
    {
        if (HeroId == TEXT("scout"))
        {
            return { TEXT("scout"), TEXT("SCOUT"), TEXT("Скаут"), TEXT("СКАУТ"), TEXT("Быстрый разведчик и преследователь"), TEXT("LV 6"), TEXT("HERO LV 6"), TEXT("Уровень 6/999                                                251/510 XP"), TEXT("characters/scout.jpg"), CWMenu::Green, TEXT("2/4"), TEXT("2/8"), TEXT("0/7"), TEXT("6.2"), TEXT("10.3"), TEXT("5.2"), TEXT("7.2") };
        }
        if (HeroId == TEXT("shadow"))
        {
            return { TEXT("shadow"), TEXT("SHADOW"), TEXT("Тень"), TEXT("ТЕНЬ"), TEXT("Скрытный специалист по разрывам дистанции"), TEXT("LV 28"), TEXT("HERO LV 28"), TEXT("Уровень 28/999                                               4820/7200 XP"), TEXT("characters/shadow.jpg"), CWMenu::Violet, TEXT("3/4"), TEXT("3/8"), TEXT("1/7"), TEXT("8.5"), TEXT("8.8"), TEXT("7.1"), TEXT("11.4") };
        }
        if (HeroId == TEXT("medic"))
        {
            return { TEXT("medic"), TEXT("MEDIC"), TEXT("Медик"), TEXT("МЕДИК"), TEXT("Полевой лекарь и стабилизатор отряда"), TEXT("LV 4"), TEXT("HERO LV 4"), TEXT("Уровень 4/999                                                 9145/11300 XP"), TEXT("characters/medis.jpg"), FLinearColor(0.42f, 1.0f, 0.68f, 1.0f), TEXT("2/4"), TEXT("4/8"), TEXT("1/7"), TEXT("7.2"), TEXT("6.9"), TEXT("9.8"), TEXT("9.3") };
        }
        if (HeroId == TEXT("raider"))
        {
            return { TEXT("raider"), TEXT("RAIDER"), TEXT("Рейдер"), TEXT("РЕЙДЕР"), TEXT("Тяжёлый штурмовик ближнего давления"), TEXT("LV 4"), TEXT("HERO LV 4"), TEXT("Уровень 4/999                                                 1580/4100 XP"), TEXT("characters/raider.jpg"), CWMenu::Blood, TEXT("1/4"), TEXT("2/8"), TEXT("0/7"), TEXT("10.8"), TEXT("5.7"), TEXT("11.2"), TEXT("6.4") };
        }

        return { TEXT("cyber"), TEXT("CYBER"), TEXT("Кибер"), TEXT("КИБЕР"), TEXT("Универсальный адаптивный оператор"), TEXT("LV 64"), TEXT("HERO LV 64"), TEXT("Уровень 64/999                                                10949/11632 XP"), TEXT("characters/cyber.jpg"), CWMenu::Cyan, TEXT("4/4"), TEXT("4/8"), TEXT("3/7"), TEXT("9.8"), TEXT("7.9"), TEXT("10.5"), TEXT("12.8") };
    }
    TArray<FCWSkillUiData> GetHeroHeaderSkills(const FName HeroId)
    {
        if (HeroId == TEXT("scout"))
        {
            return {
                { TEXT("БРИТВЕННЫЙ ВЕТЕР"), TEXT("АКТИВНЫЙ · ЗАКРЫТ 60 ОСКОЛКОВ"), TEXT("0/10"), TEXT("hero-skills/scout_razor_wind.png"), CWMenu::Cyan },
                { TEXT("МЕТКА ОХОТНИКА"), TEXT("АКТИВНЫЙ · ЗАКРЫТ 92 ОСКОЛКА"), TEXT("0/9"), TEXT("hero-skills/scout_hunter_mark.png"), CWMenu::Cyan },
                { TEXT("ШТОРМОВАЯ СЕТЬ"), TEXT("АКТИВНЫЙ · LV 1/8"), TEXT("1/8"), TEXT("hero-skills/scout_storm_net.png"), CWMenu::Violet },
                { TEXT("НЕБЕСНЫЕ ПРЕСЛЕДОВАТЕЛИ"), TEXT("АКТИВНЫЙ · LV 1/7"), TEXT("1/7"), TEXT("hero-skills/scout_sky_chasers.png"), CWMenu::Amber },
                { TEXT("ДЛИННЫЙ ШАГ"), TEXT("ПАССИВНЫЙ · ЗАКРЫТ 48 ОСКОЛКОВ"), TEXT("0/10"), TEXT("hero-skills/scout_long_stride.png"), CWMenu::Cyan },
                { TEXT("ОСТРОЕ ЗРЕНИЕ"), TEXT("ПАССИВНЫЙ · ЗАКРЫТ 76 ОСКОЛКОВ"), TEXT("0/10"), TEXT("hero-skills/scout_vital_sight.png"), CWMenu::Green },
                { TEXT("ПРОКЛАДЫВАТЕЛЬ ПУТИ"), TEXT("АУРА · ЗАКРЫТ 112 ОСКОЛКОВ"), TEXT("0/10"), TEXT("hero-skills/scout_trailblazer.png"), CWMenu::Violet },
                { TEXT("ЭНЕРГЕТИК В КАПЕЛЬНИЦУ"), TEXT("ПАССИВНЫЙ · ЗАКРЫТ 82 ОСКОЛКА"), TEXT("0/10"), TEXT("hero-skills/scout_energy_drink_iv.png"), CWMenu::Cyan }
            };
        }

        if (HeroId == TEXT("shadow"))
        {
            return {
                { TEXT("ЧЁРНЫЕ КОМЕТЫ"), TEXT("АКТИВ · УРОН: 54"), TEXT("1/8"), TEXT("hero-skills/shadow_black_comets.png"), CWMenu::Violet },
                { TEXT("КЛЫКИ НОЧИ"), TEXT("АКТИВ · УРОН: 46"), TEXT("1/8"), TEXT("hero-skills/shadow_night_fangs.png"), CWMenu::Violet },
                { TEXT("РАЗРЫВ ПУСТОТЫ"), TEXT("АКТИВ · РАДИУС: 320"), TEXT("1/8"), TEXT("hero-skills/shadow_void_burst.png"), CWMenu::Cyan },
                { TEXT("ЦЕПЬ ЗАТМЕНИЯ"), TEXT("АКТИВ · ЦЕЛИ: 4"), TEXT("0/8"), TEXT("hero-skills/shadow_eclipse_chain.png"), CWMenu::Pink },
                { TEXT("ШАГ ПРИЗРАКА"), TEXT("ПАССИВ · СКОРОСТЬ"), TEXT("0/10"), TEXT("hero-skills/shadow_ghost_step.png"), CWMenu::Cyan },
                { TEXT("ИНСТИНКТ УБИЙЦЫ"), TEXT("ПАССИВ · УРОН"), TEXT("0/10"), TEXT("hero-skills/shadow_assassin_instinct.png"), CWMenu::Pink },
                { TEXT("ДЕЙСТВИЕ: ПЕРЕЗАРЯДКА"), TEXT("ПАССИВ · ТЕМП"), TEXT("0/10"), TEXT("hero-skills/shadow_action_reload.png"), CWMenu::Amber },
                { TEXT("ДОКТРИНА ТЕНИ"), TEXT("АУРА · ОТРЯД"), TEXT("0/10"), TEXT("hero-skills/shadow_umbral_doctrine.png"), CWMenu::Violet }
            };
        }

        if (HeroId == TEXT("medic"))
        {
            return {
                { TEXT("СТЕРИЛЬНЫЙ ВЗРЫВ"), TEXT("АКТИВ · УРОН: 37"), TEXT("1/10"), TEXT("hero-skills/medic_sterile_wave.png"), CWMenu::Cyan },
                { TEXT("ЛУЧ ТРИАЖА"), TEXT("АКТИВ · ЛЕЧЕНИЕ"), TEXT("1/8"), TEXT("hero-skills/medic_triage_beam.png"), CWMenu::Cyan },
                { TEXT("ТОКСИЧНЫЙ КОНТУР"), TEXT("АКТИВ · УРОН: 58"), TEXT("1/8"), TEXT("hero-skills/medic_toxin_arc.png"), CWMenu::Green },
                { TEXT("РАКЕТЫ СКОРОЙ"), TEXT("АКТИВ · РАДИУС"), TEXT("1/7"), TEXT("hero-skills/medic_rescue_rockets.png"), CWMenu::Amber },
                { TEXT("ПОЛЕВАЯ АПТЕЧКА"), TEXT("ПАССИВ · РЕГЕН"), TEXT("0/10"), TEXT("hero-skills/medic_field_aid.png"), CWMenu::Green },
                { TEXT("ЖИВАЯ БРОНЯ"), TEXT("ПАССИВ · HP"), TEXT("0/10"), TEXT("hero-skills/medic_vital_plating.png"), CWMenu::Cyan },
                { TEXT("ПРОТОКОЛ СПАСЕНИЯ"), TEXT("АУРА · ОТРЯД"), TEXT("0/10"), TEXT("hero-skills/medic_support_protocol.png"), CWMenu::Violet },
                { TEXT("УПС, ОДНИ БИНТЫ"), TEXT("ПАССИВ · РЕГЕН"), TEXT("0/10"), TEXT("hero-skills/medic_oops_all_bandages.png"), CWMenu::Green }
            };
        }

        if (HeroId == TEXT("raider"))
        {
            return {
                { TEXT("ШРАПНЕЛЬНЫЙ ВЗРЫВ"), TEXT("АКТИВ · УРОН"), TEXT("1/8"), TEXT("hero-skills/raider_shrapnel_burst.png"), CWMenu::Amber },
                { TEXT("ОСАДНЫЙ ЗАЛП"), TEXT("АКТИВ · РАДИУС"), TEXT("1/7"), TEXT("hero-skills/raider_siege_barrage.png"), CWMenu::Blood },
                { TEXT("ДУГА БЕРСЕРКА"), TEXT("АКТИВ · ЦЕПЬ"), TEXT("0/8"), TEXT("hero-skills/raider_berserk_arc.png"), CWMenu::Violet },
                { TEXT("ТОПОТ ВОЙНЫ"), TEXT("АКТИВ · СТАН"), TEXT("0/8"), TEXT("hero-skills/raider_war_stomp.png"), CWMenu::Amber },
                { TEXT("БОЕВАЯ ЯРОСТЬ"), TEXT("ПАССИВ · УРОН"), TEXT("0/10"), TEXT("hero-skills/raider_battle_rage.png"), CWMenu::Blood },
                { TEXT("ЖЕЛЕЗНАЯ КОЖА"), TEXT("ПАССИВ · HP"), TEXT("0/10"), TEXT("hero-skills/raider_iron_hide.png"), CWMenu::Green },
                { TEXT("ЗНАМЯ ВОЙНЫ"), TEXT("АУРА · ОТРЯД"), TEXT("0/10"), TEXT("hero-skills/raider_war_banner.png"), CWMenu::Amber },
                { TEXT("КОНТРОЛЬ ГНЕВА"), TEXT("ПАССИВ · ТЕМП"), TEXT("0/10"), TEXT("hero-skills/raider_anger_management.png"), CWMenu::Pink }
            };
        }

        return {
            { TEXT("ИМПУЛЬСНАЯ ВОЛНА"), TEXT("АКТИВ · УРОН: 100"), TEXT("5/10"), TEXT("hero-skills/cyber_pulse_wave.png"), CWMenu::Cyan },
            { TEXT("ИОННОЕ КОПЬЕ"), TEXT("АКТИВ · УРОН: 42"), TEXT("1/8"), TEXT("hero-skills/cyber_ion_lance.png"), CWMenu::Violet },
            { TEXT("ДУГОВАЯ МАТРИЦА"), TEXT("АКТИВ · УРОН: 48"), TEXT("1/10"), TEXT("hero-skills/cyber_arc_matrix.png"), CWMenu::Violet },
            { TEXT("ПРОТОКОЛ НАВЕДЕНИЯ"), TEXT("АКТИВ · УРОН: 47"), TEXT("2/7"), TEXT("hero-skills/cyber_seeker_protocol.png"), CWMenu::Amber },
            { TEXT("АДАПТИВНАЯ РАМА"), TEXT("ПАССИВ · МАКС. HP"), TEXT("0/10"), TEXT("hero-skills/cyber_adaptive_frame.png"), CWMenu::Cyan },
            { TEXT("БОЕВАЯ ПРОШИВКА"), TEXT("ПАССИВ · РИТМ ОГНЯ"), TEXT("0/10"), TEXT("hero-skills/cyber_combat_firmware.png"), CWMenu::Pink },
            { TEXT("СИНХРО-СВЯЗЬ"), TEXT("АУРА · ПОДДЕРЖКА"), TEXT("0/10"), TEXT("hero-skills/cyber_sync_link.png"), CWMenu::Violet },
            { TEXT("МАГНИТНЫЙ ПОДБОР"), TEXT("ПАССИВ · ПОДБОР"), TEXT("0/10"), TEXT("hero-talents/cyber_magnet.png"), CWMenu::Cyan }
        };
    }

    TArray<FCWTalentUiData> GetHeroTalents(const FName HeroId)
    {
        if (HeroId == TEXT("scout"))
        {
            return {
                { TEXT("Длинный шаг"), TEXT("+скорость движения сейчас +8%"), TEXT("2/5"), TEXT("hero-talents/scout_stride.png"), CWMenu::Cyan },
                { TEXT("Быстрые руки"), TEXT("+скорострельность сейчас +5%"), TEXT("2/5"), TEXT("hero-talents/scout_reload.png"), CWMenu::Amber },
                { TEXT("Уклончивый перекат"), TEXT("+заряд рывка сейчас +0"), TEXT("0/2"), TEXT("hero-talents/scout_dodge.png"), CWMenu::Violet },
                { TEXT("Уверенная очередь"), TEXT("+урон сейчас +0%"), TEXT("0/4"), TEXT("hero-talents/scout_shots.png"), CWMenu::Pink }
            };
        }

        return {
            { TEXT("Оверклок"), TEXT("+скорострельность сейчас +15%"), TEXT("5/5"), TEXT("hero-talents/cyber_overclock.png"), CWMenu::Amber },
            { TEXT("Нано-ядро"), TEXT("+урон сейчас +15%"), TEXT("5/5"), TEXT("hero-talents/cyber_nano_core.png"), CWMenu::Pink },
            { TEXT("Барьерная матрица"), TEXT("+макс. HP сейчас +40"), TEXT("5/5"), TEXT("hero-talents/cyber_barrier.png"), CWMenu::Green },
            { TEXT("Маг-сборщик"), TEXT("+радиус подбора сейчас +30"), TEXT("5/5"), TEXT("hero-talents/cyber_magnet.png"), CWMenu::Cyan }
        };
    }
}

TSharedRef<SWidget> UCWNativeMenuWidget::RebuildWidget()
{
    FxTime = 0.0f;
    MenuRevealTime = 0.0f;
    PanelRevealTime = 0.0f;
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    BuildMenu();
    return Super::RebuildWidget();
}

void UCWNativeMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        BootstrapDelegateHandle = GI->OnBootstrapReceived.AddUObject(this, &UCWNativeMenuWidget::HandleBootstrapReceived);
    }
    if (const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        if (GI->bBootstrapLoaded)
        {
            RebuildContent();
        }
    }
}

void UCWNativeMenuWidget::NativeDestruct()
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        if (BootstrapDelegateHandle.IsValid())
        {
            GI->OnBootstrapReceived.Remove(BootstrapDelegateHandle);
            BootstrapDelegateHandle.Reset();
        }
    }
    TextureCache.Empty();
    Super::NativeDestruct();
}

void UCWNativeMenuWidget::RegisterButtonFx(UButton* Button, const FLinearColor& Accent, bool bPrimary)
{
    if (!Button)
    {
        return;
    }

    FCWNativeButtonFx Item;
    Item.Button = Button;
    Item.Accent = Accent;
    Item.bPrimary = bPrimary;
    ButtonFxItems.Add(Item);
}

void UCWNativeMenuWidget::RegisterPanelFx(UBorder* Panel, const FLinearColor& Accent, bool bSelected)
{
    if (!Panel)
    {
        return;
    }

    FCWNativePanelFx Item;
    Item.Panel = Panel;
    Item.Accent = Accent;
    Item.bSelected = bSelected;
    Item.Phase = FxTime + static_cast<float>(PanelFxItems.Num()) * 0.37f;
    PanelFxItems.Add(Item);
}

FLinearColor UCWNativeMenuWidget::GetTabAccent(const FName TabId) const
{
    if (TabId == TEXT("run")) return CWMenu::Green;
    if (TabId == TEXT("story")) return CWMenu::Amber;
    if (TabId == TEXT("characters")) return CWMenu::Cyan;
    if (TabId == TEXT("profile")) return FLinearColor(0.45f, 0.55f, 1.0f, 1.0f);
    if (TabId == TEXT("rating")) return FLinearColor(0.65f, 0.28f, 1.0f, 1.0f);
    if (TabId == TEXT("news")) return CWMenu::Pink;
    return FLinearColor(0.65f, 0.72f, 0.82f, 1.0f);
}

void UCWNativeMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    FxTime += InDeltaTime;
    const float Pulse = 0.5f + 0.5f * FMath::Sin(FxTime * 2.15f);
    MenuRevealTime = FMath::Min(MenuRevealTime + InDeltaTime * 3.2f, 1.0f);
    PanelRevealTime = FMath::Min(PanelRevealTime + InDeltaTime * 4.8f, 1.0f);
    const float MenuReveal = FMath::InterpEaseOut(0.0f, 1.0f, MenuRevealTime, 3.0f);
    const float PanelReveal = FMath::InterpEaseOut(0.0f, 1.0f, PanelRevealTime, 3.0f);

    if (BackgroundImage)
    {
        BackgroundImage->SetRenderScale(FVector2D(1.02f + Pulse * 0.012f, 1.02f + Pulse * 0.012f));
        BackgroundImage->SetRenderTranslation(FVector2D(FMath::Sin(FxTime * 0.13f) * 18.0f, FMath::Cos(FxTime * 0.11f) * 10.0f));
    }

    if (BloodOverlay)
    {
        BloodOverlay->SetBrushColor(FLinearColor(0.18f + Pulse * 0.06f, 0.0f, 0.012f, 0.56f));
    }

    if (Scanline)
    {
        const float Y = FMath::Fmod(FxTime * 96.0f, 1080.0f) - 220.0f;
        Scanline->SetRenderTranslation(FVector2D(0.0f, Y));
        Scanline->SetBrushColor(FLinearColor(0.9f, 0.05f, 0.05f, 0.035f + Pulse * 0.025f));
    }

    if (FramePanel)
    {
        FramePanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.30f + Pulse * 0.08f));
    }

    if (FpsText && InDeltaTime > KINDA_SMALL_NUMBER)
    {
        FpsUpdateTime += InDeltaTime;
        FpsFrameSum += 1.0f / InDeltaTime;
        ++FpsFrameCount;

        if (FpsUpdateTime >= 0.25f && FpsFrameCount > 0)
        {
            const int32 AverageFps = FMath::RoundToInt(FpsFrameSum / static_cast<float>(FpsFrameCount));
            FpsText->SetText(FText::FromString(FString::Printf(TEXT("FPS %d"), AverageFps)));
            FpsText->SetColorAndOpacity(FSlateColor(AverageFps >= 55 ? CWMenu::Green : (AverageFps >= 30 ? CWMenu::Amber : CWMenu::Blood)));
            FpsUpdateTime = 0.0f;
            FpsFrameSum = 0.0f;
            FpsFrameCount = 0;
        }
    }

    if (FpsPanel)
    {
        FpsPanel->SetBrushColor(FLinearColor(0.01f, 0.014f, 0.019f, 0.82f + Pulse * 0.06f));
    }

    if (MainStack)
    {
        MainStack->SetRenderOpacity(MenuReveal);
        MainStack->SetRenderTranslation(FVector2D(0.0f, (1.0f - MenuReveal) * 28.0f));
        MainStack->SetRenderScale(FVector2D(0.985f + MenuReveal * 0.015f, 0.985f + MenuReveal * 0.015f));
    }

    if (ContentHost)
    {
        ContentHost->SetRenderOpacity(PanelReveal);
        ContentHost->SetRenderTranslation(FVector2D((1.0f - PanelReveal) * 26.0f, 0.0f));
    }

    if (ContentSweep)
    {
        const float SweepY = FMath::Fmod(FxTime * 170.0f, 620.0f);
        ContentSweep->SetRenderTranslation(FVector2D(0.0f, SweepY));
        ContentSweep->SetBrushColor(FLinearColor(1.0f, 0.08f, 0.05f, 0.045f + Pulse * 0.035f));
    }

    if (LogoImage)
    {
        const float LogoPulse = 0.5f + 0.5f * FMath::Sin(FxTime * 3.1f);
        LogoImage->SetRenderScale(FVector2D(0.98f + LogoPulse * 0.035f, 0.98f + LogoPulse * 0.035f));
        LogoImage->SetRenderTransformAngle(FMath::Sin(FxTime * 1.05f) * 1.2f);
    }

    for (int32 Index = 0; Index < EmberDots.Num(); ++Index)
    {
        if (!EmberDots[Index])
        {
            continue;
        }

        const float Phase = FxTime * (0.08f + Index * 0.003f) + static_cast<float>(Index) * 0.137f;
        const float Drift = FMath::Frac(Phase);
        const float X = 42.0f + FMath::Fmod(static_cast<float>(Index * 83), 1760.0f) + FMath::Sin(FxTime * 0.6f + Index) * 18.0f;
        const float Y = 1010.0f - Drift * 940.0f;
        const float Alpha = 0.08f + (1.0f - Drift) * 0.14f;
        EmberDots[Index]->SetRenderTranslation(FVector2D(X, Y));
        EmberDots[Index]->SetRenderScale(FVector2D(0.6f + Pulse * 0.35f, 0.6f + Pulse * 0.35f));
        EmberDots[Index]->SetBrushColor(FLinearColor(1.0f, 0.18f + Pulse * 0.16f, 0.04f, Alpha));
    }

    for (int32 Index = ButtonFxItems.Num() - 1; Index >= 0; --Index)
    {
        UButton* Button = ButtonFxItems[Index].Button.Get();
        if (!Button)
        {
            ButtonFxItems.RemoveAtSwap(Index);
            continue;
        }

        const FLinearColor Accent = ButtonFxItems[Index].Accent;
        const bool bHovered = Button->IsHovered();
        const bool bPressed = Button->IsPressed();
        const float Glow = bHovered ? 1.0f : 0.0f;
        const float Press = bPressed ? 1.0f : 0.0f;
        Button->SetRenderScale(FVector2D(1.0f + Glow * 0.018f - Press * 0.012f, 1.0f + Glow * 0.018f - Press * 0.012f));
        Button->SetRenderTranslation(FVector2D(0.0f, bHovered ? -1.0f : 0.0f));
        Button->SetRenderOpacity(0.92f + Glow * 0.08f);
        Button->SetBackgroundColor(ButtonFxItems[Index].bPrimary
            ? FLinearColor(Accent.R * (0.62f + Glow * 0.18f), Accent.G * (0.62f + Glow * 0.18f), Accent.B * (0.62f + Glow * 0.18f), 0.96f)
            : FLinearColor(0.026f + Accent.R * (0.04f + Glow * 0.10f), 0.032f + Accent.G * (0.035f + Glow * 0.08f), 0.043f + Accent.B * (0.045f + Glow * 0.10f), 0.88f + Glow * 0.08f));
    }

    for (int32 Index = PanelFxItems.Num() - 1; Index >= 0; --Index)
    {
        UBorder* Panel = PanelFxItems[Index].Panel.Get();
        if (!Panel)
        {
            PanelFxItems.RemoveAtSwap(Index);
            continue;
        }

        const FLinearColor Accent = PanelFxItems[Index].Accent;
        const bool bHovered = Panel->IsHovered();
        const bool bSelected = PanelFxItems[Index].bSelected;
        const float LocalPulse = 0.5f + 0.5f * FMath::Sin(FxTime * (bSelected ? 4.2f : 2.4f) + PanelFxItems[Index].Phase);
        const float Glow = (bSelected ? 0.62f + LocalPulse * 0.38f : 0.0f) + (bHovered ? 0.55f : 0.0f);
        Panel->SetRenderScale(FVector2D(1.0f + Glow * 0.018f, 1.0f + Glow * 0.018f));
        Panel->SetRenderTranslation(FVector2D(0.0f, bHovered ? -2.0f : 0.0f));
        Panel->SetRenderOpacity(0.94f + FMath::Min(Glow * 0.06f, 0.06f));
    }

    for (const TPair<FName, UButton*>& Pair : TabButtons)
    {
        if (!Pair.Value)
        {
            continue;
        }

        const bool bActive = Pair.Key == ActiveTab;
        const bool bHovered = Pair.Value->IsHovered();
        const float TabPulse = 0.5f + 0.5f * FMath::Sin(FxTime * 4.0f);
        const FLinearColor Accent = GetTabAccent(Pair.Key);
        const float TabGlow = (bActive ? 0.55f + TabPulse * 0.45f : 0.0f) + (bHovered ? 0.42f : 0.0f);
        Pair.Value->SetRenderScale(FVector2D(1.0f + TabGlow * 0.014f, 1.0f + TabGlow * 0.014f));
        Pair.Value->SetRenderTranslation(FVector2D(0.0f, bHovered ? -1.5f : 0.0f));
        Pair.Value->SetBackgroundColor(bActive
            ? FLinearColor(0.03f + Accent.R * (0.16f + TabGlow * 0.055f), 0.035f + Accent.G * (0.12f + TabGlow * 0.04f), 0.045f + Accent.B * (0.16f + TabGlow * 0.055f), 0.96f)
            : FLinearColor(0.024f + Accent.R * (0.03f + TabGlow * 0.08f), 0.03f + Accent.G * (0.024f + TabGlow * 0.06f), 0.04f + Accent.B * (0.032f + TabGlow * 0.08f), 0.82f + TabGlow * 0.10f));
    }

    RefreshStatusText();
}

void UCWNativeMenuWidget::ShowMenu()
{
    MenuRevealTime = 0.0f;
    PanelRevealTime = 0.0f;
    SetVisibility(ESlateVisibility::Visible);
}

void UCWNativeMenuWidget::HideMenu()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UCWNativeMenuWidget::ToggleMenu()
{
    if (IsMenuShown())
    {
        HideMenu();
    }
    else
    {
        ShowMenu();
    }
}

bool UCWNativeMenuWidget::IsMenuShown() const
{
    return GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden;
}

void UCWNativeMenuWidget::SelectRunTab() { SelectTab(TEXT("run")); }
void UCWNativeMenuWidget::SelectStoryTab() { SelectTab(TEXT("story")); }
void UCWNativeMenuWidget::SelectCharactersTab() { SelectTab(TEXT("characters")); }
void UCWNativeMenuWidget::SelectSkillsTab() { SelectTab(TEXT("skills")); }
void UCWNativeMenuWidget::SelectProfileTab() { SelectTab(TEXT("profile")); }
void UCWNativeMenuWidget::SelectRatingTab() { SelectTab(TEXT("rating")); }
void UCWNativeMenuWidget::SelectNewsTab() { SelectTab(TEXT("news")); }
void UCWNativeMenuWidget::SelectSettingsTab() { SelectTab(TEXT("settings")); }

void UCWNativeMenuWidget::SelectCyberHero() { SelectHero(TEXT("cyber")); }
void UCWNativeMenuWidget::SelectScoutHero() { SelectHero(TEXT("scout")); }
void UCWNativeMenuWidget::SelectShadowHero() { SelectHero(TEXT("shadow")); }
void UCWNativeMenuWidget::SelectMedicHero() { SelectHero(TEXT("medic")); }
void UCWNativeMenuWidget::SelectRaiderHero() { SelectHero(TEXT("raider")); }

void UCWNativeMenuWidget::SelectHero(const FName HeroId)
{
    SelectedHeroId = HeroId;
    ActiveTab = TEXT("characters");
    BuildMenu();
}

void UCWNativeMenuWidget::BindHeroButton(UButton* Button, const FString& HeroId)
{
    if (!Button)
    {
        return;
    }

    if (HeroId == TEXT("cyber")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectCyberHero);
    else if (HeroId == TEXT("scout")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectScoutHero);
    else if (HeroId == TEXT("shadow")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectShadowHero);
    else if (HeroId == TEXT("medic")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectMedicHero);
    else if (HeroId == TEXT("raider")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectRaiderHero);
}

void UCWNativeMenuWidget::BindSkillButton(UButton* Button, const FString& AssetPath)
{
    if (!Button)
    {
        return;
    }

    if (AssetPath.Contains(TEXT("cyber_pulse_wave"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenPulseSkillDialog);
    else if (AssetPath.Contains(TEXT("cyber_ion_lance"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenIonSkillDialog);
    else if (AssetPath.Contains(TEXT("cyber_arc_matrix"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenArcSkillDialog);
    else if (AssetPath.Contains(TEXT("cyber_seeker_protocol"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenProtocolSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_razor_wind"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutRazorSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_hunter_mark"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutHunterSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_storm_net"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutStormSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_sky_chasers"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutSkySkillDialog);
    else if (AssetPath.Contains(TEXT("scout_long_stride"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutLongStrideSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_vital_sight"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutSightSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_trailblazer"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutTrailSkillDialog);
    else if (AssetPath.Contains(TEXT("scout_energy_drink_iv"))) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenScoutEnergySkillDialog);
    else Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenPassiveSkillDialog);
}

void UCWNativeMenuWidget::BindSlotButton(UButton* Button, const FName SlotKey)
{
    if (!Button)
    {
        return;
    }

    if (SlotKey == TEXT("head")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenHeadSlotDialog);
    else if (SlotKey == TEXT("left_hand")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenLeftHandSlotDialog);
    else if (SlotKey == TEXT("armor")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenArmorSlotDialog);
    else if (SlotKey == TEXT("right_hand")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenRightHandSlotDialog);
    else if (SlotKey == TEXT("ring_1")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenRingOneSlotDialog);
    else if (SlotKey == TEXT("ring_2")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenRingTwoSlotDialog);
}

void UCWNativeMenuWidget::HandleCloseDialogClicked()
{
    if (DialogLayer)
    {
        DialogLayer->SetVisibility(ESlateVisibility::Collapsed);
        DialogLayer->SetContent(nullptr);
    }
}

void UCWNativeMenuWidget::HandleDialogActionClicked()
{
    HandleCloseDialogClicked();
}

void UCWNativeMenuWidget::OpenHeadSlotDialog()
{
    const bool bCyber = SelectedHeroId == TEXT("cyber");
    ShowItemDialog(TEXT("Голова"), bCyber ? FString(TEXT("Корона войны")) : FString(TEXT("Пусто")), bCyber ? FString(TEXT("items/war_crown.png")) : FString(), CWMenu::Amber);
}

void UCWNativeMenuWidget::OpenLeftHandSlotDialog()
{
    const bool bCyber = SelectedHeroId == TEXT("cyber");
    ShowItemDialog(TEXT("Левая рука"), bCyber ? FString(TEXT("Фазовый клинок")) : FString(TEXT("Пусто")), bCyber ? FString(TEXT("items/phase_blade.png")) : FString(), CWMenu::Pink);
}

void UCWNativeMenuWidget::OpenArmorSlotDialog()
{
    ShowItemDialog(TEXT("Броня"), TEXT("Пусто"), FString(), CWMenu::Cyan);
}

void UCWNativeMenuWidget::OpenRightHandSlotDialog()
{
    ShowItemDialog(TEXT("Правая рука"), TEXT("Пусто"), FString(), CWMenu::Pink);
}

void UCWNativeMenuWidget::OpenRingOneSlotDialog()
{
    ShowItemDialog(TEXT("Кольцо 1"), TEXT("Пусто"), FString(), CWMenu::Amber);
}

void UCWNativeMenuWidget::OpenRingTwoSlotDialog()
{
    ShowItemDialog(TEXT("Кольцо 2"), TEXT("Пусто"), FString(), CWMenu::Amber);
}

void UCWNativeMenuWidget::OpenPulseSkillDialog()
{
    ShowSkillDialog(TEXT("Импульсная волна"), TEXT("hero-skills/cyber_pulse_wave.png"), TEXT("5/10"), TEXT("ЭМП-выброс вокруг героя: урон 100, радиус 234, перезарядка 4.26s"), CWMenu::Cyan);
}

void UCWNativeMenuWidget::OpenIonSkillDialog()
{
    ShowSkillDialog(TEXT("Ионное копьё"), TEXT("hero-skills/cyber_ion_lance.png"), TEXT("1/8"), TEXT("Ионные лучи по ближним целям: урон 42, радиус 340, цели 2"), CWMenu::Violet);
}

void UCWNativeMenuWidget::OpenArcSkillDialog()
{
    ShowSkillDialog(TEXT("Дуговая матрица"), TEXT("hero-skills/cyber_arc_matrix.png"), TEXT("1/10"), TEXT("Цепные электроразряды: урон 48, радиус 360, цели 3"), CWMenu::Violet);
}

void UCWNativeMenuWidget::OpenProtocolSkillDialog()
{
    ShowSkillDialog(TEXT("Протокол наведения"), TEXT("hero-skills/cyber_seeker_protocol.png"), TEXT("2/7"), TEXT("Автономные ракеты: урон 47, радиус 1570, цели 5"), CWMenu::Amber);
}

void UCWNativeMenuWidget::OpenPassiveSkillDialog()
{
    ShowSkillDialog(TEXT("Пассивный модуль"), TEXT("hero-skills/cyber_combat_firmware.png"), TEXT("0/10"), TEXT("Пассивное усиление героя. Следующий уровень усилит боевые параметры."), CWMenu::Cyan);
}

void UCWNativeMenuWidget::OpenScoutRazorSkillDialog()
{
    ShowSkillDialog(TEXT("Бритвенный ветер"), TEXT("hero-skills/scout_razor_wind.png"), TEXT("0/10"), TEXT("Быстро разрезает ближайших врагов: урон 25, радиус 340, цель 3."), CWMenu::Cyan);
}

void UCWNativeMenuWidget::OpenScoutHunterSkillDialog()
{
    ShowSkillDialog(TEXT("Метка охотника"), TEXT("hero-skills/scout_hunter_mark.png"), TEXT("0/9"), TEXT("Мгновенно пронзает врагов: урон 36, радиус 390, цель 2."), CWMenu::Cyan);
}

void UCWNativeMenuWidget::OpenScoutStormSkillDialog()
{
    ShowSkillDialog(TEXT("Штормовая сеть"), TEXT("hero-skills/scout_storm_net.png"), TEXT("1/8"), TEXT("Электрическая цепь: урон 43, радиус 380, цели 4."), CWMenu::Violet);
}

void UCWNativeMenuWidget::OpenScoutSkySkillDialog()
{
    ShowSkillDialog(TEXT("Небесные преследователи"), TEXT("hero-skills/scout_sky_chasers.png"), TEXT("1/7"), TEXT("Быстрые ракеты для дальних целей: урон 32, радиус 1600, цели 5."), CWMenu::Amber);
}

void UCWNativeMenuWidget::OpenScoutLongStrideSkillDialog()
{
    ShowSkillDialog(TEXT("Длинный шаг"), TEXT("hero-skills/scout_long_stride.png"), TEXT("0/10"), TEXT("Пассивная скорость движения: следующий уровень усилит мобильность."), CWMenu::Cyan);
}

void UCWNativeMenuWidget::OpenScoutSightSkillDialog()
{
    ShowSkillDialog(TEXT("Острое зрение"), TEXT("hero-skills/scout_vital_sight.png"), TEXT("0/10"), TEXT("Пассивная точность и темп огня: следующий уровень усилит стрельбу."), CWMenu::Green);
}

void UCWNativeMenuWidget::OpenScoutTrailSkillDialog()
{
    ShowSkillDialog(TEXT("Прокладыватель пути"), TEXT("hero-skills/scout_trailblazer.png"), TEXT("0/10"), TEXT("Аура поддержки: скорость других героев и подбор ресурсов."), CWMenu::Violet);
}

void UCWNativeMenuWidget::OpenScoutEnergySkillDialog()
{
    ShowSkillDialog(TEXT("Энергетик в капельницу"), TEXT("hero-skills/scout_energy_drink_iv.png"), TEXT("0/10"), TEXT("Пассивная перезарядка и скорость движения."), CWMenu::Cyan);
}

void UCWNativeMenuWidget::HandleCreateRunClicked()
{
    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI)
    {
        return;
    }

    GI->DefaultRoomCode.Reset();
    if (NameInput)
    {
        GI->DefaultPlayerName = NameInput->GetText().ToString();
    }

    if (!GI->IsSocketConnected())
    {
        GI->bAutoJoinOnConnect = true;
        GI->ConnectToServer(GI->DefaultServerUrl);
    }
    else
    {
        GI->JoinRoom(TEXT(""), GI->DefaultPlayerName, GI->DefaultPlayerClass);
    }
    HideMenu();
}

void UCWNativeMenuWidget::HandleJoinRunClicked()
{
    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI)
    {
        return;
    }

    GI->DefaultRoomCode = RoomCodeInput ? RoomCodeInput->GetText().ToString().TrimStartAndEnd().ToUpper() : GI->DefaultRoomCode;
    if (NameInput)
    {
        GI->DefaultPlayerName = NameInput->GetText().ToString();
    }

    if (!GI->IsSocketConnected())
    {
        GI->bAutoJoinOnConnect = true;
        GI->ConnectToServer(GI->DefaultServerUrl);
    }
    else
    {
        GI->JoinRoom(GI->DefaultRoomCode, GI->DefaultPlayerName, GI->DefaultPlayerClass);
    }
    HideMenu();
}

void UCWNativeMenuWidget::HandleReconnectClicked()
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        GI->bAutoJoinOnConnect = false;
        GI->ConnectToServer(GI->DefaultServerUrl);
    }
}

void UCWNativeMenuWidget::HandleCloseClicked()
{
    HideMenu();
}

void UCWNativeMenuWidget::BuildMenu()
{
    if (!WidgetTree)
    {
        return;
    }

    ActiveTab = TEXT("characters");
    TabButtons.Reset();
    ButtonFxItems.Reset();
    PanelFxItems.Reset();

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuRoot"));
    WidgetTree->RootWidget = RootCanvas;

    BackgroundImage = MakeImage(TEXT("other/landing-image.jpg"), FVector2D(1920.0f, 1080.0f));
    BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    RootCanvas->AddChild(BackgroundImage);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackgroundImage->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        CanvasSlot->SetOffsets(FMargin(0.0f));
    }

    BloodOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BloodOverlay"));
    BloodOverlay->SetBrushColor(FLinearColor(0.12f, 0.0f, 0.012f, 0.58f));
    BloodOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    RootCanvas->AddChild(BloodOverlay);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BloodOverlay->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        CanvasSlot->SetOffsets(FMargin(0.0f));
    }

    Scanline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scanline"));
    Scanline->SetBrushColor(FLinearColor(1.0f, 0.0f, 0.0f, 0.04f));
    Scanline->SetVisibility(ESlateVisibility::HitTestInvisible);
    RootCanvas->AddChild(Scanline);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Scanline->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
        CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 72.0f));
    }

    EmberDots.Reset();
    for (int32 Index = 0; Index < 28; ++Index)
    {
        UBorder* Ember = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Ember->SetBrushColor(FLinearColor(1.0f, 0.18f, 0.04f, 0.12f));
        Ember->SetPadding(FMargin(0.0f));
        Ember->SetVisibility(ESlateVisibility::HitTestInvisible);
        RootCanvas->AddChild(Ember);
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Ember->Slot))
        {
            const float Size = 3.0f + static_cast<float>(Index % 5);
            CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, Size, Size));
        }
        EmberDots.Add(Ember);
    }

    FramePanel = MakePanel(FLinearColor(0.0f, 0.0f, 0.0f, 0.34f), 20.0f);
    RootCanvas->AddChild(FramePanel);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FramePanel->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        CanvasSlot->SetOffsets(FMargin(28.0f, 24.0f, 28.0f, 24.0f));
    }

    MainStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainStack"));
    FramePanel->SetContent(MainStack);

    UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
    MainStack->AddChildToVerticalBox(TopBar);

    LogoImage = MakeImage(TEXT("other/crimson wars logo.png"), FVector2D(96.0f, 96.0f));
    CWMenu::Pad(TopBar->AddChildToHorizontalBox(LogoImage), 0.0f, 0.0f, 18.0f, 0.0f);

    UVerticalBox* Brand = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BrandStack"));
    UHorizontalBoxSlot* BrandSlot = TopBar->AddChildToHorizontalBox(Brand);
    BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(BrandSlot, 0.0f, 6.0f, 18.0f, 0.0f);
    Brand->AddChildToVerticalBox(MakeKicker(TEXT("CRIMSON WARS NATIVE")));
    Brand->AddChildToVerticalBox(MakeText(TEXT("Battle Hub"), 44.0f, CWMenu::WhiteText, true));
    Brand->AddChildToVerticalBox(MakeText(TEXT("3D-клиент: забеги, герои, экипировка, рейтинг, новости и будущий ragdoll-мясной балет."), 15.0f, CWMenu::MutedText));

    TopBar->AddChildToHorizontalBox(MakeStatTile(TEXT("Threat"), TEXT("LV 07"), CWMenu::Blood));
    TopBar->AddChildToHorizontalBox(MakeStatTile(TEXT("PvP"), TEXT("LIVE"), CWMenu::Violet));
    TopBar->AddChildToHorizontalBox(MakeStatTile(TEXT("Server"), TEXT("WS"), CWMenu::Green));

    UBorder* PlayerCard = MakePanel(FLinearColor(0.02f, 0.025f, 0.035f, 0.82f), 14.0f);
    CWMenu::Pad(MainStack->AddChildToVerticalBox(PlayerCard), 0.0f, 16.0f, 0.0f, 12.0f);
    UHorizontalBox* PlayerRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PlayerCardRow"));
    PlayerCard->SetContent(PlayerRow);
    CWMenu::Pad(PlayerRow->AddChildToHorizontalBox(MakeImage(TEXT("characters/cyber.jpg"), FVector2D(108.0f, 108.0f))), 0.0f, 0.0f, 16.0f, 0.0f);

    UVerticalBox* PlayerText = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerCardText"));
    UHorizontalBoxSlot* PlayerTextSlot = PlayerRow->AddChildToHorizontalBox(PlayerText);
    PlayerTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PlayerText->AddChildToVerticalBox(MakeText(TEXT("Fighter / Cyber"), 24.0f, CWMenu::WhiteText, true));
    PlayerText->AddChildToVerticalBox(MakeText(TEXT("Профиль, герой, таланты, шмот и быстрые слоты будут жить здесь же, как в веб-хабе, только в нативном слое."), 14.0f, CWMenu::MutedText));
    UHorizontalBox* PlayerStats = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PlayerStats"));
    CWMenu::Pad(PlayerText->AddChildToVerticalBox(PlayerStats), 0.0f, 10.0f, 0.0f, 0.0f);
    PlayerStats->AddChildToHorizontalBox(MakeStatTile(TEXT("Hero"), TEXT("Lv 1"), CWMenu::Cyan));
    PlayerStats->AddChildToHorizontalBox(MakeStatTile(TEXT("Skills"), TEXT("0/0"), CWMenu::Amber));
    PlayerStats->AddChildToHorizontalBox(MakeStatTile(TEXT("Shards"), TEXT("0"), CWMenu::Violet));

    UVerticalBox* RightStatus = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightStatus"));
    CWMenu::Pad(PlayerRow->AddChildToHorizontalBox(RightStatus), 16.0f, 0.0f, 0.0f, 0.0f);
    RightStatus->AddChildToVerticalBox(MakeKicker(TEXT("CONNECTION")));
    StatusText = MakeText(TEXT("Waiting for server..."), 14.0f, CWMenu::Green, true);
    RightStatus->AddChildToVerticalBox(StatusText);
    UButton* CloseButton = MakeButton(TEXT("Закрыть меню"), CWMenu::MutedText);
    CloseButton->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::HandleCloseClicked);
    CWMenu::Pad(RightStatus->AddChildToVerticalBox(CloseButton), 0.0f, 10.0f, 0.0f, 0.0f);

    TopBar->SetVisibility(ESlateVisibility::Collapsed);
    PlayerCard->SetVisibility(ESlateVisibility::Collapsed);
    AddBattleHubProfileHeader(MainStack);

    TabBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabBar"));
    MainStack->AddChildToVerticalBox(TabBar);
    auto AddNavTab = [this](const FName TabId, const FString& Label, const FLinearColor& Accent)
    {
        UHorizontalBoxSlot* Slot = TabBar->AddChildToHorizontalBox(MakeTabButton(TabId, Label, Accent));
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CWMenu::Pad(Slot, 0.0f, 0.0f, 10.0f, 0.0f);
    };
    AddNavTab(TEXT("run"), TEXT("Забег"), CWMenu::Green);
    AddNavTab(TEXT("story"), TEXT("Сюжет"), CWMenu::Amber);
    AddNavTab(TEXT("characters"), TEXT("Персонажи"), CWMenu::Cyan);
    AddNavTab(TEXT("profile"), TEXT("Профиль"), FLinearColor(0.45f, 0.55f, 1.0f, 1.0f));
    AddNavTab(TEXT("rating"), TEXT("Рейтинг"), FLinearColor(0.65f, 0.28f, 1.0f, 1.0f));
    AddNavTab(TEXT("news"), TEXT("Новости"), CWMenu::Pink);
    UHorizontalBoxSlot* LastTabSlot = TabBar->AddChildToHorizontalBox(MakeTabButton(TEXT("settings"), TEXT("Настройки"), FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
    LastTabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ContentHost = MakePanel(CWMenu::Panel, 18.0f);
    UVerticalBoxSlot* ContentSlot = MainStack->AddChildToVerticalBox(ContentHost);
    ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(ContentSlot, 0.0f, 14.0f, 0.0f, 0.0f);

    ContentSweep = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ContentSweep"));
    ContentSweep->SetBrushColor(FLinearColor(1.0f, 0.04f, 0.04f, 0.055f));
    ContentSweep->SetVisibility(ESlateVisibility::HitTestInvisible);
    RootCanvas->AddChild(ContentSweep);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ContentSweep->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
        CanvasSlot->SetOffsets(FMargin(48.0f, 318.0f, 48.0f, 3.0f));
    }

    DialogLayer = MakePanel(FLinearColor(0.0f, 0.0f, 0.0f, 0.66f), 0.0f);
    DialogLayer->SetVisibility(ESlateVisibility::Collapsed);
    RootCanvas->AddChild(DialogLayer);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DialogLayer->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        CanvasSlot->SetOffsets(FMargin(0.0f));
    }

    FpsPanel = MakePanel(FLinearColor(0.01f, 0.014f, 0.019f, 0.86f), 8.0f);
    FpsPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    RootCanvas->AddChild(FpsPanel);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FpsPanel->Slot))
    {
        CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
        CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        CanvasSlot->SetOffsets(FMargin(-44.0f, 34.0f, 118.0f, 34.0f));
    }
    FpsText = MakeText(TEXT("FPS --"), 14.0f, CWMenu::Green, true);
    FpsText->SetAutoWrapText(false);
    FpsText->SetJustification(ETextJustify::Center);
    FpsPanel->SetContent(FpsText);

    SelectTab(ActiveTab);
}

void UCWNativeMenuWidget::AddBattleHubProfileHeader(UVerticalBox* Parent)
{
    if (!Parent)
    {
        return;
    }

    const FCWHeroUiData Hero = GetHeroUiData(SelectedHeroId);

    UBorder* Shell = MakePanel(FLinearColor(0.012f, 0.014f, 0.018f, 0.92f), 9.0f);
    CWMenu::Pad(Parent->AddChildToVerticalBox(Shell), 0.0f, 0.0f, 0.0f, 10.0f);

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BattleHubProfileHeader"));
    Shell->SetContent(HeaderRow);

    USizeBox* AvatarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    AvatarSize->SetWidthOverride(132.0f);
    AvatarSize->SetHeightOverride(254.0f);
    CWMenu::Pad(HeaderRow->AddChildToHorizontalBox(AvatarSize), 0.0f, 0.0f, 12.0f, 0.0f);

    UBorder* AvatarCard = MakePanel(FLinearColor(0.03f, 0.05f, 0.045f, 0.94f), 7.0f);
    AvatarSize->SetContent(AvatarCard);
    UVerticalBox* AvatarStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    AvatarCard->SetContent(AvatarStack);
    UVerticalBoxSlot* AvatarImageSlot = AvatarStack->AddChildToVerticalBox(MakeFittedImage(Hero.Asset, FVector2D(118.0f, 216.0f)));
    AvatarImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UTextBlock* HeroLevel = MakeText(Hero.ProfileLevel, 12.0f, CWMenu::WhiteText, true);
    HeroLevel->SetJustification(ETextJustify::Center);
    AvatarStack->AddChildToVerticalBox(HeroLevel);

    UVerticalBox* Center = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ProfileCenter"));
    UHorizontalBoxSlot* CenterSlot = HeaderRow->AddChildToHorizontalBox(Center);
    CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(CenterSlot, 0.0f, 0.0f, 14.0f, 0.0f);

    UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Center->AddChildToVerticalBox(NameRow);
    UHorizontalBoxSlot* NameSlot = NameRow->AddChildToHorizontalBox(MakeText(TEXT("WizardJIOCb"), 31.0f, CWMenu::WhiteText, true));
    NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(NameRow->AddChildToHorizontalBox(MakeProfileMetric(TEXT("PROFILE"), TEXT("LV 44"), CWMenu::Blood)), 10.0f, 0.0f, 8.0f, 0.0f);
    NameRow->AddChildToHorizontalBox(MakeText(TEXT("ONLINE"), 13.0f, CWMenu::MutedText, true));

    CWMenu::Pad(Center->AddChildToVerticalBox(MakeProgressBar(0.06f, FLinearColor(1.0f, 0.72f, 0.18f, 1.0f), 7.0f)), 0.0f, 4.0f, 0.0f, 4.0f);
    Center->AddChildToVerticalBox(MakeText(TEXT("LV 44 · XP 1806 / 29446 · 6%"), 11.0f, CWMenu::MutedText, true));

    UHorizontalBox* Metrics = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CWMenu::Pad(Center->AddChildToVerticalBox(Metrics), 0.0f, 8.0f, 0.0f, 8.0f);
    const TArray<TPair<FString, FString>> MetricItems = {
        { TEXT("НАВЫКИ"), TEXT("23/40") },
        { TEXT("СЮЖЕТ"), TEXT("1/16") },
        { TEXT("ГЕРОИ"), TEXT("5/5") },
        { TEXT("ЗАБЕГИ"), TEXT("84") },
        { TEXT("ОСКОЛКИ"), TEXT("118755") },
        { TEXT("SP"), TEXT("8") }
    };
    for (int32 Index = 0; Index < MetricItems.Num(); ++Index)
    {
        UHorizontalBoxSlot* MetricSlot = Metrics->AddChildToHorizontalBox(MakeProfileMetric(MetricItems[Index].Key, MetricItems[Index].Value, Index % 2 == 0 ? CWMenu::Cyan : CWMenu::Amber));
        MetricSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CWMenu::Pad(MetricSlot, 0.0f, 0.0f, Index + 1 < MetricItems.Num() ? 8.0f : 0.0f, 0.0f);
    }

    UBorder* SkillsPanel = MakePanel(FLinearColor(0.015f, 0.02f, 0.03f, 0.82f), 8.0f);
    Center->AddChildToVerticalBox(SkillsPanel);
    UVerticalBox* SkillsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    SkillsPanel->SetContent(SkillsStack);
    UHorizontalBox* SkillsHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    SkillsStack->AddChildToVerticalBox(SkillsHead);
    UHorizontalBoxSlot* SkillsTitleSlot = SkillsHead->AddChildToHorizontalBox(MakeKicker(TEXT("HERO SKILLS")));
    SkillsTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SkillsHead->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%s ОТКРЫТЫ"), *Hero.Skills), 12.0f, CWMenu::WhiteText, true));

    UVerticalBox* SkillGrid = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    CWMenu::Pad(SkillsStack->AddChildToVerticalBox(SkillGrid), 0.0f, 8.0f, 0.0f, 0.0f);
    const TArray<FCWSkillUiData> SkillItems = GetHeroHeaderSkills(SelectedHeroId);
    for (int32 RowIndex = 0; RowIndex < 2; ++RowIndex)
    {
        UHorizontalBox* SkillRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        CWMenu::Pad(SkillGrid->AddChildToVerticalBox(SkillRow), 0.0f, RowIndex == 0 ? 0.0f : 6.0f, 0.0f, 0.0f);
        for (int32 ColIndex = 0; ColIndex < 4; ++ColIndex)
        {
            const int32 SkillIndex = RowIndex * 4 + ColIndex;
            UHorizontalBoxSlot* SkillSlot = SkillRow->AddChildToHorizontalBox(MakeSkillChip(SkillItems[SkillIndex].Title, SkillItems[SkillIndex].Meta, SkillItems[SkillIndex].Icon, SkillItems[SkillIndex].Accent));
            SkillSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CWMenu::Pad(SkillSlot, 0.0f, 0.0f, ColIndex < 3 ? 6.0f : 0.0f, 0.0f);
        }
    }

    USizeBox* IndexSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    IndexSize->SetWidthOverride(196.0f);
    UHorizontalBoxSlot* IndexSlot = HeaderRow->AddChildToHorizontalBox(IndexSize);
    IndexSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    UBorder* IndexPanel = MakePanel(FLinearColor(0.08f, 0.01f, 0.022f, 0.9f), 12.0f);
    IndexSize->SetContent(IndexPanel);
    UVerticalBox* IndexStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    IndexPanel->SetContent(IndexStack);
    UTextBlock* RankLabel = MakeText(TEXT("GLOBAL RANK"), 11.0f, CWMenu::MutedText, true);
    RankLabel->SetJustification(ETextJustify::Center);
    IndexStack->AddChildToVerticalBox(RankLabel);
    UTextBlock* RankValue = MakeText(TEXT("#1"), 34.0f, CWMenu::WhiteText, true);
    RankValue->SetJustification(ETextJustify::Center);
    IndexStack->AddChildToVerticalBox(RankValue);
    UTextBlock* RankDetail = MakeText(TEXT("146 674 ИНД. · 1/9 · ТОП 12%"), 11.0f, CWMenu::MutedText);
    RankDetail->SetJustification(ETextJustify::Center);
    IndexStack->AddChildToVerticalBox(RankDetail);
    StatusText = MakeText(TEXT("Connected / menu"), 13.0f, CWMenu::Green, true);
    StatusText->SetJustification(ETextJustify::Center);
    CWMenu::Pad(IndexStack->AddChildToVerticalBox(StatusText), 0.0f, 14.0f, 0.0f, 10.0f);

    UUniformGridPanel* PresenceGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    IndexStack->AddChildToVerticalBox(PresenceGrid);
    const TArray<TPair<FString, FString>> Presence = {
        { TEXT("ONLINE"), TEXT("4") },
        { TEXT("IN GAME"), TEXT("0") },
        { TEXT("IN MENU"), TEXT("4") },
        { TEXT("REGISTERED"), TEXT("15") }
    };
    for (int32 Index = 0; Index < Presence.Num(); ++Index)
    {
        PresenceGrid->AddChildToUniformGrid(MakeProfileMetric(Presence[Index].Key, Presence[Index].Value, Index == 3 ? CWMenu::Green : CWMenu::MutedText), Index / 2, Index % 2);
    }
}

void UCWNativeMenuWidget::RebuildContent()
{
    if (!ContentHost)
    {
        return;
    }

    if (ActiveTab == TEXT("story")) ContentHost->SetContent(MakeStoryPanel());
    else if (ActiveTab == TEXT("characters")) ContentHost->SetContent(MakeCharactersPanel());
    else if (ActiveTab == TEXT("skills")) ContentHost->SetContent(MakeSkillsPanel());
    else if (ActiveTab == TEXT("profile")) ContentHost->SetContent(MakeProfilePanel());
    else if (ActiveTab == TEXT("rating")) ContentHost->SetContent(MakeRatingPanel());
    else if (ActiveTab == TEXT("news")) ContentHost->SetContent(MakeNewsPanel());
    else if (ActiveTab == TEXT("settings")) ContentHost->SetContent(MakeSettingsPanel());
    else ContentHost->SetContent(MakeRunPanel());
}

void UCWNativeMenuWidget::SelectTab(const FName TabId)
{
    ActiveTab = TabId;
    PanelRevealTime = 0.0f;
    for (const TPair<FName, UButton*>& Pair : TabButtons)
    {
        if (!Pair.Value)
        {
            continue;
        }
        const FLinearColor Accent = GetTabAccent(Pair.Key);
        Pair.Value->SetBackgroundColor(Pair.Key == ActiveTab
            ? FLinearColor(0.03f + Accent.R * 0.18f, 0.035f + Accent.G * 0.13f, 0.045f + Accent.B * 0.18f, 0.96f)
            : FLinearColor(0.024f + Accent.R * 0.03f, 0.03f + Accent.G * 0.024f, 0.04f + Accent.B * 0.032f, 0.82f));
    }
    RebuildContent();
}

void UCWNativeMenuWidget::RefreshStatusText()
{
    if (!StatusText)
    {
        return;
    }

    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI)
    {
        StatusText->SetText(FText::FromString(TEXT("No game instance")));
        return;
    }

    if (!GI->IsSocketConnected())
    {
        StatusText->SetText(FText::FromString(GI->bBootstrapLoaded ? TEXT("Menu data loaded / ws offline") : TEXT("Offline / loading menu data")));
        return;
    }

    if (GI->CurrentRoomCode.IsEmpty())
    {
        StatusText->SetText(FText::FromString(TEXT("Connected / menu")));
        return;
    }

    StatusText->SetText(FText::FromString(FString::Printf(TEXT("Room %s / %d players / %d mobs"),
        *GI->CurrentRoomCode,
        GI->LatestState.Players.Num(),
        GI->LatestState.Enemies.Num())));
}

void UCWNativeMenuWidget::HandleBootstrapReceived(const FCWNativeBootstrapSnapshot& Snapshot)
{
    if (!Snapshot.bOk)
    {
        return;
    }
    PanelRevealTime = 0.0f;
    RebuildContent();
}

UTextBlock* UCWNativeMenuWidget::MakeText(const FString& Text, float Size, const FLinearColor& Color, bool bBold)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Block->SetText(FText::FromString(Text));
    Block->SetColorAndOpacity(FSlateColor(Color));
    Block->SetFont(CWMenu::Font(Size, bBold));
    Block->SetAutoWrapText(true);
    return Block;
}

UTextBlock* UCWNativeMenuWidget::MakeKicker(const FString& Text)
{
    UTextBlock* Block = MakeText(Text.ToUpper(), 12.0f, FLinearColor(1.0f, 0.35f, 0.24f, 1.0f), true);
    return Block;
}

UButton* UCWNativeMenuWidget::MakeButton(const FString& Text, const FLinearColor& Accent, bool bPrimary)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetBackgroundColor(bPrimary
        ? FLinearColor(Accent.R * 0.74f, Accent.G * 0.74f, Accent.B * 0.74f, 0.96f)
        : FLinearColor(0.028f + Accent.R * 0.05f, 0.034f + Accent.G * 0.04f, 0.045f + Accent.B * 0.05f, 0.88f));
    Button->SetColorAndOpacity(FLinearColor::White);

    USizeBox* LabelFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LabelFrame->SetHeightOverride(42.0f);
    LabelFrame->SetMinDesiredWidth(122.0f);

    UBorder* LabelPanel = MakePanel(FLinearColor(0.018f + Accent.R * 0.04f, 0.022f + Accent.G * 0.035f, 0.030f + Accent.B * 0.045f, 0.58f), 0.0f);
    LabelPanel->SetHorizontalAlignment(HAlign_Center);
    LabelPanel->SetVerticalAlignment(VAlign_Center);
    LabelFrame->SetContent(LabelPanel);

    UTextBlock* Label = MakeText(Text, 15.0f, CWMenu::WhiteText, true);
    Label->SetJustification(ETextJustify::Center);
    Label->SetAutoWrapText(false);
    LabelPanel->SetContent(Label);
    if (USizeBoxSlot* SizeSlot = Cast<USizeBoxSlot>(Label->Slot))
    {
        SizeSlot->SetHorizontalAlignment(HAlign_Center);
        SizeSlot->SetVerticalAlignment(VAlign_Center);
        SizeSlot->SetPadding(FMargin(0.0f));
    }
    Button->SetContent(LabelFrame);
    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(LabelFrame->Slot))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(0.0f));
    }
    RegisterButtonFx(Button, Accent, bPrimary);
    return Button;
}

UBorder* UCWNativeMenuWidget::MakePanel(const FLinearColor& Tint, float InPadding)
{
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(Tint);
    Panel->SetPadding(FMargin(InPadding));
    return Panel;
}

UImage* UCWNativeMenuWidget::MakeImage(const FString& RelativeAssetPath, const FVector2D& DesiredSize)
{
    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    const FString LocalAssetPath = ToLocalAssetPath(RelativeAssetPath);
    if (UTexture2D* Texture = LoadTexture(LocalAssetPath))
    {
        Image->SetBrushFromTexture(Texture, false);
    }
    Image->SetDesiredSizeOverride(DesiredSize);
    return Image;
}

UWidget* UCWNativeMenuWidget::MakeFittedImage(const FString& RelativeAssetPath, const FVector2D& FrameSize)
{
    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Frame->SetWidthOverride(FrameSize.X);
    Frame->SetHeightOverride(FrameSize.Y);
    Frame->SetClipping(EWidgetClipping::ClipToBounds);

    UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    Frame->SetContent(Scale);

    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    const FString LocalAssetPath = ToLocalAssetPath(RelativeAssetPath);
    if (UTexture2D* Texture = LoadTexture(LocalAssetPath))
    {
        Image->SetBrushFromTexture(Texture, true);
    }
    Scale->SetContent(Image);

    return Frame;
}

UWidget* UCWNativeMenuWidget::MakeCoverImage(const FString& RelativeAssetPath, const FVector2D& FrameSize)
{
    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Frame->SetWidthOverride(FrameSize.X);
    Frame->SetHeightOverride(FrameSize.Y);
    Frame->SetClipping(EWidgetClipping::ClipToBounds);

    UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Scale->SetStretch(EStretch::ScaleToFill);
    Scale->SetStretchDirection(EStretchDirection::Both);
    Frame->SetContent(Scale);

    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    const FString LocalAssetPath = ToLocalAssetPath(RelativeAssetPath);
    if (UTexture2D* Texture = LoadTexture(LocalAssetPath))
    {
        Image->SetBrushFromTexture(Texture, true);
    }
    Scale->SetContent(Image);
    if (UScaleBoxSlot* ScaleSlot = Cast<UScaleBoxSlot>(Image->Slot))
    {
        ScaleSlot->SetHorizontalAlignment(HAlign_Center);
        ScaleSlot->SetVerticalAlignment(VAlign_Top);
    }

    return Frame;
}

UWidget* UCWNativeMenuWidget::MakePortraitCrop(const FString& RelativeAssetPath, const FVector2D& FrameSize, float YOffset)
{
    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Frame->SetWidthOverride(FrameSize.X);
    Frame->SetHeightOverride(FrameSize.Y);
    Frame->SetClipping(EWidgetClipping::ClipToBounds);

    UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    Frame->SetContent(Overlay);

    UImage* Image = MakeImage(RelativeAssetPath, FVector2D(FrameSize.X, FrameSize.X * 1.5f));
    Image->SetRenderTranslation(FVector2D(0.0f, YOffset));
    UOverlaySlot* ImageSlot = Overlay->AddChildToOverlay(Image);
    ImageSlot->SetHorizontalAlignment(HAlign_Center);
    ImageSlot->SetVerticalAlignment(VAlign_Top);

    return Frame;
}

UWidget* UCWNativeMenuWidget::MakeSquareImage(const FString& RelativeAssetPath, float Size)
{
    return MakeFittedImage(RelativeAssetPath, FVector2D(Size, Size));
}

UWidget* UCWNativeMenuWidget::MakeActionPill(const FString& Text, const FLinearColor& Accent)
{
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetHeightOverride(36.0f);

    UBorder* Pill = MakePanel(FLinearColor(0.035f + Accent.R * 0.04f, 0.035f + Accent.G * 0.03f, 0.042f + Accent.B * 0.04f, 0.9f), 0.0f);
    Pill->SetHorizontalAlignment(HAlign_Center);
    Pill->SetVerticalAlignment(VAlign_Center);
    Size->SetContent(Pill);

    UTextBlock* Label = MakeText(Text, 13.0f, CWMenu::WhiteText, true);
    Label->SetJustification(ETextJustify::Center);
    Label->SetAutoWrapText(false);
    Pill->SetContent(Label);

    return Size;
}

UWidget* UCWNativeMenuWidget::MakeTabButton(const FName TabId, const FString& Label, const FLinearColor& Accent)
{
    UButton* Button = MakeButton(Label, Accent, TabId == ActiveTab);
    Button->SetBackgroundColor(TabId == ActiveTab
        ? FLinearColor(0.025f + Accent.R * 0.18f, 0.035f + Accent.G * 0.13f, 0.045f + Accent.B * 0.18f, 0.96f)
        : FLinearColor(0.025f + Accent.R * 0.035f, 0.03f + Accent.G * 0.03f, 0.04f + Accent.B * 0.04f, 0.82f));

    if (TabId == TEXT("run")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectRunTab);
    else if (TabId == TEXT("story")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectStoryTab);
    else if (TabId == TEXT("characters")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectCharactersTab);
    else if (TabId == TEXT("skills")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectSkillsTab);
    else if (TabId == TEXT("profile")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectProfileTab);
    else if (TabId == TEXT("rating")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectRatingTab);
    else if (TabId == TEXT("news")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectNewsTab);
    else if (TabId == TEXT("settings")) Button->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::SelectSettingsTab);

    TabButtons.Add(TabId, Button);
    return Button;
}

UWidget* UCWNativeMenuWidget::MakeStatTile(const FString& Label, const FString& Value, const FLinearColor& Accent)
{
    UBorder* Tile = MakePanel(FLinearColor(0.03f, 0.035f, 0.046f, 0.74f), 10.0f);
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Tile->SetContent(Box);
    Box->AddChildToVerticalBox(MakeText(Label, 11.0f, CWMenu::MutedText, true));
    Box->AddChildToVerticalBox(MakeText(Value, 20.0f, Accent, true));
    return Tile;
}

UWidget* UCWNativeMenuWidget::MakeProfileMetric(const FString& Label, const FString& Value, const FLinearColor& Accent)
{
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetMinDesiredWidth(118.0f);

    UBorder* Tile = MakePanel(FLinearColor(0.018f, 0.023f, 0.033f, 0.88f), 8.0f);
    RegisterPanelFx(Tile, Accent, false);
    Size->SetContent(Tile);

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Tile->SetContent(Box);

    UTextBlock* LabelText = MakeText(Label, 10.5f, CWMenu::MutedText, true);
    LabelText->SetJustification(ETextJustify::Center);
    Box->AddChildToVerticalBox(LabelText);

    UTextBlock* ValueText = MakeText(Value, 17.0f, Accent, true);
    ValueText->SetJustification(ETextJustify::Center);
    Box->AddChildToVerticalBox(ValueText);

    return Size;
}

UWidget* UCWNativeMenuWidget::MakeProgressBar(float Percent, const FLinearColor& Fill, float Height)
{
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetHeightOverride(Height);

    UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
    Bar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
    Bar->SetFillColorAndOpacity(Fill);
    Size->SetContent(Bar);

    return Size;
}

UWidget* UCWNativeMenuWidget::MakeSkillChip(const FString& Title, const FString& Meta, const FString& AssetPath, const FLinearColor& Accent)
{
    UButton* Hit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Hit->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
    Hit->SetColorAndOpacity(FLinearColor::White);
    BindSkillButton(Hit, AssetPath);
    RegisterButtonFx(Hit, Accent, false);

    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetHeightOverride(58.0f);

    UBorder* Chip = MakePanel(FLinearColor(0.018f + Accent.R * 0.045f, 0.022f + Accent.G * 0.03f, 0.035f + Accent.B * 0.045f, 0.9f), 8.0f);
    RegisterPanelFx(Chip, Accent, false);
    Size->SetContent(Chip);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Chip->SetContent(Row);
    CWMenu::Pad(Row->AddChildToHorizontalBox(MakeSquareImage(AssetPath, 34.0f)), 0.0f, 0.0f, 8.0f, 0.0f);

    UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextStack);
    TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TextStack->AddChildToVerticalBox(MakeText(Title, 10.5f, CWMenu::WhiteText, true));
    TextStack->AddChildToVerticalBox(MakeText(Meta, 8.5f, Accent, true));

    Hit->SetContent(Size);
    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Size->Slot))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ButtonSlot->SetVerticalAlignment(VAlign_Fill);
        ButtonSlot->SetPadding(FMargin(0.0f));
    }
    return Hit;
}

UWidget* UCWNativeMenuWidget::MakeSkillListRow(const FString& Title, const FString& Meta, const FString& Level, const FString& AssetPath, const FLinearColor& Accent)
{
    UButton* Hit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Hit->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
    Hit->SetColorAndOpacity(FLinearColor::White);
    BindSkillButton(Hit, AssetPath);
    RegisterButtonFx(Hit, Accent, false);

    UBorder* RowPanel = MakePanel(FLinearColor(0.015f + Accent.R * 0.045f, 0.022f + Accent.G * 0.025f, 0.036f + Accent.B * 0.05f, 0.92f), 10.0f);
    RegisterPanelFx(RowPanel, Accent, false);
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowPanel->SetContent(Row);

    CWMenu::Pad(Row->AddChildToHorizontalBox(MakeSquareImage(AssetPath, 46.0f)), 0.0f, 0.0f, 10.0f, 0.0f);

    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* CopySlot = Row->AddChildToHorizontalBox(Copy);
    CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Copy->AddChildToVerticalBox(MakeText(Title, 14.0f, CWMenu::WhiteText, true));
    Copy->AddChildToVerticalBox(MakeText(Meta, 11.0f, Accent, true));

    USizeBox* LevelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LevelBox->SetWidthOverride(70.0f);
    UBorder* LevelPanel = MakePanel(FLinearColor(0.02f, 0.027f, 0.04f, 0.94f), 8.0f);
    LevelBox->SetContent(LevelPanel);
    UTextBlock* LevelText = MakeText(Level, 13.0f, CWMenu::WhiteText, true);
    LevelText->SetJustification(ETextJustify::Center);
    LevelText->SetAutoWrapText(false);
    LevelPanel->SetContent(LevelText);
    Row->AddChildToHorizontalBox(LevelBox);

    Hit->SetContent(RowPanel);
    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RowPanel->Slot))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ButtonSlot->SetVerticalAlignment(VAlign_Fill);
        ButtonSlot->SetPadding(FMargin(0.0f));
    }
    return Hit;
}

UWidget* UCWNativeMenuWidget::MakeRosterCard(const FString& Index, const FString& HeroId, const FString& Name, const FString& Level, const FString& AssetPath, const FLinearColor& Accent, bool bSelected)
{
    UButton* Hit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Hit->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
    Hit->SetColorAndOpacity(FLinearColor::White);
    BindHeroButton(Hit, HeroId);
    RegisterButtonFx(Hit, Accent, bSelected);

    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetWidthOverride(148.0f);
    Size->SetHeightOverride(148.0f);

    UBorder* Frame = MakePanel(bSelected
        ? FLinearColor(0.10f + Accent.R * 0.22f, 0.12f + Accent.G * 0.18f, 0.14f + Accent.B * 0.18f, 0.98f)
        : FLinearColor(0.048f + Accent.R * 0.055f, 0.052f + Accent.G * 0.045f, 0.062f + Accent.B * 0.055f, 0.92f), 2.0f);
    Size->SetContent(Frame);
    RegisterPanelFx(Frame, Accent, bSelected);

    UBorder* Card = MakePanel(bSelected
        ? FLinearColor(0.025f + Accent.R * 0.055f, 0.032f + Accent.G * 0.055f, 0.04f + Accent.B * 0.055f, 0.96f)
        : FLinearColor(0.018f, 0.022f, 0.03f, 0.94f), 7.0f);
    Frame->SetContent(Card);

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Card->SetContent(Box);

    UHorizontalBox* BadgeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Box->AddChildToVerticalBox(BadgeRow);
    CWMenu::Pad(BadgeRow->AddChildToHorizontalBox(MakeText(Index, 11.0f, Accent, true)), 0.0f, 0.0f, 5.0f, 0.0f);
    UHorizontalBoxSlot* HeroIdSlot = BadgeRow->AddChildToHorizontalBox(MakeText(HeroId.ToUpper(), 10.0f, CWMenu::MutedText, true));
    HeroIdSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BadgeRow->AddChildToHorizontalBox(MakeText(Level, 10.0f, CWMenu::WhiteText, true));

    CWMenu::Pad(Box->AddChildToVerticalBox(MakePortraitCrop(AssetPath, FVector2D(126.0f, 76.0f), -8.0f)), 0.0f, 5.0f, 0.0f, 4.0f);

    UTextBlock* NameText = MakeText(Name, 16.0f, CWMenu::WhiteText, true);
    NameText->SetJustification(ETextJustify::Center);
    Box->AddChildToVerticalBox(NameText);

    UTextBlock* StateText = MakeText(bSelected ? TEXT("ВЫБРАН") : TEXT("ОТКРЫТ"), 10.0f, bSelected ? Accent : CWMenu::MutedText, true);
    StateText->SetJustification(ETextJustify::Center);
    Box->AddChildToVerticalBox(StateText);

    Hit->SetContent(Size);
    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Size->Slot))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Center);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(0.0f));
    }
    return Hit;
}

UWidget* UCWNativeMenuWidget::MakeBonusRow(const FString& Label, const FString& Value, const FLinearColor& Accent)
{
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetMinDesiredHeight(34.0f);

    UBorder* RowPanel = MakePanel(FLinearColor(0.014f, 0.019f, 0.027f, 0.82f), 8.0f);
    RegisterPanelFx(RowPanel, Accent, false);
    Size->SetContent(RowPanel);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowPanel->SetContent(Row);

    UTextBlock* LabelText = MakeText(Label, 10.5f, CWMenu::MutedText, true);
    LabelText->SetAutoWrapText(false);
    CWMenu::Pad(Row->AddChildToHorizontalBox(LabelText), 0.0f, 0.0f, 8.0f, 0.0f);

    UTextBlock* ValueText = MakeText(Value, 11.5f, Accent, true);
    ValueText->SetAutoWrapText(true);
    ValueText->SetJustification(ETextJustify::Right);
    UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueText);
    ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ValueSlot->SetVerticalAlignment(VAlign_Center);

    return Size;
}

UWidget* UCWNativeMenuWidget::MakeEquipmentSlot(
    const FString& Code,
    const FString& Label,
    const FLinearColor& Accent,
    const FString& AssetPath,
    const FString& ItemName,
    const FString& ItemMeta,
    const FString& ItemStats,
    const FString& ActionText,
    const FName SlotKey)
{
    UButton* Hit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Hit->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
    Hit->SetColorAndOpacity(FLinearColor::White);
    BindSlotButton(Hit, SlotKey);
    RegisterButtonFx(Hit, Accent, false);

    UBorder* SlotPanel = MakePanel(FLinearColor(0.012f, 0.018f, 0.026f, 0.9f), 8.0f);
    RegisterPanelFx(SlotPanel, Accent, false);
    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    SlotPanel->SetContent(Stack);

    UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Stack->AddChildToVerticalBox(Head);

    USizeBox* BadgeSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    BadgeSize->SetWidthOverride(46.0f);
    BadgeSize->SetHeightOverride(46.0f);
    UBorder* Badge = MakePanel(FLinearColor(0.03f + Accent.R * 0.04f, 0.04f + Accent.G * 0.04f, 0.055f + Accent.B * 0.04f, 0.94f), 6.0f);
    BadgeSize->SetContent(Badge);
    if (!AssetPath.IsEmpty())
    {
        Badge->SetPadding(FMargin(2.0f));
        Badge->SetContent(MakeSquareImage(AssetPath, 40.0f));
    }
    else
    {
        UTextBlock* CodeText = MakeText(Code, 15.0f, Accent, true);
        CodeText->SetJustification(ETextJustify::Center);
        Badge->SetContent(CodeText);
    }
    CWMenu::Pad(Head->AddChildToHorizontalBox(BadgeSize), 0.0f, 0.0f, 8.0f, 0.0f);

    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* CopySlot = Head->AddChildToHorizontalBox(Copy);
    CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    const FString DisplayName = ItemName.IsEmpty() ? FString(TEXT("Пусто")) : ItemName;
    const FString DisplayMeta = ItemMeta.IsEmpty() ? FString(TEXT("Выберите предмет из инвентаря")) : ItemMeta;
    const FString ButtonText = ActionText.IsEmpty() ? FString(TEXT("СНАРЯДИТЬ")) : ActionText;
    Copy->AddChildToVerticalBox(MakeText(Label, 10.5f, Accent, true));
    UTextBlock* NameText = MakeText(DisplayName, 13.0f, CWMenu::WhiteText, true);
    NameText->SetAutoWrapText(false);
    Copy->AddChildToVerticalBox(NameText);
    Copy->AddChildToVerticalBox(MakeText(DisplayMeta, 10.0f, CWMenu::MutedText));
    if (!ItemStats.IsEmpty())
    {
        UTextBlock* StatsText = MakeText(ItemStats, 10.0f, Accent, true);
        StatsText->SetAutoWrapText(true);
        Copy->AddChildToVerticalBox(StatsText);
    }

    CWMenu::Pad(Stack->AddChildToVerticalBox(MakeActionPill(ButtonText, Accent)), 0.0f, 8.0f, 0.0f, 0.0f);
    Hit->SetContent(SlotPanel);
    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(SlotPanel->Slot))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ButtonSlot->SetVerticalAlignment(VAlign_Fill);
        ButtonSlot->SetPadding(FMargin(0.0f));
    }
    return Hit;
}

UWidget* UCWNativeMenuWidget::MakeDialogListRow(const FString& Title, const FString& Meta, const FString& AssetPath, const FString& ActionText, const FLinearColor& Accent)
{
    UBorder* RowPanel = MakePanel(FLinearColor(0.016f + Accent.R * 0.035f, 0.022f + Accent.G * 0.025f, 0.032f + Accent.B * 0.04f, 0.92f), 10.0f);
    RegisterPanelFx(RowPanel, Accent, false);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowPanel->SetContent(Row);
    if (!AssetPath.IsEmpty())
    {
        CWMenu::Pad(Row->AddChildToHorizontalBox(MakeSquareImage(AssetPath, 52.0f)), 0.0f, 0.0f, 12.0f, 0.0f);
    }

    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* CopySlot = Row->AddChildToHorizontalBox(Copy);
    CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UTextBlock* TitleText = MakeText(Title, 15.0f, CWMenu::WhiteText, true);
    TitleText->SetAutoWrapText(false);
    Copy->AddChildToVerticalBox(TitleText);
    Copy->AddChildToVerticalBox(MakeText(Meta, 11.0f, Accent, true));

    UButton* Action = MakeButton(ActionText, Accent, true);
    Action->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::HandleDialogActionClicked);
    CWMenu::Pad(Row->AddChildToHorizontalBox(Action), 12.0f, 5.0f, 0.0f, 5.0f);

    return RowPanel;
}

UWidget* UCWNativeMenuWidget::MakeTalentRow(const FString& Title, const FString& Meta, const FString& Level, const FString& AssetPath, const FLinearColor& Accent)
{
    UButton* Hit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Hit->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
    Hit->SetColorAndOpacity(FLinearColor::White);
    Hit->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenPassiveSkillDialog);
    RegisterButtonFx(Hit, Accent, false);

    UBorder* RowPanel = MakePanel(FLinearColor(0.012f + Accent.R * 0.035f, 0.018f + Accent.G * 0.026f, 0.028f + Accent.B * 0.04f, 0.94f), 9.0f);
    RegisterPanelFx(RowPanel, Accent, true);
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RowPanel->SetContent(Row);
    CWMenu::Pad(Row->AddChildToHorizontalBox(MakeSquareImage(AssetPath, 44.0f)), 0.0f, 0.0f, 10.0f, 0.0f);

    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* CopySlot = Row->AddChildToHorizontalBox(Copy);
    CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UTextBlock* TitleText = MakeText(Title, 13.0f, CWMenu::WhiteText, true);
    TitleText->SetAutoWrapText(false);
    Copy->AddChildToVerticalBox(TitleText);
    Copy->AddChildToVerticalBox(MakeText(Meta, 10.5f, Accent, true));

    USizeBox* LevelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    LevelBox->SetWidthOverride(66.0f);
    UBorder* LevelPanel = MakePanel(FLinearColor(0.04f + Accent.R * 0.05f, 0.036f + Accent.G * 0.04f, 0.03f + Accent.B * 0.05f, 0.94f), 0.0f);
    LevelPanel->SetHorizontalAlignment(HAlign_Center);
    LevelPanel->SetVerticalAlignment(VAlign_Center);
    LevelBox->SetContent(LevelPanel);
    UTextBlock* LevelText = MakeText(Level, 14.0f, CWMenu::WhiteText, true);
    LevelText->SetJustification(ETextJustify::Center);
    LevelText->SetAutoWrapText(false);
    LevelPanel->SetContent(LevelText);
    Row->AddChildToHorizontalBox(LevelBox);

    Hit->SetContent(RowPanel);
    return Hit;
}

UWidget* UCWNativeMenuWidget::MakeInventoryCard(const FString& Title, const FString& Meta, const FString& AssetPath, const FLinearColor& Accent)
{
    UButton* Hit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Hit->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
    Hit->SetColorAndOpacity(FLinearColor::White);
    Hit->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::OpenRightHandSlotDialog);
    RegisterButtonFx(Hit, Accent, false);

    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetMinDesiredHeight(178.0f);
    UBorder* Card = MakePanel(FLinearColor(0.014f + Accent.R * 0.025f, 0.019f + Accent.G * 0.018f, 0.034f + Accent.B * 0.035f, 0.94f), 10.0f);
    RegisterPanelFx(Card, Accent, false);
    Size->SetContent(Card);

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Card->SetContent(Stack);

    UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Stack->AddChildToVerticalBox(Top);
    CWMenu::Pad(Top->AddChildToHorizontalBox(MakeSquareImage(AssetPath, 64.0f)), 0.0f, 0.0f, 10.0f, 0.0f);
    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* CopySlot = Top->AddChildToHorizontalBox(Copy);
    CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UTextBlock* TitleText = MakeText(Title, 13.0f, CWMenu::WhiteText, true);
    TitleText->SetAutoWrapText(false);
    Copy->AddChildToVerticalBox(TitleText);
    Copy->AddChildToVerticalBox(MakeText(Meta, 10.0f, CWMenu::MutedText));
    Copy->AddChildToVerticalBox(MakeText(TEXT("Lv 1 · x15 · Продажа: 9"), 9.5f, CWMenu::MutedText, true));

    CWMenu::Pad(Stack->AddChildToVerticalBox(MakeText(TEXT("Эффект: усиление боевых параметров. Клавиши 1/2/3 в бою."), 10.5f, Accent, true)), 0.0f, 8.0f, 0.0f, 8.0f);

    UUniformGridPanel* Actions = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    Stack->AddChildToVerticalBox(Actions);
    Actions->AddChildToUniformGrid(MakeActionPill(TEXT("СЛОТ 1"), Accent), 0, 0);
    Actions->AddChildToUniformGrid(MakeActionPill(TEXT("СЛОТ 2"), Accent), 0, 1);
    Actions->AddChildToUniformGrid(MakeActionPill(TEXT("СЛОТ 3"), Accent), 1, 0);
    Actions->AddChildToUniformGrid(MakeActionPill(TEXT("ПРОДАТЬ"), CWMenu::MutedText), 1, 1);

    Hit->SetContent(Size);
    return Hit;
}

void UCWNativeMenuWidget::ShowDialog(const FString& Title, const FString& Subtitle, UWidget* Body, const FLinearColor& Accent)
{
    if (!DialogLayer)
    {
        return;
    }

    DialogLayer->SetVisibility(ESlateVisibility::Visible);
    DialogLayer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.68f));

    UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    DialogLayer->SetContent(Overlay);

    USizeBox* ModalSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ModalSize->SetWidthOverride(760.0f);
    ModalSize->SetMinDesiredHeight(430.0f);
    UOverlaySlot* ModalOverlaySlot = Overlay->AddChildToOverlay(ModalSize);
    ModalOverlaySlot->SetHorizontalAlignment(HAlign_Center);
    ModalOverlaySlot->SetVerticalAlignment(VAlign_Center);

    UBorder* ModalFrame = MakePanel(FLinearColor(0.06f + Accent.R * 0.09f, 0.08f + Accent.G * 0.07f, 0.10f + Accent.B * 0.08f, 0.98f), 2.0f);
    RegisterPanelFx(ModalFrame, Accent, true);
    ModalSize->SetContent(ModalFrame);
    UBorder* ModalPanel = MakePanel(FLinearColor(0.012f, 0.018f, 0.026f, 0.98f), 18.0f);
    ModalFrame->SetContent(ModalPanel);

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    ModalPanel->SetContent(Stack);

    UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Stack->AddChildToVerticalBox(Head);
    UVerticalBox* HeadCopy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* HeadCopySlot = Head->AddChildToHorizontalBox(HeadCopy);
    HeadCopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    HeadCopy->AddChildToVerticalBox(MakeKicker(TEXT("CRIMSON WARS / ACTION")));
    HeadCopy->AddChildToVerticalBox(MakeText(Title, 28.0f, CWMenu::WhiteText, true));
    HeadCopy->AddChildToVerticalBox(MakeText(Subtitle, 13.0f, CWMenu::MutedText));

    UButton* Close = MakeButton(TEXT("Закрыть"), CWMenu::MutedText);
    Close->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::HandleCloseDialogClicked);
    CWMenu::Pad(Head->AddChildToHorizontalBox(Close), 16.0f, 8.0f, 0.0f, 0.0f);

    if (Body)
    {
        CWMenu::Pad(Stack->AddChildToVerticalBox(Body), 0.0f, 18.0f, 0.0f, 0.0f);
    }
}

void UCWNativeMenuWidget::ShowItemDialog(const FString& SlotName, const FString& CurrentItem, const FString& CurrentAsset, const FLinearColor& Accent)
{
    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Body->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("Слот: %s. Выбери предмет из списка и нажми действие."), *SlotName), 14.0f, CWMenu::MutedText));
    CWMenu::Pad(Body->AddChildToVerticalBox(MakeDialogListRow(
        CurrentItem.IsEmpty() ? FString(TEXT("Пусто")) : CurrentItem,
        CurrentItem == TEXT("Пусто") ? FString(TEXT("Сейчас в слоте ничего не надето.")) : FString(TEXT("Сейчас надето. Можно снять или заменить.")),
        CurrentAsset,
        CurrentItem == TEXT("Пусто") ? FString(TEXT("НАДЕТЬ")) : FString(TEXT("СНЯТЬ")),
        Accent)), 0.0f, 12.0f, 0.0f, 8.0f);
    CWMenu::Pad(Body->AddChildToVerticalBox(MakeDialogListRow(TEXT("Корона войны"), TEXT("Легендарный · Урон +3% · Скорострельность +1.8% · HP +18"), TEXT("items/war_crown.png"), TEXT("НАДЕТЬ"), CWMenu::Amber)), 0.0f, 0.0f, 0.0f, 8.0f);
    CWMenu::Pad(Body->AddChildToVerticalBox(MakeDialogListRow(TEXT("Фазовый клинок"), TEXT("Эпический · Урон +2.6% · Скорость +1.5%"), TEXT("items/phase_blade.png"), TEXT("НАДЕТЬ"), CWMenu::Pink)), 0.0f, 0.0f, 0.0f, 8.0f);
    Body->AddChildToVerticalBox(MakeDialogListRow(TEXT("Пустой слот"), TEXT("Освободить слот и убрать бонус предмета."), FString(), TEXT("ОЧИСТИТЬ"), CWMenu::MutedText));

    ShowDialog(TEXT("Экипировка"), FString::Printf(TEXT("%s / %s"), *GetHeroUiData(SelectedHeroId).Name, *SlotName), Body, Accent);
}

void UCWNativeMenuWidget::ShowSkillDialog(const FString& SkillTitle, const FString& AssetPath, const FString& Level, const FString& Meta, const FLinearColor& Accent)
{
    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Body->AddChildToVerticalBox(MakeDialogListRow(SkillTitle, FString::Printf(TEXT("%s · %s"), *Level, *Meta), AssetPath, TEXT("УЛУЧШИТЬ"), Accent));
    CWMenu::Pad(Body->AddChildToVerticalBox(MakeDialogListRow(TEXT("Следующий уровень"), TEXT("Стоимость: 28-62 осколка. Улучшение усилит урон, радиус или перезарядку."), AssetPath, TEXT("+1 LV"), Accent)), 0.0f, 8.0f, 0.0f, 8.0f);
    Body->AddChildToVerticalBox(MakeDialogListRow(TEXT("Сбросить выбор"), TEXT("Пока заглушка для будущего дерева прокачки и подтверждения."), FString(), TEXT("ОТМЕНА"), CWMenu::MutedText));

    ShowDialog(TEXT("Прокачка умения"), FString::Printf(TEXT("%s / %s"), *GetHeroUiData(SelectedHeroId).Name, *SkillTitle), Body, Accent);
}

UWidget* UCWNativeMenuWidget::MakeHeroCard(const FString& HeroId, const FString& Name, const FString& Role, const FString& AssetPath, const FLinearColor& Accent)
{
    UBorder* Card = MakePanel(FLinearColor(0.025f, 0.03f, 0.042f, 0.88f), 12.0f);
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Card->SetContent(Box);
    Box->AddChildToVerticalBox(MakeFittedImage(AssetPath, FVector2D(190.0f, 154.0f)));
    CWMenu::Pad(Box->AddChildToVerticalBox(MakeKicker(HeroId)), 0.0f, 8.0f, 0.0f, 0.0f);
    Box->AddChildToVerticalBox(MakeText(Name, 22.0f, Accent, true));
    Box->AddChildToVerticalBox(MakeText(Role, 13.0f, CWMenu::MutedText));
    return Card;
}

UWidget* UCWNativeMenuWidget::MakeLoadoutSlot(const FString& SlotLabel, const FString& ItemName, const FLinearColor& Accent, const FString& AssetPath)
{
    UBorder* Card = MakePanel(FLinearColor(0.018f, 0.024f, 0.033f, 0.88f), 10.0f);
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Card->SetContent(Box);
    if (!AssetPath.IsEmpty())
    {
        Box->AddChildToVerticalBox(MakeSquareImage(AssetPath, 64.0f));
    }
    Box->AddChildToVerticalBox(MakeText(SlotLabel, 11.0f, Accent, true));
    Box->AddChildToVerticalBox(MakeText(ItemName, 14.0f, CWMenu::WhiteText, true));
    return Card;
}

UWidget* UCWNativeMenuWidget::MakeSettingRow(const FString& Label, const FString& Value)
{
    UBorder* Row = MakePanel(FLinearColor(0.02f, 0.026f, 0.036f, 0.78f), 10.0f);
    UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Row->SetContent(Box);
    UHorizontalBoxSlot* LabelSlot = Box->AddChildToHorizontalBox(MakeText(Label, 15.0f, CWMenu::WhiteText, true));
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Box->AddChildToHorizontalBox(MakeText(Value, 14.0f, CWMenu::MutedText));
    return Row;
}

UWidget* UCWNativeMenuWidget::MakeRecordRow(const FString& Rank, const FString& Name, const FString& Value)
{
    UBorder* Row = MakePanel(FLinearColor(0.018f, 0.024f, 0.034f, 0.78f), 9.0f);
    UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Row->SetContent(Box);
    CWMenu::Pad(Box->AddChildToHorizontalBox(MakeText(Rank, 16.0f, CWMenu::Amber, true)), 0.0f, 0.0f, 14.0f, 0.0f);
    UHorizontalBoxSlot* NameSlot = Box->AddChildToHorizontalBox(MakeText(Name, 15.0f, CWMenu::WhiteText, true));
    NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Box->AddChildToHorizontalBox(MakeText(Value, 15.0f, CWMenu::Green, true));
    return Row;
}

UWidget* UCWNativeMenuWidget::MakeNewsCard(const FString& Title, const FString& Meta, const FString& Body)
{
    UBorder* Card = MakePanel(FLinearColor(0.025f, 0.032f, 0.044f, 0.86f), 14.0f);
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Card->SetContent(Box);
    Box->AddChildToVerticalBox(MakeText(Title, 20.0f, CWMenu::WhiteText, true));
    Box->AddChildToVerticalBox(MakeText(Meta, 12.0f, CWMenu::Pink, true));
    CWMenu::Pad(Box->AddChildToVerticalBox(MakeText(Body, 14.0f, CWMenu::MutedText)), 0.0f, 8.0f, 0.0f, 0.0f);
    return Card;
}

UWidget* UCWNativeMenuWidget::MakeRunPanel()
{
    UHorizontalBox* Layout = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* LeftSlot = Layout->AddChildToHorizontalBox(Left);
    LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(LeftSlot, 0.0f, 0.0f, 14.0f, 0.0f);

    Left->AddChildToVerticalBox(MakeKicker(TEXT("RUN SETUP")));
    Left->AddChildToVerticalBox(MakeText(TEXT("Забег"), 34.0f, CWMenu::Green, true));
    Left->AddChildToVerticalBox(MakeText(TEXT("Тот же вход в матч, что в браузере: быстрый старт, код комнаты, режим, карты, список активных комнат и готовность к будущему 3D-забегу."), 15.0f, CWMenu::MutedText));

    NameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NativeNameInput"));
    NameInput->SetHintText(FText::FromString(TEXT("Никнейм")));
    NameInput->SetText(FText::FromString(TEXT("Native")));
    CWMenu::Pad(Left->AddChildToVerticalBox(NameInput), 0.0f, 14.0f, 0.0f, 8.0f);

    RoomCodeInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NativeRoomInput"));
    RoomCodeInput->SetHintText(FText::FromString(TEXT("Код комнаты, если подключаемся")));
    CWMenu::Pad(Left->AddChildToVerticalBox(RoomCodeInput), 0.0f, 0.0f, 0.0f, 10.0f);

    UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Left->AddChildToVerticalBox(Actions);
    UButton* CreateButton = MakeButton(TEXT("Создать забег"), CWMenu::Green, true);
    CreateButton->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::HandleCreateRunClicked);
    CWMenu::Pad(Actions->AddChildToHorizontalBox(CreateButton), 0.0f, 0.0f, 10.0f, 0.0f);
    UButton* JoinButton = MakeButton(TEXT("Войти по коду"), CWMenu::Amber, true);
    JoinButton->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::HandleJoinRunClicked);
    CWMenu::Pad(Actions->AddChildToHorizontalBox(JoinButton), 0.0f, 0.0f, 10.0f, 0.0f);
    UButton* ReconnectButton = MakeButton(TEXT("Reconnect WS"), CWMenu::Cyan);
    ReconnectButton->OnClicked.AddDynamic(this, &UCWNativeMenuWidget::HandleReconnectClicked);
    Actions->AddChildToHorizontalBox(ReconnectButton);

    UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CWMenu::Pad(Left->AddChildToVerticalBox(Cards), 0.0f, 16.0f, 0.0f, 0.0f);
    Cards->AddChildToHorizontalBox(MakeStatTile(TEXT("Normal"), TEXT("Co-op"), CWMenu::Green));
    Cards->AddChildToHorizontalBox(MakeStatTile(TEXT("Hardcore"), TEXT("No mercy"), CWMenu::Blood));
    Cards->AddChildToHorizontalBox(MakeStatTile(TEXT("PvP"), TEXT("Arena"), CWMenu::Violet));

    UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* RightSlot = Layout->AddChildToHorizontalBox(Right);
    RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Right->AddChildToVerticalBox(MakeKicker(TEXT("ACTIVE ROOMS")));
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (GI && GI->Bootstrap.Rooms.Num() > 0)
    {
        for (const FCWNativeRoomMenuItem& Room : GI->Bootstrap.Rooms)
        {
            Right->AddChildToVerticalBox(MakeRecordRow(
                Room.Code.IsEmpty() ? FString(TEXT("--")) : Room.Code,
                FString::Printf(TEXT("%s / %d of %d players"), *Room.GameMode, Room.Players, FMath::Max(Room.MaxPlayers, Room.Players)),
                TEXT("join")));
        }
    }
    else
    {
        Right->AddChildToVerticalBox(MakeRecordRow(TEXT("#"), TEXT("Нет активных комнат или сервер ещё загружает lobby"), TEXT("idle")));
    }

    if (GI && GI->Bootstrap.Maps.Num() > 0)
    {
        FString MapNames;
        for (int32 Index = 0; Index < GI->Bootstrap.Maps.Num(); ++Index)
        {
            if (Index > 0) MapNames += TEXT(" / ");
            MapNames += GI->Bootstrap.Maps[Index].Name;
        }
        Right->AddChildToVerticalBox(MakeRecordRow(TEXT("MAP"), MapNames, FString::FromInt(GI->Bootstrap.Maps.Num())));
    }
    else
    {
        Right->AddChildToVerticalBox(MakeRecordRow(TEXT("MAP"), TEXT("Mall Night / Clinic Yard / Ringroad / Reactor"), TEXT("4")));
    }
    Right->AddChildToVerticalBox(MakeRecordRow(TEXT("SYNC"), TEXT("tick 45 / state 30 / input 30"), GI && GI->bBootstrapLoaded ? FString(TEXT("live")) : FString(TEXT("ready"))));
    CWMenu::Pad(Right->AddChildToVerticalBox(MakeImage(TEXT("backgrounds/start-1.png"), FVector2D(520.0f, 220.0f))), 0.0f, 16.0f, 0.0f, 0.0f);

    return Layout;
}

UWidget* UCWNativeMenuWidget::MakeStoryPanel()
{
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Box->AddChildToVerticalBox(MakeKicker(TEXT("CAMPAIGN")));
    Box->AddChildToVerticalBox(MakeText(TEXT("Сюжетные операции"), 32.0f, CWMenu::Amber, true));
    Box->AddChildToVerticalBox(MakeText(TEXT("Панель кампаний повторит веб: главы, уровни, модификаторы, цели, прогресс и награды."), 15.0f, CWMenu::MutedText));
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (GI && GI->Bootstrap.Campaigns.Num() > 0)
    {
        for (int32 Index = 0; Index < GI->Bootstrap.Campaigns.Num(); ++Index)
        {
            const FCWNativeCampaignMenuItem& Campaign = GI->Bootstrap.Campaigns[Index];
            CWMenu::Pad(Box->AddChildToVerticalBox(MakeRecordRow(
                FString::Printf(TEXT("%d"), Index + 1),
                Campaign.Tagline.IsEmpty() ? Campaign.Name : FString::Printf(TEXT("%s / %s"), *Campaign.Name, *Campaign.Tagline),
                FString::Printf(TEXT("%d levels"), Campaign.LevelCount))),
                0.0f, Index == 0 ? 16.0f : 0.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        CWMenu::Pad(Box->AddChildToVerticalBox(MakeRecordRow(TEXT("I"), TEXT("Hellmart Sweep / добраться до первого босса"), TEXT("locked data"))), 0.0f, 16.0f, 0.0f, 0.0f);
        Box->AddChildToVerticalBox(MakeRecordRow(TEXT("II"), TEXT("Clinic Yard / удержать двор"), TEXT("locked data")));
        Box->AddChildToVerticalBox(MakeRecordRow(TEXT("III"), TEXT("Reactor Sprawl / токсичный фронт"), TEXT("locked data")));
    }
    return Box;
}

UWidget* UCWNativeMenuWidget::MakeCharactersPanel()
{
    const FCWHeroUiData Hero = GetHeroUiData(SelectedHeroId);
    const bool bScout = SelectedHeroId == TEXT("scout");
    const bool bCyber = SelectedHeroId == TEXT("cyber");

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Scroll->AddChild(Root);

    UBorder* RosterPanel = MakePanel(FLinearColor(0.005f, 0.008f, 0.012f, 0.92f), 0.0f);
    UVerticalBoxSlot* RosterPanelSlot = Root->AddChildToVerticalBox(RosterPanel);
    RosterPanelSlot->SetHorizontalAlignment(HAlign_Fill);
    CWMenu::Pad(RosterPanelSlot, 0.0f, 10.0f, 0.0f, 12.0f);

    UOverlay* RosterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    RosterPanel->SetContent(RosterOverlay);
    UImage* RosterBg = MakeImage(TEXT("backgrounds/screen-replay.jpg"), FVector2D(1560.0f, 286.0f));
    RosterBg->SetColorAndOpacity(FLinearColor(0.40f, 0.52f, 0.62f, 0.20f));
    RosterOverlay->AddChildToOverlay(RosterBg);

    UVerticalBox* RosterStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UOverlaySlot* RosterStackSlot = RosterOverlay->AddChildToOverlay(RosterStack);
    RosterStackSlot->SetPadding(FMargin(16.0f));
    RosterStackSlot->SetHorizontalAlignment(HAlign_Fill);
    RosterStackSlot->SetVerticalAlignment(VAlign_Fill);

    UHorizontalBox* RosterHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    RosterStack->AddChildToVerticalBox(RosterHead);
    UVerticalBox* RosterTitle = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* RosterTitleSlot = RosterHead->AddChildToHorizontalBox(RosterTitle);
    RosterTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    RosterTitle->AddChildToVerticalBox(MakeKicker(TEXT("РОСТЕР")));
    RosterTitle->AddChildToVerticalBox(MakeText(TEXT("Персонажи арены"), 23.0f, CWMenu::WhiteText, true));
    UVerticalBox* RosterMeta = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    RosterHead->AddChildToHorizontalBox(RosterMeta);
    UTextBlock* HeroMeta = MakeText(FString::Printf(TEXT("%s %s"), *Hero.Name, *Hero.Level), 13.0f, Hero.Accent, true);
    HeroMeta->SetJustification(ETextJustify::Right);
    RosterMeta->AddChildToVerticalBox(HeroMeta);
    UTextBlock* HeroUnlock = MakeText(TEXT("5/5 ОТКРЫТ"), 11.0f, CWMenu::MutedText, true);
    HeroUnlock->SetJustification(ETextJustify::Right);
    RosterMeta->AddChildToVerticalBox(HeroUnlock);

    UHorizontalBox* SelectedNameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UVerticalBoxSlot* SelectedNameSlot = RosterStack->AddChildToVerticalBox(SelectedNameRow);
    SelectedNameSlot->SetHorizontalAlignment(HAlign_Fill);
    CWMenu::Pad(SelectedNameSlot, 0.0f, 8.0f, 0.0f, 8.0f);
    SelectedNameRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UVerticalBox* SelectedCopy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    SelectedNameRow->AddChildToHorizontalBox(SelectedCopy);
    UTextBlock* SelectedKicker = MakeText(TEXT("СЕЙЧАС ВЫБРАН"), 10.0f, FLinearColor(0.74f, 1.0f, 0.78f, 1.0f), true);
    SelectedKicker->SetJustification(ETextJustify::Center);
    SelectedCopy->AddChildToVerticalBox(SelectedKicker);
    UTextBlock* SelectedTitle = MakeText(Hero.UpperName, 31.0f, CWMenu::WhiteText, true);
    SelectedTitle->SetJustification(ETextJustify::Center);
    SelectedCopy->AddChildToVerticalBox(SelectedTitle);
    UTextBlock* SelectedSub = MakeText(FString::Printf(TEXT("%s | ВЫБРАН"), *Hero.Level), 12.0f, CWMenu::WhiteText, true);
    SelectedSub->SetJustification(ETextJustify::Center);
    SelectedCopy->AddChildToVerticalBox(SelectedSub);
    SelectedNameRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UHorizontalBox* Roster = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UVerticalBoxSlot* RosterSlot = RosterStack->AddChildToVerticalBox(Roster);
    RosterSlot->SetHorizontalAlignment(HAlign_Fill);
    Roster->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(Roster->AddChildToHorizontalBox(MakeRosterCard(TEXT("01"), TEXT("cyber"), TEXT("КИБЕР"), TEXT("LV 64"), TEXT("characters/cyber.jpg"), CWMenu::Cyan, SelectedHeroId == TEXT("cyber"))), 0.0f, 0.0f, 12.0f, 0.0f);
    CWMenu::Pad(Roster->AddChildToHorizontalBox(MakeRosterCard(TEXT("02"), TEXT("scout"), TEXT("СКАУТ"), TEXT("LV 6"), TEXT("characters/scout.jpg"), CWMenu::Green, SelectedHeroId == TEXT("scout"))), 0.0f, 0.0f, 12.0f, 0.0f);
    CWMenu::Pad(Roster->AddChildToHorizontalBox(MakeRosterCard(TEXT("03"), TEXT("shadow"), TEXT("ТЕНЬ"), TEXT("LV 28"), TEXT("characters/shadow.jpg"), CWMenu::Violet, SelectedHeroId == TEXT("shadow"))), 0.0f, 0.0f, 12.0f, 0.0f);
    CWMenu::Pad(Roster->AddChildToHorizontalBox(MakeRosterCard(TEXT("04"), TEXT("medic"), TEXT("МЕДИК"), TEXT("LV 4"), TEXT("characters/medis.jpg"), FLinearColor(0.42f, 1.0f, 0.68f, 1.0f), SelectedHeroId == TEXT("medic"))), 0.0f, 0.0f, 12.0f, 0.0f);
    CWMenu::Pad(Roster->AddChildToHorizontalBox(MakeRosterCard(TEXT("05"), TEXT("raider"), TEXT("РЕЙДЕР"), TEXT("LV 4"), TEXT("characters/raider.jpg"), CWMenu::Blood, SelectedHeroId == TEXT("raider"))), 0.0f, 0.0f, 12.0f, 0.0f);
    Roster->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UHorizontalBox* CharacterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Root->AddChildToVerticalBox(CharacterRow);

    USizeBox* DossierSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    DossierSize->SetWidthOverride(520.0f);
    UHorizontalBoxSlot* DossierOuterSlot = CharacterRow->AddChildToHorizontalBox(DossierSize);
    CWMenu::Pad(DossierOuterSlot, 0.0f, 0.0f, 12.0f, 0.0f);

    UBorder* DossierFrame = MakePanel(FLinearColor(0.035f + Hero.Accent.R * 0.03f, 0.08f + Hero.Accent.G * 0.06f, 0.10f + Hero.Accent.B * 0.08f, 0.95f), 2.0f);
    RegisterPanelFx(DossierFrame, Hero.Accent, false);
    DossierSize->SetContent(DossierFrame);
    UBorder* DossierPanel = MakePanel(FLinearColor(0.015f, 0.026f, 0.038f, 0.94f), 12.0f);
    DossierFrame->SetContent(DossierPanel);
    UVerticalBox* DossierStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    DossierPanel->SetContent(DossierStack);

    UHorizontalBox* DossierHeader = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    DossierStack->AddChildToVerticalBox(DossierHeader);
    CWMenu::Pad(DossierHeader->AddChildToHorizontalBox(MakePortraitCrop(Hero.Asset, FVector2D(92.0f, 92.0f), -16.0f)), 0.0f, 0.0f, 12.0f, 0.0f);

    UVerticalBox* DossierTitle = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* DossierTitleSlot = DossierHeader->AddChildToHorizontalBox(DossierTitle);
    DossierTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DossierTitle->AddChildToVerticalBox(MakeKicker(TEXT("ТАКТИЧЕСКОЕ ДОСЬЕ")));
    DossierTitle->AddChildToVerticalBox(MakeText(Hero.Name, 25.0f, CWMenu::WhiteText, true));
    DossierTitle->AddChildToVerticalBox(MakeText(Hero.Role, 12.0f, CWMenu::MutedText));
    CWMenu::Pad(DossierHeader->AddChildToHorizontalBox(MakeProfileMetric(TEXT("СТАТУС"), TEXT("ОТКРЫТ"), CWMenu::Green)), 12.0f, 8.0f, 0.0f, 0.0f);

    CWMenu::Pad(DossierStack->AddChildToVerticalBox(MakeProgressBar(bCyber ? 0.94f : (bScout ? 0.49f : 0.48f), FLinearColor(1.0f, 0.61f, 0.14f, 1.0f), 8.0f)), 0.0f, 10.0f, 0.0f, 4.0f);
    DossierStack->AddChildToVerticalBox(MakeText(Hero.XpLine, 11.0f, CWMenu::WhiteText, true));

    UUniformGridPanel* DossierMetrics = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    CWMenu::Pad(DossierStack->AddChildToVerticalBox(DossierMetrics), 0.0f, 12.0f, 0.0f, 12.0f);
    DossierMetrics->AddChildToUniformGrid(MakeProfileMetric(TEXT("ТАЛАНТЫ"), Hero.Talents, CWMenu::Cyan), 0, 0);
    DossierMetrics->AddChildToUniformGrid(MakeProfileMetric(TEXT("HERO SKILLS"), Hero.Skills, CWMenu::Violet), 0, 1);
    DossierMetrics->AddChildToUniformGrid(MakeProfileMetric(TEXT("СНАРЯЖЕНИЕ"), Hero.Gear, CWMenu::Amber), 0, 2);

    DossierStack->AddChildToVerticalBox(MakeKicker(TEXT("ХАРАКТЕРИСТИКИ")));
    UUniformGridPanel* Stats = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    CWMenu::Pad(DossierStack->AddChildToVerticalBox(Stats), 0.0f, 8.0f, 0.0f, 12.0f);
    Stats->AddChildToUniformGrid(MakeProfileMetric(TEXT("POW"), Hero.Pow, CWMenu::Cyan), 0, 0);
    Stats->AddChildToUniformGrid(MakeProfileMetric(TEXT("AGI"), Hero.Agi, CWMenu::Amber), 0, 1);
    Stats->AddChildToUniformGrid(MakeProfileMetric(TEXT("VIT"), Hero.Vit, FLinearColor(0.72f, 1.0f, 0.82f, 1.0f)), 0, 2);
    Stats->AddChildToUniformGrid(MakeProfileMetric(TEXT("TEC"), Hero.Tec, CWMenu::Pink), 0, 3);

    DossierStack->AddChildToVerticalBox(MakeKicker(TEXT("БОЕВЫЕ БОНУСЫ")));
    UUniformGridPanel* Bonuses = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    CWMenu::Pad(DossierStack->AddChildToVerticalBox(Bonuses), 0.0f, 8.0f, 0.0f, 12.0f);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Урон"), bScout ? TEXT("+38%") : TEXT("+320%"), CWMenu::Pink), 0, 0);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Скорострельность"), bScout ? TEXT("+25%") : TEXT("+120%"), CWMenu::Amber), 0, 1);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Перезарядка"), TEXT("0%"), CWMenu::MutedText), 1, 0);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Скорость"), bScout ? TEXT("+55%") : TEXT("+199%"), CWMenu::Cyan), 1, 1);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Здоровье"), bScout ? TEXT("+74") : TEXT("+242"), FLinearColor(0.62f, 1.0f, 0.78f, 1.0f)), 2, 0);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Реген"), bScout ? TEXT("+0.69/s") : TEXT("+1.35/s"), FLinearColor(0.62f, 1.0f, 0.78f, 1.0f)), 2, 1);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Подбор"), bScout ? TEXT("+22") : TEXT("+113"), CWMenu::Cyan), 3, 0);
    Bonuses->AddChildToUniformGrid(MakeBonusRow(TEXT("Рывки"), TEXT("0"), CWMenu::MutedText), 3, 1);

    DossierStack->AddChildToVerticalBox(MakeKicker(TEXT("ОТКУДА ИДУТ БОНУСЫ")));
    DossierStack->AddChildToVerticalBox(MakeBonusRow(TEXT("ГЕРОЙ + УРОВЕНЬ"), bScout ? TEXT("Урон +26% | Скорострельность +25% | Скорость +43%") : TEXT("Урон +292% | Скорострельность +103% | Скорость +195%"), CWMenu::Cyan));
    DossierStack->AddChildToVerticalBox(MakeBonusRow(TEXT("ТАЛАНТЫ"), bScout ? TEXT("Скорость +8% | Скорострельность +5%") : TEXT("Урон +15% | Скорострельность +15% | Здоровье +40"), CWMenu::Amber));
    DossierStack->AddChildToVerticalBox(MakeBonusRow(TEXT("ПАССИВНЫЕ НАВЫКИ"), TEXT("нет"), CWMenu::Violet));
    DossierStack->AddChildToVerticalBox(MakeBonusRow(TEXT("АУРЫ ОТРЯДА"), bScout ? TEXT("Поддержка от других героев: нет") : TEXT("Урон +7.2% | Здоровье +24 | Реген +0.48/s"), CWMenu::Cyan));
    DossierStack->AddChildToVerticalBox(MakeBonusRow(TEXT("СНАРЯЖЕНИЕ"), bScout ? TEXT("нет экипированных предметов") : TEXT("Урон +5.6% | Скорострельность +1.8% | Скорость +3.7%"), CWMenu::Amber));

    UBorder* TalentsFrame = MakePanel(FLinearColor(0.012f, 0.018f, 0.028f, 0.94f), 12.0f);
    RegisterPanelFx(TalentsFrame, CWMenu::Amber, false);
    CWMenu::Pad(DossierStack->AddChildToVerticalBox(TalentsFrame), 0.0f, 16.0f, 0.0f, 0.0f);
    UVerticalBox* TalentsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    TalentsFrame->SetContent(TalentsStack);
    TalentsStack->AddChildToVerticalBox(MakeText(TEXT("Таланты героя"), 18.0f, CWMenu::WhiteText, true));
    TalentsStack->AddChildToVerticalBox(MakeText(TEXT("Пассивные улучшения аккаунта для выбранного героя."), 11.0f, CWMenu::MutedText));
    const TArray<FCWTalentUiData> TalentItems = GetHeroTalents(SelectedHeroId);
    for (int32 TalentIndex = 0; TalentIndex < TalentItems.Num(); ++TalentIndex)
    {
        const FCWTalentUiData& Talent = TalentItems[TalentIndex];
        CWMenu::Pad(TalentsStack->AddChildToVerticalBox(MakeTalentRow(Talent.Title, Talent.Meta, Talent.Level, Talent.Icon, Talent.Accent)), 0.0f, TalentIndex == 0 ? 10.0f : 0.0f, 0.0f, TalentIndex + 1 < TalentItems.Num() ? 8.0f : 0.0f);
    }

    UBorder* EquipmentFrame = MakePanel(FLinearColor(0.045f, 0.14f, 0.18f, 0.95f), 2.0f);
    RegisterPanelFx(EquipmentFrame, CWMenu::Cyan, false);
    UHorizontalBoxSlot* EquipmentSlot = CharacterRow->AddChildToHorizontalBox(EquipmentFrame);
    EquipmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(EquipmentSlot, 0.0f, 0.0f, 12.0f, 0.0f);
    UBorder* EquipmentPanel = MakePanel(FLinearColor(0.012f, 0.019f, 0.026f, 0.94f), 12.0f);
    EquipmentFrame->SetContent(EquipmentPanel);

    UVerticalBox* EquipmentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    EquipmentPanel->SetContent(EquipmentStack);
    UHorizontalBox* EquipmentHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    EquipmentStack->AddChildToVerticalBox(EquipmentHead);
    UVerticalBox* EquipmentTitle = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* EquipmentTitleSlot = EquipmentHead->AddChildToHorizontalBox(EquipmentTitle);
    EquipmentTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    EquipmentTitle->AddChildToVerticalBox(MakeText(TEXT("Экипировка"), 20.0f, CWMenu::WhiteText, true));
    EquipmentTitle->AddChildToVerticalBox(MakeText(TEXT("Слоты экипировки расположены вокруг героя, ниже инвентарь и боевые расходники."), 12.0f, CWMenu::MutedText));
    EquipmentHead->AddChildToHorizontalBox(MakeText(TEXT("Лом: 15"), 12.0f, CWMenu::MutedText, true));

    UHorizontalBox* PaperRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CWMenu::Pad(EquipmentStack->AddChildToVerticalBox(PaperRow), 0.0f, 12.0f, 0.0f, 0.0f);
    UVerticalBox* LeftSlots = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* LeftSlotsSlot = PaperRow->AddChildToHorizontalBox(LeftSlots);
    LeftSlotsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(LeftSlotsSlot, 0.0f, 74.0f, 10.0f, 0.0f);
    CWMenu::Pad(LeftSlots->AddChildToVerticalBox(MakeEquipmentSlot(
        TEXT("LH"),
        TEXT("ЛЕВАЯ РУКА"),
        CWMenu::Pink,
        bCyber ? FString(TEXT("items/phase_blade.png")) : FString(),
        bCyber ? FString(TEXT("Фазовый клинок")) : FString(),
        bCyber ? FString(TEXT("Эпический · Lv 1")) : FString(),
        bCyber ? FString(TEXT("Даёт: Урон +2.6% · Скорость +1.5%")) : FString(),
        bCyber ? FString(TEXT("СНЯТЬ")) : FString(),
        FName(TEXT("left_hand")))), 0.0f, 0.0f, 0.0f, 12.0f);
    LeftSlots->AddChildToVerticalBox(MakeEquipmentSlot(TEXT("AR"), TEXT("БРОНЯ"), CWMenu::Cyan, FString(), FString(), FString(), FString(), FString(), FName(TEXT("armor"))));

    UVerticalBox* CenterPaper = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* CenterPaperSlot = PaperRow->AddChildToHorizontalBox(CenterPaper);
    CenterPaperSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(CenterPaperSlot, 0.0f, 0.0f, 10.0f, 0.0f);
    CenterPaper->AddChildToVerticalBox(MakeEquipmentSlot(
        TEXT("HD"),
        TEXT("ГОЛОВА"),
        CWMenu::Amber,
        bCyber ? FString(TEXT("items/war_crown.png")) : FString(),
        bCyber ? FString(TEXT("Корона войны")) : FString(),
        bCyber ? FString(TEXT("Легендарный · Lv 1")) : FString(),
        bCyber ? FString(TEXT("Даёт: Урон +3% · Скорострельность +1.8% · HP +18")) : FString(),
        bCyber ? FString(TEXT("СНЯТЬ")) : FString(),
        FName(TEXT("head"))));
    CWMenu::Pad(CenterPaper->AddChildToVerticalBox(MakePortraitCrop(Hero.Asset, FVector2D(270.0f, 330.0f), 0.0f)), 0.0f, 14.0f, 0.0f, 0.0f);
    CenterPaper->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("%s                         %s/999"), *Hero.Name, *Hero.Level), 12.0f, CWMenu::WhiteText, true));

    UVerticalBox* RightSlots = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* RightSlotsSlot = PaperRow->AddChildToHorizontalBox(RightSlots);
    RightSlotsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(RightSlots->AddChildToVerticalBox(MakeEquipmentSlot(TEXT("RH"), TEXT("ПРАВАЯ РУКА"), CWMenu::Pink, FString(), FString(), FString(), FString(), FString(), FName(TEXT("right_hand")))), 0.0f, 74.0f, 0.0f, 12.0f);
    CWMenu::Pad(RightSlots->AddChildToVerticalBox(MakeEquipmentSlot(TEXT("RG"), TEXT("КОЛЬЦО 1"), CWMenu::Amber, FString(), FString(), FString(), FString(), FString(), FName(TEXT("ring_1")))), 0.0f, 0.0f, 0.0f, 12.0f);
    RightSlots->AddChildToVerticalBox(MakeEquipmentSlot(TEXT("RG"), TEXT("КОЛЬЦО 2"), CWMenu::Amber, FString(), FString(), FString(), FString(), FString(), FName(TEXT("ring_2"))));

    UBorder* ConsumablesFrame = MakePanel(FLinearColor(0.012f, 0.018f, 0.03f, 0.92f), 10.0f);
    RegisterPanelFx(ConsumablesFrame, CWMenu::Cyan, false);
    CWMenu::Pad(EquipmentStack->AddChildToVerticalBox(ConsumablesFrame), 0.0f, 18.0f, 0.0f, 0.0f);
    UVerticalBox* ConsumablesStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    ConsumablesFrame->SetContent(ConsumablesStack);
    ConsumablesStack->AddChildToVerticalBox(MakeKicker(TEXT("БОЕВЫЕ РАСХОДНИКИ")));
    UHorizontalBox* QuickSlots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CWMenu::Pad(ConsumablesStack->AddChildToVerticalBox(QuickSlots), 0.0f, 8.0f, 0.0f, 0.0f);
    UHorizontalBoxSlot* Quick1 = QuickSlots->AddChildToHorizontalBox(MakeEquipmentSlot(TEXT("FX"), TEXT("БЫСТРЫЙ СЛОТ 1"), CWMenu::Cyan, FString(), FString(), FString(), FString(), FString(), FName(TEXT("right_hand"))));
    Quick1->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(Quick1, 0.0f, 0.0f, 10.0f, 0.0f);
    UHorizontalBoxSlot* Quick2 = QuickSlots->AddChildToHorizontalBox(MakeEquipmentSlot(TEXT("FX"), TEXT("БЫСТРЫЙ СЛОТ 2"), CWMenu::Amber, FString(), FString(), FString(), FString(), FString(), FName(TEXT("ring_1"))));
    Quick2->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(Quick2, 0.0f, 0.0f, 10.0f, 0.0f);
    UHorizontalBoxSlot* Quick3 = QuickSlots->AddChildToHorizontalBox(MakeEquipmentSlot(TEXT("FX"), TEXT("БЫСТРЫЙ СЛОТ 3"), CWMenu::Violet, FString(), FString(), FString(), FString(), FString(), FName(TEXT("ring_2"))));
    Quick3->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UBorder* InventoryFrame = MakePanel(FLinearColor(0.010f, 0.016f, 0.026f, 0.94f), 12.0f);
    RegisterPanelFx(InventoryFrame, CWMenu::Violet, false);
    CWMenu::Pad(EquipmentStack->AddChildToVerticalBox(InventoryFrame), 0.0f, 16.0f, 0.0f, 0.0f);
    UVerticalBox* InventoryStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    InventoryFrame->SetContent(InventoryStack);
    UHorizontalBox* InventoryHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    InventoryStack->AddChildToVerticalBox(InventoryHead);
    UVerticalBox* InventoryTitle = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* InventoryTitleSlot = InventoryHead->AddChildToHorizontalBox(InventoryTitle);
    InventoryTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    InventoryTitle->AddChildToVerticalBox(MakeText(TEXT("Инвентарь"), 18.0f, CWMenu::WhiteText, true));
    InventoryTitle->AddChildToVerticalBox(MakeText(TEXT("Выбирайте предметы, снаряжайте их в слот, улучшайте или продавайте прямо здесь."), 11.0f, CWMenu::MutedText));
    InventoryHead->AddChildToHorizontalBox(MakeText(TEXT("106"), 11.0f, CWMenu::MutedText, true));

    UHorizontalBox* Filters = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CWMenu::Pad(InventoryStack->AddChildToVerticalBox(Filters), 0.0f, 12.0f, 0.0f, 12.0f);
    const TArray<FString> FilterItems = { TEXT("ВСЕ"), TEXT("РАСХОДНИКИ"), TEXT("ШАПКА"), TEXT("ОРУЖИЕ"), TEXT("БРОНЯ"), TEXT("ШТАНЫ"), TEXT("КОЛЬЦА"), TEXT("ПРОЧЕЕ") };
    for (const FString& Filter : FilterItems)
    {
        CWMenu::Pad(Filters->AddChildToHorizontalBox(MakeButton(Filter, Filter == TEXT("ВСЕ") ? CWMenu::Cyan : CWMenu::MutedText)), 0.0f, 0.0f, 8.0f, 0.0f);
    }

    InventoryStack->AddChildToVerticalBox(MakeKicker(TEXT("РАСХОДНИКИ")));
    UUniformGridPanel* InventoryGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    CWMenu::Pad(InventoryStack->AddChildToVerticalBox(InventoryGrid), 0.0f, 8.0f, 0.0f, 0.0f);
    InventoryGrid->AddChildToUniformGrid(MakeInventoryCard(TEXT("Стим-пак"), TEXT("Редкий · Быстрый слот"), TEXT("items/stim_pack.png"), CWMenu::Cyan), 0, 0);
    InventoryGrid->AddChildToUniformGrid(MakeInventoryCard(TEXT("Бинты"), TEXT("Обычный · Быстрый слот"), TEXT("items/bandage.png"), CWMenu::Amber), 0, 1);
    InventoryGrid->AddChildToUniformGrid(MakeInventoryCard(TEXT("Рой дронов"), TEXT("Эпический · Быстрый слот"), TEXT("items/drone_swarm.png"), CWMenu::Violet), 0, 2);
    InventoryGrid->AddChildToUniformGrid(MakeInventoryCard(TEXT("Орбитальный маркер"), TEXT("Легендарный · Быстрый слот"), TEXT("items/orbital_marker.png"), CWMenu::Amber), 1, 0);
    InventoryGrid->AddChildToUniformGrid(MakeInventoryCard(TEXT("Реген-инъектор"), TEXT("Эпический · Быстрый слот"), TEXT("items/regen_injector.png"), CWMenu::Green), 1, 1);
    InventoryGrid->AddChildToUniformGrid(MakeInventoryCard(TEXT("Адреналиновый укол"), TEXT("Эпический · Быстрый слот"), TEXT("items/adrenaline_shot.png"), CWMenu::Pink), 1, 2);

    USizeBox* UniqueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    UniqueSize->SetWidthOverride(540.0f);
    CharacterRow->AddChildToHorizontalBox(UniqueSize);
    UBorder* UniqueFrame = MakePanel(FLinearColor(0.05f, 0.17f, 0.22f, 0.96f), 2.0f);
    RegisterPanelFx(UniqueFrame, CWMenu::Cyan, false);
    UniqueSize->SetContent(UniqueFrame);
    UBorder* UniquePanel = MakePanel(FLinearColor(0.012f, 0.02f, 0.032f, 0.94f), 14.0f);
    UniqueFrame->SetContent(UniquePanel);
    UVerticalBox* UniqueStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UniquePanel->SetContent(UniqueStack);
    UniqueStack->AddChildToVerticalBox(MakeText(TEXT("Уникальные навыки"), 20.0f, CWMenu::WhiteText, true));
    UniqueStack->AddChildToVerticalBox(MakeText(TEXT("4 активных и 3 пассивных. Аурные пассивки усиливают других героев."), 13.0f, CWMenu::MutedText));
    if (bScout)
    {
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Бритвенный ветер (Активный)"), TEXT("Быстро разрезает ближайших врагов · Урон 25 · Радиус 340"), TEXT("0/10  +"), TEXT("hero-skills/scout_razor_wind.png"), CWMenu::Cyan)), 0.0f, 10.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Метка охотника (Активный)"), TEXT("Мгновенно пронзает врагов · Урон 36 · Радиус 390"), TEXT("0/9  +"), TEXT("hero-skills/scout_hunter_mark.png"), CWMenu::Cyan)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Штормовая сеть (Активный)"), TEXT("Электрическая цепь · Урон 43 · Радиус 380 · Цели 4"), TEXT("1/8  +"), TEXT("hero-skills/scout_storm_net.png"), CWMenu::Violet)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Небесные преследователи (Активный)"), TEXT("Быстрые ракеты · Урон 32 · Радиус 1600 · Цели 5"), TEXT("1/7  +"), TEXT("hero-skills/scout_sky_chasers.png"), CWMenu::Amber)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Длинный шаг (Пассивный)"), TEXT("+скорость движения ур. 1 +2.8%"), TEXT("0/10  +"), TEXT("hero-skills/scout_long_stride.png"), CWMenu::MutedText)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Острое зрение (Пассивный)"), TEXT("+точность и ритм огня · Урон +2%"), TEXT("0/10  +"), TEXT("hero-skills/scout_vital_sight.png"), CWMenu::Cyan)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Прокладыватель пути (Пассивная аура)"), TEXT("Скорость других героев +1.8% · Подбор +8"), TEXT("0/10  +"), TEXT("hero-skills/scout_trailblazer.png"), CWMenu::Violet)), 0.0f, 0.0f, 0.0f, 8.0f);
        UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Энергетик в капельницу (Пассивный)"), TEXT("Перезарядка +3.2% · Скорость движения +2%"), TEXT("0/10  +"), TEXT("hero-skills/scout_energy_drink_iv.png"), CWMenu::Cyan));
    }
    else
    {
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Импульсная волна (Активный)"), TEXT("Урон 100 · Радиус 234 · Перезарядка 4.26s"), TEXT("5/10  +"), TEXT("hero-skills/cyber_pulse_wave.png"), CWMenu::Cyan)), 0.0f, 10.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Ионное копьё (Активный)"), TEXT("Урон 42 · Радиус 340 · Цели 2 · Перезарядка 2.4s"), TEXT("1/8  +"), TEXT("hero-skills/cyber_ion_lance.png"), CWMenu::Violet)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Дуговая матрица (Активный)"), TEXT("Урон 48 · Радиус 360 · Цели 3 · Перезарядка 5.6s"), TEXT("1/10  +"), TEXT("hero-skills/cyber_arc_matrix.png"), CWMenu::Violet)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Протокол наведения (Активный)"), TEXT("Урон 47 · Радиус 1570 · Цели 5 · Перезарядка 6.84s"), TEXT("2/7  +"), TEXT("hero-skills/cyber_seeker_protocol.png"), CWMenu::Amber)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Адаптивная рама (Пассивный)"), TEXT("+макс. HP и устойчивость ур. 1 +14"), TEXT("0/10  +"), TEXT("hero-skills/cyber_adaptive_frame.png"), CWMenu::MutedText)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Боевая прошивка (Пассивный)"), TEXT("Урон +3.8% · Скорострельность +1.8%"), TEXT("0/10  +"), TEXT("hero-skills/cyber_combat_firmware.png"), CWMenu::Cyan)), 0.0f, 0.0f, 0.0f, 8.0f);
        CWMenu::Pad(UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Синхро-связь (Пассивная аура)"), TEXT("Урон отряда +2% · Скорострельность +1.2%"), TEXT("0/10  +"), TEXT("hero-skills/cyber_sync_link.png"), CWMenu::Violet)), 0.0f, 0.0f, 0.0f, 8.0f);
        UniqueStack->AddChildToVerticalBox(MakeSkillListRow(TEXT("Магнитный подбор (Пассивный)"), TEXT("Перезарядка +2.8% · Подбор +6"), TEXT("0/10  +"), TEXT("hero-talents/cyber_magnet.png"), CWMenu::Cyan));
    }

    return Scroll;
}

UWidget* UCWNativeMenuWidget::MakeSkillsPanel()
{
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Root->AddChildToVerticalBox(MakeKicker(TEXT("SKILLS / TALENTS")));
    Root->AddChildToVerticalBox(MakeText(TEXT("Навыки героя"), 32.0f, CWMenu::Violet, true));
    Root->AddChildToVerticalBox(MakeText(TEXT("Здесь будет дерево талантов героя, уникальные навыки и боевые выборы уровня. Сейчас закладываем визуальный ритм под web-версию."), 15.0f, CWMenu::MutedText));

    UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
    CWMenu::Pad(Root->AddChildToVerticalBox(Grid), 0.0f, 18.0f, 0.0f, 0.0f);
    const TArray<FString> Nodes = { TEXT("DMG"), TEXT("HP"), TEXT("SPD"), TEXT("SYN"), TEXT("GST"), TEXT("ECL"), TEXT("TOX"), TEXT("SUP"), TEXT("BRK") };
    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        Grid->AddChildToUniformGrid(MakeStatTile(TEXT("Node"), Nodes[Index], Index % 2 == 0 ? CWMenu::Violet : CWMenu::Cyan), Index / 3, Index % 3);
    }
    return Root;
}

UWidget* UCWNativeMenuWidget::MakeProfilePanel()
{
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Root->AddChildToVerticalBox(MakeKicker(TEXT("PROFILE / AUTH")));
    Root->AddChildToVerticalBox(MakeText(TEXT("Профиль"), 32.0f, FLinearColor(0.48f, 0.58f, 1.0f, 1.0f), true));
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Гость"), TEXT("быстрый вход без аккаунта")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Логин / регистрация"), TEXT("следующий шаг: HTTP API профиля")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("История забегов"), TEXT("records + replays")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Герои профиля"), GI && GI->bBootstrapLoaded
        ? FString::Printf(TEXT("%d heroes / %d items in native catalog"), GI->Bootstrap.Heroes.Num(), GI->Bootstrap.Items.Num())
        : FString(TEXT("уровни, карты, кампания"))));
    return Root;
}

UWidget* UCWNativeMenuWidget::MakeRatingPanel()
{
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Root->AddChildToVerticalBox(MakeKicker(TEXT("LEADERBOARD")));
    Root->AddChildToVerticalBox(MakeText(TEXT("Рейтинг игроков"), 32.0f, FLinearColor(0.72f, 0.34f, 1.0f, 1.0f), true));
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (GI && GI->Bootstrap.Leaderboard.Num() > 0)
    {
        for (const FCWNativeLeaderboardMenuItem& Row : GI->Bootstrap.Leaderboard)
        {
            Root->AddChildToVerticalBox(MakeRecordRow(
                FString::Printf(TEXT("#%d"), Row.Rank),
                Row.Name.IsEmpty() ? FString(TEXT("Player")) : Row.Name,
                Row.Value.IsEmpty() ? FString(TEXT("-")) : Row.Value));
        }
    }
    else
    {
        Root->AddChildToVerticalBox(MakeRecordRow(TEXT("#1"), TEXT("Global profile index"), TEXT("idx")));
        Root->AddChildToVerticalBox(MakeRecordRow(TEXT("#2"), TEXT("Best kills run"), TEXT("kills")));
        Root->AddChildToVerticalBox(MakeRecordRow(TEXT("#3"), TEXT("Best PvP kills"), TEXT("pvp")));
        Root->AddChildToVerticalBox(MakeRecordRow(TEXT("#4"), TEXT("Best DPS / score / time"), TEXT("runs")));
    }
    return Root;
}

UWidget* UCWNativeMenuWidget::MakeNewsPanel()
{
    UHorizontalBox* Layout = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBoxSlot* LeftSlot = Layout->AddChildToHorizontalBox(Left);
    LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CWMenu::Pad(LeftSlot, 0.0f, 0.0f, 14.0f, 0.0f);
    Left->AddChildToVerticalBox(MakeKicker(TEXT("NEWS")));
    Left->AddChildToVerticalBox(MakeText(TEXT("Новости"), 32.0f, CWMenu::Pink, true));
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (GI && GI->Bootstrap.News.Num() > 0)
    {
        for (const FCWNativeNewsMenuItem& Item : GI->Bootstrap.News)
        {
            Left->AddChildToVerticalBox(MakeNewsCard(
                Item.Title,
                Item.Kind.IsEmpty() ? FString(TEXT("news")) : Item.Kind,
                Item.Summary.IsEmpty() ? FString(TEXT("Новость загружена с сервера Crimson Wars.")) : Item.Summary));
        }
    }
    else
    {
        Left->AddChildToVerticalBox(MakeNewsCard(TEXT("Native client online shell"), TEXT("devlog / Unreal"), TEXT("Собираем нативный hub, затем перенесем забег и боевку.")));
        Left->AddChildToVerticalBox(MakeNewsCard(TEXT("Ragdoll pipeline"), TEXT("monsters / bosses"), TEXT("Следующий большой пласт: скелеты, физические ассеты, смерть и одежда на персонажах.")));
    }
    const FString NewsArt = GI && !GI->Bootstrap.HeroesBackground.IsEmpty()
        ? GI->Bootstrap.HeroesBackground
        : FString(TEXT("other/heroes-v2.jpg"));
    Layout->AddChildToHorizontalBox(MakeImage(NewsArt, FVector2D(520.0f, 420.0f)));
    return Layout;
}

UWidget* UCWNativeMenuWidget::MakeSettingsPanel()
{
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Root->AddChildToVerticalBox(MakeKicker(TEXT("SETTINGS")));
    Root->AddChildToVerticalBox(MakeText(TEXT("Настройки"), 32.0f, FLinearColor(0.68f, 0.74f, 0.82f, 1.0f), true));
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Graphics"), TEXT("Quality / shadows / tracers / effects")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Audio"), TEXT("SFX volume / game sounds / live audio")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("HUD"), TEXT("Minimap / enemy HP / FPS / chat")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Network"), GI ? GI->DefaultServerUrl : TEXT("Server URL / input Hz / sync diagnostics")));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Bootstrap"), GI && GI->bBootstrapLoaded ? GI->ResolveHttpBaseUrl() : FString(TEXT("waiting for /api/native/bootstrap"))));
    Root->AddChildToVerticalBox(MakeSettingRow(TEXT("Assets"), GetAssetsRoot()));
    return Root;
}

FString UCWNativeMenuWidget::GetAssetsRoot() const
{
    const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    return GI ? GI->WebAssetsRoot : TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
}

FString UCWNativeMenuWidget::ToLocalAssetPath(const FString& AssetPath) const
{
    FString Path = AssetPath.TrimStartAndEnd();
    if (Path.IsEmpty())
    {
        return Path;
    }

    Path.ReplaceInline(TEXT("\\"), TEXT("/"));

    if (Path.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) || Path.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase))
    {
        int32 AssetsIndex = INDEX_NONE;
        if (Path.FindChar(TEXT('?'), AssetsIndex))
        {
            Path = Path.Left(AssetsIndex);
        }
        AssetsIndex = Path.Find(TEXT("/assets/"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
        if (AssetsIndex != INDEX_NONE)
        {
            Path = Path.Mid(AssetsIndex + 8);
        }
    }

    if (Path.StartsWith(TEXT("/assets/"), ESearchCase::IgnoreCase))
    {
        Path = Path.Mid(8);
    }
    else if (Path.StartsWith(TEXT("assets/"), ESearchCase::IgnoreCase))
    {
        Path = Path.Mid(7);
    }
    else if (Path.StartsWith(TEXT("/")))
    {
        Path = Path.Mid(1);
    }

    Path.ReplaceInline(TEXT("%20"), TEXT(" "));
    return Path;
}

UTexture2D* UCWNativeMenuWidget::LoadTexture(const FString& RelativeAssetPath)
{
    if (UTexture2D** Cached = TextureCache.Find(RelativeAssetPath))
    {
        return *Cached;
    }

    UTexture2D* Texture = FCWNativeAssetLibrary::LoadTexture(this, GetAssetsRoot(), RelativeAssetPath);
    TextureCache.Add(RelativeAssetPath, Texture);
    return Texture;
}
