#include "CWNativePlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "CWNativeGameInstance.h"
#include "CWNativeRunHudWidget.h"
#include "CWNativeRunViewWidget.h"
#include "CWNativeWebMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/ICursor.h"
#include "Input/Events.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SViewport.h"

class FCWNativeInputPreProcessor final : public IInputProcessor
{
public:
    explicit FCWNativeInputPreProcessor(ACWNativePlayerPawn* InOwner)
        : Owner(InOwner)
    {
    }

    virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
    {
    }

    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
    {
        ACWNativePlayerPawn* Pawn = Owner.Get();
        if (!Pawn)
        {
            return false;
        }

        const FKey Key = InKeyEvent.GetKey();
        if (Key == EKeys::F10)
        {
            Pawn->HandleGlobalF10Pressed();
            return true;
        }

        if (Pawn->IsNativeRunMenuOpen())
        {
            if (Key == EKeys::Escape)
            {
                Pawn->HandleNativeHudClickAtViewportPosition(FVector2D(-1.0f, -1.0f), FVector2D::ZeroVector);
            }
            return true;
        }

        if (Pawn->IsNativeRunInputActive())
        {
            Pawn->SetNativeKeyState(Key, true);
            if (Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D || Key == EKeys::SpaceBar)
            {
                return true;
            }
        }

        return false;
    }

    virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
    {
        ACWNativePlayerPawn* Pawn = Owner.Get();
        if (!Pawn || !Pawn->IsNativeRunInputActive())
        {
            return false;
        }

        const FKey Key = InKeyEvent.GetKey();
        Pawn->SetNativeKeyState(Key, false);
        return Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D || Key == EKeys::SpaceBar;
    }

    virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
    {
        ACWNativePlayerPawn* Pawn = Owner.Get();
        if (!Pawn || !Pawn->IsNativeRunInputActive())
        {
            return false;
        }

        Pawn->UpdateNativeCursorScreenPosition(MouseEvent.GetScreenSpacePosition());
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
            if (Pawn->HandleNativeHudClickAtScreenPosition(MouseEvent.GetScreenSpacePosition()))
            {
                return true;
            }
            Pawn->SetNativeMouseButtonState(MouseEvent.GetEffectingButton(), true);
            return false;
        }

        Pawn->SetNativeMouseButtonState(MouseEvent.GetEffectingButton(), true);
        return false;
    }

    virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
    {
        ACWNativePlayerPawn* Pawn = Owner.Get();
        if (!Pawn || !Pawn->IsNativeRunInputActive())
        {
            return false;
        }

        Pawn->UpdateNativeCursorScreenPosition(MouseEvent.GetScreenSpacePosition());
        Pawn->SetNativeMouseButtonState(MouseEvent.GetEffectingButton(), false);
        return false;
    }

    virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
    {
        ACWNativePlayerPawn* Pawn = Owner.Get();
        if (!Pawn || !Pawn->IsNativeRunInputActive())
        {
            return false;
        }

        Pawn->UpdateNativeCursorScreenPosition(MouseEvent.GetScreenSpacePosition());
        return false;
    }

private:
    TWeakObjectPtr<ACWNativePlayerPawn> Owner;
};

ACWNativePlayerPawn::ACWNativePlayerPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 2600.0f;
    SpringArm->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
    Camera->OrthoWidth = 3600.0f;
    Camera->bConstrainAspectRatio = false;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LocalRunnerPreview"));
    VisualMesh->SetupAttachment(SceneRoot);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 82.0f));
    VisualMesh->SetRelativeScale3D(FVector(0.46f, 0.46f, 1.28f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CapsuleMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CapsuleMesh.Object);
    }
}

void ACWNativePlayerPawn::BeginPlay()
{
    Super::BeginPlay();

    if (!NativeInputPreProcessor.IsValid() && FSlateApplication::IsInitialized())
    {
        NativeInputPreProcessor = MakeShared<FCWNativeInputPreProcessor>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(NativeInputPreProcessor, 0);
    }

    EnsurePlayerUi();
}

void ACWNativePlayerPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (NativeInputPreProcessor.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(NativeInputPreProcessor);
        NativeInputPreProcessor.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void ACWNativePlayerPawn::EnsurePlayerUi()
{
    if (WebMenuWidget && (!bUseNativeRenderer || (RunHudWidget && RunViewWidget)))
    {
        return;
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        if (!WebMenuWidget)
        {
            WebMenuWidget = CreateWidget<UCWNativeWebMenuWidget>(PC, UCWNativeWebMenuWidget::StaticClass());
            if (WebMenuWidget)
            {
                WebMenuWidget->AddToViewport(10);
                WebMenuWidget->ShowMenu();
                UE_LOG(LogTemp, Display, TEXT("Crimson Wars native UI: web menu widget added to viewport."));
            }
        }

        if (bUseNativeRenderer)
        {
            EnsureNativeRunUi(PC);
        }
    }
}

void ACWNativePlayerPawn::EnsureNativeRunUi(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    if (!RunViewWidget)
    {
        RunViewWidget = CreateWidget<UCWNativeRunViewWidget>(PC, UCWNativeRunViewWidget::StaticClass());
        if (RunViewWidget)
        {
            RunViewWidget->AddToViewport(0);
        }
    }

    if (!RunHudWidget)
    {
        RunHudWidget = CreateWidget<UCWNativeRunHudWidget>(PC, UCWNativeRunHudWidget::StaticClass());
        if (RunHudWidget)
        {
            RunHudWidget->AddToViewport(1);
        }
    }

    if (RunViewWidget)
    {
        RunViewWidget->SetVisibility(bUseNative3DView ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
    if (RunHudWidget)
    {
        RunHudWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void ACWNativePlayerPawn::ApplyNativeRunInputMode(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;
    if (FSlateApplication::IsInitialized())
    {
        UpdateNativeCursorScreenPosition(FSlateApplication::Get().GetCursorPos());
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    if (RunHudWidget)
    {
        InputMode.SetWidgetToFocus(RunHudWidget->TakeWidget());
    }
    PC->SetInputMode(InputMode);

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
        GEngine->GameViewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
    }
}

bool ACWNativePlayerPawn::IsNativeRunInputActive() const
{
    return bUseNativeRenderer && !bNativeRunMenuOpen && (!WebMenuWidget || !WebMenuWidget->IsMenuShown());
}

bool ACWNativePlayerPawn::HandleNativeHudClickAtMouse()
{
    if (FSlateApplication::IsInitialized())
    {
        return HandleNativeHudClickAtScreenPosition(FSlateApplication::Get().GetCursorPos());
    }
    return HandleNativeHudClickAtScreenPosition(FVector2D::ZeroVector);
}

bool ACWNativePlayerPawn::HandleNativeHudClickAtScreenPosition(const FVector2D& ScreenPosition)
{
    if (!bUseNativeRenderer || (WebMenuWidget && WebMenuWidget->IsMenuShown()))
    {
        return false;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return false;
    }

    FVector2D MousePosition = FVector2D::ZeroVector;
    FVector2D ViewportSize = FVector2D::ZeroVector;
    bool bHasMousePosition = false;

    if (RunHudWidget)
    {
        const FGeometry HudGeometry = RunHudWidget->GetCachedGeometry();
        const FVector2D HudSize = HudGeometry.GetLocalSize();
        if (HudSize.X > 2.0f && HudSize.Y > 2.0f)
        {
            MousePosition = HudGeometry.AbsoluteToLocal(ScreenPosition);
            ViewportSize = HudSize;
            bHasMousePosition = true;
        }
    }

    if (GEngine && GEngine->GameViewport)
    {
        if (!bHasMousePosition)
        {
            const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
            if (ViewportWidget.IsValid())
            {
                const FGeometry ViewportGeometry = ViewportWidget->GetCachedGeometry();
                ViewportSize = ViewportGeometry.GetLocalSize();
                MousePosition = ViewportGeometry.AbsoluteToLocal(ScreenPosition);
                bHasMousePosition = ViewportSize.X > 2.0f && ViewportSize.Y > 2.0f;
            }
        }
    }

    if (!bHasMousePosition)
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        int32 ViewportX = 0;
        int32 ViewportY = 0;
        PC->GetViewportSize(ViewportX, ViewportY);
        if (!PC->GetMousePosition(MouseX, MouseY) || ViewportX <= 2 || ViewportY <= 2)
        {
            return false;
        }
        MousePosition = FVector2D(MouseX, MouseY);
        ViewportSize = FVector2D(static_cast<float>(ViewportX), static_cast<float>(ViewportY));
    }

    return HandleNativeHudClickAtViewportPosition(MousePosition, ViewportSize);
}

bool ACWNativePlayerPawn::HandleNativeHudClickAtViewportPosition(const FVector2D& MousePosition, const FVector2D& ViewportSize)
{
    if (!bUseNativeRenderer || (WebMenuWidget && WebMenuWidget->IsMenuShown()))
    {
        return false;
    }

    if (bNativeRunMenuOpen && (MousePosition.X < 0.0f || MousePosition.Y < 0.0f || ViewportSize.X <= 2.0f || ViewportSize.Y <= 2.0f))
    {
        bNativeRunMenuOpen = false;
        ResetNativeInputState();
        return true;
    }

    if (MousePosition.X < 0.0f || MousePosition.Y < 0.0f || MousePosition.X > ViewportSize.X || MousePosition.Y > ViewportSize.Y)
    {
        return false;
    }

    auto InRect = [MousePosition](const FVector2D& Pos, const FVector2D& Size)
    {
        return MousePosition.X >= Pos.X && MousePosition.X <= Pos.X + Size.X && MousePosition.Y >= Pos.Y && MousePosition.Y <= Pos.Y + Size.Y;
    };

    const FVector2D MapPos(14.0f, 14.0f);
    const float MiniMapSize = FMath::Clamp(ViewportSize.Y * 0.23f, 138.0f, 188.0f);
    const FVector2D MenuPos(MapPos.X + MiniMapSize + 10.0f, 18.0f);
    const FVector2D PlayersPos(ViewportSize.X - 142.0f, 18.0f);
    const FVector2D NativePos(ViewportSize.X - 122.0f, ViewportSize.Y - 74.0f);
    const FVector2D ChatPos(14.0f, FMath::Max(MapPos.Y + MiniMapSize + 46.0f, ViewportSize.Y - 158.0f));

    if (bNativeRunMenuOpen)
    {
        const FVector2D PanelSize(FMath::Clamp(ViewportSize.X * 0.42f, 500.0f, 680.0f), FMath::Clamp(ViewportSize.Y * 0.58f, 420.0f, 560.0f));
        const FVector2D PanelPos((ViewportSize.X - PanelSize.X) * 0.5f, (ViewportSize.Y - PanelSize.Y) * 0.5f);
        const FVector2D ClosePos = PanelPos + FVector2D(PanelSize.X - 48.0f, 18.0f);
        const FVector2D ResumePos = PanelPos + FVector2D(28.0f, PanelSize.Y - 78.0f);
        const FVector2D WebPos = ResumePos + FVector2D(154.0f, 0.0f);
        const FVector2D LeavePos = PanelPos + FVector2D(PanelSize.X - 166.0f, PanelSize.Y - 78.0f);

        if (InRect(ClosePos, FVector2D(30.0f, 30.0f)) || InRect(ResumePos, FVector2D(136.0f, 42.0f)))
        {
            bNativeRunMenuOpen = false;
            ResetNativeInputState();
            return true;
        }
        if (InRect(WebPos, FVector2D(156.0f, 42.0f)))
        {
            bNativeRunMenuOpen = false;
            ResetNativeInputState();
            SetNativeRenderMode(false, FString(), FString(), false);
            return true;
        }
        if (InRect(LeavePos, FVector2D(138.0f, 42.0f)))
        {
            bNativeRunMenuOpen = false;
            ResetNativeInputState();
            LeavePressed();
            SetNativeRenderMode(false, FString(), FString(), false);
            return true;
        }

        return true;
    }

    if (InRect(MenuPos, FVector2D(66.0f, 30.0f)) || InRect(PlayersPos, FVector2D(122.0f, 42.0f)) || InRect(ChatPos, FVector2D(282.0f, 31.0f)))
    {
        ResetNativeInputState();
        ToggleMenuPressed();
        return true;
    }

    if (InRect(NativePos, FVector2D(106.0f, 30.0f)))
    {
        ResetNativeInputState();
        HandleGlobalF10Pressed();
        return true;
    }

    if (InRect(NativePos + FVector2D(42.0f, 36.0f), FVector2D(64.0f, 30.0f)))
    {
        ResetNativeInputState();
        ToggleMenuPressed();
        return true;
    }

    return false;
}

void ACWNativePlayerPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    EnsurePlayerUi();
    if (WebMenuWidget)
    {
        FString SyncedRoomCode;
        FString SyncedHeroId;
        if (WebMenuWidget->ConsumeNativeRenderRequest(SyncedRoomCode, SyncedHeroId))
        {
            ApplySyncedNativeIdentity();
            SetNativeRenderMode(true, SyncedRoomCode, SyncedHeroId);
        }
    }

    const bool bHasServerPlayer = SyncActorToServerPlayer();
    if (VisualMesh)
    {
        VisualMesh->SetVisibility(bUseNativeRenderer && !bHasServerPlayer, true);
    }
    if (bUseNativeRenderer)
    {
        RefreshNativeMovementFromKeys();
    }
    if (bUseNativeRenderer && IsNativeRunInputActive() && !bHasServerPlayer)
    {
        ApplyLocalMovement(DeltaSeconds);
    }

    if (!bUseNativeRenderer || bNativeRunMenuOpen)
    {
        return;
    }

    InputAccumulator += DeltaSeconds;
    const float EffectiveInputHz = bShooting ? FMath::Max(InputSendHz, 120.0f) : FMath::Max(InputSendHz, 90.0f);
    const float SendInterval = 1.0f / FMath::Max(1.0f, EffectiveInputHz);
    if (InputAccumulator < SendInterval)
    {
        return;
    }

    InputAccumulator = FMath::Fmod(InputAccumulator, SendInterval);

    SendNativeRunInput(true);
}

void ACWNativePlayerPawn::ApplyLocalMovement(float DeltaSeconds)
{
    const FVector InputVector(FMath::Clamp(MoveForwardValue, -1.0f, 1.0f), FMath::Clamp(MoveRightValue, -1.0f, 1.0f), 0.0f);
    if (InputVector.IsNearlyZero())
    {
        return;
    }

    AddActorWorldOffset(InputVector.GetClampedToMaxSize(1.0f) * LocalRunSpeed * DeltaSeconds, true);
}

void ACWNativePlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ACWNativePlayerPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ACWNativePlayerPawn::MoveRight);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ACWNativePlayerPawn::FirePressed);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ACWNativePlayerPawn::FireReleased);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACWNativePlayerPawn::JumpPressed);
    PlayerInputComponent->BindAction(TEXT("NativeJoin"), IE_Pressed, this, &ACWNativePlayerPawn::JoinPressed);
    PlayerInputComponent->BindAction(TEXT("NativeLeave"), IE_Pressed, this, &ACWNativePlayerPawn::LeavePressed);
    PlayerInputComponent->BindAction(TEXT("ToggleNativeMenu"), IE_Pressed, this, &ACWNativePlayerPawn::ToggleMenuPressed);
    PlayerInputComponent->BindAction(TEXT("ToggleNativeRenderer"), IE_Pressed, this, &ACWNativePlayerPawn::ToggleNativeRenderModePressed);
}

void ACWNativePlayerPawn::MoveForward(float Value)
{
    MoveForwardValue = Value;
}

void ACWNativePlayerPawn::MoveRight(float Value)
{
    MoveRightValue = Value;
}

void ACWNativePlayerPawn::SetNativeKeyState(const FKey& Key, bool bIsDown)
{
    if (Key == EKeys::W)
    {
        bNativeWDown = bIsDown;
    }
    else if (Key == EKeys::A)
    {
        bNativeADown = bIsDown;
    }
    else if (Key == EKeys::S)
    {
        bNativeSDown = bIsDown;
    }
    else if (Key == EKeys::D)
    {
        bNativeDDown = bIsDown;
    }
    else if (Key == EKeys::SpaceBar && bIsDown)
    {
        bJumpQueued = true;
    }

    RefreshNativeMovementFromKeys();
}

void ACWNativePlayerPawn::SetNativeMouseButtonState(const FKey& Key, bool bIsDown)
{
    if (Key != EKeys::LeftMouseButton)
    {
        return;
    }

    bNativeLeftMouseDown = bIsDown;
    bShooting = bIsDown;
    if (IsNativeRunInputActive())
    {
        SendNativeRunInput(false);
        InputAccumulator = 0.0f;
    }
}

void ACWNativePlayerPawn::UpdateNativeCursorScreenPosition(const FVector2D& ScreenPosition)
{
    bool bWasDifferent = !bHasNativeCursorScreenPosition || FVector2D::DistSquared(NativeCursorScreenPosition, ScreenPosition) > 0.25f;
    NativeCursorScreenPosition = ScreenPosition;
    bHasNativeCursorScreenPosition = true;

    auto UpdateViewportPositionFromGeometry = [&](const FGeometry& Geometry) -> bool
    {
        const FVector2D GeometrySize = Geometry.GetLocalSize();
        if (GeometrySize.X <= 2.0f || GeometrySize.Y <= 2.0f)
        {
            return false;
        }

        const FVector2D LocalMouse = Geometry.AbsoluteToLocal(ScreenPosition);
        const FVector2D ClampedLocalMouse(
            FMath::Clamp(LocalMouse.X, 0.0f, GeometrySize.X),
            FMath::Clamp(LocalMouse.Y, 0.0f, GeometrySize.Y));

        const bool bViewportWasDifferent =
            !bHasNativeCursorViewportPosition ||
            FVector2D::DistSquared(NativeCursorViewportPosition, ClampedLocalMouse) > 0.25f ||
            !NativeCursorViewportSize.Equals(GeometrySize, 0.5f);

        NativeCursorViewportPosition = ClampedLocalMouse;
        NativeCursorViewportSize = GeometrySize;
        bHasNativeCursorViewportPosition = true;
        bWasDifferent = bWasDifferent || bViewportWasDifferent;
        return true;
    };

    const bool bUpdatedFromRunView = RunViewWidget && UpdateViewportPositionFromGeometry(RunViewWidget->GetCachedGeometry());
    if (!bUpdatedFromRunView && RunHudWidget)
    {
        UpdateViewportPositionFromGeometry(RunHudWidget->GetCachedGeometry());
    }

    if (bWasDifferent && IsNativeRunInputActive())
    {
        InputAccumulator = FMath::Max(InputAccumulator, 1.0f);
        if (bShooting)
        {
            SendNativeRunInput(false);
            InputAccumulator = 0.0f;
        }
    }
}

bool ACWNativePlayerPawn::GetNativeCursorScreenPosition(FVector2D& OutScreenPosition) const
{
    if (bHasNativeCursorScreenPosition)
    {
        OutScreenPosition = NativeCursorScreenPosition;
        return true;
    }

    if (FSlateApplication::IsInitialized())
    {
        OutScreenPosition = FSlateApplication::Get().GetCursorPos();
        return true;
    }

    return false;
}

void ACWNativePlayerPawn::UpdateNativeCursorViewportPosition(const FVector2D& LocalPosition, const FVector2D& LocalViewportSize)
{
    if (LocalViewportSize.X <= 2.0f || LocalViewportSize.Y <= 2.0f)
    {
        return;
    }

    const FVector2D ClampedPosition(
        FMath::Clamp(LocalPosition.X, 0.0f, LocalViewportSize.X),
        FMath::Clamp(LocalPosition.Y, 0.0f, LocalViewportSize.Y));
    const bool bWasDifferent =
        !bHasNativeCursorViewportPosition ||
        FVector2D::DistSquared(NativeCursorViewportPosition, ClampedPosition) > 0.25f ||
        !NativeCursorViewportSize.Equals(LocalViewportSize, 0.5f);

    NativeCursorViewportPosition = ClampedPosition;
    NativeCursorViewportSize = LocalViewportSize;
    bHasNativeCursorViewportPosition = true;

    if (bWasDifferent && IsNativeRunInputActive())
    {
        InputAccumulator = FMath::Max(InputAccumulator, 1.0f);
        if (bShooting)
        {
            SendNativeRunInput(false);
            InputAccumulator = 0.0f;
        }
    }
}

bool ACWNativePlayerPawn::GetNativeCursorViewportPosition(FVector2D& OutLocalPosition, FVector2D& OutViewportSize) const
{
    if (!bHasNativeCursorViewportPosition || NativeCursorViewportSize.X <= 2.0f || NativeCursorViewportSize.Y <= 2.0f)
    {
        return false;
    }

    OutLocalPosition = NativeCursorViewportPosition;
    OutViewportSize = NativeCursorViewportSize;
    return true;
}

void ACWNativePlayerPawn::RefreshNativeMovementFromKeys()
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const bool bWDown = bNativeWDown || (PC && PC->IsInputKeyDown(EKeys::W));
    const bool bADown = bNativeADown || (PC && PC->IsInputKeyDown(EKeys::A));
    const bool bSDown = bNativeSDown || (PC && PC->IsInputKeyDown(EKeys::S));
    const bool bDDown = bNativeDDown || (PC && PC->IsInputKeyDown(EKeys::D));
    const bool bLeftMouseDown = bNativeLeftMouseDown || (PC && PC->IsInputKeyDown(EKeys::LeftMouseButton));

    MoveForwardValue = (bWDown ? 1.0f : 0.0f) + (bSDown ? -1.0f : 0.0f);
    MoveRightValue = (bDDown ? 1.0f : 0.0f) + (bADown ? -1.0f : 0.0f);
    bShooting = bLeftMouseDown;
}

void ACWNativePlayerPawn::ResetNativeInputState()
{
    bNativeWDown = false;
    bNativeADown = false;
    bNativeSDown = false;
    bNativeDDown = false;
    bNativeLeftMouseDown = false;
    MoveForwardValue = 0.0f;
    MoveRightValue = 0.0f;
    bShooting = false;
    bJumpQueued = false;
}

void ACWNativePlayerPawn::FirePressed()
{
    bShooting = true;
    SendNativeRunInput(false);
    InputAccumulator = 0.0f;
}

void ACWNativePlayerPawn::FireReleased()
{
    bShooting = false;
    SendNativeRunInput(false);
    InputAccumulator = 0.0f;
}

void ACWNativePlayerPawn::JumpPressed()
{
    bJumpQueued = true;
}

void ACWNativePlayerPawn::JoinPressed()
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        if (!GI->IsSocketConnected())
        {
            GI->ConnectToServer(GI->DefaultServerUrl);
            return;
        }

        GI->JoinRoom(GI->DefaultRoomCode, GI->DefaultPlayerName, GI->DefaultPlayerClass);
    }
}

void ACWNativePlayerPawn::LeavePressed()
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        GI->LeaveRoom();
    }
}

void ACWNativePlayerPawn::ToggleMenuPressed()
{
    if (!bUseNativeRenderer)
    {
        if (WebMenuWidget)
        {
            WebMenuWidget->ToggleMenu();
        }
        return;
    }

    bNativeRunMenuOpen = !bNativeRunMenuOpen;
    ResetNativeInputState();
}

void ACWNativePlayerPawn::ToggleNativeRenderModePressed()
{
    HandleGlobalF10Pressed();
}

void ACWNativePlayerPawn::HandleGlobalF10Pressed()
{
    FString SyncedRoomCode;
    FString SyncedHeroId;
    if (WebMenuWidget)
    {
        ApplySyncedNativeIdentity();
        SyncedRoomCode = WebMenuWidget->GetSyncedRoomCode();
        SyncedHeroId = WebMenuWidget->GetSyncedHeroId();
    }
    if (SyncedRoomCode.IsEmpty())
    {
        if (const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
        {
            SyncedRoomCode = GI->CurrentRoomCode;
            if (SyncedHeroId.IsEmpty())
            {
                SyncedHeroId = GI->DefaultPlayerClass;
            }
        }
    }

    if (!bUseNativeRenderer)
    {
        if (WebMenuWidget)
        {
            WebMenuWidget->RequestNativeRendererFromWeb();
            return;
        }
        SetNativeRenderMode(true, SyncedRoomCode, SyncedHeroId, false);
        return;
    }

    if (!bUseNative3DView)
    {
        SetNativeRenderMode(true, SyncedRoomCode, SyncedHeroId, true);
        return;
    }

    SetNativeRenderMode(false, SyncedRoomCode, SyncedHeroId, false);
}

void ACWNativePlayerPawn::ApplySyncedNativeIdentity()
{
    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI || !WebMenuWidget)
    {
        return;
    }

    GI->NativeHandoffToken = WebMenuWidget->GetSyncedHandoffToken();
    GI->NativeHandoffPlayerId = WebMenuWidget->GetSyncedPlayerId();
    GI->NativeHandoffPlayerAccountId = WebMenuWidget->GetSyncedPlayerAccountId();

    const FString SyncedName = WebMenuWidget->GetSyncedPlayerName().TrimStartAndEnd();
    if (!SyncedName.IsEmpty())
    {
        GI->DefaultPlayerName = SyncedName;
    }
}

void ACWNativePlayerPawn::SetNativeRenderMode(bool bEnableNativeRenderer, const FString& RoomCode, const FString& HeroId, bool bEnableNative3DView)
{
    const bool bWasNativeRenderer = bUseNativeRenderer;
    bUseNativeRenderer = bEnableNativeRenderer;
    bUseNative3DView = bEnableNativeRenderer && bEnableNative3DView;
    if (!bUseNativeRenderer)
    {
        bNativeRunMenuOpen = false;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (bUseNativeRenderer)
    {
        if (!bWasNativeRenderer)
        {
            ResetNativeInputState();
        }
        if (WebMenuWidget)
        {
            WebMenuWidget->NotifyNativeRendererActive(true);
            WebMenuWidget->HideMenu();
        }
        if (PC)
        {
            EnsureNativeRunUi(PC);
            ApplyNativeRunInputMode(PC);
            EnableInput(PC);
        }
        ApplyNativeRendererWidgetVisibility();

        StartNativeRoomFromSync(RoomCode, HeroId);
        UE_LOG(LogTemp, Display, TEXT("Crimson Wars native renderer enabled mode=%s room=%s hero=%s"), bUseNative3DView ? TEXT("3d") : TEXT("2d"), *RoomCode, *HeroId);
        return;
    }

    ResetNativeInputState();
    bUseNative3DView = false;
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        GI->ReleaseNativeHandoff();
        GI->NativeHandoffToken.Reset();
        GI->NativeHandoffPlayerId.Reset();
        GI->NativeHandoffPlayerAccountId = 0;
    }
    if (RunViewWidget)
    {
        RunViewWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (RunHudWidget)
    {
        RunHudWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (WebMenuWidget)
    {
        WebMenuWidget->ShowMenu();
        WebMenuWidget->NotifyNativeRendererActive(false);
        WebMenuWidget->RestoreWebRunFocus();
    }

    UE_LOG(LogTemp, Display, TEXT("Crimson Wars web renderer enabled."));
}

void ACWNativePlayerPawn::ApplyNativeRendererWidgetVisibility()
{
    if (RunViewWidget)
    {
        RunViewWidget->SetVisibility(bUseNativeRenderer && !bUseNative3DView ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (RunHudWidget)
    {
        RunHudWidget->SetVisibility(bUseNativeRenderer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void ACWNativePlayerPawn::StartNativeRoomFromSync(const FString& RoomCode, const FString& HeroId)
{
    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI)
    {
        return;
    }

    FString NormalizedRoomCode = RoomCode.TrimStartAndEnd().ToUpper();
    const FString NormalizedHeroId = HeroId.TrimStartAndEnd().ToLower();
    if (!NormalizedHeroId.IsEmpty())
    {
        GI->DefaultPlayerClass = NormalizedHeroId;
    }
    if (NormalizedRoomCode.IsEmpty() && !GI->CurrentRoomCode.IsEmpty())
    {
        NormalizedRoomCode = GI->CurrentRoomCode;
    }
    GI->DefaultRoomCode = NormalizedRoomCode;
    GI->bAutoJoinOnConnect = true;

    if (!GI->IsSocketConnected())
    {
        GI->ConnectToServer(GI->DefaultServerUrl);
        return;
    }

    const bool bNeedsNativeHandoff = !GI->NativeHandoffToken.IsEmpty() && !GI->NativeHandoffPlayerId.IsEmpty();
    if (bNeedsNativeHandoff || !GI->HasJoinedRoom() || (!GI->DefaultRoomCode.IsEmpty() && GI->CurrentRoomCode != GI->DefaultRoomCode))
    {
        GI->JoinRoom(GI->DefaultRoomCode, GI->DefaultPlayerName, GI->DefaultPlayerClass);
    }
}

bool ACWNativePlayerPawn::SyncActorToServerPlayer()
{
    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI)
    {
        return false;
    }

    const FString& MyId = GI->MyId;
    const FCWPlayerSnapshot* SelectedPlayer = nullptr;
    for (const FCWPlayerSnapshot& Player : GI->LatestState.Players)
    {
        if (!MyId.IsEmpty() && Player.Id == MyId)
        {
            SelectedPlayer = &Player;
            break;
        }

        if (!SelectedPlayer && !Player.bIsCompanion)
        {
            SelectedPlayer = &Player;
        }
    }

    if (!SelectedPlayer)
    {
        return false;
    }

    const FVector TargetLocation(SelectedPlayer->X, SelectedPlayer->Y, 120.0f);
    SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, GetWorld()->GetDeltaSeconds(), 18.0f));
    return true;
}

bool ACWNativePlayerPawn::SendNativeRunInput(bool bConsumeJump)
{
    if (!bUseNativeRenderer || !IsNativeRunInputActive())
    {
        return false;
    }

    UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
    if (!GI || !GI->HasJoinedRoom())
    {
        return false;
    }

    RefreshNativeMovementFromKeys();
    const FVector Aim = ResolveAimWorld();
    const float MoveX = FMath::Clamp(MoveRightValue, -1.0f, 1.0f);
    const float MoveY = FMath::Clamp(-MoveForwardValue, -1.0f, 1.0f);
    GI->SendInput(MoveX, MoveY, Aim.X, Aim.Y, bShooting, bJumpQueued);
    if (bConsumeJump)
    {
        bJumpQueued = false;
    }
    return true;
}

FVector ACWNativePlayerPawn::ResolveAimWorld() const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        const FVector Origin = GetActorLocation();
        return FVector(Origin.X + 240.0f, Origin.Y, 0.0f);
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PC->GetViewportSize(ViewportX, ViewportY);

    auto FindAuthoritativePlayer = [this]() -> const FCWPlayerSnapshot*
    {
        const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>();
        if (!GI)
        {
            return nullptr;
        }

        const FCWPlayerSnapshot* SelectedPlayer = nullptr;
        for (const FCWPlayerSnapshot& Player : GI->LatestState.Players)
        {
            if (!GI->MyId.IsEmpty() && Player.Id == GI->MyId)
            {
                return &Player;
            }
            if (!SelectedPlayer && !Player.bIsCompanion)
            {
                SelectedPlayer = &Player;
            }
        }
        return SelectedPlayer;
    };

    auto CursorVectorToServerAim = [&](const FVector2D& LocalMouse, const FVector2D& LocalViewportSize, FVector2D& OutAimWorld) -> bool
    {
        const FCWPlayerSnapshot* SelectedPlayer = FindAuthoritativePlayer();
        if (!SelectedPlayer || LocalViewportSize.X <= 2.0f || LocalViewportSize.Y <= 2.0f)
        {
            return false;
        }

        constexpr float NativeViewWorldWidth = 2350.0f;
        const float Scale = FMath::Clamp(LocalViewportSize.X / NativeViewWorldWidth, 0.38f, 1.05f);
        const FVector2D ScreenAimVector = LocalMouse - LocalViewportSize * 0.5f;
        OutAimWorld = FVector2D(
            SelectedPlayer->X + ScreenAimVector.X / Scale,
            SelectedPlayer->Y + ScreenAimVector.Y / Scale);
        return true;
    };

    if (bUseNativeRenderer && RunViewWidget)
    {
        FVector2D LocalMouse = FVector2D::ZeroVector;
        FVector2D ViewportSize = FVector2D::ZeroVector;
        if (RunViewWidget->GetViewportMousePosition(LocalMouse, ViewportSize))
        {
            FVector2D AimWorld;
            if (RunViewWidget->ScreenToLocalPlayerAimWorld(LocalMouse, ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
            if (CursorVectorToServerAim(LocalMouse, ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
            if (RunViewWidget->ScreenToWorld(LocalMouse, ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
        }

        if (GetNativeCursorViewportPosition(LocalMouse, ViewportSize))
        {
            FVector2D AimWorld;
            if (RunViewWidget->ScreenToLocalPlayerAimWorld(LocalMouse, ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
            if (CursorVectorToServerAim(LocalMouse, ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
            if (RunViewWidget->ScreenToWorld(LocalMouse, ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
        }
    }

    if (PC->GetMousePosition(MouseX, MouseY) && ViewportX > 2 && ViewportY > 2)
    {
        if (bUseNativeRenderer && RunViewWidget)
        {
            FVector2D AimWorld;
            const FVector2D ViewportSize(static_cast<float>(ViewportX), static_cast<float>(ViewportY));
            if (RunViewWidget->ScreenToLocalPlayerAimWorld(FVector2D(MouseX, MouseY), ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
            if (CursorVectorToServerAim(FVector2D(MouseX, MouseY), ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
            if (RunViewWidget->ScreenToWorld(FVector2D(MouseX, MouseY), ViewportSize, AimWorld))
            {
                return FVector(AimWorld.X, AimWorld.Y, 0.0f);
            }
        }

        const FCWPlayerSnapshot* SelectedPlayer = FindAuthoritativePlayer();
        if (SelectedPlayer)
        {
            constexpr float NativeViewWorldWidth = 2350.0f;
            const float Scale = FMath::Clamp(static_cast<float>(ViewportX) / NativeViewWorldWidth, 0.38f, 1.05f);
            return FVector(
                SelectedPlayer->X + (MouseX - static_cast<float>(ViewportX) * 0.5f) / Scale,
                SelectedPlayer->Y + (MouseY - static_cast<float>(ViewportY) * 0.5f) / Scale,
                0.0f);
        }
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (PC->GetMousePosition(MouseX, MouseY) && PC->DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection))
    {
        const float Denom = RayDirection.Z;
        if (!FMath::IsNearlyZero(Denom))
        {
            const float T = -RayOrigin.Z / Denom;
            if (T > 0.0f)
            {
                return RayOrigin + RayDirection * T;
            }
        }
    }

    const FVector Origin = GetActorLocation();
    return FVector(Origin.X + 240.0f, Origin.Y, 0.0f);
}
