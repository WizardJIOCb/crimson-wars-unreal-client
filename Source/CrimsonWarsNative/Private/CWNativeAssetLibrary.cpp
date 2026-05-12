#include "CWNativeAssetLibrary.h"

#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace
{
    UTexture2D* TryImportTexture(const FString& FullPath)
    {
        if (!FPaths::FileExists(FullPath))
        {
            return nullptr;
        }

        TArray64<uint8> CompressedData;
        if (!FFileHelper::LoadFileToArray(CompressedData, *FullPath) || CompressedData.Num() <= 0)
        {
            return nullptr;
        }

        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
        const EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(CompressedData.GetData(), CompressedData.Num());
        if (ImageFormat == EImageFormat::Invalid)
        {
            return nullptr;
        }

        TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat, *FullPath);
        if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
        {
            return nullptr;
        }

        TArray64<uint8> RawBgra;
        if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawBgra) || RawBgra.Num() <= 0)
        {
            return nullptr;
        }

        UTexture2D* Texture = UTexture2D::CreateTransient(
            ImageWrapper->GetWidth(),
            ImageWrapper->GetHeight(),
            PF_B8G8R8A8,
            NAME_None,
            RawBgra);
        if (Texture)
        {
            Texture->SRGB = true;
            Texture->NeverStream = true;
            Texture->LODGroup = TEXTUREGROUP_UI;
            Texture->Filter = TF_Nearest;
            Texture->UpdateResource();
            Texture->AddToRoot();
        }
        return Texture;
    }
}

FString FCWNativeAssetLibrary::NormalizeAssetPath(const FString& Root, const FString& RelativePath)
{
    FString CleanRoot = Root;
    CleanRoot.ReplaceInline(TEXT("\\"), TEXT("/"));
    CleanRoot.TrimStartAndEndInline();
    CleanRoot.RemoveFromEnd(TEXT("/"));

    FString CleanRelative = RelativePath;
    CleanRelative.ReplaceInline(TEXT("\\"), TEXT("/"));
    CleanRelative.TrimStartAndEndInline();
    CleanRelative.RemoveFromStart(TEXT("/"));

    return FPaths::ConvertRelativePathToFull(FPaths::Combine(CleanRoot, CleanRelative));
}

UTexture2D* FCWNativeAssetLibrary::LoadTexture(UObject* Outer, const FString& Root, const FString& RelativePath)
{
    (void)Outer;
    const FString FullPath = NormalizeAssetPath(Root, RelativePath);
    const bool bIsWebp = FPaths::GetExtension(FullPath).Equals(TEXT("webp"), ESearchCase::IgnoreCase);
    const FString PngFallbackPath = bIsWebp ? FPaths::ChangeExtension(FullPath, TEXT("png")) : FString();
    const FString ImportPath = bIsWebp && !PngFallbackPath.IsEmpty() && FPaths::FileExists(PngFallbackPath)
        ? PngFallbackPath
        : FullPath;

    UTexture2D* Texture = TryImportTexture(ImportPath);
    if (!Texture && ImportPath != PngFallbackPath && !PngFallbackPath.IsEmpty())
    {
        Texture = TryImportTexture(PngFallbackPath);
    }

    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("Crimson Wars native UI asset missing or unsupported: %s"), *FullPath);
        return nullptr;
    }

    return Texture;
}
