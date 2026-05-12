#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CWProtocolTypes.h"
#include "Styling/SlateBrush.h"
#include "CWNativeRunHudWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class UTexture2D;

UCLASS()
class CRIMSONWARSNATIVE_API UCWNativeRunHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    void BuildHud();
    void LoadHudAssets();
    void LoadTexture(const FString& Key, const FString& RelativePath);
    const FSlateBrush* GetBrush(const FString& Key) const;
    void HandleStateReceived(const FCWRoomSnapshot& State);
    void UpdateFromState(const FCWRoomSnapshot& State);
    void UpdateFps(float InDeltaTime);
    UTextBlock* MakeHudText(const FString& Text, float Size, const FLinearColor& Color, bool bBold = false);
    UBorder* MakeHudPanel(float InPadding = 10.0f);

    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UTextBlock* RoomText = nullptr;

    UPROPERTY()
    UTextBlock* HealthText = nullptr;

    UPROPERTY()
    UTextBlock* WeaponText = nullptr;

    UPROPERTY()
    UTextBlock* ThreatText = nullptr;

    UPROPERTY()
    UTextBlock* SkillsText = nullptr;

    UPROPERTY()
    UTextBlock* MinimapText = nullptr;

    UPROPERTY()
    UTextBlock* FpsText = nullptr;

    FCWRoomSnapshot CachedState;
    FString CachedMyId;
    FString AssetsRoot;
    FDelegateHandle StateDelegateHandle;

    UPROPERTY()
    TMap<FString, UTexture2D*> TextureCache;

    TMap<FString, FSlateBrush> BrushCache;

    float FpsAccumulator = 0.0f;
    int32 FpsFrames = 0;
    int32 CachedFps = 0;
};
