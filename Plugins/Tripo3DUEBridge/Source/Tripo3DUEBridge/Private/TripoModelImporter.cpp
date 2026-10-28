// Copyright Tripo3D. All Rights Reserved.

#include "TripoModelImporter.h"
#include "TripoVersionCompat.h"
#include "TripoProtocol.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetImportTask.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/TextureFactory.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Animation/AnimSequence.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "Misc/MessageDialog.h"
#include "Containers/Ticker.h"

// For ZIP extraction - UE has built-in zip support
#include "Serialization/Archive.h"

// Include minizip for ZIP extraction
THIRD_PARTY_INCLUDES_START
#include "zlib.h"
#include "minizip/unzip.h"
THIRD_PARTY_INCLUDES_END

bool FTripoModelImporter::ImportModel(
	const FString& FileId,
	const FString& FileName,
	const FString& FileType,
	const TArray<uint8>& FileData)
{
	UE_LOG(LogTemp, Log, TEXT("Starting import for %s (Type: %s, Size: %.2f MB)"),
		*FileName, *FileType, FileData.Num() / 1024.0f / 1024.0f);

	// Create temp directory
	FString TempDir = FPaths::Combine(
		FPaths::ProjectIntermediateDir(),
		TripoProtocol::TEMP_DIR_NAME,
		FileId
	);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	// Create temp directory
	if (!PlatformFile.CreateDirectoryTree(*TempDir))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create temp directory: %s"), *TempDir);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Created temp directory: %s"), *TempDir);

	FString ModelFilePath;
	TArray<FString> TextureFiles;

	// Handle different file types
	if (FileType.Equals(TEXT("zip"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Log, TEXT("Processing ZIP file: %s"), *FileName);
		
		// Extract ZIP directly to temp directory (no intermediate folder)
		UE_LOG(LogTemp, Log, TEXT("Extracting ZIP to: %s"), *TempDir);
		if (!ExtractZip(FileData, TempDir))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to extract ZIP file"));
			DeleteDirectoryWithRetry(TempDir);
			return false;
		}

		// Find model file in temp directory
		if (!FindModelFile(TempDir, ModelFilePath))
		{
			UE_LOG(LogTemp, Error, TEXT("No FBX, GLB, GLTF, or OBJ file found in ZIP archive"));
			DeleteDirectoryWithRetry(TempDir);
			return false;
		}
		UE_LOG(LogTemp, Log, TEXT("Found model file: %s"), *FPaths::GetCleanFilename(ModelFilePath));
	}
	else if (FileType.Equals(TEXT("fbx"), ESearchCase::IgnoreCase) || 
	         FileType.Equals(TEXT("obj"), ESearchCase::IgnoreCase) ||
	         FileType.Equals(TEXT("glb"), ESearchCase::IgnoreCase) ||
	         FileType.Equals(TEXT("gltf"), ESearchCase::IgnoreCase))
	{
		// Save model file directly
		ModelFilePath = FPaths::Combine(TempDir, FileName);
		if (!FFileHelper::SaveArrayToFile(FileData, *ModelFilePath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save model file to: %s"), *ModelFilePath);
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Unsupported file type: %s"), *FileType);
		return false;
	}

	// Create asset package path
	FString AssetPackagePath = FPaths::Combine(TripoProtocol::ASSET_IMPORT_ROOT, FileId);

	// NOTE: Don't import textures separately - let UAssetImportTask handle it
	// UAssetImportTask will automatically import textures from the .fbm folder and link them to materials

	// Import FBX/OBJ model (this will also import textures and materials automatically)
	UObject* ImportedAsset = nullptr;
	if (!ImportFBX(ModelFilePath, AssetPackagePath, ImportedAsset))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to import model: %s"), *ModelFilePath);
		DeleteDirectoryWithRetry(TempDir);
		return false;
	}

	// Spawn in level
	// Remove .zip extension from actor name if present
	FString ActorName = FileName;
	if (ActorName.EndsWith(TEXT(".zip"), ESearchCase::IgnoreCase))
	{
		ActorName = FPaths::GetBaseFilename(ActorName);
	}
	if (ImportedAsset && !SpawnInLevel(ImportedAsset, ActorName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn model in level (import succeeded)"));
	}

	// Clean up temp directory
	DeleteDirectoryWithRetry(TempDir);

	UE_LOG(LogTemp, Log, TEXT("Successfully imported %s"), *FileName);
	return true;
}

bool FTripoModelImporter::ExtractZip(const TArray<uint8>& ZipData, const FString& ExtractPath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	// Create extract directory
	if (!PlatformFile.CreateDirectoryTree(*ExtractPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create extraction directory: %s"), *ExtractPath);
		return false;
	}

	// Save ZIP data to temporary file (minizip needs a file path)
	FString TempZipPath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("temp_archive.zip"));
	if (!FFileHelper::SaveArrayToFile(ZipData, *TempZipPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save temporary ZIP file"));
		return false;
	}

	// Open ZIP file
	unzFile ZipFile = unzOpen(TCHAR_TO_UTF8(*TempZipPath));
	if (!ZipFile)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to open ZIP file: %s"), *TempZipPath);
		PlatformFile.DeleteFile(*TempZipPath);
		return false;
	}

	// Get global info
	unz_global_info GlobalInfo;
	if (unzGetGlobalInfo(ZipFile, &GlobalInfo) != UNZ_OK)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get ZIP global info"));
		unzClose(ZipFile);
		PlatformFile.DeleteFile(*TempZipPath);
		return false;
	}

	// Extract all files
	for (uLong i = 0; i < GlobalInfo.number_entry; ++i)
	{
		char Filename[512];
		unz_file_info FileInfo;

		if (unzGetCurrentFileInfo(ZipFile, &FileInfo, Filename, sizeof(Filename), nullptr, 0, nullptr, 0) != UNZ_OK)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get file info for entry %d"), i);
			continue;
		}

		FString EntryName = UTF8_TO_TCHAR(Filename);
		FString FullPath = FPaths::Combine(ExtractPath, EntryName);

		// Check if it's a directory
		if (EntryName.EndsWith(TEXT("/")))
		{
			PlatformFile.CreateDirectoryTree(*FullPath);
		}
		else
		{
			// Create directory for file
			FString DirectoryPath = FPaths::GetPath(FullPath);
			PlatformFile.CreateDirectoryTree(*DirectoryPath);

			// Open file in ZIP
			if (unzOpenCurrentFile(ZipFile) != UNZ_OK)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to open file in ZIP: %s"), *EntryName);
				unzGoToNextFile(ZipFile);
				continue;
			}

			// Read and extract file
			TArray<uint8> FileData;
			FileData.SetNumUninitialized(FileInfo.uncompressed_size);

			int BytesRead = unzReadCurrentFile(ZipFile, FileData.GetData(), FileData.Num());
			unzCloseCurrentFile(ZipFile);

			if (BytesRead < 0)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to read file from ZIP: %s"), *EntryName);
				unzGoToNextFile(ZipFile);
				continue;
			}

			// Write to disk
			if (!FFileHelper::SaveArrayToFile(FileData, *FullPath))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to write extracted file: %s"), *FullPath);
				unzGoToNextFile(ZipFile);
				continue;
			}
		}

		// Move to next file
		if ((i + 1) < GlobalInfo.number_entry)
		{
			if (unzGoToNextFile(ZipFile) != UNZ_OK)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to move to next file in ZIP"));
				break;
			}
		}
	}

	unzClose(ZipFile);
	PlatformFile.DeleteFile(*TempZipPath);

	UE_LOG(LogTemp, Log, TEXT("Extracted %d files from ZIP"), GlobalInfo.number_entry);
	return true;
}

bool FTripoModelImporter::FindModelFile(const FString& DirectoryPath, FString& OutModelPath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	TArray<FString> FoundFiles;
	PlatformFile.FindFilesRecursively(FoundFiles, *DirectoryPath, nullptr);

	// Look for FBX files first, then GLB, then GLTF, then OBJ
	for (const FString& FilePath : FoundFiles)
	{
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension == TEXT("fbx"))
		{
			OutModelPath = FilePath;
			return true;
		}
	}

	for (const FString& FilePath : FoundFiles)
	{
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension == TEXT("glb"))
		{
			OutModelPath = FilePath;
			return true;
		}
	}

	for (const FString& FilePath : FoundFiles)
	{
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension == TEXT("gltf"))
		{
			OutModelPath = FilePath;
			return true;
		}
	}

	for (const FString& FilePath : FoundFiles)
	{
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension == TEXT("obj"))
		{
			OutModelPath = FilePath;
			return true;
		}
	}

	return false;
}

bool FTripoModelImporter::IsNormalMapTexture(const FString& Filename)
{
	FString LowerFilename = Filename.ToLower();
	
	// Check for common normal map naming patterns
	return LowerFilename.Contains(TEXT("normal")) ||
	       LowerFilename.Contains(TEXT("_n.")) ||
	       LowerFilename.Contains(TEXT("_nrm.")) ||
	       LowerFilename.Contains(TEXT("_norm.")) ||
	       LowerFilename.EndsWith(TEXT("_n.png")) ||
	       LowerFilename.EndsWith(TEXT("_n.jpg")) ||
	       LowerFilename.EndsWith(TEXT("_n.tga")) ||
	       LowerFilename.EndsWith(TEXT("_n.tif"));
}

int32 FTripoModelImporter::ImportTextures(const FString& SourcePath, const FString& DestPackagePath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	TArray<FString> TextureExtensions = { TEXT("png"), TEXT("jpg"), TEXT("jpeg"), TEXT("tga"), TEXT("bmp"), TEXT("tif"), TEXT("tiff") };
	TArray<FString> FoundFiles;
	PlatformFile.FindFilesRecursively(FoundFiles, *SourcePath, nullptr);

	int32 ImportCount = 0;
	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	TextureFactory->AddToRoot(); // Prevent GC during import

	for (const FString& FilePath : FoundFiles)
	{
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (!TextureExtensions.Contains(Extension))
		{
			continue;
		}

		FString Filename = FPaths::GetBaseFilename(FilePath);
		FString PackageName = FPaths::Combine(DestPackagePath, Filename);
		
		// Configure texture factory for normal maps
		bool bIsNormalMap = IsNormalMapTexture(Filename);
		if (bIsNormalMap)
		{
			UE_LOG(LogTemp, Log, TEXT("Detected normal map: %s"), *Filename);
			TextureFactory->CompressionSettings = TC_Normalmap;
			TextureFactory->LODGroup = TEXTUREGROUP_WorldNormalMap;
			TextureFactory->bFlipNormalMapGreenChannel = false;
		}
		else
		{
			TextureFactory->CompressionSettings = TC_Default;
			TextureFactory->LODGroup = TEXTUREGROUP_World;
		}

		// Import texture
		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();

		bool bOutCanceled = false;
		UObject* ImportedTexture = TextureFactory->ImportObject(
			UTexture2D::StaticClass(),
			Package,
			*Filename,
			RF_Public | RF_Standalone,
			FilePath,
			nullptr,
			bOutCanceled
		);

		if (ImportedTexture)
		{
			// Additional normal map settings
			if (bIsNormalMap)
			{
				UTexture2D* Texture = Cast<UTexture2D>(ImportedTexture);
				if (Texture)
				{
					Texture->SRGB = false;
					Texture->CompressionSettings = TC_Normalmap;
					Texture->LODGroup = TEXTUREGROUP_WorldNormalMap;
					Texture->UpdateResource();
				}
			}

			FAssetRegistryModule::AssetCreated(ImportedTexture);
			ImportedTexture->MarkPackageDirty();
			ImportCount++;

			UE_LOG(LogTemp, Log, TEXT("Imported texture: %s%s"), 
				*Filename, bIsNormalMap ? TEXT(" (Normal Map)") : TEXT(""));
		}
	}

	TextureFactory->RemoveFromRoot();
	return ImportCount;
}

bool FTripoModelImporter::ImportFBX(const FString& SourcePath, const FString& DestPackagePath, UObject*& OutImportedAsset)
{
	// Verify file exists
	if (!FPaths::FileExists(SourcePath))
	{
		UE_LOG(LogTemp, Error, TEXT("FBX file not found: %s"), *SourcePath);
		return false;
	}

	// Create and configure AssetImportTask (UE5 recommended API)
	// Using UAssetImportTask + UFbxImportUI ensures we use the legacy FBX importer
	// across all UE versions (5.4-5.7+), bypassing the Interchange framework
	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	if (!ImportTask)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create AssetImportTask"));
		return false;
	}

	ImportTask->Filename = SourcePath;
	ImportTask->DestinationPath = DestPackagePath;
	ImportTask->bAutomated = true;  // No dialogs
	ImportTask->bReplaceExisting = true;
	ImportTask->bSave = false;  // Don't auto-save, we'll handle that

	bool bUsingDefaultImporter = true;

#if TRIPO_HAS_FBX_IMPORT_UI
	UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
	if (!ImportUI)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UFbxImportUI for legacy FBX validation branch"));
		return false;
	}

	ImportUI->bImportMesh = true;
	ImportUI->bImportMaterials = true;
	ImportUI->bImportTextures = true;

#if TRIPO_HAS_SKELETAL_GEOMETRY_FLAG
	ImportUI->bImportAsSkeletalGeometry = true;
	UE_LOG(LogTemp, Log, TEXT("[Tripo] Legacy FBX validation branch: bImportAsSkeletalGeometry = true"));
#endif

	UFbxStaticMeshImportData* StaticMeshData = ImportUI->StaticMeshImportData;
	if (StaticMeshData)
	{
		StaticMeshData->ImportUniformScale = TRIPO_FBX_SCALE_FACTOR;
		StaticMeshData->bCombineMeshes = false;
		StaticMeshData->bGenerateLightmapUVs = true;
		StaticMeshData->bAutoGenerateCollision = true;

		UE_LOG(LogTemp, Log, TEXT("[Tripo] Legacy FBX validation branch scale factor: %.2f"),
			StaticMeshData->ImportUniformScale);
	}

	UFbxTextureImportData* TextureData = ImportUI->TextureImportData;
	if (TextureData)
	{
		TextureData->MaterialSearchLocation = TRIPO_MATERIAL_SEARCH_LOCATION;
		TextureData->BaseMaterialName = FSoftObjectPath(TEXT("/Game/Materials/M_Tripo"));
	}

	ImportTask->Options = ImportUI;
	bUsingDefaultImporter = false;

	UE_LOG(LogTemp, Log, TEXT("[Tripo] Legacy FBX validation branch: using UFbxImportUI"));
	UE_LOG(LogTemp, Log, TEXT("[Tripo] bImportMaterials: %s, bImportTextures: %s"),
		ImportUI->bImportMaterials ? TEXT("true") : TEXT("false"),
		ImportUI->bImportTextures ? TEXT("true") : TEXT("false"));
#else
	UE_LOG(LogTemp, Log, TEXT("[Tripo] Validation branch: using bare UAssetImportTask with default importer auto-detection (UE 5.7 and earlier)"));
#endif

	UE_LOG(LogTemp, Log, TEXT("Starting FBX import: %s -> %s"), *SourcePath, *DestPackagePath);
	UE_LOG(LogTemp, Log, TEXT("[Tripo] ImportTask->Options assigned: %s"),
		bUsingDefaultImporter ? TEXT("false") : TEXT("true"));

	// Execute import
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().ImportAssetTasks({ImportTask});

	// Check import results
	if (ImportTask->ImportedObjectPaths.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FBX import failed - no objects imported from: %s"), *SourcePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("FBX import succeeded, imported %d objects"), ImportTask->ImportedObjectPaths.Num());

	// Find the main mesh asset from imported objects BEFORE organizing
	// (organizing will move assets and invalidate paths)
	int32 ImportedAnimationCount = 0;
	int32 ImportedSkeletonCount = 0;
	for (const FString& ImportedPath : ImportTask->ImportedObjectPaths)
	{
		UObject* ImportedObject = LoadObject<UObject>(nullptr, *ImportedPath);
		if (!ImportedObject)
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("Imported object: %s (%s)"), *ImportedObject->GetName(), *ImportedObject->GetClass()->GetName());

		if (ImportedObject->IsA<UAnimSequence>())
		{
			ImportedAnimationCount++;
		}
		else if (ImportedObject->GetClass()->GetName() == TEXT("Skeleton"))
		{
			ImportedSkeletonCount++;
		}

		// Find StaticMesh or SkeletalMesh
		if (ImportedObject->IsA<UStaticMesh>() || ImportedObject->IsA<USkeletalMesh>())
		{
			OutImportedAsset = ImportedObject;
			UE_LOG(LogTemp, Log, TEXT("Found main mesh asset: %s"), *OutImportedAsset->GetName());
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Tripo] Imported animation assets: %d, skeleton assets: %d"),
		ImportedAnimationCount, ImportedSkeletonCount);

	// Organize imported assets into folders (do this after finding the mesh)
	OrganizeImportedAssets(ImportTask->ImportedObjectPaths, DestPackagePath);

	// Return success if we found a mesh
	if (OutImportedAsset)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully imported FBX: %s (%s)"), 
			*OutImportedAsset->GetName(), *OutImportedAsset->GetClass()->GetName());
		return true;
	}

	// If no mesh found but objects were imported, try to find any valid object after organizing
	if (ImportTask->ImportedObjectPaths.Num() > 0)
	{
		// Scan the destination folder for any mesh assets
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByPath(FName(*DestPackagePath), AssetDataList, true);
		
		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName() ||
			    AssetData.AssetClassPath == USkeletalMesh::StaticClass()->GetClassPathName())
			{
				OutImportedAsset = AssetData.GetAsset();
				if (OutImportedAsset)
				{
					UE_LOG(LogTemp, Warning, TEXT("Found mesh after organizing: %s (%s)"), 
						*OutImportedAsset->GetName(), *OutImportedAsset->GetClass()->GetName());
					return true;
				}
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("No suitable asset found in import results"));
	return false;
}

void FTripoModelImporter::OrganizeImportedAssets(const TArray<FString>& ImportedObjectPaths, const FString& BasePackagePath)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();
	
	// Track which folders we need to create
	TSet<FString> TexturesToMove;
	TSet<FString> AnimationsToMove;
	
	// First pass: identify assets that need to be moved
	for (const FString& ImportedPath : ImportedObjectPaths)
	{
		UObject* ImportedObject = LoadObject<UObject>(nullptr, *ImportedPath);
		if (!ImportedObject)
		{
			continue;
		}
		
		// Check if it's a texture
		if (ImportedObject->IsA<UTexture>())
		{
			TexturesToMove.Add(ImportedPath);
		}
		// Check if it's an animation sequence
		else if (ImportedObject->GetClass()->GetName() == TEXT("AnimSequence"))
		{
			AnimationsToMove.Add(ImportedPath);
		}
	}
	
	// Move textures to Textures subfolder
	if (TexturesToMove.Num() > 0)
	{
		FString TexturesFolder = FPaths::Combine(BasePackagePath, TEXT("Textures"));
		
		TArray<FAssetRenameData> RenameData;
		for (const FString& TexturePath : TexturesToMove)
		{
			UObject* TextureObject = LoadObject<UObject>(nullptr, *TexturePath);
			if (TextureObject)
			{
				FString AssetName = FPaths::GetBaseFilename(TexturePath);
				FString NewPath = FPaths::Combine(TexturesFolder, AssetName);
				
				FAssetRenameData RenameEntry;
				RenameEntry.Asset = TextureObject;
				RenameEntry.NewPackagePath = TexturesFolder;
				RenameEntry.NewName = AssetName;
				RenameData.Add(RenameEntry);
			}
		}
		
		if (RenameData.Num() > 0)
		{
			AssetTools.RenameAssets(RenameData);
			UE_LOG(LogTemp, Log, TEXT("Moved %d texture(s) to Textures folder"), RenameData.Num());
		}
	}
	
	// Move animations to Animations subfolder
	if (AnimationsToMove.Num() > 0)
	{
		FString AnimationsFolder = FPaths::Combine(BasePackagePath, TEXT("Animations"));
		
		TArray<FAssetRenameData> RenameData;
		for (const FString& AnimPath : AnimationsToMove)
		{
			UObject* AnimObject = LoadObject<UObject>(nullptr, *AnimPath);
			if (AnimObject)
			{
				FString AssetName = FPaths::GetBaseFilename(AnimPath);
				FString NewPath = FPaths::Combine(AnimationsFolder, AssetName);
				
				FAssetRenameData RenameEntry;
				RenameEntry.Asset = AnimObject;
				RenameEntry.NewPackagePath = AnimationsFolder;
				RenameEntry.NewName = AssetName;
				RenameData.Add(RenameEntry);
			}
		}
		
		if (RenameData.Num() > 0)
		{
			AssetTools.RenameAssets(RenameData);
			UE_LOG(LogTemp, Log, TEXT("Moved %d animation(s) to Animations folder"), RenameData.Num());
		}
	}
}

bool FTripoModelImporter::SpawnInLevel(UObject* Asset, const FString& ModelName)
{
	if (!Asset)
	{
		return false;
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid world found for spawning actor"));
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Don't set a fixed name - let UE generate unique names automatically
	// SpawnParams.Name = FName(*ModelName);

	AActor* SpawnedActor = nullptr;

	// Handle Static Mesh
	if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
	{
		SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (SpawnedActor)
		{
			UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(SpawnedActor);
			MeshComponent->SetStaticMesh(StaticMesh);
			MeshComponent->RegisterComponent();
			SpawnedActor->SetRootComponent(MeshComponent);

			// Validate materials were imported and applied
			if (MeshComponent && MeshComponent->GetStaticMesh())
			{
				int32 NumMaterials = MeshComponent->GetNumMaterials();
				UE_LOG(LogTemp, Log, TEXT("[Tripo] Spawned static mesh has %d material slot(s)"), NumMaterials);

				// Check each material slot
				for (int32 i = 0; i < NumMaterials; i++)
				{
					UMaterialInterface* Mat = MeshComponent->GetMaterial(i);
					if (!Mat)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Tripo] Material slot %d is null"), i);
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[Tripo] Material slot %d: %s"), i, *Mat->GetName());
					}
				}
			}
		}
	}
	// Handle Skeletal Mesh
	else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
	{
		SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (SpawnedActor)
		{
			USkeletalMeshComponent* MeshComponent = NewObject<USkeletalMeshComponent>(SpawnedActor);
			MeshComponent->SetSkeletalMesh(SkeletalMesh);
			MeshComponent->RegisterComponent();
			SpawnedActor->SetRootComponent(MeshComponent);
		}
	}

	if (SpawnedActor)
	{
		// Set actor label (display name) without causing name conflicts
		SpawnedActor->SetActorLabel(ModelName);
		
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(SpawnedActor, true, true);
		
		UE_LOG(LogTemp, Log, TEXT("Spawned actor: %s"), *SpawnedActor->GetActorLabel());
		return true;
	}

	return false;
}

bool FTripoModelImporter::DeleteDirectoryWithRetry(const FString& DirectoryPath, int32 MaxRetries, float RetryDelay)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (int32 i = 0; i < MaxRetries; ++i)
	{
		if (PlatformFile.DeleteDirectoryRecursively(*DirectoryPath))
		{
			UE_LOG(LogTemp, Log, TEXT("Deleted temp directory: %s"), *DirectoryPath);
			return true;
		}

		if (i < MaxRetries - 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to delete directory (attempt %d/%d), retrying..."),
				i + 1, MaxRetries);
			FPlatformProcess::Sleep(RetryDelay);
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to delete directory after %d attempts: %s"),
		MaxRetries, *DirectoryPath);
	return false;
}

void FTripoModelImporter::CleanupOldAssets(int32 DaysOld)
{
	// Get all subdirectories in import root
	FString ImportRoot = TripoProtocol::ASSET_IMPORT_ROOT.Replace(TEXT("/Game/"), TEXT(""));
	FString ContentPath = FPaths::Combine(FPaths::ProjectContentDir(), ImportRoot);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	if (!PlatformFile.DirectoryExists(*ContentPath))
	{
		return;
	}

	TArray<FString> SubDirectories;
	PlatformFile.FindFiles(SubDirectories, *ContentPath, nullptr);

	FDateTime Now = FDateTime::Now();
	FTimespan MaxAge = FTimespan::FromDays(DaysOld);

	for (const FString& DirPath : SubDirectories)
	{
		FDateTime ModifiedTime = PlatformFile.GetTimeStamp(*DirPath);
		FTimespan Age = Now - ModifiedTime;

		if (Age > MaxAge)
		{
			UE_LOG(LogTemp, Log, TEXT("Deleting old asset folder (%.1f days old): %s"),
				Age.GetTotalDays(), *DirPath);
			
			// Delete from content browser
			FString PackagePath = FPaths::Combine(
				TripoProtocol::ASSET_IMPORT_ROOT,
				FPaths::GetBaseFilename(DirPath)
			);
			
			TArray<FString> AssetsToDelete;
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			AssetRegistryModule.Get().ScanPathsSynchronous(TArray<FString>{PackagePath}, true);
			
			// Delete the directory
			PlatformFile.DeleteDirectoryRecursively(*DirPath);
		}
	}
}
