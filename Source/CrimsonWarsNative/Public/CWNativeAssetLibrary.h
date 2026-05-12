#pragma once

#include "CoreMinimal.h"

class UTexture2D;

class CRIMSONWARSNATIVE_API FCWNativeAssetLibrary
{
public:
    static FString NormalizeAssetPath(const FString& Root, const FString& RelativePath);
    static UTexture2D* LoadTexture(UObject* Outer, const FString& Root, const FString& RelativePath);
};
