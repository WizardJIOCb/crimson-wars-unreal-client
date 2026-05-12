#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CWProtocolTypes.h"
#include "Styling/SlateBrush.h"
#include "CWNativeRunViewWidget.generated.h"

class UCanvasPanel;
class UTexture2D;

UCLASS()
class CRIMSONWARSNATIVE_API UCWNativeRunViewWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    bool GetViewportMousePosition(FVector2D& OutMousePosition, FVector2D& OutViewportSize) const;
    bool ScreenToWorld(const FVector2D& ScreenPosition, const FVector2D& ViewportSize, FVector2D& OutWorld) const;
    bool ScreenToLocalPlayerAimWorld(const FVector2D& ScreenPosition, const FVector2D& ViewportSize, FVector2D& OutWorld) const;

private:
    struct FRenderPlayerState
    {
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Velocity = FVector2D::ZeroVector;
        FVector2D TargetPosition = FVector2D::ZeroVector;
        FVector2D LastServerPosition = FVector2D::ZeroVector;
        double LastServerNowMs = 0.0;
        double LastLocalSampleSeconds = 0.0;
        bool bInitialized = false;
        double LastTrailAt = 0.0;
    };

    struct FRenderEntityState
    {
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Velocity = FVector2D::ZeroVector;
        FVector2D TargetPosition = FVector2D::ZeroVector;
        FVector2D LastServerPosition = FVector2D::ZeroVector;
        double LastServerNowMs = 0.0;
        double LastLocalSampleSeconds = 0.0;
        bool bInitialized = false;
    };

    struct FTransientFx
    {
        FString Type;
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D EndPosition = FVector2D::ZeroVector;
        FVector2D Direction = FVector2D(1.0f, 0.0f);
        FLinearColor Color = FLinearColor::White;
        float Age = 0.0f;
        float Life = 0.35f;
        float Radius = 24.0f;
    };

    void HandleStateReceived(const FCWRoomSnapshot& State);
    void HandleSkillFxReceived(const FCWSkillFxEvent& Event);
    void HandleMeleeFxReceived(const FCWMeleeFxEvent& Event);
    void HandleWorldFxReceived(const FCWWorldFxEvent& Event);
    void UpdateRenderPlayers(float DeltaTime);
    void UpdateRenderEntities(float DeltaTime);
    void UpdateTransientFx(float DeltaTime);
    void EmitStateTransitionFx(const FCWRoomSnapshot& PreviousState, const FCWRoomSnapshot& NextState);
    void EmitSkillFxEvent(const FCWSkillFxEvent& Event);
    void EmitMeleeFxEvent(const FCWMeleeFxEvent& Event);
    void EmitWorldFxEvent(const FCWWorldFxEvent& Event);
    void AddFx(const FString& Type, const FVector2D& Position, const FVector2D& Direction, const FLinearColor& Color, float Life, float Radius, const FVector2D& EndPosition = FVector2D::ZeroVector);
    void LoadRunAssets();
    void LoadSkillIconTextures();
    void LoadTexture(const FString& Key, const FString& RelativePath);
    const FSlateBrush* GetBrush(const FString& Key) const;
    const FCWPlayerSnapshot* FindFocusPlayer() const;
    FVector2D GetRenderPosition(const FCWPlayerSnapshot& Player) const;
    FVector2D GetRenderEnemyPosition(const FCWEnemySnapshot& Enemy) const;
    FVector2D GetRenderBulletPosition(const FCWBulletSnapshot& Bullet) const;
    FVector2D GetRenderDropPosition(const FCWPickupSnapshot& Drop) const;
    FVector2D GetRenderXpOrbPosition(const FCWPickupSnapshot& Orb) const;
    FVector2D GetRenderSkillOrbPosition(const FCWPickupSnapshot& Orb) const;
    FVector2D WorldToScreen(float X, float Y, const FVector2D& Size, const FCWPlayerSnapshot* FocusPlayer, float Scale) const;
    FLinearColor SkillOrbAccentColor(const FString& SkillId) const;
    FString SkillOrbFxKind(const FString& SkillId) const;
    FString SkillOrbBadge(const FString& SkillId) const;
    FString FindSkillIconTextureKey(const FString& SkillId) const;

    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    TMap<FString, UTexture2D*> TextureCache;

    TMap<FString, FSlateBrush> BrushCache;
    FString AssetsRoot;
    FCWRoomSnapshot CachedState;
    FString CachedMyId;
    FDelegateHandle StateDelegateHandle;
    FDelegateHandle SkillFxDelegateHandle;
    FDelegateHandle MeleeFxDelegateHandle;
    FDelegateHandle WorldFxDelegateHandle;
    TMap<FString, FRenderPlayerState> RenderPlayers;
    TMap<FString, FRenderEntityState> RenderEnemies;
    TMap<FString, FRenderEntityState> RenderBullets;
    TMap<FString, FRenderEntityState> RenderDrops;
    TMap<FString, FRenderEntityState> RenderXpOrbs;
    TMap<FString, FRenderEntityState> RenderSkillOrbs;
    TArray<FString> SkillIconTextureKeys;
    TArray<FTransientFx> TransientFx;
    TSet<FString> SeenShotEventIds;
    TSet<FString> SeenSkillFxEventIds;
    TSet<FString> SeenMeleeFxEventIds;
    TSet<FString> SeenWorldFxEventIds;
    TSet<FString> SeenObjectImpactEventIds;
    TSet<FString> HiddenDropKeys;
    TSet<FString> HiddenXpOrbKeys;
    TSet<FString> HiddenSkillOrbKeys;
    TMap<FString, float> PreviousEnemyHp;
    TMap<FString, FVector2D> PreviousBulletPositions;
    TMap<FString, float> PreviousSkillCooldowns;
};
