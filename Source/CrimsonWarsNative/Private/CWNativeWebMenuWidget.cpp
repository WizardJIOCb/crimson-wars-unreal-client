#include "CWNativeWebMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "CWNativeGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "WebBrowser.h"

TSharedRef<SWidget> UCWNativeWebMenuWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    BuildWidgetTree();
    return Super::RebuildWidget();
}

void UCWNativeWebMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Browser)
    {
        Browser->OnUrlChanged.AddUniqueDynamic(this, &UCWNativeWebMenuWidget::HandleUrlChanged);
    }

    ShowMenu();
}

void UCWNativeWebMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bMenuShown && Browser)
    {
        FocusRefreshSeconds += InDeltaTime;
        if (FocusRefreshSeconds >= 0.5f)
        {
            FocusRefreshSeconds = 0.0f;
            FocusBrowser();
        }
    }
}

void UCWNativeWebMenuWidget::BuildWidgetTree()
{
    if (!WidgetTree)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NativeWebMenuRoot"));
    WidgetTree->RootWidget = RootCanvas;

    Browser = WidgetTree->ConstructWidget<UWebBrowser>(UWebBrowser::StaticClass(), TEXT("CrimsonWarsWebBrowser"));
    Browser->SetVisibility(ESlateVisibility::Visible);
    RootCanvas->AddChild(Browser);
    if (UCanvasPanelSlot* BrowserSlot = Cast<UCanvasPanelSlot>(Browser->Slot))
    {
        BrowserSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        BrowserSlot->SetOffsets(FMargin(0.0f));
    }
}

void UCWNativeWebMenuWidget::ShowMenu()
{
    bMenuShown = true;
    SetVisibility(ESlateVisibility::Visible);
    ApplyInputMode(true);
    LoadMenuUrl();
    FocusBrowser();
}

void UCWNativeWebMenuWidget::HideMenu()
{
    bMenuShown = false;
    SetVisibility(ESlateVisibility::Collapsed);
    ApplyInputMode(false);
}

void UCWNativeWebMenuWidget::ToggleMenu()
{
    if (bMenuShown)
    {
        HideMenu();
    }
    else
    {
        ShowMenu();
    }
}

void UCWNativeWebMenuWidget::LoadMenuUrl()
{
    if (!Browser)
    {
        return;
    }

    const FString Url = BuildMenuUrl();
    if (!bInitialUrlLoaded || !Browser->GetUrl().StartsWith(Url.Left(24)))
    {
        Browser->LoadURL(Url);
        bInitialUrlLoaded = true;
        UE_LOG(LogTemp, Display, TEXT("Crimson Wars native web UI: loading %s"), *Url);
    }
}

FString UCWNativeWebMenuWidget::BuildMenuUrl() const
{
    FString BaseUrl = TEXT("http://127.0.0.1:8080");
    if (const UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        const FString Resolved = GI->ResolveHttpBaseUrl();
        if (!Resolved.IsEmpty())
        {
            BaseUrl = Resolved;
        }
    }

    while (BaseUrl.EndsWith(TEXT("/")))
    {
        BaseUrl.LeftChopInline(1);
    }

    return BaseUrl + TEXT("/play?tab=characters&native=ue&uev=20260512webreturn1");
}

void UCWNativeWebMenuWidget::HandleUrlChanged(const FText& Text)
{
    const FString Url = Text.ToString();
    UpdateNativeSyncFromUrl(Url);
    if (Url.StartsWith(TEXT("cw-native://start-run"), ESearchCase::IgnoreCase))
    {
        StartNativeRunFromUrl(Url);
    }
}

void UCWNativeWebMenuWidget::StartNativeRunFromUrl(const FString& Url)
{
    FString HeroId = GetQueryParam(Url, TEXT("hero"));
    if (HeroId.IsEmpty())
    {
        HeroId = TEXT("cyber");
    }
    const FString RoomCode = GetQueryParam(Url, TEXT("room")).ToUpper();

    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        GI->DefaultPlayerClass = HeroId.ToLower();
        GI->DefaultRoomCode = RoomCode;
        GI->bAutoJoinOnConnect = true;

        if (GI->IsSocketConnected())
        {
            GI->JoinRoom(GI->DefaultRoomCode, GI->DefaultPlayerName, GI->DefaultPlayerClass);
        }
        else
        {
            GI->ConnectToServer(GI->DefaultServerUrl);
        }
    }

    HideMenu();
    UE_LOG(LogTemp, Display, TEXT("Crimson Wars native web UI: start run requested hero=%s"), *HeroId);
}

FString UCWNativeWebMenuWidget::GetQueryParam(const FString& Url, const FString& Key) const
{
    int32 HashIndex = INDEX_NONE;
    if (Url.FindChar(TEXT('#'), HashIndex) && HashIndex >= 0 && HashIndex + 1 < Url.Len())
    {
        const FString Hash = Url.Mid(HashIndex + 1);
        FString HashValue = GetQueryParam(TEXT("?") + Hash, Key);
        if (!HashValue.IsEmpty())
        {
            return HashValue;
        }
    }

    FString Query;
    if (!Url.Split(TEXT("?"), nullptr, &Query))
    {
        return FString();
    }

    FString Left;
    FString Rest = Query;
    while (!Rest.IsEmpty())
    {
        FString Pair;
        if (Rest.Split(TEXT("&"), &Pair, &Rest))
        {
            Left = Pair;
        }
        else
        {
            Left = Rest;
            Rest.Reset();
        }

        FString PairKey;
        FString PairValue;
        if (Left.Split(TEXT("="), &PairKey, &PairValue) && PairKey.Equals(Key, ESearchCase::IgnoreCase))
        {
            PairValue.ReplaceInline(TEXT("%20"), TEXT(" "));
            PairValue.ReplaceInline(TEXT("%2F"), TEXT("/"));
            PairValue.ReplaceInline(TEXT("%2f"), TEXT("/"));
            return PairValue;
        }
    }

    return FString();
}

void UCWNativeWebMenuWidget::UpdateNativeSyncFromUrl(const FString& Url)
{
    const FString RoomCode = GetQueryParam(Url, TEXT("cw-native-room")).ToUpper();
    if (!RoomCode.IsEmpty())
    {
        SyncedRoomCode = RoomCode;
    }

    const FString HeroId = GetQueryParam(Url, TEXT("hero")).ToLower();
    if (!HeroId.IsEmpty())
    {
        SyncedHeroId = HeroId;
    }

    const FString PlayerId = GetQueryParam(Url, TEXT("cw-native-player"));
    if (!PlayerId.IsEmpty())
    {
        SyncedPlayerId = PlayerId;
    }

    const FString PlayerName = GetQueryParam(Url, TEXT("cw-native-name"));
    if (!PlayerName.IsEmpty())
    {
        SyncedPlayerName = PlayerName;
    }

    const FString PlayerAccount = GetQueryParam(Url, TEXT("cw-native-account"));
    if (!PlayerAccount.IsEmpty())
    {
        SyncedPlayerAccountId = FMath::Max(0, FCString::Atoi(*PlayerAccount));
    }

    const FString HandoffToken = GetQueryParam(Url, TEXT("cw-native-token"));
    if (!HandoffToken.IsEmpty())
    {
        SyncedHandoffToken = HandoffToken;
    }

    const FString Action = GetQueryParam(Url, TEXT("cw-native-action")).ToLower();
    if (Action.Equals(TEXT("native")))
    {
        const FString Token = GetQueryParam(Url, TEXT("t"));
        if (!Token.IsEmpty() && Token != LastNativeRenderRequestToken)
        {
            LastNativeRenderRequestToken = Token;
            bNativeRenderRequested = true;
        }
    }
}

bool UCWNativeWebMenuWidget::ConsumeNativeRenderRequest(FString& OutRoomCode, FString& OutHeroId)
{
    if (Browser)
    {
        UpdateNativeSyncFromUrl(Browser->GetUrl());
    }

    if (!bNativeRenderRequested)
    {
        return false;
    }

    bNativeRenderRequested = false;
    OutRoomCode = SyncedRoomCode;
    OutHeroId = SyncedHeroId.IsEmpty() ? TEXT("cyber") : SyncedHeroId;
    return true;
}

void UCWNativeWebMenuWidget::RequestNativeRendererFromWeb()
{
    if (!Browser)
    {
        return;
    }

    Browser->ExecuteJavascript(TEXT(
        "(function(){"
        "if(window.cwNativeRequestRenderer){window.cwNativeRequestRenderer();return;}"
        "window.dispatchEvent(new KeyboardEvent('keydown',{code:'F10',key:'F10',bubbles:true}));"
        "})();"));
}

void UCWNativeWebMenuWidget::ApplyInputMode(bool bShowMenu)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;

    if (bShowMenu)
    {
        FInputModeUIOnly InputMode;
        if (Browser)
        {
            InputMode.SetWidgetToFocus(Browser->TakeWidget());
        }
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }
    else
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        if (GEngine && GEngine->GameViewport)
        {
            GEngine->GameViewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
            GEngine->GameViewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
        }
    }
}

void UCWNativeWebMenuWidget::FocusBrowser()
{
    if (!Browser)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    Browser->SetIsEnabled(true);
    Browser->SetFocus();
    Browser->SetUserFocus(PC);
}

void UCWNativeWebMenuWidget::NotifyNativeRendererActive(bool bActive)
{
    if (!Browser)
    {
        return;
    }

    Browser->ExecuteJavascript(FString::Printf(
        TEXT("(function(active){window.cwNativeRendererActive=active;if(window.cwNativeSetRendererActive){window.cwNativeSetRendererActive(active);}else{['game','game-webgl'].forEach(function(id){var el=document.getElementById(id);if(el){el.hidden=active;el.classList.toggle('hidden',active);el.style.visibility=active?'hidden':'visible';el.style.opacity=active?'0':'1';}});}})(%s);if(!%s&&window.cwNativeRestoreWebRunSurface){window.cwNativeRestoreWebRunSurface();}"),
        bActive ? TEXT("true") : TEXT("false"),
        bActive ? TEXT("true") : TEXT("false")));
}

void UCWNativeWebMenuWidget::RestoreWebRunFocus()
{
    if (!Browser)
    {
        return;
    }

    Browser->ExecuteJavascript(TEXT(
        "window.cwNativeRendererActive=false;"
        "(function(){"
        "function show(id){var el=document.getElementById(id);if(!el)return;el.classList.remove('hidden');el.hidden=false;el.style.visibility='visible';el.style.opacity='1';}"
        "var overlay=document.getElementById('join-overlay');"
        "var hasRun=!!((window.cwGame||window.game||{}).state);"
        "if(overlay&&hasRun){overlay.style.display='none';overlay.classList.remove('death-mode','death-cinematic-active','death-rewards-visible');}"
        "show('game');show('game-webgl');"
        "var hud=document.getElementById('hud');if(hud){hud.classList.remove('menu-hidden');}"
        "['top-center-hud','bottom-hud','minimap-wrap','chat-wrap','fps-corner'].forEach(show);"
        "document.body&&document.body.classList.remove('levelup-open');"
        "if(window.cwNativeRestoreWebRunSurface){window.cwNativeRestoreWebRunSurface();}"
        "else if(window.cwNativeFocusGameSurface){window.cwNativeFocusGameSurface();}"
        "window.dispatchEvent(new Event('resize'));"
        "setTimeout(function(){"
        "var game=document.getElementById('game');"
        "if(game){game.tabIndex=0;game.focus({preventScroll:true});}"
        "window.focus();"
        "},0);"
        "})();"));
}
