#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CWNativeMenuWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UHorizontalBox;
class UImage;
class UOverlay;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
struct FCWNativeBootstrapSnapshot;

struct FCWNativeButtonFx
{
    TWeakObjectPtr<UButton> Button;
    FLinearColor Accent = FLinearColor::White;
    bool bPrimary = false;
};

struct FCWNativePanelFx
{
    TWeakObjectPtr<UBorder> Panel;
    FLinearColor Accent = FLinearColor::White;
    bool bSelected = false;
    float Phase = 0.0f;
};

UCLASS()
class CRIMSONWARSNATIVE_API UCWNativeMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Menu")
    void ShowMenu();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Menu")
    void HideMenu();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|Menu")
    void ToggleMenu();

    UFUNCTION(BlueprintPure, Category = "Crimson Wars|Menu")
    bool IsMenuShown() const;

private:
    UFUNCTION()
    void SelectRunTab();

    UFUNCTION()
    void SelectStoryTab();

    UFUNCTION()
    void SelectCharactersTab();

    UFUNCTION()
    void SelectSkillsTab();

    UFUNCTION()
    void SelectProfileTab();

    UFUNCTION()
    void SelectRatingTab();

    UFUNCTION()
    void SelectNewsTab();

    UFUNCTION()
    void SelectSettingsTab();

    UFUNCTION()
    void HandleCreateRunClicked();

    UFUNCTION()
    void HandleJoinRunClicked();

    UFUNCTION()
    void HandleReconnectClicked();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleCloseDialogClicked();

    UFUNCTION()
    void HandleDialogActionClicked();

    UFUNCTION()
    void SelectCyberHero();

    UFUNCTION()
    void SelectScoutHero();

    UFUNCTION()
    void SelectShadowHero();

    UFUNCTION()
    void SelectMedicHero();

    UFUNCTION()
    void SelectRaiderHero();

    UFUNCTION()
    void OpenHeadSlotDialog();

    UFUNCTION()
    void OpenLeftHandSlotDialog();

    UFUNCTION()
    void OpenArmorSlotDialog();

    UFUNCTION()
    void OpenRightHandSlotDialog();

    UFUNCTION()
    void OpenRingOneSlotDialog();

    UFUNCTION()
    void OpenRingTwoSlotDialog();

    UFUNCTION()
    void OpenPulseSkillDialog();

    UFUNCTION()
    void OpenIonSkillDialog();

    UFUNCTION()
    void OpenArcSkillDialog();

    UFUNCTION()
    void OpenProtocolSkillDialog();

    UFUNCTION()
    void OpenPassiveSkillDialog();

    UFUNCTION()
    void OpenScoutRazorSkillDialog();

    UFUNCTION()
    void OpenScoutHunterSkillDialog();

    UFUNCTION()
    void OpenScoutStormSkillDialog();

    UFUNCTION()
    void OpenScoutSkySkillDialog();

    UFUNCTION()
    void OpenScoutLongStrideSkillDialog();

    UFUNCTION()
    void OpenScoutSightSkillDialog();

    UFUNCTION()
    void OpenScoutTrailSkillDialog();

    UFUNCTION()
    void OpenScoutEnergySkillDialog();

    void BuildMenu();
    void AddBattleHubProfileHeader(UVerticalBox* Parent);
    void RebuildContent();
    void SelectTab(const FName TabId);
    void SelectHero(const FName HeroId);
    void BindHeroButton(UButton* Button, const FString& HeroId);
    void BindSkillButton(UButton* Button, const FString& AssetPath);
    void BindSlotButton(UButton* Button, const FName SlotKey);
    void RefreshStatusText();
    void HandleBootstrapReceived(const FCWNativeBootstrapSnapshot& Snapshot);
    void RegisterButtonFx(UButton* Button, const FLinearColor& Accent, bool bPrimary);
    void RegisterPanelFx(UBorder* Panel, const FLinearColor& Accent, bool bSelected = false);
    FLinearColor GetTabAccent(const FName TabId) const;

    UTextBlock* MakeText(const FString& Text, float Size, const FLinearColor& Color, bool bBold = false);
    UTextBlock* MakeKicker(const FString& Text);
    UButton* MakeButton(const FString& Text, const FLinearColor& Accent, bool bPrimary = false);
    UBorder* MakePanel(const FLinearColor& Tint, float InPadding = 18.0f);
    UImage* MakeImage(const FString& RelativeAssetPath, const FVector2D& DesiredSize);
    UWidget* MakeFittedImage(const FString& RelativeAssetPath, const FVector2D& FrameSize);
    UWidget* MakeCoverImage(const FString& RelativeAssetPath, const FVector2D& FrameSize);
    UWidget* MakePortraitCrop(const FString& RelativeAssetPath, const FVector2D& FrameSize, float YOffset = 0.0f);
    UWidget* MakeSquareImage(const FString& RelativeAssetPath, float Size);
    UWidget* MakeActionPill(const FString& Text, const FLinearColor& Accent);
    UWidget* MakeTabButton(const FName TabId, const FString& Label, const FLinearColor& Accent);
    UWidget* MakeStatTile(const FString& Label, const FString& Value, const FLinearColor& Accent);
    UWidget* MakeProfileMetric(const FString& Label, const FString& Value, const FLinearColor& Accent);
    UWidget* MakeProgressBar(float Percent, const FLinearColor& Fill, float Height = 8.0f);
    UWidget* MakeSkillChip(const FString& Title, const FString& Meta, const FString& AssetPath, const FLinearColor& Accent);
    UWidget* MakeSkillListRow(const FString& Title, const FString& Meta, const FString& Level, const FString& AssetPath, const FLinearColor& Accent);
    UWidget* MakeRosterCard(const FString& Index, const FString& HeroId, const FString& Name, const FString& Level, const FString& AssetPath, const FLinearColor& Accent, bool bSelected);
    UWidget* MakeBonusRow(const FString& Label, const FString& Value, const FLinearColor& Accent);
    UWidget* MakeEquipmentSlot(
        const FString& Code,
        const FString& Label,
        const FLinearColor& Accent,
        const FString& AssetPath = FString(),
        const FString& ItemName = FString(),
        const FString& ItemMeta = FString(),
        const FString& ItemStats = FString(),
        const FString& ActionText = FString(),
        const FName SlotKey = NAME_None);
    UWidget* MakeDialogListRow(const FString& Title, const FString& Meta, const FString& AssetPath, const FString& ActionText, const FLinearColor& Accent);
    UWidget* MakeTalentRow(const FString& Title, const FString& Meta, const FString& Level, const FString& AssetPath, const FLinearColor& Accent);
    UWidget* MakeInventoryCard(const FString& Title, const FString& Meta, const FString& AssetPath, const FLinearColor& Accent);
    void ShowDialog(const FString& Title, const FString& Subtitle, UWidget* Body, const FLinearColor& Accent);
    void ShowItemDialog(const FString& SlotName, const FString& CurrentItem, const FString& CurrentAsset, const FLinearColor& Accent);
    void ShowSkillDialog(const FString& SkillTitle, const FString& AssetPath, const FString& Level, const FString& Meta, const FLinearColor& Accent);
    UWidget* MakeHeroCard(const FString& HeroId, const FString& Name, const FString& Role, const FString& AssetPath, const FLinearColor& Accent);
    UWidget* MakeLoadoutSlot(const FString& SlotLabel, const FString& ItemName, const FLinearColor& Accent, const FString& AssetPath = FString());
    UWidget* MakeSettingRow(const FString& Label, const FString& Value);
    UWidget* MakeRecordRow(const FString& Rank, const FString& Name, const FString& Value);
    UWidget* MakeNewsCard(const FString& Title, const FString& Meta, const FString& Body);
    UWidget* MakeRunPanel();
    UWidget* MakeStoryPanel();
    UWidget* MakeCharactersPanel();
    UWidget* MakeSkillsPanel();
    UWidget* MakeProfilePanel();
    UWidget* MakeRatingPanel();
    UWidget* MakeNewsPanel();
    UWidget* MakeSettingsPanel();

    FString GetAssetsRoot() const;
    FString ToLocalAssetPath(const FString& AssetPath) const;
    UTexture2D* LoadTexture(const FString& RelativeAssetPath);

    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UImage* BackgroundImage = nullptr;

    UPROPERTY()
    UImage* LogoImage = nullptr;

    UPROPERTY()
    UBorder* BloodOverlay = nullptr;

    UPROPERTY()
    UBorder* Scanline = nullptr;

    UPROPERTY()
    UBorder* FramePanel = nullptr;

    UPROPERTY()
    UBorder* ContentSweep = nullptr;

    UPROPERTY()
    UBorder* DialogLayer = nullptr;

    UPROPERTY()
    UBorder* FpsPanel = nullptr;

    UPROPERTY()
    UTextBlock* FpsText = nullptr;

    UPROPERTY()
    UVerticalBox* MainStack = nullptr;

    UPROPERTY()
    UHorizontalBox* TabBar = nullptr;

    UPROPERTY()
    UBorder* ContentHost = nullptr;

    UPROPERTY()
    UTextBlock* StatusText = nullptr;

    UPROPERTY()
    UEditableTextBox* RoomCodeInput = nullptr;

    UPROPERTY()
    UEditableTextBox* NameInput = nullptr;

    UPROPERTY()
    TMap<FName, UButton*> TabButtons;

    UPROPERTY()
    TMap<FString, UTexture2D*> TextureCache;

    UPROPERTY()
    TArray<UBorder*> EmberDots;

    TArray<FCWNativeButtonFx> ButtonFxItems;
    TArray<FCWNativePanelFx> PanelFxItems;

    FName ActiveTab = TEXT("characters");
    FName SelectedHeroId = TEXT("scout");
    float FxTime = 0.0f;
    float FpsUpdateTime = 0.0f;
    float FpsFrameSum = 0.0f;
    int32 FpsFrameCount = 0;
    float MenuRevealTime = 1.0f;
    float PanelRevealTime = 1.0f;
    FDelegateHandle BootstrapDelegateHandle;
};
