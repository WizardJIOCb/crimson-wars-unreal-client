// Copyright Tripo3D. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Model importer for Tripo3D assets
 * Handles ZIP extraction, texture import, and model import (FBX, GLB, GLTF, OBJ)
 */
class TRIPO3DUEBRIDGE_API FTripoModelImporter
{
public:
	/**
	 * Import a model file (ZIP, FBX, GLB, GLTF, or OBJ)
	 * @param FileId Unique identifier for this import
	 * @param FileName Original file name
	 * @param FileType File type (zip, fbx, glb, gltf, obj)
	 * @param FileData Binary file data
	 * @return True if import was successful
	 */
	static bool ImportModel(
		const FString& FileId,
		const FString& FileName,
		const FString& FileType,
		const TArray<uint8>& FileData
	);

private:
	/**
	 * Extract ZIP file to temp directory
	 * @param ZipData Binary ZIP data
	 * @param ExtractPath Directory to extract to
	 * @return True if extraction was successful
	 */
	static bool ExtractZip(const TArray<uint8>& ZipData, const FString& ExtractPath);

	/**
	 * Find model file in directory (FBX, GLB, GLTF, or OBJ)
	 * @param DirectoryPath Path to search
	 * @param OutModelPath Full path to found model file
	 * @return True if model file was found
	 */
	static bool FindModelFile(const FString& DirectoryPath, FString& OutModelPath);

	/**
	 * Check if filename indicates a normal map texture
	 * @param Filename Name of texture file
	 * @return True if this is a normal map
	 */
	static bool IsNormalMapTexture(const FString& Filename);

	/**
	 * Import textures from directory
	 * @param SourcePath Source directory containing textures
	 * @param DestPackagePath Destination package path in UE
	 * @return Number of textures imported
	 */
	static int32 ImportTextures(const FString& SourcePath, const FString& DestPackagePath);

	/**
	 * Import FBX file as asset
	 * @param SourcePath Path to FBX file
	 * @param DestPackagePath Destination package path in UE
	 * @param OutImportedAsset Output parameter for imported asset
	 * @return True if import was successful
	 */
	static bool ImportFBX(const FString& SourcePath, const FString& DestPackagePath, UObject*& OutImportedAsset);

	/**
	 * Organize imported assets into subfolders
	 * @param ImportedObjectPaths Paths to all imported objects
	 * @param BasePackagePath Base package path
	 */
	static void OrganizeImportedAssets(const TArray<FString>& ImportedObjectPaths, const FString& BasePackagePath);

	/**
	 * Spawn imported model in current level
	 * @param Asset The imported static mesh or skeletal mesh asset
	 * @param ModelName Name to give the spawned actor
	 * @return True if actor was spawned successfully
	 */
	static bool SpawnInLevel(UObject* Asset, const FString& ModelName);

	/**
	 * Delete directory with retry logic
	 * @param DirectoryPath Path to delete
	 * @param MaxRetries Maximum number of retry attempts
	 * @param RetryDelay Delay between retries in seconds
	 * @return True if deletion was successful
	 */
	static bool DeleteDirectoryWithRetry(const FString& DirectoryPath, int32 MaxRetries = 5, float RetryDelay = 0.1f);

public:
	/**
	 * Clean up old imported assets (older than specified days)
	 * @param DaysOld Assets older than this will be deleted
	 */
	static void CleanupOldAssets(int32 DaysOld = 7);
};
