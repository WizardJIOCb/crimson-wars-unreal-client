#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CWNativeWebMenuWidget.generated.h"

class UCanvasPanel;
class UWebBrowser;

UCLASS()
class CRIMSONWARSNATIVE_API UCWNativeWebMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|UI")
    void ShowMenu();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|UI")
    void HideMenu();

    UFUNCTION(BlueprintCallable, Category = "Crimson Wars|UI")
    void ToggleMenu();

    UFUNCTION(BlueprintPure, Category = "Crimson Wars|UI")
    bool IsMenuShown() const { return bMenuShown; }

    bool ConsumeNativeRenderRequest(FString& OutRoomCode, FString& OutHeroId);
    FString GetSyncedRoomCode() const { return SyncedRoomCode; }
    FString GetSyncedHeroId() const { return SyncedHeroId; }
    FString GetSyncedPlayerId() const { return SyncedPlayerId; }
    FString GetSyncedPlayerName() const { return SyncedPlayerName; }
    FString GetSyncedHandoffToken() const { return SyncedHandoffToken; }
    int32 GetSyncedPlayerAccountId() const { return SyncedPlayerAccountId; }
    void RequestNativeRendererFromWeb();
    void NotifyNativeRendererActive(bool bActive);
    void RestoreWebRunFocus();

private:
    void BuildWidgetTree();
    void LoadMenuUrl();
    void StartNativeRunFromUrl(const FString& Url);
    FString BuildMenuUrl() const;
    FString GetQueryParam(const FString& Url, const FString& Key) const;
    void UpdateNativeSyncFromUrl(const FString& Url);
    void ApplyInputMode(bool bShowMenu);
    void FocusBrowser();

    UFUNCTION()
    void HandleUrlChanged(const FText& Text);

    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UWebBrowser* Browser = nullptr;

    bool bMenuShown = true;
    bool bInitialUrlLoaded = false;
    float FocusRefreshSeconds = 0.0f;
    bool bNativeRenderRequested = false;
    FString LastNativeRenderRequestToken;
    FString SyncedRoomCode;
    FString SyncedHeroId = TEXT("cyber");
    FString SyncedPlayerId;
    FString SyncedPlayerName;
    FString SyncedHandoffToken;
    int32 SyncedPlayerAccountId = 0;
};
