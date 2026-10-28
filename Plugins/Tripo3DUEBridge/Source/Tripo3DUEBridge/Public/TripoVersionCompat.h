// Copyright Tripo3D. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

/**
 * Version Compatibility Header for Tripo3D UE Bridge
 *
 * This header provides compatibility macros and includes for supporting
 * Unreal Engine versions 5.1 through 5.7+
 *
 * Supported Versions:
 * - UE 5.1
 * - UE 5.2
 * - UE 5.3
 * - UE 5.4
 * - UE 5.5
 * - UE 5.6
 * - UE 5.7+
 */

// ============================================================================
// Version Detection Macros
// ============================================================================

// Helper macros for version comparison
#ifndef UE_VERSION_OLDER_THAN
	#define UE_VERSION_OLDER_THAN(Major, Minor) \
		(ENGINE_MAJOR_VERSION < (Major) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION < (Minor)))
#endif

#ifndef UE_VERSION_NEWER_THAN
	#define UE_VERSION_NEWER_THAN(Major, Minor) \
		(ENGINE_MAJOR_VERSION > (Major) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION > (Minor)))
#endif

#ifndef UE_VERSION_AT_LEAST
	#define UE_VERSION_AT_LEAST(Major, Minor) \
		(ENGINE_MAJOR_VERSION > (Major) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION >= (Minor)))
#endif

#ifndef UE_VERSION_EQUAL
	#define UE_VERSION_EQUAL(Major, Minor) \
		(ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION == (Minor))
#endif

// ============================================================================
// Styling API Compatibility (FEditorStyle -> FAppStyle transition in 5.1)
// ============================================================================

// FAppStyle is available in UE 5.0+, but EditorStyleSet.h header may vary
#if UE_VERSION_AT_LEAST(5, 1)
	// UE 5.1+: Use FAppStyle (FEditorStyle is deprecated)
	#include "Styling/AppStyle.h"
	#define TRIPO_STYLE_CLASS FAppStyle
#else
	// UE 5.0 and earlier: Use FEditorStyle
	#include "EditorStyleSet.h"
	#define TRIPO_STYLE_CLASS FEditorStyle
#endif

// Helper macro for getting style brushes
#define TRIPO_GET_BRUSH(BrushName) TRIPO_STYLE_CLASS::Get().GetBrush(BrushName)
#define TRIPO_GET_FONT_STYLE(FontName) TRIPO_STYLE_CLASS::Get().GetFontStyle(FontName)
#define TRIPO_GET_COLOR(ColorName) TRIPO_STYLE_CLASS::Get().GetColor(ColorName)

// ============================================================================
// Module API Compatibility
// ============================================================================

// Add future version-specific module API compatibility here
// Example:
// #if UE_VERSION_AT_LEAST(5, 3)
//     // UE 5.3+ specific module APIs
// #endif

// ============================================================================
// Other Version-Specific Features
// ============================================================================

// Example: If any features are version-specific, add conditional compilation here
// #if UE_VERSION_AT_LEAST(5, 4)
//     // Feature available in 5.4+
//     #define TRIPO_HAS_FEATURE_X 1
// #else
//     #define TRIPO_HAS_FEATURE_X 0
// #endif

// ============================================================================
// FBX Import API Compatibility
// ============================================================================

/**
 * FBX Import Compatibility Notes:
 *
 * CRITICAL CHANGES FROM UE 5.4 TO 5.7:
 *
 * 1. Interchange Framework (UE 5.4+):
 *    - UE 5.4+ introduced the Interchange framework as the default FBX importer
 *    - Legacy FBX importer (UFbxImportUI) is still available but not default
 *    - Can be controlled via console variable: Interchange.FeatureFlags.Import.FBX
 *
 * 2. FBX Scale Factor:
 *    - UE 5.4+: Default scale changed to handle meter→centimeter conversion differently
 *    - Using UAssetImportTask + UFbxImportUI bypasses Interchange and uses legacy importer
 *    - Scale factor 100.0f ensures consistent size across UE 5.4 and 5.7
 *
 * 3. Material Import Behavior:
 *    - UE 5.7 Interchange: Materials may be imported as instances by default
 *    - Legacy FBX importer maintains consistent material import behavior
 *    - Our code uses UFbxImportUI which ensures consistent behavior
 *
 * 4. UAssetImportTask API:
 *    - Remains consistent across UE 5.4 and 5.7
 *    - When Options is set to UFbxImportUI, it uses legacy FBX importer
 *    - This provides predictable, version-independent behavior
 */

// FBX scale factor for version-specific import validation
#if UE_VERSION_AT_LEAST(5, 8)
	// Reserved for future branches that still need explicit legacy FBX import settings
	#define TRIPO_FBX_SCALE_FACTOR 100.0f
	#define TRIPO_HAS_FBX_IMPORT_UI 1
#else
	// UE 5.7 and earlier validation branch uses bare UAssetImportTask/default importer
	#define TRIPO_FBX_SCALE_FACTOR 1.0f
	#define TRIPO_HAS_FBX_IMPORT_UI 0
#endif

// Material search location enum - API changed in UE 5.1
#if UE_VERSION_AT_LEAST(5, 1)
	#define TRIPO_MATERIAL_SEARCH_LOCATION EMaterialSearchLocation::UnderRoot
#else
	#define TRIPO_MATERIAL_SEARCH_LOCATION EMaterialSearchLocation_UnderRoot
#endif

// FBX Import behavior flags
// These document the current validation strategy across versions
#if UE_VERSION_AT_LEAST(5, 4)
	#define TRIPO_HAS_INTERCHANGE_FRAMEWORK 1
#else
	#define TRIPO_HAS_INTERCHANGE_FRAMEWORK 0
#endif

#if UE_VERSION_AT_LEAST(5, 8)
	#define TRIPO_USES_LEGACY_FBX_IMPORTER 1
#else
	#define TRIPO_USES_LEGACY_FBX_IMPORTER 0
#endif

// UFbxImportUI API differences
// CRITICAL: bImportAsSkeletalGeometry doesn't exist in UE 5.6 and earlier, added in UE 5.7+
// UE 5.6 and earlier: Property doesn't exist, mesh type determined automatically
// UE 5.7+: bImportAsSkeletalGeometry property available
#if UE_VERSION_AT_LEAST(5, 8)
	// UE 5.7+: bImportAsSkeletalGeometry exists
	#define TRIPO_HAS_SKELETAL_GEOMETRY_FLAG 1
#else
	// UE 5.7 and earlier: bImportAsSkeletalGeometry is not available in our validated engine installs
	#define TRIPO_HAS_SKELETAL_GEOMETRY_FLAG 0
#endif

// ============================================================================
// Build Configuration Info
// ============================================================================

// Log the detected UE version for debugging
#define TRIPO_UE_VERSION_STRING \
	FString::Printf(TEXT("UE %d.%d.%d"), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ENGINE_PATCH_VERSION)

// Compile-time message about version
#if UE_VERSION_AT_LEAST(5, 1)
	#pragma message("Tripo3D UE Bridge: Compiling for UE 5.1+")
#endif
