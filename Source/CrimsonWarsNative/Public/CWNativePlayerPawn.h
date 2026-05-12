#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CWNativePlayerPawn.generated.h"

class UCameraComponent;
class UCWNativeRunHudWidget;
class UCWNativeRunViewWidget;
class UCWNativeWebMenuWidget;
class UStaticMeshComponent;
class USpringArmComponent;

UCLASS()
class CRIMSONWARSNATIVE_API ACWNativePlayerPawn : public APawn
{
    GENERATED_BODY()

public:
    ACWNativePlayerPawn();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    bool IsNativeRendererActive() const { return bUseNativeRenderer; }
    bool IsNativeRunMenuOpen() const { return bNativeRunMenuOpen; }
    bool IsNativeStatsPanelOpen() const { return bNativeStatsPanelOpen; }
    bool IsNativePlayersPanelOpen() const { return bNativePlayersPanelOpen; }
    bool IsNativeChatOpen() const { return bNativeChatOpen; }
    FString GetNativeChatDraft() const { return NativeChatDraft; }
    bool IsNativeRunInputActive() const;
    bool HandleNativeHudClickAtMouse();
    bool HandleNativeHudClickAtScreenPosition(const FVector2D& ScreenPosition);
    bool HandleNativeHudClickAtViewportPosition(const FVector2D& MousePosition, const FVector2D& ViewportSize);
    bool HandleNativeChatKeyDown(const FKey& Key);
    bool HandleNativeChatCharacter(TCHAR Character);
    bool UseNativeQuickSlotHotkey(int32 Hotkey);
    void SetNativeKeyState(const FKey& Key, bool bIsDown);
    void SetNativeMouseButtonState(const FKey& Key, bool bIsDown);
    void UpdateNativeCursorScreenPosition(const FVector2D& ScreenPosition);
    bool GetNativeCursorScreenPosition(FVector2D& OutScreenPosition) const;
    void UpdateNativeCursorViewportPosition(const FVector2D& LocalPosition, const FVector2D& LocalViewportSize);
    bool GetNativeCursorViewportPosition(FVector2D& OutLocalPosition, FVector2D& OutViewportSize) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|Input")
    float InputSendHz = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|Input")
    float LocalRunSpeed = 560.0f;

private:
    void EnsurePlayerUi();
    void EnsureNativeRunUi(APlayerController* PC);
    void ApplyNativeRunInputMode(APlayerController* PC);
    void ApplySyncedNativeIdentity();
    void SetNativeRenderMode(bool bEnableNativeRenderer, const FString& RoomCode = FString(), const FString& HeroId = FString(), bool bEnableNative3DView = false);
    void ApplyNativeRendererWidgetVisibility();
    void StartNativeRoomFromSync(const FString& RoomCode, const FString& HeroId);
    void ApplyLocalMovement(float DeltaSeconds);
    void MoveForward(float Value);
    void MoveRight(float Value);
    void FirePressed();
    void FireReleased();
    void JumpPressed();
    void JoinPressed();
    void LeavePressed();
    void ToggleMenuPressed();
    void ResetNativeInputState();
    void RefreshNativeMovementFromKeys();
    void BeginNativeChat();
    bool SubmitNativeChat();
    void CancelNativeChat();
    bool SendNativeRunInput(bool bConsumeJump);
    bool SyncActorToServerPlayer();
    FVector ResolveAimWorld() const;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    USceneComponent* SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    USpringArmComponent* SpringArm = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UCameraComponent* Camera = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UStaticMeshComponent* VisualMesh = nullptr;

    UPROPERTY()
    UCWNativeWebMenuWidget* WebMenuWidget = nullptr;

    UPROPERTY()
    UCWNativeRunHudWidget* RunHudWidget = nullptr;

    UPROPERTY()
    UCWNativeRunViewWidget* RunViewWidget = nullptr;

    bool bUseNativeRenderer = false;
    bool bUseNative3DView = false;
    bool bNativeRunMenuOpen = false;
    bool bNativeStatsPanelOpen = false;
    bool bNativePlayersPanelOpen = false;
    bool bNativeChatOpen = false;
    FString NativeChatDraft;
    float MoveForwardValue = 0.0f;
    float MoveRightValue = 0.0f;
    bool bShooting = false;
    bool bJumpQueued = false;
    bool bNativeWDown = false;
    bool bNativeADown = false;
    bool bNativeSDown = false;
    bool bNativeDDown = false;
    bool bNativeLeftMouseDown = false;
    bool bHasNativeCursorScreenPosition = false;
    bool bHasNativeCursorViewportPosition = false;
    FVector2D NativeCursorScreenPosition = FVector2D::ZeroVector;
    FVector2D NativeCursorViewportPosition = FVector2D::ZeroVector;
    FVector2D NativeCursorViewportSize = FVector2D::ZeroVector;
    float InputAccumulator = 0.0f;
    TSharedPtr<class FCWNativeInputPreProcessor> NativeInputPreProcessor;
};
