// Copyright Tripo3D. All Rights Reserved.

#include "Tripo3DUEBridgeModule.h"
#include "TripoVersionCompat.h"
#include "STripoWebSocketWindow.h"
#include "TripoModelImporter.h"
#include "LevelEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "FTripo3DUEBridgeModule"

const FName FTripo3DUEBridgeModule::TripoWebSocketTabName("TripoWebSocketTab");

void FTripo3DUEBridgeModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("Tripo Bridge module starting up"));

	InitializeStyle();

	RegisterTabSpawner();

	RegisterMenuExtensions();

	PerformStartupCleanup();

	UE_LOG(LogTemp, Log, TEXT("Tripo Bridge module started successfully"));
}

void FTripo3DUEBridgeModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("Tripo Bridge module shutting down"));

	UnregisterMenuExtensions();

	UnregisterTabSpawner();

	ShutdownStyle();
}

void FTripo3DUEBridgeModule::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TripoWebSocketTabName,
		FOnSpawnTab::CreateRaw(this, &FTripo3DUEBridgeModule::SpawnTripoWebSocketTab)
	)
	.SetDisplayName(LOCTEXT("TripoWebSocketTabTitle", "Tripo Bridge"))
	.SetTooltipText(LOCTEXT("TripoWebSocketTabTooltip", "Open the Tripo Bridge window"))
	.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FTripo3DUEBridgeModule::UnregisterTabSpawner()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TripoWebSocketTabName);
	}
}

TSharedRef<SDockTab> FTripo3DUEBridgeModule::SpawnTripoWebSocketTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.ContentPadding(0)
		[
			SNew(SBox)
			.WidthOverride(400.0f)
			.HeightOverride(600.0f)
			[
				SNew(STripoWebSocketWindow)
			]
		];
}

// The server is accessible at ws://127.0.0.1:60620
void FTripo3DUEBridgeModule::RegisterMenuExtensions()
{
	// Get the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	// Create menu extension
	TSharedPtr<FExtender> NewMenuExtender = MakeShared<FExtender>();
	NewMenuExtender->AddMenuExtension(
		"WindowLayout",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("OpenTripoWebSocket", "Tripo Bridge"),
				LOCTEXT("OpenTripoWebSocketTooltip", "Open Tripo Bridge window"),
				FSlateIcon("Tripo3DUEBridgeStyle", "Tripo3DUEBridge.MenuIcon"),
				FUIAction(
					FExecuteAction::CreateLambda([this]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(TripoWebSocketTabName);
					}),
					FCanExecuteAction()
				)
			);
	})
	);

	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(NewMenuExtender);
	MenuExtender = NewMenuExtender;
}

void FTripo3DUEBridgeModule::UnregisterMenuExtensions()
{
	if (MenuExtender.IsValid())
	{
		FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
		if (LevelEditorModule)
		{
			LevelEditorModule->GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);
		}
		MenuExtender.Reset();
	}
}

void FTripo3DUEBridgeModule::InitializeStyle()
{
	// Only initialize once
	if (StyleSet.IsValid())
	{
		return;
	}

	// Create new style set
	StyleSet = MakeShareable(new FSlateStyleSet("Tripo3DUEBridgeStyle"));

	// Get plugin base directory
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Tripo3DUEBridge"));
	if (Plugin.IsValid())
	{
		FString ContentDir = Plugin->GetBaseDir() / TEXT("Resources");
		StyleSet->SetContentRoot(ContentDir);

		// Register icon brush (16x16 is standard for menu icons)
		StyleSet->Set("Tripo3DUEBridge.MenuIcon", new FSlateImageBrush(ContentDir / TEXT("logo32.png"), FVector2D(16.0f, 16.0f)));

		// Register the style set
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);

		UE_LOG(LogTemp, Log, TEXT("Tripo Bridge style registered successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find Tripo3DUEBridge plugin"));
	}
}

void FTripo3DUEBridgeModule::ShutdownStyle()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}
}

void FTripo3DUEBridgeModule::PerformStartupCleanup()
{
	// Check if cleanup is needed (once per 24 hours)
	const FString ConfigSection = TEXT("Tripo3DUEBridge");
	const FString ConfigKey = TEXT("LastCleanupTime");
	const FString ConfigFile = GEditorPerProjectIni;

	FString LastCleanupStr;
	GConfig->GetString(*ConfigSection, *ConfigKey, LastCleanupStr, ConfigFile);

	FDateTime LastCleanup;
	if (!LastCleanupStr.IsEmpty())
	{
		FDateTime::Parse(LastCleanupStr, LastCleanup);
	}

	FDateTime Now = FDateTime::Now();
	FTimespan TimeSinceCleanup = Now - LastCleanup;

	// Cleanup if more than 24 hours have passed
	if (TimeSinceCleanup.GetTotalHours() >= 24.0)
	{
		UE_LOG(LogTemp, Log, TEXT("Performing startup cleanup of old Tripo assets"));
		
		// Clean up assets older than 7 days
		FTripoModelImporter::CleanupOldAssets(7);

		// Update last cleanup time
		GConfig->SetString(*ConfigSection, *ConfigKey, *Now.ToString(), ConfigFile);
		GConfig->Flush(false, ConfigFile);

		UE_LOG(LogTemp, Log, TEXT("Startup cleanup completed"));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTripo3DUEBridgeModule, Tripo3DUEBridge)
