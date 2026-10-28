#include "CWNativeWorldActor.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "CWNativeAssetLibrary.h"
#include "CWNativeGameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Math/RotationMatrix.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"

#include <initializer_list>

namespace
{
    constexpr float BasicMeshSizeCm = 100.0f;
    constexpr float DefaultArenaWidth = 4800.0f;
    constexpr float DefaultArenaHeight = 2800.0f;
    constexpr float GridStep = 800.0f;
    constexpr float ProjectileFxZ = 64.0f;
    constexpr float GroundFxZ = 10.0f;
    constexpr float SkeletalVisualScale = 0.5f;
    constexpr float LocalPlayerVisualScaleDivisor = 1.7f;
    constexpr int32 MaxRuntimeFxComponents = 820;

    float StableUnitFromString(const FString& Key)
    {
        const uint32 Hash = GetTypeHash(Key);
        return static_cast<float>(Hash & 0xffffu) / 65535.0f;
    }

    FString PrettyLabelFromKey(const FString& RawKey)
    {
        FString Clean = RawKey.TrimStartAndEnd();
        if (Clean.StartsWith(TEXT("native_"), ESearchCase::IgnoreCase))
        {
            Clean.RightChopInline(7);
        }
        Clean.ReplaceInline(TEXT("_"), TEXT(" "));
        Clean.ReplaceInline(TEXT("-"), TEXT(" "));
        Clean.ReplaceInline(TEXT("."), TEXT(" "));

        TArray<FString> Words;
        Clean.ParseIntoArray(Words, TEXT(" "), true);

        FString Result;
        for (FString Word : Words)
        {
            Word.TrimStartAndEndInline();
            if (Word.IsEmpty())
            {
                continue;
            }

            Word = Word.ToLower();
            const FString PrettyWord = Word.Left(1).ToUpper() + Word.Mid(1);
            if (!Result.IsEmpty())
            {
                Result += TEXT(" ");
            }
            Result += PrettyWord;
        }

        return Result.IsEmpty() ? TEXT("Pickup") : Result;
    }

    void DestroyPickupWorldVisual(FCWPickupWorldVisual& Visual)
    {
        if (Visual.Aura)
        {
            Visual.Aura->DestroyComponent();
            Visual.Aura = nullptr;
        }
        if (Visual.Icon)
        {
            Visual.Icon->DestroyComponent();
            Visual.Icon = nullptr;
        }
        if (Visual.Label)
        {
            Visual.Label->DestroyComponent();
            Visual.Label = nullptr;
        }
        Visual.IconKey.Reset();
    }

    FString NormalizeSurfaceMaterialKey(const FString& Material)
    {
        const FString Key = Material.TrimStartAndEnd().ToLower();
        if (Key.Contains(TEXT("asphalt")))
        {
            return TEXT("asphalt");
        }
        if (Key.Contains(TEXT("grass")))
        {
            return TEXT("grass");
        }
        if (Key.Contains(TEXT("dirt")) || Key.Contains(TEXT("soil")) || Key.Contains(TEXT("earth")))
        {
            return TEXT("dirt");
        }
        if (Key.Contains(TEXT("toxic")) || Key.Contains(TEXT("slime")))
        {
            return TEXT("toxic");
        }
        if (Key.Contains(TEXT("metal")) || Key.Contains(TEXT("car")) || Key.Contains(TEXT("reactor")) || Key.Contains(TEXT("bus")) || Key.Contains(TEXT("tank")))
        {
            return TEXT("metal");
        }
        if (Key.Contains(TEXT("wood")) || Key.Contains(TEXT("tree")) || Key.Contains(TEXT("shack")))
        {
            return TEXT("wood");
        }
        if (Key.Contains(TEXT("glass")))
        {
            return TEXT("glass");
        }
        return TEXT("concrete");
    }

    FLinearColor SurfaceDustColor(const FString& SurfaceKey)
    {
        if (SurfaceKey == TEXT("asphalt")) return FLinearColor(0.32f, 0.35f, 0.38f, 1.0f);
        if (SurfaceKey == TEXT("grass")) return FLinearColor(0.24f, 0.32f, 0.12f, 1.0f);
        if (SurfaceKey == TEXT("dirt")) return FLinearColor(0.44f, 0.29f, 0.15f, 1.0f);
        if (SurfaceKey == TEXT("toxic")) return FLinearColor(0.42f, 0.62f, 0.12f, 1.0f);
        if (SurfaceKey == TEXT("metal")) return FLinearColor(0.38f, 0.42f, 0.46f, 1.0f);
        if (SurfaceKey == TEXT("wood")) return FLinearColor(0.48f, 0.27f, 0.10f, 1.0f);
        if (SurfaceKey == TEXT("glass")) return FLinearColor(0.54f, 0.82f, 0.95f, 1.0f);
        return FLinearColor(0.52f, 0.54f, 0.55f, 1.0f);
    }

    FLinearColor SurfaceChunkColor(const FString& SurfaceKey, float Seed = 0.0f)
    {
        if (SurfaceKey == TEXT("asphalt"))
        {
            return FLinearColor(FMath::Lerp(0.035f, 0.13f, Seed), FMath::Lerp(0.038f, 0.14f, Seed), FMath::Lerp(0.042f, 0.15f, Seed), 1.0f);
        }
        if (SurfaceKey == TEXT("grass"))
        {
            return Seed > 0.56f ? FLinearColor(0.12f, 0.24f, 0.06f, 1.0f) : FLinearColor(0.34f, 0.24f, 0.12f, 1.0f);
        }
        if (SurfaceKey == TEXT("dirt"))
        {
            return FLinearColor(FMath::Lerp(0.26f, 0.58f, Seed), FMath::Lerp(0.16f, 0.34f, Seed), FMath::Lerp(0.07f, 0.16f, Seed), 1.0f);
        }
        if (SurfaceKey == TEXT("toxic"))
        {
            return Seed > 0.52f ? FLinearColor(0.54f, 0.82f, 0.16f, 1.0f) : FLinearColor(0.18f, 0.30f, 0.10f, 1.0f);
        }
        if (SurfaceKey == TEXT("metal"))
        {
            return FLinearColor(FMath::Lerp(0.26f, 0.58f, Seed), FMath::Lerp(0.30f, 0.62f, Seed), FMath::Lerp(0.34f, 0.68f, Seed), 1.0f);
        }
        if (SurfaceKey == TEXT("wood"))
        {
            return FLinearColor(FMath::Lerp(0.30f, 0.62f, Seed), FMath::Lerp(0.15f, 0.32f, Seed), FMath::Lerp(0.06f, 0.12f, Seed), 1.0f);
        }
        if (SurfaceKey == TEXT("glass"))
        {
            return FLinearColor(0.58f, 0.90f, 1.0f, 1.0f);
        }
        return FLinearColor(FMath::Lerp(0.42f, 0.78f, Seed), FMath::Lerp(0.45f, 0.80f, Seed), FMath::Lerp(0.48f, 0.84f, Seed), 1.0f);
    }

    FLinearColor SurfaceScorchColor(const FString& SurfaceKey)
    {
        if (SurfaceKey == TEXT("grass")) return FLinearColor(0.020f, 0.050f, 0.018f, 1.0f);
        if (SurfaceKey == TEXT("dirt")) return FLinearColor(0.095f, 0.055f, 0.026f, 1.0f);
        if (SurfaceKey == TEXT("toxic")) return FLinearColor(0.045f, 0.090f, 0.026f, 1.0f);
        if (SurfaceKey == TEXT("metal")) return FLinearColor(0.050f, 0.052f, 0.054f, 1.0f);
        if (SurfaceKey == TEXT("wood")) return FLinearColor(0.080f, 0.034f, 0.014f, 1.0f);
        return FLinearColor(0.018f, 0.020f, 0.023f, 1.0f);
    }

    uint32 Hash2D(int32 X, int32 Y, uint32 Seed)
    {
        uint32 H = static_cast<uint32>(X) * 374761393u + static_cast<uint32>(Y) * 668265263u + Seed * 1442695041u;
        H = (H ^ (H >> 13u)) * 1274126177u;
        return H ^ (H >> 16u);
    }

    float Hash01(int32 X, int32 Y, uint32 Seed)
    {
        return static_cast<float>(Hash2D(X, Y, Seed) & 0xffffu) / 65535.0f;
    }

    int32 PositiveModulo(int32 Value, int32 Period)
    {
        const int32 Mod = Value % Period;
        return Mod < 0 ? Mod + Period : Mod;
    }

    float Hash01Tiled(int32 X, int32 Y, int32 PeriodX, int32 PeriodY, uint32 Seed)
    {
        return Hash01(PositiveModulo(X, PeriodX), PositiveModulo(Y, PeriodY), Seed);
    }

    float SmoothNoise2DTiled(float X, float Y, int32 PeriodX, int32 PeriodY, uint32 Seed)
    {
        const int32 X0 = FMath::FloorToInt(X);
        const int32 Y0 = FMath::FloorToInt(Y);
        const float Fx = X - static_cast<float>(X0);
        const float Fy = Y - static_cast<float>(Y0);
        const float Sx = Fx * Fx * (3.0f - 2.0f * Fx);
        const float Sy = Fy * Fy * (3.0f - 2.0f * Fy);
        const float A = Hash01Tiled(X0, Y0, PeriodX, PeriodY, Seed);
        const float B = Hash01Tiled(X0 + 1, Y0, PeriodX, PeriodY, Seed);
        const float C = Hash01Tiled(X0, Y0 + 1, PeriodX, PeriodY, Seed);
        const float D = Hash01Tiled(X0 + 1, Y0 + 1, PeriodX, PeriodY, Seed);
        return FMath::Lerp(FMath::Lerp(A, B, Sx), FMath::Lerp(C, D, Sx), Sy);
    }

    UTexture2D* CreateProceduralGroundTexture(UObject* /*Outer*/, const FString& MapId)
    {
        constexpr int32 TextureSize = 2048;
        UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8, TEXT("CW_ProceduralGround"));
        if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() <= 0)
        {
            return nullptr;
        }

        const uint32 Seed = GetTypeHash(MapId.IsEmpty() ? FString(TEXT("default_ground")) : MapId);
        FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
        void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
        FColor* Pixels = static_cast<FColor*>(RawData);
        if (!Pixels)
        {
            Mip.BulkData.Unlock();
            return nullptr;
        }

        const FLinearColor DeepSoil(0.125f, 0.108f, 0.078f, 1.0f);
        const FLinearColor DrySoil(0.285f, 0.235f, 0.155f, 1.0f);
        const FLinearColor Moss(0.130f, 0.245f, 0.105f, 1.0f);
        const FLinearColor Field(0.185f, 0.335f, 0.140f, 1.0f);
        const FLinearColor Stone(0.325f, 0.315f, 0.270f, 1.0f);
        const FLinearColor Leaf(0.245f, 0.355f, 0.160f, 1.0f);

        for (int32 Y = 0; Y < TextureSize; ++Y)
        {
            for (int32 X = 0; X < TextureSize; ++X)
            {
                const float U = static_cast<float>(X) / static_cast<float>(TextureSize);
                const float V = static_cast<float>(Y) / static_cast<float>(TextureSize);
                const float Broad = SmoothNoise2DTiled(U * 8.0f, V * 8.0f, 8, 8, Seed + 11u);
                const float Mid = SmoothNoise2DTiled(U * 24.0f, V * 24.0f, 24, 24, Seed + 31u);
                const float Fine = SmoothNoise2DTiled(U * 96.0f, V * 96.0f, 96, 96, Seed + 67u);
                const float Detail = SmoothNoise2DTiled(U * 220.0f, V * 220.0f, 220, 220, Seed + 89u);
                const float Grain = Hash01Tiled(X, Y, TextureSize, TextureSize, Seed + 103u);

                FLinearColor Color = FMath::Lerp(DeepSoil, DrySoil, Broad * 0.48f + Fine * 0.22f);
                Color = FMath::Lerp(Color, Moss, FMath::Clamp((Mid - 0.40f) * 0.95f, 0.0f, 0.42f));
                Color = FMath::Lerp(Color, Field, FMath::Clamp((Broad + Mid * 0.45f - 0.78f) * 0.70f, 0.0f, 0.28f));
                Color = FMath::Lerp(Color, Leaf, FMath::Clamp((Detail - 0.64f) * 0.75f, 0.0f, 0.18f));

                const float PathA = FMath::Abs(FMath::Sin((U * 2.0f + V * 3.0f + Broad * 0.18f) * TWO_PI));
                const float PathMask = FMath::Clamp((0.048f - PathA) * 4.8f, 0.0f, 0.20f);
                Color = FMath::Lerp(Color, DrySoil, PathMask);

                const bool bPebble = Fine > 0.76f && Grain > 0.86f;
                if (bPebble)
                {
                    Color = FMath::Lerp(Color, Stone, 0.42f);
                }

                const float Shade = 0.92f + Mid * 0.08f + Fine * 0.05f + (Grain - 0.5f) * 0.10f;
                Color.R = FMath::Clamp(Color.R * Shade, 0.0f, 1.0f);
                Color.G = FMath::Clamp(Color.G * Shade, 0.0f, 1.0f);
                Color.B = FMath::Clamp(Color.B * Shade, 0.0f, 1.0f);
                Pixels[Y * TextureSize + X] = Color.ToFColor(true);
            }
        }

        Mip.BulkData.Unlock();
        Texture->SRGB = true;
        Texture->NeverStream = true;
        Texture->LODGroup = TEXTUREGROUP_World;
        Texture->Filter = TF_Trilinear;
        Texture->UpdateResource();
        return Texture;
    }

    float StaticMeshScaleForHeight(UStaticMesh* Mesh, float TargetHeight, float FallbackScale)
    {
        if (!Mesh)
        {
            return FallbackScale;
        }

        const FBoxSphereBounds Bounds = Mesh->GetBounds();
        const float MeshHeight = FMath::Max(1.0f, Bounds.BoxExtent.Z * 2.0f);
        return TargetHeight / MeshHeight;
    }

    FString MapObjectTextureRelativePath(const FString& MeshKey)
    {
        const FString Key = MeshKey.TrimStartAndEnd().ToLower();
        if (Key == TEXT("car_red")) return TEXT("map-props/car_red.png");
        if (Key == TEXT("car_blue")) return TEXT("map-props/car_blue.png");
        if (Key == TEXT("ambulance")) return TEXT("map-props/ambulance.png");
        if (Key == TEXT("barrier")) return TEXT("map-props/barrier.png");
        if (Key == TEXT("bus_yellow")) return TEXT("map-props/bus_yellow.png");
        if (Key == TEXT("industrial_tank")) return TEXT("map-props/industrial_tank.png");
        if (Key == TEXT("mall_block")) return TEXT("map-props/mall_block.png");
        if (Key == TEXT("shack")) return TEXT("map-props/shack.png");
        if (Key == TEXT("clinic_block")) return TEXT("map-props/clinic_block.png");
        if (Key == TEXT("reactor_block")) return TEXT("map-props/reactor_block.png");
        if (Key == TEXT("shopping_cart_barricade")) return TEXT("map-props/shopping_cart_barricade.png");
        if (Key == TEXT("build_1")) return TEXT("buildings/build-1.png");
        if (Key == TEXT("build_2")) return TEXT("buildings/build-2.png");
        if (Key == TEXT("build_3")) return TEXT("buildings/build-3.png");
        if (Key == TEXT("build_4")) return TEXT("buildings/build-4.png");
        if (Key == TEXT("build_5")) return TEXT("buildings/build-5.png");
        if (Key == TEXT("build_6")) return TEXT("buildings/build-6.png");
        if (Key == TEXT("build_7")) return TEXT("buildings/build-7.png");
        return FString();
    }

    float MapObjectStaticMeshYawOffsetDeg(const FString& MeshKey)
    {
        const FString Key = MeshKey.TrimStartAndEnd().ToLower();
        if (Key == TEXT("bus_yellow")
            || Key == TEXT("ambulance")
            || Key == TEXT("barrier")
            || Key == TEXT("shopping_cart_barricade")
            || Key == TEXT("mall_block")
            || Key == TEXT("clinic_block")
            || Key == TEXT("reactor_block")
            || Key == TEXT("build_2")
            || Key == TEXT("build_5"))
        {
            return 90.0f;
        }
        return 0.0f;
    }

    float MapObjectStaticMeshVisualScaleMultiplier(const FString& MeshKey)
    {
        const FString Key = MeshKey.TrimStartAndEnd().ToLower();
        if (Key == TEXT("tree") || Key == TEXT("gnarled_tree") || Key == TEXT("glowing_twisted_tree"))
        {
            return 1.8f;
        }
        return 1.0f;
    }

    float MapObjectFootprintWorldScale(const FString& MeshKey)
    {
        const FString Key = MeshKey.TrimStartAndEnd().ToLower();
        if (Key == TEXT("bus_yellow") || Key == TEXT("yellow_bus") || Key == TEXT("abandoned_bus"))
        {
            return 3.1f;
        }
        if (Key == TEXT("car_red")
            || Key == TEXT("car_blue")
            || Key == TEXT("red_hatchback")
            || Key == TEXT("burnt_sedan")
            || Key == TEXT("post_apocalyptic_car")
            || Key == TEXT("wrecked_police_car")
            || Key == TEXT("futuristic_police_vehicle")
            || Key == TEXT("ambulance")
            || Key == TEXT("ambulance_van")
            || Key == TEXT("military_ambulance"))
        {
            return 3.25f;
        }
        if (Key == TEXT("barrier")
            || Key == TEXT("concrete_barrier")
            || Key == TEXT("futuristic_barrier")
            || Key == TEXT("shopping_cart_barricade")
            || Key == TEXT("rusty_shopping_cart")
            || Key == TEXT("futuristic_vending_machine")
            || Key == TEXT("toxic_waste_barrels"))
        {
            return 2.85f;
        }
        if (Key == TEXT("mall_block")
            || Key == TEXT("clinic_block")
            || Key == TEXT("reactor_block")
            || Key == TEXT("industrial_tank")
            || Key == TEXT("road_shack")
            || Key == TEXT("shack")
            || Key == TEXT("build_1")
            || Key == TEXT("build_2")
            || Key == TEXT("build_3")
            || Key == TEXT("build_4")
            || Key == TEXT("build_5")
            || Key == TEXT("build_6")
            || Key == TEXT("build_7")
            || Key == TEXT("bullet_bistro_bunker")
            || Key == TEXT("fuel_hell_checkpoint")
            || Key == TEXT("haven_nope_tower")
            || Key == TEXT("fix_or_die_garage")
            || Key == TEXT("almost_alive_clinic")
            || Key == TEXT("hellmart_24_7")
            || Key == TEXT("dead_signal_station")
            || Key == TEXT("abandoned_building")
            || Key == TEXT("abandoned_building_1")
            || Key == TEXT("abandoned_building_2")
            || Key == TEXT("abandoned_gas_station")
            || Key == TEXT("cyberpunk_storefront")
            || Key == TEXT("haunted_house")
            || Key == TEXT("industrial_facility")
            || Key == TEXT("industrial_factory")
            || Key == TEXT("ruined_sci_fi_facility")
            || Key == TEXT("ruined_storefront")
            || Key == TEXT("rusty_gas_station")
            || Key == TEXT("steampunk_reactor"))
        {
            return 1.75f;
        }
        return 2.7f;
    }

    TArray<FString> MapObjectStaticMeshCandidatePaths(const FString& MeshKey)
    {
        const FString Key = MeshKey.TrimStartAndEnd().ToLower();
        TArray<FString> Paths;

        auto AddLegacyTripoPath = [&Paths](const FString& AssetKey)
        {
            Paths.Add(FString::Printf(
                TEXT("/Game/CrimsonWars/MapObjects/Tripo3D/%s/SM_%s.SM_%s"),
                *AssetKey,
                *AssetKey,
                *AssetKey));
        };
        auto AddObjectSceneImportPath = [&Paths](const TCHAR* Folder, const TCHAR* Asset)
        {
            FString AssetName(Asset);
            FString MeshName = AssetName;
            MeshName.RemoveFromStart(TEXT("SceneImport_"));

            Paths.Add(FString::Printf(TEXT("/Game/Objects/%s/%s.%s"), Folder, Asset, *MeshName));
            Paths.Add(FString::Printf(TEXT("/Game/Objects/%s/%s.%s"), Folder, *MeshName, *MeshName));
            Paths.Add(FString::Printf(TEXT("/Game/Objects/%s/%s.%s"), Folder, Asset, Asset));
        };

        if (Key == TEXT("car_red") || Key == TEXT("red_hatchback") || Key == TEXT("post_apocalyptic_car"))
        {
            AddObjectSceneImportPath(TEXT("post+apocalyptic+car+3d+model"), TEXT("SceneImport_tripo_convert_166a9618-f30a-41f0-9786-7b5abaf4d21d"));
        }
        else if (Key == TEXT("car_blue") || Key == TEXT("burnt_sedan") || Key == TEXT("wrecked_police_car"))
        {
            AddObjectSceneImportPath(TEXT("wrecked+police+car+3d+model"), TEXT("SceneImport_tripo_convert_b0137c71-5d6f-46b4-ba3d-885cc05935e9"));
            AddObjectSceneImportPath(TEXT("futuristic+police+vehicle+3d+model"), TEXT("SceneImport_tripo_convert_68919fcf-bfec-4bfb-ad45-cda2aff569cf"));
        }
        else if (Key == TEXT("bus_yellow") || Key == TEXT("yellow_bus") || Key == TEXT("abandoned_bus"))
        {
            AddObjectSceneImportPath(TEXT("abandoned+bus+3d+model"), TEXT("SceneImport_tripo_convert_746aaabf-b333-41bd-9c79-d3062b0ff5c6"));
        }
        else if (Key == TEXT("ambulance") || Key == TEXT("ambulance_van") || Key == TEXT("military_ambulance"))
        {
            AddObjectSceneImportPath(TEXT("military+ambulance+3d+model"), TEXT("SceneImport_tripo_convert_4cfb6c3c-f3d7-413b-ba66-73a1d9494740"));
        }
        else if (Key == TEXT("barrier") || Key == TEXT("concrete_barrier"))
        {
            AddObjectSceneImportPath(TEXT("concrete+barrier+3d+model"), TEXT("SceneImport_tripo_convert_94f2870b-457e-47ab-ac25-2dc2719b3615"));
        }
        else if (Key == TEXT("futuristic_barrier"))
        {
            AddObjectSceneImportPath(TEXT("futuristic+barrier+3d+model"), TEXT("SceneImport_tripo_convert_63d2749a-97a3-4625-949e-b41285a6930e"));
        }
        else if (Key == TEXT("shopping_cart_barricade") || Key == TEXT("rusty_shopping_cart"))
        {
            AddObjectSceneImportPath(TEXT("rusty+shopping+cart+3d+model"), TEXT("SceneImport_tripo_convert_630c93d6-0a49-4249-98a2-407ae2785e60"));
        }
        else if (Key == TEXT("shack") || Key == TEXT("road_shack") || Key == TEXT("abandoned_building") || Key == TEXT("build_1"))
        {
            AddObjectSceneImportPath(TEXT("abandoned+building+3d+model"), TEXT("SceneImport_tripo_convert_1eea6edd-4610-4061-9893-32169f79b0c9"));
            AddObjectSceneImportPath(TEXT("ruined+storefront+3d+model"), TEXT("SceneImport_tripo_convert_3d7f35a3-633b-4d2e-87d0-6145e1a83c24"));
        }
        else if (Key == TEXT("abandoned_building_1") || Key == TEXT("haunted_house") || Key == TEXT("build_3"))
        {
            AddObjectSceneImportPath(TEXT("haunted+house+3d+model"), TEXT("SceneImport_tripo_convert_7d18c390-3684-4ddc-8dd4-c9a19f0a781b"));
            AddObjectSceneImportPath(TEXT("abandoned+building+3d+model (1)"), TEXT("SceneImport_tripo_convert_4c37b9dc-6ac2-492b-a8c6-b3c5ea64bfb9"));
            AddObjectSceneImportPath(TEXT("ruined+storefront+3d+model"), TEXT("SceneImport_tripo_convert_3d7f35a3-633b-4d2e-87d0-6145e1a83c24"));
        }
        else if (Key == TEXT("abandoned_building_2") || Key == TEXT("build_4"))
        {
            AddObjectSceneImportPath(TEXT("abandoned+building+3d+model (2)"), TEXT("SceneImport_tripo_convert_30645e79-fc2a-40d3-a5d7-85700d699422"));
            AddObjectSceneImportPath(TEXT("rusty+gas+station+3d+model"), TEXT("SceneImport_tripo_convert_ac50ea3a-0ab8-4421-ba9c-ef1f9c47a7c1"));
        }
        else if (Key == TEXT("mall_block") || Key == TEXT("cyberpunk_storefront") || Key == TEXT("build_6"))
        {
            AddObjectSceneImportPath(TEXT("cyberpunk+storefront+3d+model"), TEXT("SceneImport_tripo_convert_a5363aa7-7bdd-46b0-b0c3-407f0c62683d"));
            AddObjectSceneImportPath(TEXT("ruined+storefront+3d+model"), TEXT("SceneImport_tripo_convert_3d7f35a3-633b-4d2e-87d0-6145e1a83c24"));
        }
        else if (Key == TEXT("clinic_block") || Key == TEXT("clinic_building") || Key == TEXT("build_5"))
        {
            AddObjectSceneImportPath(TEXT("clinic+building+3d+model"), TEXT("SceneImport_tripo_convert_623e736d-e7b2-426b-af1c-1e7364c71213"));
        }
        else if (Key == TEXT("industrial_tank") || Key == TEXT("toxic_waste_barrels"))
        {
            AddObjectSceneImportPath(TEXT("toxic+waste+barrels+3d+model"), TEXT("SceneImport_tripo_convert_0ce3d759-6967-4178-88f2-b38c477a64b3"));
            AddObjectSceneImportPath(TEXT("industrial+facility+3d+model"), TEXT("SceneImport_tripo_convert_0bf51c8e-2ee2-4178-9fbc-3b568b84bcc5"));
        }
        else if (Key == TEXT("reactor_block") || Key == TEXT("steampunk_reactor"))
        {
            AddObjectSceneImportPath(TEXT("steampunk+reactor+3d+model"), TEXT("SceneImport_tripo_convert_570dbe34-6008-4e09-a466-b47f53a2bee5"));
        }
        else if (Key == TEXT("fuel_hell_checkpoint") || Key == TEXT("build_2") || Key == TEXT("rusty_gas_station"))
        {
            AddObjectSceneImportPath(TEXT("rusty+gas+station+3d+model"), TEXT("SceneImport_tripo_convert_ac50ea3a-0ab8-4421-ba9c-ef1f9c47a7c1"));
            AddObjectSceneImportPath(TEXT("abandoned+gas+station+3d+model"), TEXT("SceneImport_tripo_convert_a7fc3c9a-bb3a-47e6-9209-a9125b03a76b"));
        }
        else if (Key == TEXT("industrial_facility"))
        {
            AddObjectSceneImportPath(TEXT("industrial+facility+3d+model"), TEXT("SceneImport_tripo_convert_0bf51c8e-2ee2-4178-9fbc-3b568b84bcc5"));
        }
        else if (Key == TEXT("industrial_factory"))
        {
            AddObjectSceneImportPath(TEXT("industrial+factory+3d+model"), TEXT("SceneImport_tripo_convert_7d6a58c4-cecb-4c68-b1b0-c62fa6073dad"));
        }
        else if (Key == TEXT("ruined_sci_fi_facility") || Key == TEXT("build_7"))
        {
            AddObjectSceneImportPath(TEXT("ruined+sci-fi+facility+3d+model"), TEXT("SceneImport_tripo_convert_3ffecfb5-0596-4c77-ba18-af2733518b1b"));
            AddObjectSceneImportPath(TEXT("industrial+facility+3d+model"), TEXT("SceneImport_tripo_convert_0bf51c8e-2ee2-4178-9fbc-3b568b84bcc5"));
        }
        else if (Key == TEXT("ruined_storefront"))
        {
            AddObjectSceneImportPath(TEXT("ruined+storefront+3d+model"), TEXT("SceneImport_tripo_convert_3d7f35a3-633b-4d2e-87d0-6145e1a83c24"));
        }
        else if (Key == TEXT("futuristic_vending_machine"))
        {
            AddObjectSceneImportPath(TEXT("futuristic+vending+machine+3d+model"), TEXT("SceneImport_tripo_convert_2f285c81-6a44-43a1-99c0-039928d84f7f"));
        }
        else if (Key == TEXT("futuristic_police_vehicle"))
        {
            AddObjectSceneImportPath(TEXT("futuristic+police+vehicle+3d+model"), TEXT("SceneImport_tripo_convert_68919fcf-bfec-4bfb-ad45-cda2aff569cf"));
        }
        else if (Key == TEXT("glowing_twisted_tree"))
        {
            AddObjectSceneImportPath(TEXT("glowing+twisted+tree+3d+model"), TEXT("SceneImport_tripo_convert_fe86c785-bbee-4299-b080-094260028956"));
        }
        else if (Key == TEXT("gnarled_tree") || Key == TEXT("tree"))
        {
            AddObjectSceneImportPath(TEXT("gnarled+tree+3d+model"), TEXT("SceneImport_tripo_convert_1be4838f-0fbf-4e21-8a78-0c3c57f8badf"));
        }

        AddLegacyTripoPath(Key);
        return Paths;
    }

    void SetupInstanceComponent(UInstancedStaticMeshComponent* Component)
    {
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetMobility(EComponentMobility::Movable);
        Component->NumCustomDataFloats = 0;
        Component->SetCastShadow(false);
    }

    UAnimSequence* LoadFirstAnimSequencePath(std::initializer_list<FString> Paths)
    {
        for (const FString& Path : Paths)
        {
            if (Path.IsEmpty())
            {
                continue;
            }
            if (UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *Path))
            {
                return Anim;
            }
        }
        return nullptr;
    }

    void ConfigureGroundTextureFor3D(UTexture2D* Texture)
    {
        if (!Texture)
        {
            return;
        }

        Texture->SRGB = true;
        Texture->NeverStream = false;
        Texture->LODGroup = TEXTUREGROUP_World;
        Texture->Filter = TF_Trilinear;
        Texture->LODBias = 0;
        Texture->UpdateResource();
    }
}

ACWNativeWorldActor::ACWNativeWorldActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    KeyLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("NativeKeyLight"));
    KeyLight->SetupAttachment(SceneRoot);
    KeyLight->SetRelativeRotation(FRotator(-55.0f, -35.0f, 0.0f));
    KeyLight->SetIntensity(4.2f);
    KeyLight->SetCastShadows(false);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("NativeSkyFill"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetIntensity(1.15f);
    SkyLight->SetMobility(EComponentMobility::Movable);

    GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArenaGround"));
    GroundMesh->SetupAttachment(SceneRoot);
    GroundMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GroundMesh->SetCollisionObjectType(ECC_WorldStatic);
    GroundMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    GroundMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
    GroundMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    GroundMesh->SetMobility(EComponentMobility::Movable);
    GroundMesh->SetCastShadow(false);

    GridLineInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArenaGrid"));
    GridLineInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(GridLineInstances);

    TreeStaticMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TripoTrees"));
    TreeStaticMeshInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(TreeStaticMeshInstances);
    TreeStaticMeshInstances->SetCastShadow(true);

    PlayerInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Players"));
    PlayerInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(PlayerInstances);

    EnemyInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Enemies"));
    EnemyInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(EnemyInstances);

    BulletInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bullets"));
    BulletInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(BulletInstances);

    BulletCoreInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BulletCores"));
    BulletCoreInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(BulletCoreInstances);

    RocketInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rockets"));
    RocketInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(RocketInstances);

    RocketNoseInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RocketNoses"));
    RocketNoseInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(RocketNoseInstances);

    RocketFlameInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RocketFlames"));
    RocketFlameInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(RocketFlameInstances);

    DropInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Drops"));
    DropInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(DropInstances);

    XpVacuumInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpVacuumDrops"));
    XpVacuumInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpVacuumInstances);

    XpOrbInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpOrbs"));
    XpOrbInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpOrbInstances);

    XpOrbHaloInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpOrbHalos"));
    XpOrbHaloInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpOrbHaloInstances);

    XpOrbSparkInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpOrbSparks"));
    XpOrbSparkInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpOrbSparkInstances);

    XpVacuumHaloInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpVacuumHalos"));
    XpVacuumHaloInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpVacuumHaloInstances);

    XpVacuumSparkInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpVacuumSparks"));
    XpVacuumSparkInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpVacuumSparkInstances);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlackUnlitMaterial(TEXT("/Engine/EngineDebugMaterials/BlackUnlitMaterial.BlackUnlitMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialError(TEXT("/Engine/EngineDebugMaterials/MaterialError_Mat.MaterialError_Mat"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlueTemplateMaterial(TEXT("/Engine/TemplateResources/MI_Template_BaseBlue.MI_Template_BaseBlue"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMeshMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentFxMaterial(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ClayPreviewMaterial(TEXT("/Engine/EngineDebugMaterials/M_GeoInsp_Clay.M_GeoInsp_Clay"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveTexturedMaterial(TEXT("/Engine/EngineMaterials/EmissiveTexturedMaterial.EmissiveTexturedMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WidgetTranslucentMaterial(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundTextureMaterial(TEXT("/Game/CrimsonWars/Materials/M_NativeGroundField.M_NativeGroundField"));

    if (CylinderMesh.Succeeded())
    {
        CylinderMeshAsset = CylinderMesh.Object;
        PlayerInstances->SetStaticMesh(CylinderMesh.Object);
        EnemyInstances->SetStaticMesh(CylinderMesh.Object);
        BulletInstances->SetStaticMesh(CylinderMesh.Object);
        RocketInstances->SetStaticMesh(CylinderMesh.Object);
    }

    if (SphereMesh.Succeeded())
    {
        SphereMeshAsset = SphereMesh.Object;
        BulletCoreInstances->SetStaticMesh(SphereMesh.Object);
        RocketNoseInstances->SetStaticMesh(SphereMesh.Object);
        RocketFlameInstances->SetStaticMesh(SphereMesh.Object);
        XpOrbSparkInstances->SetStaticMesh(SphereMesh.Object);
        XpVacuumSparkInstances->SetStaticMesh(SphereMesh.Object);
    }

    if (CubeMesh.Succeeded())
    {
        CubeMeshAsset = CubeMesh.Object;
        GridLineInstances->SetStaticMesh(CubeMesh.Object);
        DropInstances->SetStaticMesh(CubeMesh.Object);
        if (BulletInstances && !BulletInstances->GetStaticMesh())
        {
            BulletInstances->SetStaticMesh(CubeMesh.Object);
        }
        if (RocketInstances && !RocketInstances->GetStaticMesh())
        {
            RocketInstances->SetStaticMesh(CubeMesh.Object);
        }
        XpVacuumInstances->SetStaticMesh(CubeMesh.Object);
        XpOrbInstances->SetStaticMesh(CubeMesh.Object);
    }
    if (ConeMesh.Succeeded())
    {
        XpVacuumInstances->SetStaticMesh(ConeMesh.Object);
        XpOrbInstances->SetStaticMesh(ConeMesh.Object);
    }
    if (PlaneMesh.Succeeded())
    {
        PlaneMeshAsset = PlaneMesh.Object;
        GroundMesh->SetStaticMesh(PlaneMesh.Object);
    }
    else if (CubeMesh.Succeeded())
    {
        GroundMesh->SetStaticMesh(CubeMesh.Object);
    }

    if (CylinderMesh.Succeeded())
    {
        XpOrbHaloInstances->SetStaticMesh(CylinderMesh.Object);
        XpVacuumHaloInstances->SetStaticMesh(CylinderMesh.Object);
    }

    if (ShapeMaterial.Succeeded())
    {
        BaseShapeMaterial = ShapeMaterial.Object;
    }
    if (BlackUnlitMaterial.Succeeded())
    {
        BlackMaterial = BlackUnlitMaterial.Object;
    }
    if (MaterialError.Succeeded())
    {
        EnemyFallbackMaterial = MaterialError.Object;
    }
    if (BlueTemplateMaterial.Succeeded())
    {
        PlayerFallbackMaterial = BlueTemplateMaterial.Object;
    }
    if (EmissiveMeshMaterial.Succeeded())
    {
        EmissiveFallbackMaterial = EmissiveMeshMaterial.Object;
    }
    if (TranslucentFxMaterial.Succeeded())
    {
        TranslucentFxMaterialParent = TranslucentFxMaterial.Object;
    }
    if (ClayPreviewMaterial.Succeeded())
    {
        SkeletalEnemyPreviewMaterial = ClayPreviewMaterial.Object;
    }
    if (EmissiveTexturedMaterial.Succeeded())
    {
        SkeletalTextureMaterialParent = EmissiveTexturedMaterial.Object;
    }
    if (WidgetTranslucentMaterial.Succeeded())
    {
        WorldIconTextureMaterialParent = WidgetTranslucentMaterial.Object;
    }
    if (GroundTextureMaterial.Succeeded())
    {
        GroundTextureMaterialParent = GroundTextureMaterial.Object;
    }

    if (EmissiveFallbackMaterial)
    {
        EmissiveFallbackMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh);
    }
    if (SkeletalEnemyPreviewMaterial)
    {
        SkeletalEnemyPreviewMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh);
    }
    if (SkeletalTextureMaterialParent)
    {
        SkeletalTextureMaterialParent->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh);
        SkeletalTextureMaterialParent->CheckMaterialUsage_Concurrent(MATUSAGE_StaticMesh);
    }
    if (TranslucentFxMaterialParent)
    {
        TranslucentFxMaterialParent->CheckMaterialUsage_Concurrent(MATUSAGE_StaticMesh);
    }
    if (WorldIconTextureMaterialParent)
    {
        WorldIconTextureMaterialParent->CheckMaterialUsage_Concurrent(MATUSAGE_StaticMesh);
    }
}

void ACWNativeWorldActor::BeginPlay()
{
    Super::BeginPlay();

    bEnableRuntimeFx = true;
    SetupArenaVisuals();

    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        AssetsRoot = GI->WebAssetsRoot;
        StateDelegateHandle = GI->OnStateReceived.AddUObject(this, &ACWNativeWorldActor::HandleStateReceived);
        BootstrapDelegateHandle = GI->OnBootstrapReceived.AddUObject(this, &ACWNativeWorldActor::HandleBootstrapReceived);
        SkillFxDelegateHandle = GI->OnSkillFxReceived.AddUObject(this, &ACWNativeWorldActor::HandleSkillFxReceived);
        MeleeFxDelegateHandle = GI->OnMeleeFxReceived.AddUObject(this, &ACWNativeWorldActor::HandleMeleeFxReceived);
        WorldFxDelegateHandle = GI->OnWorldFxReceived.AddUObject(this, &ACWNativeWorldActor::HandleWorldFxReceived);
        HandleBootstrapReceived(GI->Bootstrap);
        LatestStateReceivedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        RenderState(GI->LatestState);
    }
    if (AssetsRoot.IsEmpty())
    {
        AssetsRoot = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
    }
}

void ACWNativeWorldActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        if (StateDelegateHandle.IsValid())
        {
            GI->OnStateReceived.Remove(StateDelegateHandle);
            StateDelegateHandle.Reset();
        }
        if (BootstrapDelegateHandle.IsValid())
        {
            GI->OnBootstrapReceived.Remove(BootstrapDelegateHandle);
            BootstrapDelegateHandle.Reset();
        }
        if (SkillFxDelegateHandle.IsValid())
        {
            GI->OnSkillFxReceived.Remove(SkillFxDelegateHandle);
            SkillFxDelegateHandle.Reset();
        }
        if (MeleeFxDelegateHandle.IsValid())
        {
            GI->OnMeleeFxReceived.Remove(MeleeFxDelegateHandle);
            MeleeFxDelegateHandle.Reset();
        }
        if (WorldFxDelegateHandle.IsValid())
        {
            GI->OnWorldFxReceived.Remove(WorldFxDelegateHandle);
            WorldFxDelegateHandle.Reset();
        }
    }

    for (FCWActiveFxComponent& Fx : ActiveFxComponents)
    {
        if (Fx.Component)
        {
            Fx.Component->DestroyComponent();
        }
    }
    ActiveFxComponents.Reset();
    DestroyMapObjectStaticMeshVisuals();
    DestroySkillOrbWorldVisuals();
    for (TPair<FString, FCWPickupWorldVisual>& Pair : WeaponPickupWorldVisuals)
    {
        DestroyPickupWorldVisual(Pair.Value);
    }
    WeaponPickupWorldVisuals.Reset();
    DestroyPlayerSkeletalVisuals();
    DestroyEnemySkeletalVisuals();

    Super::EndPlay(EndPlayReason);
}

void ACWNativeWorldActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateSkeletalVisualTargets(DeltaSeconds);
    if (bHasPreviousState)
    {
        RebuildPickupInstances(PreviousState, DeltaSeconds);
        RebuildProjectileInstances(DeltaSeconds);
    }

    if (!bEnableRuntimeFx && ActiveFxComponents.Num() > 0)
    {
        for (FCWActiveFxComponent& Fx : ActiveFxComponents)
        {
            if (Fx.Component)
            {
                Fx.Component->DestroyComponent();
            }
        }
        ActiveFxComponents.Reset();
    }

    TArray<TPair<FVector, float>> PendingBloodSplats;
    for (int32 Index = ActiveFxComponents.Num() - 1; Index >= 0; --Index)
    {
        FCWActiveFxComponent& Fx = ActiveFxComponents[Index];
        if (!Fx.Component)
        {
            ActiveFxComponents.RemoveAtSwap(Index);
            continue;
        }

        Fx.Age += DeltaSeconds;
        if (!Fx.Velocity.IsNearlyZero() || !Fx.Acceleration.IsNearlyZero())
        {
            Fx.Component->AddWorldOffset(Fx.Velocity * DeltaSeconds + Fx.Acceleration * (0.5f * DeltaSeconds * DeltaSeconds));
            Fx.Velocity += Fx.Acceleration * DeltaSeconds;
            if (Fx.bAlignToVelocity && !Fx.Velocity.IsNearlyZero())
            {
                Fx.Component->SetWorldRotation(Fx.Velocity.Rotation());
            }
        }

        if (Fx.bBloodDroplet)
        {
            const FVector DropLocation = Fx.Component->GetComponentLocation();
            if (DropLocation.Z <= GroundZ + 3.0f && Fx.Velocity.Z <= 0.0f)
            {
                if (Fx.bLeavesBloodSplat)
                {
                    PendingBloodSplats.Add(TPair<FVector, float>(FVector(DropLocation.X, DropLocation.Y, GroundZ + 1.4f), Fx.BloodSplatRadius));
                }
                Fx.Component->DestroyComponent();
                ActiveFxComponents.RemoveAtSwap(Index);
                continue;
            }
        }

        const float T = Fx.Life > KINDA_SMALL_NUMBER ? FMath::Clamp(Fx.Age / Fx.Life, 0.0f, 1.0f) : 1.0f;
        const float ScaleT = Fx.bGroundBloodPool
            ? FMath::Clamp(Fx.Age / 0.22f, 0.0f, 1.0f)
            : T;
        const float Ease = 1.0f - FMath::Pow(1.0f - ScaleT, 2.0f);
        Fx.Component->SetWorldScale3D(FMath::Lerp(Fx.StartScale, Fx.EndScale, Ease));
        if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Fx.Component->GetMaterial(0)))
        {
            const float FadeStartAge = FMath::Max(0.0f, Fx.Life - FMath::Max(0.05f, Fx.FadeOutSeconds));
            const float FadeT = FMath::Clamp((Fx.Age - FadeStartAge) / FMath::Max(0.05f, Fx.FadeOutSeconds), 0.0f, 1.0f);
            const float Fade = 1.0f - (FadeT * FadeT * (3.0f - 2.0f * FadeT));
            FLinearColor FadedColor = Fx.BaseColor;
            FadedColor.A *= Fade;
            Material->SetVectorParameterValue(TEXT("Color"), FadedColor);
            Material->SetVectorParameterValue(TEXT("BaseColor"), FadedColor);
            Material->SetVectorParameterValue(TEXT("DiffuseColor"), FadedColor);
            Material->SetVectorParameterValue(TEXT("TintColor"), FadedColor);
            Material->SetVectorParameterValue(TEXT("SpriteColor"), FadedColor);
            Material->SetVectorParameterValue(TEXT("EmissiveColor"), FadedColor * FMath::Max(0.0f, Fx.EmissiveStrength) * Fade);
            Material->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(FadedColor.A, 0.0f, 1.0f));
            Material->SetScalarParameterValue(TEXT("Alpha"), FMath::Clamp(FadedColor.A, 0.0f, 1.0f));
        }
        if (FMath::Abs(Fx.SpinYawDegPerSec) > KINDA_SMALL_NUMBER)
        {
            Fx.Component->AddWorldRotation(FRotator(0.0f, Fx.SpinYawDegPerSec * DeltaSeconds, 0.0f));
        }

        if (T >= 1.0f)
        {
            Fx.Component->DestroyComponent();
            ActiveFxComponents.RemoveAtSwap(Index);
        }
    }

    for (const TPair<FVector, float>& BloodSplat : PendingBloodSplats)
    {
        SpawnBloodPoolFx(BloodSplat.Key, BloodSplat.Value, FMath::Lerp(2.6f, 5.2f, StableUnitFromString(FString::Printf(TEXT("blood_splat_%0.1f_%0.1f"), BloodSplat.Key.X, BloodSplat.Key.Y))), BloodSplat.Value);
    }

    for (int32 Index = ActiveEnemyDeathVisuals.Num() - 1; Index >= 0; --Index)
    {
        FCWActiveEnemyDeathVisual& Visual = ActiveEnemyDeathVisuals[Index];
        if (!Visual.Component)
        {
            ActiveEnemyDeathVisuals.RemoveAtSwap(Index);
            continue;
        }

        Visual.Age += DeltaSeconds;
        auto UpdateCorpseSink = [this, &Visual, Index]() -> bool
        {
            if (!Visual.bSinkStarted)
            {
                return false;
            }

            const float SinkT = FMath::Clamp((Visual.Age - Visual.SinkStartAge) / FMath::Max(0.1f, Visual.SinkDuration), 0.0f, 1.0f);
            const float Ease = SinkT * SinkT * (3.0f - 2.0f * SinkT);
            Visual.Component->SetWorldLocation(Visual.SinkStartLocation - FVector(0.0f, 0.0f, Visual.SinkDepth * Ease));
            Visual.Component->SetWorldScale3D(Visual.SinkStartScale * FMath::Lerp(1.0f, 0.92f, SinkT));
            if (SinkT >= 1.0f)
            {
                Visual.Component->DestroyComponent();
                ActiveEnemyDeathVisuals.RemoveAtSwap(Index);
            }
            return true;
        };

        auto BeginCorpseSink = [this, &Visual]()
        {
            Visual.bSinkStarted = true;
            Visual.SinkStartAge = Visual.Age;
            Visual.SinkStartLocation = Visual.Component->GetComponentLocation();
            Visual.SinkStartScale = Visual.Component->GetComponentScale();
            Visual.Component->SetSimulatePhysics(false);
            Visual.Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            if (!Visual.bBloodCleanupSpawned)
            {
                Visual.bBloodCleanupSpawned = true;
                const FVector CleanupLocation(Visual.SinkStartLocation.X, Visual.SinkStartLocation.Y, GroundZ + 8.0f);
                SpawnBloodPoolFx(CleanupLocation, 42.0f, 4.2f, Visual.Age);
                SpawnParticleSprayFx(
                    CleanupLocation + FVector(0.0f, 0.0f, 18.0f),
                    FVector::UpVector,
                    FVector::RightVector,
                    FLinearColor(0.15f, 0.002f, 0.006f, 1.0f),
                    8,
                    35.0f,
                    140.0f,
                    0.55f,
                    0.035f,
                    0.09f,
                    55.0f,
                    -260.0f);
            }
        };

        if (Visual.bRagdoll)
        {
            if (UpdateCorpseSink())
            {
                continue;
            }

            const float PhysicsSpeed = FMath::Max(Visual.Component->GetPhysicsLinearVelocity().Size(), Visual.Component->GetComponentVelocity().Size());
            if (Visual.Age > 0.9f && PhysicsSpeed < 28.0f)
            {
                Visual.StillAge += DeltaSeconds;
            }
            else
            {
                Visual.StillAge = 0.0f;
            }

            if (Visual.StillAge > 0.85f || Visual.Age > 5.5f)
            {
                BeginCorpseSink();
                continue;
            }
        }

        if (Visual.bKinematicFall)
        {
            if (UpdateCorpseSink())
            {
                continue;
            }

            const float FallT = FMath::Clamp(Visual.Age / FMath::Max(0.1f, Visual.FallDuration), 0.0f, 1.0f);
            const float FallEase = FallT * FallT * (3.0f - 2.0f * FallT);
            const FQuat FallRotation = FQuat::Slerp(Visual.FallStartRotation.Quaternion(), Visual.FallEndRotation.Quaternion(), FallEase);
            Visual.Component->SetWorldRotation(FallRotation);
            Visual.Component->SetWorldLocation(Visual.FallStartLocation - FVector(0.0f, 0.0f, FMath::Lerp(0.0f, 16.0f, FallEase)));

            if (Visual.Age > Visual.FallDuration + Visual.LieDuration)
            {
                BeginCorpseSink();
                continue;
            }
        }

        if (Visual.Age >= Visual.Life)
        {
            Visual.Component->DestroyComponent();
            ActiveEnemyDeathVisuals.RemoveAtSwap(Index);
        }
    }
}

void ACWNativeWorldActor::HandleStateReceived(const FCWRoomSnapshot& State)
{
    LatestStateReceivedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    RenderState(State);
}

void ACWNativeWorldActor::HandleBootstrapReceived(const FCWNativeBootstrapSnapshot& Bootstrap)
{
    FxProfiles = Bootstrap.FxProfiles;
}

void ACWNativeWorldActor::HandleSkillFxReceived(const FCWSkillFxEvent& Event)
{
    if (!Event.Id.IsEmpty())
    {
        if (SeenSkillFxEventIds.Contains(Event.Id))
        {
            return;
        }
        SeenSkillFxEventIds.Add(Event.Id);
        if (SeenSkillFxEventIds.Num() > 512)
        {
            SeenSkillFxEventIds.Reset();
        }
    }
    MarkPlayerAttackVisual(Event.PlayerId, FVector2D(Event.X, Event.Y), FVector2D(Event.AimX, Event.AimY), 0.44f);
    SpawnSkillEventFx(Event);
}

void ACWNativeWorldActor::HandleMeleeFxReceived(const FCWMeleeFxEvent& Event)
{
    MarkPlayerAttackVisual(Event.PlayerId, FVector2D(Event.X, Event.Y), FVector2D(Event.ImpactX, Event.ImpactY), 0.52f);
    SpawnMeleeStyleFx(Event);
}

void ACWNativeWorldActor::HandleWorldFxReceived(const FCWWorldFxEvent& Event)
{
    if (Event.Kind.Equals(TEXT("xp_surge_pull"), ESearchCase::IgnoreCase) || Event.FxKey.Equals(TEXT("world.xp_surge_pull"), ESearchCase::IgnoreCase))
    {
        SpawnXpSurgePullFx(Event);
    }
    else
    {
        SpawnGenericWorldFx(Event);
    }
}

FString ACWNativeWorldActor::MakeRunIdentity(const FCWRoomSnapshot& State) const
{
    if (State.RoomCode.IsEmpty())
    {
        return FString();
    }

    return FString::Printf(TEXT("%s|%.0f"), *State.RoomCode, State.RoomStartedAt);
}

bool ACWNativeWorldActor::IsEmptyRoomState(const FCWRoomSnapshot& State) const
{
    return State.RoomCode.IsEmpty()
        && State.Players.IsEmpty()
        && State.Enemies.IsEmpty()
        && State.Bullets.IsEmpty();
}

void ACWNativeWorldActor::ResetTransientRunVisualState()
{
    for (FCWActiveFxComponent& Fx : ActiveFxComponents)
    {
        if (Fx.Component)
        {
            Fx.Component->DestroyComponent();
        }
    }
    ActiveFxComponents.Reset();

    DestroyPlayerSkeletalVisuals();
    DestroyEnemySkeletalVisuals();
    DestroySkillOrbWorldVisuals();
    for (TPair<FString, FCWPickupWorldVisual>& Pair : WeaponPickupWorldVisuals)
    {
        DestroyPickupWorldVisual(Pair.Value);
    }
    WeaponPickupWorldVisuals.Reset();

    PreviousState = FCWRoomSnapshot();
    bHasPreviousState = false;
    SeenSkillFxEventIds.Reset();
    SeenShotEventIds.Reset();
    SeenObjectImpactEventIds.Reset();
    PreviousSkillCooldowns.Reset();
    LastRocketBullets.Reset();
    RetiredProjectileVisualUntil.Reset();
    RecentProjectileImpactClips.Reset();
    SeenEnemyDeathBloodIds.Reset();
    PlayerPreviousLocations.Reset();
    PlayerPreviousSampleTimes.Reset();
    PlayerAttackVisualUntil.Reset();
    PlayerAttackVisualRotations.Reset();
    PlayerSkeletalTargets.Reset();
    EnemyPreviousLocations.Reset();
    EnemyPreviousSampleTimes.Reset();
    EnemySkeletalTargets.Reset();
    DropRenderPositions.Reset();
    XpOrbRenderPositions.Reset();
    XpVacuumRenderPositions.Reset();
    SkillOrbRenderPositions.Reset();
    SkillOrbAuraPulseTimes.Reset();
    ProjectileVisualStates.Reset();
}

void ACWNativeWorldActor::SyncRunIdentity(const FCWRoomSnapshot& State)
{
    if (IsEmptyRoomState(State))
    {
        ResetTransientRunVisualState();
        LastRunIdentity.Reset();
        return;
    }

    const FString RunIdentity = MakeRunIdentity(State);
    const bool bIdentityChanged = !RunIdentity.IsEmpty() && !LastRunIdentity.IsEmpty() && RunIdentity != LastRunIdentity;
    const bool bKillCounterRewound = bHasPreviousState
        && !State.RoomCode.IsEmpty()
        && State.RoomCode == PreviousState.RoomCode
        && State.TotalEnemyKills < PreviousState.TotalEnemyKills;

    if (bIdentityChanged || bKillCounterRewound)
    {
        ResetTransientRunVisualState();
    }

    if (!RunIdentity.IsEmpty())
    {
        LastRunIdentity = RunIdentity;
    }
}

void ACWNativeWorldActor::RenderState(const FCWRoomSnapshot& State)
{
    HideTemplateFloorActors();
    SyncRunIdentity(State);

    EmitStateDrivenFx(State);
    EnsureGroundMaterial();
    ApplyGroundTextureForState(State);
    if (GroundMesh)
    {
        GroundMesh->SetMaterial(0, GroundMaterial ? GroundMaterial : BlackMaterial);
    }
    if (GridLineInstances)
    {
        GridLineInstances->SetVisibility(bShowDebugGrid, true);
        GridLineInstances->SetMaterial(0, GridMaterial ? GridMaterial : PlayerFallbackMaterial);
    }
    UpdateArenaSurface(State);
    if (bUseStaticMeshMapObjects)
    {
        UpdateMapObjectStaticMeshVisuals(State);
        UpdateTreeStaticMeshVisuals(State);
    }
    else
    {
        DestroyMapObjectStaticMeshVisuals();
    }

    PlayerInstances->ClearInstances();
    EnemyInstances->ClearInstances();
    RebuildPickupInstances(State, bHasPreviousState && GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f);
    SyncProjectileVisualTargets(State);

    if (bUseSkeletalPlayerVisuals)
    {
        RetireMissingPlayerSkeletalVisuals(State);
    }
    else
    {
        DestroyPlayerSkeletalVisuals();
    }

    TSet<FString> AllowedSkeletalEnemyIds;
    if (bUseSkeletalEnemyVisuals)
    {
        FVector2D FocusLocation = FVector2D::ZeroVector;
        bool bHasFocusLocation = false;
        for (const FCWPlayerSnapshot& Player : State.Players)
        {
            if (Player.bAlive && !Player.bIsCompanion)
            {
                FocusLocation = FVector2D(Player.X, Player.Y);
                bHasFocusLocation = true;
                break;
            }
        }
        if (!bHasFocusLocation && State.Players.Num() > 0)
        {
            FocusLocation = FVector2D(State.Players[0].X, State.Players[0].Y);
            bHasFocusLocation = true;
        }

        struct FCWEnemySkeletalCandidate
        {
            FString Id;
            float DistanceSq = 0.0f;
            bool bImportant = false;
        };

        if (bHasFocusLocation)
        {
            constexpr int32 MaxAnimatedEnemyVisuals = 120;
            constexpr float AnimatedEnemyRadiusSq = 2600.0f * 2600.0f;
            TArray<FCWEnemySkeletalCandidate> Candidates;
            Candidates.Reserve(State.Enemies.Num());
            for (const FCWEnemySnapshot& Enemy : State.Enemies)
            {
                if (Enemy.Id.IsEmpty())
                {
                    continue;
                }

                FCWEnemySkeletalCandidate Candidate;
                Candidate.Id = Enemy.Id;
                Candidate.DistanceSq = FVector2D::DistSquared(FocusLocation, FVector2D(Enemy.X, Enemy.Y));
                Candidate.bImportant = Enemy.Radius >= 38.0f
                    || Enemy.SpriteScale >= 1.35f
                    || Enemy.Type.Contains(TEXT("boss"), ESearchCase::IgnoreCase)
                    || Enemy.MobId.Contains(TEXT("boss"), ESearchCase::IgnoreCase);
                if (Candidate.DistanceSq <= AnimatedEnemyRadiusSq || Candidate.bImportant)
                {
                    Candidates.Add(MoveTemp(Candidate));
                }
            }

            Candidates.Sort([](const FCWEnemySkeletalCandidate& A, const FCWEnemySkeletalCandidate& B)
            {
                if (A.bImportant != B.bImportant)
                {
                    return A.bImportant;
                }
                return A.DistanceSq < B.DistanceSq;
            });

            const int32 CandidateCount = FMath::Min(MaxAnimatedEnemyVisuals, Candidates.Num());
            for (int32 Index = 0; Index < CandidateCount; ++Index)
            {
                AllowedSkeletalEnemyIds.Add(Candidates[Index].Id);
            }
        }

        TArray<FString> CulledSkeletalIds;
        for (const TPair<FString, USkeletalMeshComponent*>& Pair : EnemySkeletalComponents)
        {
            if (!AllowedSkeletalEnemyIds.Contains(Pair.Key))
            {
                CulledSkeletalIds.Add(Pair.Key);
            }
        }
        for (const FString& EnemyId : CulledSkeletalIds)
        {
            USkeletalMeshComponent* Component = nullptr;
            EnemySkeletalComponents.RemoveAndCopyValue(EnemyId, Component);
            EnemySkeletalMobIds.Remove(EnemyId);
            EnemyPreviousLocations.Remove(EnemyId);
            EnemyPreviousSampleTimes.Remove(EnemyId);
            EnemySkeletalTargets.Remove(EnemyId);
            if (Component)
            {
                Component->DestroyComponent();
            }
        }

        RetireMissingEnemySkeletalVisuals(State);
    }
    else
    {
        DestroyEnemySkeletalVisuals();
    }

    for (const FCWPlayerSnapshot& Player : State.Players)
    {
        if (bUseSkeletalPlayerVisuals && UpdatePlayerSkeletalVisual(Player))
        {
            continue;
        }
        const float Radius = Player.bIsCompanion ? 20.0f : 28.0f;
        PlayerInstances->AddInstance(MakeCylinderTransform(Player.X, Player.Y, Radius, Player.bAlive ? 120.0f : 28.0f));
    }

    for (const FCWEnemySnapshot& Enemy : State.Enemies)
    {
        if (bUseSkeletalEnemyVisuals && AllowedSkeletalEnemyIds.Contains(Enemy.Id) && UpdateEnemySkeletalVisual(Enemy, State))
        {
            continue;
        }
        EnemyInstances->AddInstance(MakeCylinderTransform(Enemy.X, Enemy.Y, FMath::Max(8.0f, Enemy.Radius), FMath::Max(40.0f, Enemy.Radius * 3.0f)));
    }

    if (!bLoggedFirstState && State.Players.Num() > 0)
    {
        bLoggedFirstState = true;
        UE_LOG(LogTemp, Display, TEXT("Crimson Wars native world: rendered state map=%s size=%.0fx%.0f players=%d enemies=%d bullets=%d"),
            *State.MapId,
            State.World.Width,
            State.World.Height,
            State.Players.Num(),
            State.Enemies.Num(),
            State.Bullets.Num());
    }

    if (IsEmptyRoomState(State))
    {
        PreviousState = FCWRoomSnapshot();
        bHasPreviousState = false;
    }
    else
    {
        PreviousState = State;
        bHasPreviousState = true;
    }
}

void ACWNativeWorldActor::EmitStateDrivenFx(const FCWRoomSnapshot& State)
{
    TMap<FString, float> NextSkillCooldowns;
    TMap<FString, FCWBulletSnapshot> CurrentRocketBullets;

    SpawnStateDrivenEnemyBloodFx(State);

    if (bHasPreviousState)
    {
        for (const FCWPlayerSnapshot& Player : State.Players)
        {
            for (const FCWSkillSnapshot& Skill : Player.Skills)
            {
                if (Skill.Level <= 0 || Skill.Id.IsEmpty())
                {
                    continue;
                }

                const FString SkillKey = FString::Printf(TEXT("%s:%s"), *Player.Id, *Skill.Id);
                const float PreviousCooldown = PreviousSkillCooldowns.FindRef(SkillKey);
                const bool bKnownSkill = PreviousSkillCooldowns.Contains(SkillKey);
                const bool bActive = Skill.Kind.Equals(TEXT("active"), ESearchCase::IgnoreCase);
                const bool bFreshCooldown = Skill.CooldownMs > 120.0f && (!bKnownSkill || PreviousCooldown <= 85.0f || Skill.CooldownMs > PreviousCooldown + 350.0f);
                const bool bNewPassive = !bActive && !bKnownSkill;
                if (bActive && bFreshCooldown)
                {
                    SpawnSkillCastFx(Player, Skill, State);
                }
                else if (bNewPassive)
                {
                    const FLinearColor Color = ResolveFxColor(FindFxProfile(Skill.FxKey) ? FindFxProfile(Skill.FxKey)->PrimaryColor : Skill.FxKey, FLinearColor(0.45f, 0.8f, 1.0f, 1.0f));
                    SpawnRingFx(MakeWorldLocation(Player.X, Player.Y, GroundFxZ + 5.0f), 70.0f, Color, 0.5f, 2.0f, 2.0f);
                }

                NextSkillCooldowns.Add(SkillKey, Skill.CooldownMs);
            }
        }
    }
    else
    {
        for (const FCWPlayerSnapshot& Player : State.Players)
        {
            for (const FCWSkillSnapshot& Skill : Player.Skills)
            {
                if (Skill.Level > 0 && !Skill.Id.IsEmpty())
                {
                    NextSkillCooldowns.Add(FString::Printf(TEXT("%s:%s"), *Player.Id, *Skill.Id), Skill.CooldownMs);
                }
            }
        }
    }

    for (const FCWShotEventSnapshot& Event : State.ShotEvents)
    {
        if (Event.Id.IsEmpty() || SeenShotEventIds.Contains(Event.Id))
        {
            continue;
        }
        SeenShotEventIds.Add(Event.Id);
        SpawnProjectileShotFx(Event);
    }
    if (SeenShotEventIds.Num() > 512)
    {
        SeenShotEventIds.Reset();
    }

    for (const FCWObjectImpactEventSnapshot& Event : State.ObjectImpactEvents)
    {
        if (Event.Id.IsEmpty() || SeenObjectImpactEventIds.Contains(Event.Id))
        {
            continue;
        }
        SeenObjectImpactEventIds.Add(Event.Id);
        SpawnObjectImpactFx(Event);
    }
    if (SeenObjectImpactEventIds.Num() > 512)
    {
        SeenObjectImpactEventIds.Reset();
    }

    for (const FCWBulletSnapshot& Bullet : State.Bullets)
    {
        const bool bRocket = Bullet.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase) || Bullet.FxKey.Equals(TEXT("projectile.rocket"), ESearchCase::IgnoreCase);
        if (!bRocket || Bullet.Id.IsEmpty())
        {
            continue;
        }
        CurrentRocketBullets.Add(Bullet.Id, Bullet);
        AddRocketTrailFx(Bullet);
    }

    if (bHasPreviousState)
    {
        for (const TPair<FString, FCWBulletSnapshot>& Pair : LastRocketBullets)
        {
            if (!CurrentRocketBullets.Contains(Pair.Key))
            {
                if (!RetiredProjectileVisualUntil.Contains(Pair.Key))
                {
                    SpawnRocketExplosionFx(Pair.Value, State);
                }
            }
        }
    }

    PreviousSkillCooldowns = MoveTemp(NextSkillCooldowns);
    LastRocketBullets = MoveTemp(CurrentRocketBullets);
}

void ACWNativeWorldActor::SpawnStateDrivenEnemyBloodFx(const FCWRoomSnapshot& State)
{
    if (!bHasPreviousState)
    {
        return;
    }

    TMap<FString, FCWEnemySnapshot> PreviousEnemiesById;
    PreviousEnemiesById.Reserve(PreviousState.Enemies.Num());
    for (const FCWEnemySnapshot& Enemy : PreviousState.Enemies)
    {
        if (!Enemy.Id.IsEmpty())
        {
            PreviousEnemiesById.Add(Enemy.Id, Enemy);
        }
    }

    TSet<FString> LiveEnemyIds;
    LiveEnemyIds.Reserve(State.Enemies.Num());
    for (const FCWEnemySnapshot& Enemy : State.Enemies)
    {
        if (Enemy.Id.IsEmpty())
        {
            continue;
        }

        LiveEnemyIds.Add(Enemy.Id);
        const FCWEnemySnapshot* PreviousEnemy = PreviousEnemiesById.Find(Enemy.Id);
        if (!PreviousEnemy)
        {
            continue;
        }

        const float HpLost = PreviousEnemy->Hp - Enemy.Hp;
        if (HpLost <= 0.5f)
        {
            continue;
        }

        FVector2D HitDirection = FVector2D(Enemy.X - PreviousEnemy->X, Enemy.Y - PreviousEnemy->Y).GetSafeNormal();
        float BestPlayerDistSq = TNumericLimits<float>::Max();
        for (const FCWPlayerSnapshot& Player : State.Players)
        {
            if (!Player.bAlive)
            {
                continue;
            }

            const FVector2D CandidateDirection(Enemy.X - Player.X, Enemy.Y - Player.Y);
            const float DistSq = CandidateDirection.SizeSquared();
            if (DistSq < BestPlayerDistSq && DistSq > 1.0f)
            {
                BestPlayerDistSq = DistSq;
                HitDirection = CandidateDirection.GetSafeNormal();
            }
        }
        if (HitDirection.IsNearlyZero())
        {
            HitDirection = FVector2D(1.0f, 0.0f);
        }

        const float DamageScale = FMath::Clamp(HpLost / FMath::Max(1.0f, Enemy.MaxHp) * 5.0f, 0.45f, 2.2f);
        const FVector BloodLocation = MakeWorldLocation(Enemy.X, Enemy.Y, FMath::Clamp(Enemy.Radius * 2.4f, 34.0f, 92.0f));
        SpawnBloodHitFx(BloodLocation, FVector(HitDirection.X, HitDirection.Y, 0.0f), DamageScale, FMath::Max(Enemy.Radius, 14.0f));
        AddProjectileImpactClip(FString(), FVector2D(Enemy.X, Enemy.Y), HitDirection, FMath::Max(Enemy.Radius * 2.7f, 86.0f), 0.32f);
    }

    for (const FCWEnemySnapshot& PreviousEnemy : PreviousState.Enemies)
    {
        if (PreviousEnemy.Id.IsEmpty() || LiveEnemyIds.Contains(PreviousEnemy.Id) || SeenEnemyDeathBloodIds.Contains(PreviousEnemy.Id))
        {
            continue;
        }

        SpawnEnemyDeathVisual(PreviousEnemy, nullptr);
        SpawnBloodDeathFx(PreviousEnemy);
    }
}

UMaterialInstanceDynamic* ACWNativeWorldActor::MakeFxMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength)
{
    UMaterialInterface* Parent = TranslucentFxMaterialParent ? TranslucentFxMaterialParent : (BaseShapeMaterial ? BaseShapeMaterial : EmissiveFallbackMaterial);
    if (!Parent)
    {
        return nullptr;
    }

    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent, this, FName(*Name));
    if (!Material)
    {
        return nullptr;
    }

    const FLinearColor HotColor = Color * FMath::Max(1.0f, EmissiveStrength);
    Material->SetVectorParameterValue(TEXT("Color"), Color);
    Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
    Material->SetVectorParameterValue(TEXT("DiffuseColor"), Color);
    Material->SetVectorParameterValue(TEXT("TintColor"), Color);
    Material->SetVectorParameterValue(TEXT("SpriteColor"), Color);
    Material->SetVectorParameterValue(TEXT("EmissiveColor"), HotColor);
    Material->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Color.A, 0.0f, 1.0f));
    Material->SetScalarParameterValue(TEXT("Alpha"), FMath::Clamp(Color.A, 0.0f, 1.0f));
    Material->SetScalarParameterValue(TEXT("EmissiveStrength"), FMath::Max(0.0f, EmissiveStrength));
    Material->SetScalarParameterValue(TEXT("Roughness"), 0.28f);
    return Material;
}

UMaterialInstanceDynamic* ACWNativeWorldActor::MakeTexturedFxMaterial(const FString& Name, UTexture2D* Texture, const FLinearColor& Tint, float EmissiveStrength)
{
    UMaterialInterface* Parent = WorldIconTextureMaterialParent ? WorldIconTextureMaterialParent : (SkeletalTextureMaterialParent ? SkeletalTextureMaterialParent : EmissiveFallbackMaterial);
    if (!Parent || !Texture)
    {
        return nullptr;
    }

    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent, this, FName(*Name));
    if (!Material)
    {
        return nullptr;
    }

    Material->SetTextureParameterValue(TEXT("SpriteTexture"), Texture);
    Material->SetTextureParameterValue(TEXT("SlateUI"), Texture);
    Material->SetTextureParameterValue(TEXT("LinearColor"), Texture);
    Material->SetTextureParameterValue(TEXT("Texture"), Texture);
    Material->SetTextureParameterValue(TEXT("EmissiveTexture"), Texture);
    Material->SetTextureParameterValue(TEXT("BaseTexture"), Texture);
    Material->SetTextureParameterValue(TEXT("BaseColorMap"), Texture);
    Material->SetTextureParameterValue(TEXT("DiffuseColorMap"), Texture);
    Material->SetVectorParameterValue(TEXT("SpriteColor"), Tint);
    Material->SetVectorParameterValue(TEXT("Color"), Tint);
    Material->SetVectorParameterValue(TEXT("BaseColor"), Tint);
    Material->SetVectorParameterValue(TEXT("TintColor"), Tint);
    Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
    Material->SetVectorParameterValue(TEXT("EmissiveColor"), Tint * FMath::Max(0.0f, EmissiveStrength));
    Material->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
    Material->SetScalarParameterValue(TEXT("Alpha"), 1.0f);
    Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 1.0f);
    Material->SetScalarParameterValue(TEXT("EmissiveStrength"), FMath::Max(0.0f, EmissiveStrength));
    Material->SetScalarParameterValue(TEXT("Roughness"), 0.18f);
    return Material;
}

UMaterialInstanceDynamic* ACWNativeWorldActor::GetSkillIconMaterial(const FString& IconKey, UTexture2D* Texture, const FLinearColor& Accent)
{
    if (IconKey.IsEmpty() || !Texture)
    {
        return nullptr;
    }

    const FString MaterialKey = FString::Printf(TEXT("%s:%02x%02x%02x"),
        *IconKey,
        FMath::Clamp(FMath::RoundToInt(Accent.R * 255.0f), 0, 255),
        FMath::Clamp(FMath::RoundToInt(Accent.G * 255.0f), 0, 255),
        FMath::Clamp(FMath::RoundToInt(Accent.B * 255.0f), 0, 255));
    if (UMaterialInstanceDynamic* const* Cached = SkillIconMaterialCache.Find(MaterialKey))
    {
        return *Cached;
    }

    const FLinearColor Tint(
        FMath::Min(1.0f, Accent.R + 0.36f),
        FMath::Min(1.0f, Accent.G + 0.36f),
        FMath::Min(1.0f, Accent.B + 0.36f),
        1.0f);
    UMaterialInstanceDynamic* Material = MakeTexturedFxMaterial(FString::Printf(TEXT("SkillIcon_%s"), *IconKey), Texture, Tint, 1.35f);
    if (Material)
    {
        SkillIconMaterialCache.Add(MaterialKey, Material);
    }
    return Material;
}

UStaticMeshComponent* ACWNativeWorldActor::SpawnFxMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& StartScale, const FVector& EndScale, const FLinearColor& Color, float Life, float EmissiveStrength, float SpinYawDegPerSec, const FVector& Velocity, const FVector& Acceleration, bool bAlignToVelocity, UMaterialInterface* OverrideMaterial)
{
    if (!bEnableRuntimeFx || !Mesh || Life <= KINDA_SMALL_NUMBER)
    {
        return nullptr;
    }

    while (ActiveFxComponents.Num() >= MaxRuntimeFxComponents)
    {
        FCWActiveFxComponent OldFx = ActiveFxComponents[0];
        if (OldFx.Component)
        {
            OldFx.Component->DestroyComponent();
        }
        ActiveFxComponents.RemoveAt(0, 1, EAllowShrinking::No);
    }

    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
    if (!Component)
    {
        return nullptr;
    }

    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(false);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetWorldLocation(Location);
    Component->SetWorldRotation(Rotation);
    Component->SetWorldScale3D(StartScale);
    Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
    Component->RegisterComponent();
    AddInstanceComponent(Component);

    if (OverrideMaterial)
    {
        Component->SetMaterial(0, OverrideMaterial);
    }
    else if (UMaterialInstanceDynamic* Material = MakeFxMaterial(FString::Printf(TEXT("RuntimeFx_%d"), ActiveFxComponents.Num()), Color, EmissiveStrength))
    {
        Component->SetMaterial(0, Material);
    }

    FCWActiveFxComponent Fx;
    Fx.Component = Component;
    Fx.Age = 0.0f;
    Fx.Life = Life;
    Fx.StartScale = StartScale;
    Fx.EndScale = EndScale;
    Fx.Velocity = Velocity;
    Fx.Acceleration = Acceleration;
    Fx.SpinYawDegPerSec = SpinYawDegPerSec;
    Fx.bAlignToVelocity = bAlignToVelocity;
    Fx.FadeOutSeconds = FMath::Clamp(Life * 0.28f, 0.22f, 0.72f);
    Fx.BaseColor = Color;
    Fx.EmissiveStrength = EmissiveStrength;
    ActiveFxComponents.Add(Fx);
    return Component;
}

void ACWNativeWorldActor::SpawnParticleSprayFx(const FVector& Location, const FVector& BaseDirection, const FVector& SideDirection, const FLinearColor& Color, int32 Count, float SpeedMin, float SpeedMax, float Life, float SizeMin, float SizeMax, float UpBias, float Gravity)
{
    UStaticMesh* ParticleMesh = SphereMeshAsset ? SphereMeshAsset : CubeMeshAsset;
    if (!bEnableRuntimeFx || !ParticleMesh)
    {
        return;
    }

    FVector Forward = BaseDirection.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    FVector Side = SideDirection.GetSafeNormal();
    if (Side.IsNearlyZero())
    {
        Side = FVector(-Forward.Y, Forward.X, 0.0f).GetSafeNormal();
        if (Side.IsNearlyZero())
        {
            Side = FVector::RightVector;
        }
    }

    const int32 ParticleCount = FMath::Clamp(Count, 0, 36);
    const float SafeSpeedMin = FMath::Max(0.0f, SpeedMin);
    const float SafeSpeedMax = FMath::Max(SafeSpeedMin, SpeedMax);
    const float SafeLife = FMath::Max(0.05f, Life);
    for (int32 Index = 0; Index < ParticleCount; ++Index)
    {
        const float SeedA = StableUnitFromString(FString::Printf(TEXT("particle_a_%0.1f_%0.1f_%d"), Location.X, Location.Y, Index));
        const float SeedB = StableUnitFromString(FString::Printf(TEXT("particle_b_%0.1f_%0.1f_%d"), Location.Y, Location.Z, Index));
        const float SeedC = StableUnitFromString(FString::Printf(TEXT("particle_c_%0.1f_%0.1f_%d"), Location.Z, Location.X, Index));
        const float Fan = FMath::Lerp(-1.05f, 1.05f, SeedA);
        const float ForwardWeight = FMath::Lerp(0.45f, 1.05f, SeedB);
        const float UpWeight = FMath::Lerp(0.10f, 1.0f, SeedC);
        const FVector ParticleDir = (Forward * ForwardWeight + Side * Fan + FVector(0.0f, 0.0f, UpWeight)).GetSafeNormal();
        const float Speed = FMath::Lerp(SafeSpeedMin, SafeSpeedMax, SeedB);
        const FVector Velocity = ParticleDir * Speed + FVector(0.0f, 0.0f, UpBias * SeedC);
        const float Size = FMath::Lerp(SizeMin, SizeMax, SeedA);
        const float ParticleLife = SafeLife * FMath::Lerp(0.68f, 1.18f, SeedC);
        const FVector StartScale(Size, Size, Size);
        const FVector EndScale(Size * 0.05f, Size * 0.05f, Size * 0.05f);
        const FVector StartLocation = Location + ParticleDir * FMath::Lerp(6.0f, 18.0f, SeedA) + FVector(0.0f, 0.0f, FMath::Lerp(6.0f, 24.0f, SeedC));
        SpawnFxMesh(
            ParticleMesh,
            StartLocation,
            FRotator::ZeroRotator,
            StartScale,
            EndScale,
            Color,
            ParticleLife,
            5.2f,
            FMath::Lerp(-260.0f, 260.0f, SeedA),
            Velocity,
            FVector(0.0f, 0.0f, Gravity),
            false);
    }
}

void ACWNativeWorldActor::SpawnRingFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, float Height, float EmissiveStrength)
{
    const float SafeRadius = FMath::Max(4.0f, Radius);
    const int32 SegmentCount = FMath::Clamp(FMath::RoundToInt(SafeRadius / 72.0f), 5, 10);
    const float Step = TWO_PI / static_cast<float>(SegmentCount);
    const float ArcFill = 0.24f;
    const float Width = FMath::Clamp(FMath::Max(Height * 0.45f, SafeRadius * 0.006f), 1.0f, 3.4f);
    const float EndRadius = SafeRadius;
    const float SafeLife = FMath::Min(Life, 0.48f);

    for (int32 Index = 0; Index < SegmentCount; ++Index)
    {
        const float Seed = StableUnitFromString(FString::Printf(TEXT("ring_%0.1f_%0.1f_%d"), Location.X, Location.Y, Index));
        const float A0 = Step * static_cast<float>(Index) + Step * FMath::Lerp(0.08f, 0.28f, Seed);
        const float A1 = A0 + Step * ArcFill;
        const FVector EndA = Location + FVector(FMath::Cos(A0), FMath::Sin(A0), 0.0f) * EndRadius + FVector(0.0f, 0.0f, FMath::Lerp(3.0f, 11.0f, Seed));
        const FVector EndB = Location + FVector(FMath::Cos(A1), FMath::Sin(A1), 0.0f) * (EndRadius * FMath::Lerp(0.94f, 1.03f, Seed));
        SpawnBeamFx(EndA, EndB, Width, Color, SafeLife * FMath::Lerp(0.55f, 1.0f, Seed), EmissiveStrength);
    }
}

void ACWNativeWorldActor::SpawnBurstFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, int32 SparkCount)
{
    const float SafeRadius = FMath::Max(8.0f, Radius);
    if (SafeRadius >= 130.0f)
    {
        SpawnRingFx(Location, SafeRadius, Color, Life * 0.56f, 2.0f, 3.2f);
    }
    SpawnParticleSprayFx(Location, FVector::ForwardVector, FVector::RightVector, Color, FMath::Clamp(SparkCount, 3, 12), SafeRadius * 0.75f, SafeRadius * 2.0f, Life * 0.58f, 0.014f, 0.038f, SafeRadius * 0.48f, -420.0f);

    const int32 Count = FMath::Clamp(SparkCount / 6, 0, 4);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Seed = StableUnitFromString(FString::Printf(TEXT("burst_%0.1f_%0.1f_%d"), Location.X, Location.Y, Index));
        const float Angle = (TWO_PI * static_cast<float>(Index)) / FMath::Max(1, Count) + Seed * 0.32f;
        const float LengthMul = 0.12f + 0.10f * Seed;
        const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
        const FVector Start = Location + Dir * SafeRadius * 0.08f + FVector(0.0f, 0.0f, 18.0f + Seed * 8.0f);
        const FVector End = Location + Dir * SafeRadius * LengthMul + FVector(0.0f, 0.0f, 22.0f + Seed * 18.0f);
        SpawnBeamFx(Start, End, 1.2f, Color, Life * 0.22f, 3.2f);
    }
}

void ACWNativeWorldActor::SpawnBeamFx(const FVector& Start, const FVector& End, float Width, const FLinearColor& Color, float Life, float EmissiveStrength)
{
    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Mid = (Start + End) * 0.5f;
    const float SafeWidth = FMath::Clamp(Width * 0.34f, 0.9f, 5.2f);
    const float SafeLife = FMath::Min(Life, 0.46f);
    UStaticMesh* BeamMesh = CylinderMeshAsset ? CylinderMeshAsset : CubeMeshAsset;
    if (!BeamMesh)
    {
        return;
    }

    if (BeamMesh == CylinderMeshAsset)
    {
        const FRotator Rotation = FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).Rotator();
        const float RadiusScale = SafeWidth / BasicMeshSizeCm;
        const float LengthScale = Length / BasicMeshSizeCm;
        const FVector Scale(RadiusScale, RadiusScale, LengthScale);
        const FVector EndScale(RadiusScale * 0.42f, RadiusScale * 0.42f, LengthScale * 0.88f);
        SpawnFxMesh(BeamMesh, Mid, Rotation, Scale, EndScale, Color, SafeLife, EmissiveStrength, 0.0f);
        return;
    }

    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    const FVector Scale(Length / BasicMeshSizeCm, SafeWidth / BasicMeshSizeCm, SafeWidth / BasicMeshSizeCm);
    SpawnFxMesh(BeamMesh, Mid, FRotator(0.0f, Yaw, 0.0f), Scale, Scale * FVector(1.0f, 0.55f, 0.55f), Color, SafeLife, EmissiveStrength, 0.0f);
}

void ACWNativeWorldActor::SpawnSurfaceImpactFx(const FVector& Location, const FVector& Direction, const FVector& Normal, const FString& Material, const FString& BulletKind, float Damage)
{
    const FString MaterialKey = Material.TrimStartAndEnd().ToLower();
    const FString SurfaceKey = NormalizeSurfaceMaterialKey(Material);
    const bool bRocket = BulletKind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase);
    const bool bMetal = SurfaceKey == TEXT("metal");
    const bool bWood = SurfaceKey == TEXT("wood");
    const bool bGlass = SurfaceKey == TEXT("glass");
    const bool bToxic = SurfaceKey == TEXT("toxic");

    const FLinearColor SparkColor = bMetal
        ? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f)
        : bGlass
            ? FLinearColor(0.55f, 0.92f, 1.0f, 1.0f)
            : bWood
                ? FLinearColor(1.0f, 0.46f, 0.12f, 1.0f)
                : (bToxic ? FLinearColor(0.72f, 1.0f, 0.16f, 1.0f) : FLinearColor(1.0f, 0.58f, 0.12f, 1.0f));
    const FLinearColor DustColor = SurfaceDustColor(SurfaceKey);

    FVector ImpactDir = Direction.GetSafeNormal();
    if (ImpactDir.SizeSquared2D() <= KINDA_SMALL_NUMBER)
    {
        ImpactDir = FVector::ForwardVector;
    }
    FVector SurfaceNormal = Normal.GetSafeNormal();
    if (SurfaceNormal.SizeSquared2D() <= KINDA_SMALL_NUMBER)
    {
        SurfaceNormal = -ImpactDir;
    }
    const FVector Side(-ImpactDir.Y, ImpactDir.X, 0.0f);
    const float ImpactScale = FMath::Clamp(Damage * 0.015f + (bRocket ? 1.35f : 0.72f), 0.68f, bRocket ? 2.8f : 1.45f);
    const float RingRadius = 170.0f * ImpactScale;

    if (bRocket)
    {
        SpawnRingFx(Location + FVector(0.0f, 0.0f, 2.0f), RingRadius * 0.78f, FLinearColor(1.0f, 0.38f, 0.07f, 1.0f), 0.32f, 1.4f, 5.0f);
        SpawnBeamFx(Location - ImpactDir * 16.0f + FVector(0.0f, 0.0f, 18.0f), Location + SurfaceNormal * 34.0f + FVector(0.0f, 0.0f, 38.0f), 7.0f, FLinearColor(1.0f, 0.62f, 0.15f, 1.0f), 0.18f, 6.0f);
    }
    SpawnParticleSprayFx(
        Location,
        (SurfaceNormal * 0.9f - ImpactDir * 0.35f).GetSafeNormal(),
        Side,
        SparkColor,
        bRocket ? 18 : (bMetal ? 7 : 5),
        bRocket ? 320.0f : 180.0f,
        bRocket ? 760.0f : (bMetal ? 480.0f : 360.0f),
        bRocket ? 0.42f : 0.24f,
        bRocket ? 0.018f : 0.018f,
        bRocket ? 0.062f : 0.04f,
        bRocket ? 190.0f : 95.0f,
        bRocket ? -760.0f : -620.0f);
    if (bRocket)
    {
        SpawnParticleSprayFx(
            Location,
            (SurfaceNormal * 0.55f + Side * 0.2f).GetSafeNormal(),
            -Side,
            DustColor,
            14,
            160.0f,
            420.0f,
            0.82f,
            0.028f,
            0.08f,
            180.0f,
            -520.0f);
    }

    const int32 SmokeCount = 0;
    for (int32 SmokeIndex = 0; SmokeIndex < SmokeCount; ++SmokeIndex)
    {
        const float Seed = StableUnitFromString(FString::Printf(TEXT("%s_smoke_%d"), *MaterialKey, SmokeIndex));
        const float Fan = FMath::Lerp(-0.65f, 0.65f, Seed);
        const FVector SmokeDir = (SurfaceNormal * 0.45f + Side * Fan + FVector(0.0f, 0.0f, 0.28f + Seed * 0.25f)).GetSafeNormal();
        const FVector Start = Location + FVector(0.0f, 0.0f, 20.0f) + SmokeDir * 12.0f;
        const FVector End = Location + FVector(0.0f, 0.0f, bRocket ? 84.0f : 42.0f) + SmokeDir * (bRocket ? 95.0f : 42.0f);
        SpawnBeamFx(Start, End, bRocket ? 16.0f : 7.0f, DustColor, bRocket ? 0.72f : 0.36f, 0.18f);
    }

    const int32 SparkCount = bRocket ? 4 : 0;
    for (int32 Index = 0; Index < SparkCount; ++Index)
    {
        const float SeedA = StableUnitFromString(FString::Printf(TEXT("%s_impact_a_%d"), *MaterialKey, Index));
        const float SeedB = StableUnitFromString(FString::Printf(TEXT("%s_impact_b_%d"), *BulletKind, Index));
        const float Fan = FMath::Lerp(-0.85f, 0.85f, SeedA);
        const float Length = (bRocket ? 105.0f : 46.0f) * FMath::Lerp(0.55f, 1.2f, SeedB) * ImpactScale;
        const FVector SparkDir = (SurfaceNormal * FMath::Lerp(0.65f, 1.0f, SeedB) + Side * Fan + FVector(0.0f, 0.0f, FMath::Lerp(0.08f, 0.32f, SeedA))).GetSafeNormal();
        const FVector Start = Location + SparkDir * 8.0f + FVector(0.0f, 0.0f, 18.0f);
        const FVector End = Location + SparkDir * Length + FVector(0.0f, 0.0f, FMath::Lerp(10.0f, 38.0f, SeedB));
        SpawnBeamFx(Start, End, 3.0f, SparkColor, 0.2f, bMetal || bRocket ? 5.4f : 3.2f);
    }

    const int32 DebrisCount = bRocket ? 6 : 0;
    for (int32 Index = 0; Index < DebrisCount; ++Index)
    {
        const float Seed = StableUnitFromString(FString::Printf(TEXT("%s_debris_%d"), *MaterialKey, Index));
        const float Angle = TWO_PI * Seed + Index * 0.47f;
        const FVector DebrisDir = (SurfaceNormal * 0.45f + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * 0.8f).GetSafeNormal();
        const FVector DebrisLocation = Location + DebrisDir * (bRocket ? 38.0f : 18.0f) + FVector(0.0f, 0.0f, FMath::Lerp(16.0f, 42.0f, Seed));
        const float Size = FMath::Lerp(bRocket ? 0.09f : 0.045f, bRocket ? 0.18f : 0.095f, Seed);
        SpawnFxMesh(CubeMeshAsset, DebrisLocation, FRotator(0.0f, FMath::RadiansToDegrees(Angle), Seed * 90.0f),
            FVector(Size, Size * 0.7f, Size * 0.42f), FVector(Size * 0.38f, Size * 0.28f, Size * 0.16f),
            bGlass ? FLinearColor(0.7f, 0.95f, 1.0f, 1.0f) : SurfaceChunkColor(SurfaceKey, Seed), bRocket ? 0.62f : 0.44f, 0.55f, FMath::Lerp(-360.0f, 360.0f, Seed));
    }
}

void ACWNativeWorldActor::SpawnBloodPoolFx(const FVector& Location, float Radius, float Life, float Seed)
{
    if (!bEnableRuntimeFx || !CylinderMeshAsset)
    {
        return;
    }

    const float SafeRadius = FMath::Clamp(Radius, 8.0f, 260.0f);
    const float SeedA = StableUnitFromString(FString::Printf(TEXT("blood_pool_a_%0.1f_%0.1f_%0.1f"), Location.X, Location.Y, Seed));
    const float SeedB = StableUnitFromString(FString::Printf(TEXT("blood_pool_b_%0.1f_%0.1f_%0.1f"), Location.Y, Location.X, Seed));
    const float RadiusX = SafeRadius * FMath::Lerp(0.75f, 1.35f, SeedA);
    const float RadiusY = SafeRadius * FMath::Lerp(0.48f, 0.92f, SeedB);
    const FVector StartScale(RadiusX * 0.18f / BasicMeshSizeCm, RadiusY * 0.18f / BasicMeshSizeCm, 0.006f);
    const FVector EndScale(RadiusX / BasicMeshSizeCm, RadiusY / BasicMeshSizeCm, 0.005f);
    const FLinearColor BloodColor(
        FMath::Lerp(0.095f, 0.18f, SeedA),
        FMath::Lerp(0.002f, 0.012f, SeedB),
        FMath::Lerp(0.003f, 0.010f, SeedA),
        1.0f);

    UStaticMeshComponent* PoolComponent = SpawnFxMesh(
        CylinderMeshAsset,
        FVector(Location.X, Location.Y, GroundZ + 1.2f),
        FRotator(0.0f, SeedA * 360.0f, 0.0f),
        StartScale,
        EndScale,
        BloodColor,
        FMath::Clamp(Life, 1.0f, 12.0f),
        0.04f,
        0.0f);
    if (PoolComponent && ActiveFxComponents.Num() > 0 && ActiveFxComponents.Last().Component == PoolComponent)
    {
        ActiveFxComponents.Last().bGroundBloodPool = true;
        ActiveFxComponents.Last().FadeOutSeconds = FMath::Clamp(Life * 0.22f, 0.42f, 0.82f);
    }
}

void ACWNativeWorldActor::SpawnBloodHitFx(const FVector& Location, const FVector& Direction, float DamageScale, float VisualRadius)
{
    UStaticMesh* DropMesh = SphereMeshAsset ? SphereMeshAsset : CubeMeshAsset;
    if (!bEnableRuntimeFx || !DropMesh)
    {
        return;
    }

    FVector Forward = Direction.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }
    const FVector Side(-Forward.Y, Forward.X, 0.0f);
    const float SafeDamageScale = FMath::Clamp(DamageScale, 0.35f, 3.0f);
    const float SafeRadius = FMath::Clamp(VisualRadius, 10.0f, 110.0f);
    const int32 DropCount = FMath::Clamp(FMath::RoundToInt(9.0f + SafeDamageScale * 10.0f + SafeRadius * 0.05f), 8, 34);
    const FLinearColor FreshBlood(0.72f, 0.015f, 0.018f, 1.0f);
    const FLinearColor DarkBlood(0.18f, 0.002f, 0.006f, 1.0f);

    for (int32 Index = 0; Index < DropCount; ++Index)
    {
        const float SeedA = StableUnitFromString(FString::Printf(TEXT("blood_drop_a_%0.1f_%0.1f_%d"), Location.X, Location.Y, Index));
        const float SeedB = StableUnitFromString(FString::Printf(TEXT("blood_drop_b_%0.1f_%0.1f_%d"), Location.Y, Location.Z, Index));
        const float SeedC = StableUnitFromString(FString::Printf(TEXT("blood_drop_c_%0.1f_%0.1f_%d"), Location.Z, Location.X, Index));
        const float Fan = FMath::Lerp(-1.15f, 1.15f, SeedA);
        const float Up = FMath::Lerp(0.7f, 1.85f, SeedB);
        const FVector DropDir = (Forward * FMath::Lerp(0.25f, 1.05f, SeedC) + Side * Fan + FVector(0.0f, 0.0f, Up)).GetSafeNormal();
        const float Speed = FMath::Lerp(170.0f, 560.0f + SafeDamageScale * 120.0f, SeedB);
        const FVector Velocity = DropDir * Speed + FVector(0.0f, 0.0f, FMath::Lerp(120.0f, 310.0f, SeedA));
        const float Size = FMath::Lerp(0.014f, 0.038f + SafeDamageScale * 0.006f, SeedC);
        const FVector StartLocation = Location + DropDir * FMath::Lerp(4.0f, 18.0f, SeedA);
        const FLinearColor DropColor = FMath::Lerp(DarkBlood, FreshBlood, FMath::Lerp(0.35f, 1.0f, SeedB));
        const float GravityZ = -980.0f;
        const float GroundHitZ = GroundZ + 3.0f;
        const float HeightAboveGround = FMath::Max(0.0f, StartLocation.Z - GroundHitZ);
        float FlightTime = FMath::Sqrt((2.0f * HeightAboveGround) / FMath::Abs(GravityZ));
        const float A = 0.5f * GravityZ;
        const float B = Velocity.Z;
        const float C = StartLocation.Z - GroundHitZ;
        const float Discriminant = B * B - 4.0f * A * C;
        if (Discriminant >= 0.0f && FMath::Abs(A) > KINDA_SMALL_NUMBER)
        {
            const float Root = FMath::Sqrt(Discriminant);
            const float T0 = (-B + Root) / (2.0f * A);
            const float T1 = (-B - Root) / (2.0f * A);
            FlightTime = FMath::Max(T0 > 0.0f ? T0 : 0.0f, T1 > 0.0f ? T1 : 0.0f);
        }
        const float Life = FMath::Clamp(FlightTime + FMath::Lerp(0.16f, 0.30f, SeedC), 0.78f, 2.25f);

        UStaticMeshComponent* DropComponent = SpawnFxMesh(
            DropMesh,
            StartLocation,
            FRotator::ZeroRotator,
            FVector(Size),
            FVector(Size * 0.78f),
            DropColor,
            Life,
            0.22f,
            FMath::Lerp(-220.0f, 220.0f, SeedA),
            Velocity,
            FVector(0.0f, 0.0f, GravityZ),
            false);
        if (DropComponent && ActiveFxComponents.Num() > 0 && ActiveFxComponents.Last().Component == DropComponent)
        {
            ActiveFxComponents.Last().bBloodDroplet = true;
            ActiveFxComponents.Last().bLeavesBloodSplat = SeedC > FMath::Lerp(0.54f, 0.32f, SafeDamageScale / 3.0f);
            ActiveFxComponents.Last().BloodSplatRadius = FMath::Lerp(6.0f, 18.0f + SafeDamageScale * 9.0f, SeedA);
        }
    }

    SpawnParticleSprayFx(
        Location + FVector(0.0f, 0.0f, 8.0f),
        Forward * 0.35f + FVector(0.0f, 0.0f, 0.75f),
        Side,
        FLinearColor(0.30f, 0.004f, 0.008f, 1.0f),
        FMath::Clamp(FMath::RoundToInt(5.0f + SafeDamageScale * 4.0f), 5, 14),
        40.0f,
        170.0f,
        0.48f,
        0.025f,
        0.075f,
        80.0f,
        -260.0f);

    SpawnBloodPoolFx(FVector(Location.X, Location.Y, GroundZ + 1.0f), FMath::Clamp(SafeRadius * 0.44f + SafeDamageScale * 10.0f, 12.0f, 76.0f), 4.5f + SafeDamageScale, SafeDamageScale);
}

void ACWNativeWorldActor::SpawnBloodDeathFx(const FCWEnemySnapshot& Enemy)
{
    if (Enemy.Id.IsEmpty() || SeenEnemyDeathBloodIds.Contains(Enemy.Id))
    {
        return;
    }
    SeenEnemyDeathBloodIds.Add(Enemy.Id);
    if (SeenEnemyDeathBloodIds.Num() > 768)
    {
        SeenEnemyDeathBloodIds.Reset();
    }

    const float VisualRadius = FMath::Max(Enemy.Radius, 18.0f);
    FVector2D DeathDirection(Enemy.bFaceLeft ? -1.0f : 1.0f, 0.0f);
    if (bHasPreviousState)
    {
        float BestPlayerDistSq = TNumericLimits<float>::Max();
        for (const FCWPlayerSnapshot& Player : PreviousState.Players)
        {
            const FVector2D Candidate(Enemy.X - Player.X, Enemy.Y - Player.Y);
            const float DistSq = Candidate.SizeSquared();
            if (DistSq > 1.0f && DistSq < BestPlayerDistSq)
            {
                BestPlayerDistSq = DistSq;
                DeathDirection = Candidate.GetSafeNormal();
            }
        }
    }

    const FVector BurstLocation = MakeWorldLocation(Enemy.X, Enemy.Y, FMath::Clamp(VisualRadius * 2.7f, 42.0f, 120.0f));
    const float DeathScale = FMath::Clamp(VisualRadius / 24.0f + Enemy.SpriteScale * 0.45f, 1.1f, 4.0f);
    SpawnBloodHitFx(BurstLocation, FVector(DeathDirection.X, DeathDirection.Y, 0.0f), DeathScale, VisualRadius * 1.35f);
    SpawnBloodPoolFx(FVector(Enemy.X, Enemy.Y, GroundZ + 1.0f), FMath::Clamp(VisualRadius * 1.65f, 34.0f, 220.0f), FMath::Clamp(7.0f + DeathScale * 1.4f, 7.0f, 12.0f), VisualRadius);
    SpawnParticleSprayFx(
        BurstLocation,
        FVector(DeathDirection.X, DeathDirection.Y, 0.0f) * 0.2f + FVector(0.0f, 0.0f, 1.0f),
        FVector(-DeathDirection.Y, DeathDirection.X, 0.0f),
        FLinearColor(0.16f, 0.002f, 0.006f, 1.0f),
        FMath::Clamp(FMath::RoundToInt(9.0f + DeathScale * 3.0f), 9, 22),
        70.0f,
        260.0f,
        0.72f,
        0.045f,
        0.12f,
        120.0f,
        -340.0f);
    AddProjectileImpactClip(FString(), FVector2D(Enemy.X, Enemy.Y), DeathDirection, FMath::Clamp(VisualRadius * 3.2f, 110.0f, 360.0f), 0.45f);
}

void ACWNativeWorldActor::SpawnArcFx(const FVector& Center, float AngleRad, float Range, float ArcDeg, float Width, const FLinearColor& Color, const FLinearColor& SecondaryColor, float Life, int32 Segments)
{
    const float SafeRange = FMath::Max(20.0f, Range);
    const float HalfArc = FMath::DegreesToRadians(FMath::Clamp(ArcDeg, 12.0f, 360.0f) * 0.5f);
    const int32 Count = FMath::Clamp(Segments, 4, 28);
    FVector Previous = Center + FVector(FMath::Cos(AngleRad - HalfArc), FMath::Sin(AngleRad - HalfArc), 0.0f) * SafeRange + FVector(0.0f, 0.0f, ProjectileFxZ);
    for (int32 Index = 1; Index <= Count; ++Index)
    {
        const float T = static_cast<float>(Index) / static_cast<float>(Count);
        const float A = FMath::Lerp(AngleRad - HalfArc, AngleRad + HalfArc, T);
        const FVector Current = Center + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * SafeRange + FVector(0.0f, 0.0f, ProjectileFxZ);
        SpawnBeamFx(Previous, Current, FMath::Max(5.0f, Width * 0.12f), Color, Life, 4.2f);
        if (Index % 2 == 0)
        {
            SpawnBeamFx(Previous + FVector(0.0f, 0.0f, 8.0f), Current + FVector(0.0f, 0.0f, 8.0f), FMath::Max(2.0f, Width * 0.045f), SecondaryColor, Life * 0.82f, 5.0f);
        }
        Previous = Current;
    }
}

void ACWNativeWorldActor::SpawnSkillCastFx(const FCWPlayerSnapshot& Player, const FCWSkillSnapshot& Skill, const FCWRoomSnapshot& State)
{
    const FCWNativeFxProfile* Profile = FindFxProfile(Skill.FxKey);
    const FString CastType = Skill.CastType.IsEmpty() ? Skill.Id : Skill.CastType;
    const FLinearColor Primary = ResolveFxColor(Profile ? Profile->PrimaryColor : Skill.FxKey, FLinearColor(0.5f, 0.8f, 1.0f, 1.0f));
    const FLinearColor Secondary = ResolveFxColor(Profile ? Profile->SecondaryColor : FString(), FLinearColor(0.95f, 0.98f, 1.0f, 1.0f));
    const FVector Origin = MakeWorldLocation(Player.X, Player.Y, GroundFxZ + 20.0f);

    if (CastType.Contains(TEXT("shockwave"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("stomp"), ESearchCase::IgnoreCase))
    {
        SpawnBurstFx(Origin, 220.0f, Primary, 0.55f, 18);
        SpawnRingFx(Origin + FVector(0.0f, 0.0f, 3.0f), 340.0f, Secondary, 0.72f, 4.0f, 3.0f);
        return;
    }

    if (CastType.Contains(TEXT("psi"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("wave"), ESearchCase::IgnoreCase))
    {
        SpawnBurstFx(Origin + FVector(0.0f, 0.0f, 30.0f), 190.0f, Primary, 0.5f, 14);
        SpawnRingFx(Origin, 430.0f, Secondary, 0.78f, 2.5f, 2.6f);
        return;
    }

    if (CastType.Contains(TEXT("chain"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("lightning"), ESearchCase::IgnoreCase))
    {
        int32 Links = 0;
        for (const FCWEnemySnapshot& Enemy : State.Enemies)
        {
            if (Links >= 4)
            {
                break;
            }
            const float DistSq = FVector2D::DistSquared(FVector2D(Player.X, Player.Y), FVector2D(Enemy.X, Enemy.Y));
            if (DistSq <= FMath::Square(520.0f))
            {
                SpawnBeamFx(Origin + FVector(0.0f, 0.0f, 55.0f), MakeWorldLocation(Enemy.X, Enemy.Y, ProjectileFxZ + 10.0f), 8.0f, Primary, 0.24f, 5.0f);
                SpawnBurstFx(MakeWorldLocation(Enemy.X, Enemy.Y, GroundFxZ + 8.0f), 48.0f, Secondary, 0.3f, 6);
                ++Links;
            }
        }
        SpawnBurstFx(Origin, 92.0f, Primary, 0.34f, 10);
        return;
    }

    if (CastType.Contains(TEXT("laser"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("lance"), ESearchCase::IgnoreCase))
    {
        int32 Beams = 0;
        for (const FCWEnemySnapshot& Enemy : State.Enemies)
        {
            if (Beams >= 3)
            {
                break;
            }
            const float DistSq = FVector2D::DistSquared(FVector2D(Player.X, Player.Y), FVector2D(Enemy.X, Enemy.Y));
            if (DistSq <= FMath::Square(700.0f))
            {
                const FVector Target = MakeWorldLocation(Enemy.X, Enemy.Y, ProjectileFxZ);
                SpawnBeamFx(Target + FVector(0.0f, 0.0f, 620.0f), Target, 16.0f, Primary, 0.22f, 7.0f);
                SpawnBurstFx(Target, 72.0f, Secondary, 0.34f, 8);
                ++Beams;
            }
        }
        return;
    }

    if (CastType.Contains(TEXT("missile"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("rocket"), ESearchCase::IgnoreCase))
    {
        SpawnBurstFx(Origin, 110.0f, Primary, 0.34f, 12);
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const float A = Player.AimX != Player.X || Player.AimY != Player.Y
                ? FMath::Atan2(Player.AimY - Player.Y, Player.AimX - Player.X) + (Index - 1) * 0.16f
                : Index * TWO_PI / 3.0f;
            SpawnBeamFx(Origin, Origin + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * 92.0f + FVector(0.0f, 0.0f, 28.0f), 5.0f, Primary, 0.22f, 4.2f);
        }
        return;
    }

    if (CastType.Contains(TEXT("blade"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("orbit"), ESearchCase::IgnoreCase))
    {
        for (int32 Index = 0; Index < 5; ++Index)
        {
            SpawnArcFx(Origin, Index * TWO_PI / 5.0f, 115.0f, 56.0f, 42.0f, Primary, Secondary, 0.42f, 6);
        }
        return;
    }

    SpawnBurstFx(Origin, 120.0f, Primary, 0.42f, 10);
}

void ACWNativeWorldActor::SpawnProjectileShotFx(const FCWShotEventSnapshot& Event)
{
    const bool bRocket = Event.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase) || Event.FxKey.Equals(TEXT("projectile.rocket"), ESearchCase::IgnoreCase);
    const FLinearColor Color = ResolveFxColor(Event.Color, bRocket ? FLinearColor(1.0f, 0.42f, 0.08f, 1.0f) : FLinearColor(1.0f, 0.82f, 0.2f, 1.0f));
    const FVector Start = MakeWorldLocation(Event.X, Event.Y, ProjectileFxZ);
    const FVector Dir = FVector(Event.Vx, Event.Vy, 0.0f).GetSafeNormal();
    if (bRocket)
    {
        SpawnBurstFx(Start, 62.0f, Color, 0.26f, 8);
        SpawnParticleSprayFx(Start - Dir * 28.0f, -Dir, FVector(-Dir.Y, Dir.X, 0.0f), Color, 8, 120.0f, 360.0f, 0.28f, 0.018f, 0.044f, 45.0f, -180.0f);
    }
    else
    {
        SpawnBeamFx(Start - Dir * 24.0f + FVector(0.0f, 0.0f, 4.0f), Start + Dir * 38.0f + FVector(0.0f, 0.0f, 4.0f), 2.4f, Color, 0.09f, 4.4f);
        SpawnBeamFx(Start - Dir * 10.0f + FVector(0.0f, 0.0f, 7.0f), Start + Dir * 22.0f + FVector(0.0f, 0.0f, 7.0f), 1.2f, FLinearColor(1.0f, 0.98f, 0.78f, 1.0f), 0.08f, 6.0f);
    }
}

FString ACWNativeWorldActor::ResolveSurfaceMaterialAt(const FCWRoomSnapshot& State, const FVector2D& Position) const
{
    for (int32 Index = State.TerrainZones.Num() - 1; Index >= 0; --Index)
    {
        const FCWTerrainZoneSnapshot& Zone = State.TerrainZones[Index];
        if (Zone.Material.IsEmpty() || Zone.W <= 0.0f || Zone.H <= 0.0f || Zone.Alpha <= 0.08f)
        {
            continue;
        }

        const float HalfW = FMath::Max(1.0f, Zone.W * 0.5f);
        const float HalfH = FMath::Max(1.0f, Zone.H * 0.5f);
        const float Angle = -Zone.Angle;
        const float CosA = FMath::Cos(Angle);
        const float SinA = FMath::Sin(Angle);
        const float Dx = Position.X - Zone.X;
        const float Dy = Position.Y - Zone.Y;
        const float LocalX = Dx * CosA - Dy * SinA;
        const float LocalY = Dx * SinA + Dy * CosA;
        const bool bRectLike = Zone.Shape.Equals(TEXT("rect"), ESearchCase::IgnoreCase)
            || Zone.Shape.Equals(TEXT("band"), ESearchCase::IgnoreCase);
        const bool bInside = bRectLike
            ? (FMath::Abs(LocalX) <= HalfW * 1.08f && FMath::Abs(LocalY) <= HalfH * 1.08f)
            : (FMath::Square(LocalX / HalfW) + FMath::Square(LocalY / HalfH) <= 1.12f);
        if (bInside)
        {
            return NormalizeSurfaceMaterialKey(Zone.Material);
        }
    }

    return NormalizeSurfaceMaterialKey(State.SceneTheme.BaseMaterial.IsEmpty() ? TEXT("concrete") : State.SceneTheme.BaseMaterial);
}

void ACWNativeWorldActor::SpawnRocketExplosionFx(const FCWBulletSnapshot& Bullet, const FCWRoomSnapshot& State)
{
    const FLinearColor Color = ResolveFxColor(Bullet.Color, FLinearColor(1.0f, 0.36f, 0.08f, 1.0f));
    const float Radius = FMath::Clamp(FMath::Max(118.0f, Bullet.ExplosionRadius * 1.55f), 118.0f, 230.0f);
    const FVector Origin = MakeWorldLocation(Bullet.X, Bullet.Y, GroundFxZ + 18.0f);
    SpawnRocketExplosionAt(Origin, FVector(Bullet.Vx, Bullet.Vy, 0.0f), ResolveSurfaceMaterialAt(State, FVector2D(Bullet.X, Bullet.Y)), Radius, Color);
}

void ACWNativeWorldActor::SpawnRocketExplosionAt(const FVector& Location, const FVector& Direction, const FString& SurfaceMaterial, float Radius, const FLinearColor& Color)
{
    const FString SurfaceKey = NormalizeSurfaceMaterialKey(SurfaceMaterial);
    const bool bAsphalt = SurfaceKey == TEXT("asphalt");
    const bool bGrass = SurfaceKey == TEXT("grass");
    const bool bDirt = SurfaceKey == TEXT("dirt");
    const bool bMetal = SurfaceKey == TEXT("metal");
    const bool bWood = SurfaceKey == TEXT("wood");
    const bool bGlass = SurfaceKey == TEXT("glass");
    const bool bToxic = SurfaceKey == TEXT("toxic");
    const float SafeRadius = FMath::Clamp(Radius, 105.0f, 245.0f);

    FVector ImpactDir = Direction.GetSafeNormal();
    if (ImpactDir.SizeSquared2D() <= KINDA_SMALL_NUMBER)
    {
        ImpactDir = FVector::ForwardVector;
    }
    const FVector Side(-ImpactDir.Y, ImpactDir.X, 0.0f);
    const FLinearColor DustColor = SurfaceDustColor(SurfaceKey);
    const FLinearColor FireOuter = bToxic ? FLinearColor(0.68f, 1.0f, 0.16f, 0.68f) : FLinearColor(1.0f, 0.18f, 0.025f, 0.70f);
    const FLinearColor FireMid = bToxic ? FLinearColor(0.96f, 1.0f, 0.26f, 0.74f) : FLinearColor(1.0f, 0.52f, 0.075f, 0.74f);
    const FLinearColor FireCore = FLinearColor(1.0f, 0.94f, bToxic ? 0.38f : 0.58f, 0.80f);
    UStaticMesh* FireMesh = SphereMeshAsset ? SphereMeshAsset : CubeMeshAsset;

    if (FireMesh)
    {
        const float FireScale = FMath::Clamp(SafeRadius / 190.0f, 0.68f, 1.12f);
        SpawnFxMesh(
            FireMesh,
            Location + FVector(0.0f, 0.0f, SafeRadius * 0.18f),
            FRotator::ZeroRotator,
            FVector(0.18f, 0.18f, 0.12f) * FireScale,
            FVector(0.42f, 0.42f, 0.24f) * FireScale,
            FireCore,
            0.20f,
            10.8f,
            0.0f,
            FVector::ZeroVector,
            FVector::ZeroVector,
            false);
        SpawnFxMesh(
            FireMesh,
            Location + FVector(0.0f, 0.0f, SafeRadius * 0.22f),
            FRotator(0.0f, 24.0f, 0.0f),
            FVector(0.26f, 0.26f, 0.16f) * FireScale,
            FVector(0.62f, 0.62f, 0.28f) * FireScale,
            FireMid,
            0.26f,
            8.6f,
            0.0f,
            FVector::ZeroVector,
            FVector::ZeroVector,
            false);

        const int32 FireBubbleCount = bToxic ? 24 : 22;
        for (int32 Index = 0; Index < FireBubbleCount; ++Index)
        {
            const float SeedA = StableUnitFromString(FString::Printf(TEXT("%s_fire_bubble_a_%0.1f_%0.1f_%d"), *SurfaceKey, Location.X, Location.Y, Index));
            const float SeedB = StableUnitFromString(FString::Printf(TEXT("%s_fire_bubble_b_%0.1f_%0.1f_%d"), *SurfaceKey, Location.Y, Location.Z, Index));
            const float SeedC = StableUnitFromString(FString::Printf(TEXT("%s_fire_bubble_c_%0.1f_%0.1f_%d"), *SurfaceKey, Location.Z, Location.X, Index));
            const float Angle = TWO_PI * ((static_cast<float>(Index) / static_cast<float>(FireBubbleCount)) + SeedA * 0.23f);
            const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
            const float StartDist = SafeRadius * FMath::Lerp(0.02f, 0.22f, SeedB);
            const float StartZ = FMath::Lerp(18.0f, SafeRadius * 0.30f, SeedC);
            const float Size = FMath::Lerp(0.035f, 0.095f, SeedA) * FMath::Clamp(SafeRadius / 190.0f, 0.72f, 1.12f);
            const float Squash = FMath::Lerp(0.82f, 1.24f, SeedB);
            const FVector StartScale(Size * Squash, Size, Size * FMath::Lerp(0.72f, 1.12f, SeedC));
            const FVector EndScale = StartScale * FMath::Lerp(0.08f, 0.18f, SeedB);
            const FVector Velocity = Radial * SafeRadius * FMath::Lerp(0.20f, 0.72f, SeedA) + FVector(0.0f, 0.0f, SafeRadius * FMath::Lerp(0.35f, 1.05f, SeedC));
            const FLinearColor BubbleColor = SeedC > 0.72f ? FireCore : (SeedB > 0.46f ? FireMid : FireOuter);
            SpawnFxMesh(
                FireMesh,
                Location + Radial * StartDist + FVector(0.0f, 0.0f, StartZ),
                FRotator(SeedA * 50.0f, SeedB * 360.0f, SeedC * 48.0f),
                StartScale,
                EndScale,
                BubbleColor,
                FMath::Lerp(0.14f, 0.28f, SeedC),
                SeedC > 0.72f ? 9.6f : 7.0f,
                FMath::Lerp(-220.0f, 220.0f, SeedA),
                Velocity,
                FVector(0.0f, 0.0f, -SafeRadius * FMath::Lerp(2.2f, 3.2f, SeedB)),
                false);
        }
    }

    SpawnRingFx(Location + FVector(0.0f, 0.0f, 5.0f), SafeRadius * 0.74f, FireMid, 0.28f, 2.5f, 5.8f);
    SpawnRingFx(Location + FVector(0.0f, 0.0f, 11.0f), SafeRadius * 0.48f, FireOuter, 0.24f, 3.2f, 6.4f);
    SpawnRingFx(Location + FVector(0.0f, 0.0f, 2.0f), SafeRadius * 0.58f, DustColor, 0.34f, 1.6f, bMetal ? 0.9f : 0.24f);

    const int32 FlameJetCount = bToxic ? 5 : 4;
    for (int32 Index = 0; Index < FlameJetCount; ++Index)
    {
        const float SeedA = StableUnitFromString(FString::Printf(TEXT("%s_flame_a_%0.1f_%0.1f_%d"), *SurfaceKey, Location.X, Location.Y, Index));
        const float SeedB = StableUnitFromString(FString::Printf(TEXT("%s_flame_b_%0.1f_%0.1f_%d"), *SurfaceKey, Location.Y, Location.Z, Index));
        const float Angle = TWO_PI * ((static_cast<float>(Index) / static_cast<float>(FlameJetCount)) + SeedA * 0.10f);
        const FVector Ray(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
        const FVector Start = Location + Ray * SafeRadius * FMath::Lerp(0.04f, 0.10f, SeedA) + FVector(0.0f, 0.0f, FMath::Lerp(14.0f, 26.0f, SeedB));
        const FVector End = Location
            + Ray * SafeRadius * FMath::Lerp(0.18f, 0.36f, SeedB)
            + FVector(0.0f, 0.0f, SafeRadius * FMath::Lerp(0.16f, 0.36f, SeedA));
        SpawnBeamFx(Start, End, FMath::Lerp(3.0f, 6.0f, SeedB), Index % 3 == 0 ? FireCore : FireMid, 0.15f + SeedA * 0.08f, 7.2f);
    }

    SpawnParticleSprayFx(
        Location + FVector(0.0f, 0.0f, 18.0f),
        (FVector::UpVector * 0.95f - ImpactDir * 0.18f).GetSafeNormal(),
        Side,
        FireMid,
        bToxic ? 18 : 16,
        SafeRadius * 0.72f,
        SafeRadius * 2.10f,
        0.32f,
        0.024f,
        0.060f,
        SafeRadius * 0.38f,
        -650.0f);

    FLinearColor DustPuffColor = DustColor * 0.58f;
    DustPuffColor.A = 0.46f;
    SpawnParticleSprayFx(
        Location + FVector(0.0f, 0.0f, 9.0f),
        FVector::UpVector,
        Side,
        DustPuffColor,
        bAsphalt || bDirt ? 12 : 9,
        SafeRadius * 0.28f,
        SafeRadius * 0.95f,
        0.62f,
        bAsphalt ? 0.025f : 0.030f,
        bAsphalt ? 0.075f : 0.085f,
        SafeRadius * 0.20f,
        -500.0f);

    const int32 DebrisCount = bAsphalt ? 14 : (bDirt ? 13 : (bGrass ? 11 : (bMetal ? 10 : 11)));
    for (int32 Index = 0; Index < DebrisCount; ++Index)
    {
        const float SeedA = StableUnitFromString(FString::Printf(TEXT("%s_rocket_debris_a_%0.1f_%0.1f_%d"), *SurfaceKey, Location.X, Location.Y, Index));
        const float SeedB = StableUnitFromString(FString::Printf(TEXT("%s_rocket_debris_b_%0.1f_%0.1f_%d"), *SurfaceKey, Location.Y, Location.Z, Index));
        const float SeedC = StableUnitFromString(FString::Printf(TEXT("%s_rocket_debris_c_%0.1f_%0.1f_%d"), *SurfaceKey, Location.Z, Location.X, Index));
        const float Angle = TWO_PI * ((static_cast<float>(Index) / static_cast<float>(DebrisCount)) + SeedA * 0.21f);
        const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
        const float StartDist = SafeRadius * FMath::Lerp(0.04f, 0.15f, SeedB);
        const float Speed = SafeRadius * FMath::Lerp(bAsphalt ? 0.72f : 0.56f, bAsphalt ? 1.55f : 1.28f, SeedB);
        const float Lift = SafeRadius * FMath::Lerp(bAsphalt ? 0.62f : 0.48f, bAsphalt ? 1.65f : 1.30f, SeedC);
        const FVector Velocity = Radial * Speed + FVector(0.0f, 0.0f, Lift);
        const FVector DebrisLocation = Location + Radial * StartDist + FVector(0.0f, 0.0f, FMath::Lerp(10.0f, 28.0f, SeedC));
        const float SizeCm = FMath::Lerp(bAsphalt ? 4.5f : 3.8f, bAsphalt ? 12.0f : 9.0f, SeedA);
        const float Size = SizeCm / BasicMeshSizeCm;
        const FVector StartScale = bAsphalt
            ? FVector(Size * FMath::Lerp(1.30f, 2.10f, SeedB), Size * FMath::Lerp(0.60f, 1.05f, SeedC), Size * FMath::Lerp(0.16f, 0.32f, SeedA))
            : (bGrass || bDirt
                ? FVector(Size * FMath::Lerp(0.84f, 1.28f, SeedB), Size * FMath::Lerp(0.70f, 1.18f, SeedC), Size * FMath::Lerp(0.48f, 0.86f, SeedA))
                : FVector(Size * FMath::Lerp(0.78f, 1.42f, SeedB), Size * FMath::Lerp(0.54f, 1.10f, SeedC), Size * FMath::Lerp(0.28f, 0.68f, SeedA)));
        const FVector EndScale = StartScale * FVector(0.42f, 0.42f, 0.36f);
        const FLinearColor DebrisColor = bGlass
            ? FLinearColor(0.70f, 0.95f, 1.0f, 1.0f)
            : SurfaceChunkColor(SurfaceKey, SeedB);

        SpawnFxMesh(
            CubeMeshAsset,
            DebrisLocation,
            FRotator(SeedA * 110.0f, FMath::RadiansToDegrees(Angle), SeedC * 120.0f),
            StartScale,
            EndScale,
            DebrisColor,
            FMath::Lerp(0.45f, 0.80f, SeedC),
            bMetal || bGlass ? 0.85f : 0.12f,
            FMath::Lerp(-360.0f, 360.0f, SeedA),
            Velocity,
            FVector(0.0f, 0.0f, bAsphalt ? -720.0f : -620.0f),
            true);
    }

    FLinearColor SmokeColor = FLinearColor(0.060f, 0.052f, 0.044f, 1.0f) * 0.86f + DustColor * 0.08f;
    SmokeColor.A = 0.28f;
    SpawnParticleSprayFx(
        Location + FVector(0.0f, 0.0f, 18.0f),
        FVector::UpVector,
        Side,
        SmokeColor,
        bWood ? 9 : 7,
        SafeRadius * 0.20f,
        SafeRadius * 0.62f,
        0.66f,
        0.032f,
        0.074f,
        SafeRadius * 0.18f,
        -360.0f);
}

void ACWNativeWorldActor::SpawnObjectImpactFx(const FCWObjectImpactEventSnapshot& Event)
{
    if (!Event.BulletId.IsEmpty())
    {
        const double TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        ProjectileVisualStates.Remove(Event.BulletId);
        RetiredProjectileVisualUntil.Add(Event.BulletId, TimeSeconds + 0.65);
    }

    AddProjectileImpactClip(
        Event.BulletId,
        FVector2D(Event.X, Event.Y),
        FVector2D(Event.DirX, Event.DirY),
        FMath::Clamp(Event.Damage * 0.12f + 120.0f, 92.0f, 420.0f),
        0.5f);

    const FVector Origin = MakeWorldLocation(Event.X, Event.Y, GroundFxZ + 18.0f);
    const FVector Direction(Event.DirX, Event.DirY, 0.0f);
    const bool bRocket = Event.BulletKind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase);
    if (bRocket)
    {
        const float Radius = FMath::Clamp(FMath::Max(120.0f, Event.Damage * 2.5f), 115.0f, 230.0f);
        SpawnRocketExplosionAt(Origin, Direction, Event.Material, Radius, FLinearColor(1.0f, 0.44f, 0.08f, 1.0f));
        return;
    }

    SpawnSurfaceImpactFx(Origin, Direction, FVector(Event.NormalX, Event.NormalY, 0.0f), Event.Material, Event.BulletKind, Event.Damage);
}

void ACWNativeWorldActor::SpawnXpSurgePullFx(const FCWWorldFxEvent& Event)
{
    const FLinearColor Primary = ResolveFxColor(Event.Color, FLinearColor(0.62f, 0.28f, 1.0f, 1.0f));
    const FLinearColor Secondary = ResolveFxColor(Event.SecondaryColor, FLinearColor(0.18f, 0.85f, 1.0f, 1.0f));
    const FVector Origin = MakeWorldLocation(Event.X, Event.Y, GroundFxZ + 30.0f);
    SpawnBurstFx(Origin, 260.0f, Primary, 0.78f, 24);
    SpawnRingFx(Origin, 520.0f, Secondary, 1.05f, 4.0f, 4.0f);

    int32 Links = 0;
    for (const FCWPickupSnapshot& Orb : PreviousState.XpOrbs)
    {
        if (Links >= 10)
        {
            break;
        }
        SpawnBeamFx(MakeWorldLocation(Orb.X, Orb.Y, 36.0f), Origin + FVector(0.0f, 0.0f, 28.0f), 5.0f, Links % 2 == 0 ? Secondary : Primary, 0.7f, 4.5f);
        ++Links;
    }
}

void ACWNativeWorldActor::SpawnGenericWorldFx(const FCWWorldFxEvent& Event)
{
    const FLinearColor Primary = ResolveFxColor(Event.Color, FLinearColor(0.95f, 0.45f, 0.2f, 1.0f));
    const FLinearColor Secondary = ResolveFxColor(Event.SecondaryColor, FLinearColor(1.0f, 0.9f, 0.45f, 1.0f));
    const FVector Origin = MakeWorldLocation(Event.X, Event.Y, GroundFxZ + 24.0f);
    const float Radius = FMath::Clamp(Event.Radius > 0.0f ? Event.Radius : 120.0f, 48.0f, 520.0f);
    SpawnBurstFx(Origin, Radius, Primary, 0.5f, 14);

    if (Event.Kind.Contains(TEXT("satellite"), ESearchCase::IgnoreCase) || Event.Kind.Contains(TEXT("artillery"), ESearchCase::IgnoreCase))
    {
        SpawnBeamFx(Origin + FVector(0.0f, 0.0f, 680.0f), Origin, 22.0f, Secondary, 0.28f, 6.5f);
    }
}

void ACWNativeWorldActor::SpawnSkillEventFx(const FCWSkillFxEvent& Event)
{
    const FCWNativeFxProfile* Profile = FindFxProfile(Event.FxKey);
    const FString CastType = Event.CastType.IsEmpty() ? Event.SkillId : Event.CastType;
    const FLinearColor Primary = ResolveFxColor(Profile ? Profile->PrimaryColor : Event.Color, FLinearColor(0.45f, 0.82f, 1.0f, 1.0f));
    const FLinearColor Secondary = ResolveFxColor(Profile ? Profile->SecondaryColor : Event.SecondaryColor, FLinearColor(0.96f, 0.98f, 1.0f, 1.0f));
    const FVector Origin = MakeWorldLocation(Event.X, Event.Y, GroundFxZ + 34.0f);
    const float Radius = FMath::Clamp(Event.Radius > 0.0f ? Event.Radius : (Profile ? Profile->Radius : 180.0f), 72.0f, 760.0f);
    const FVector AimDelta(Event.AimX - Event.X, Event.AimY - Event.Y, 0.0f);
    const float AimAngle = AimDelta.SizeSquared2D() > 1.0f ? FMath::Atan2(AimDelta.Y, AimDelta.X) : 0.0f;

    auto SpawnTargetMarkers = [&](float BeamWidth, float BurstRadius, float Life)
    {
        int32 TargetMarkerCount = 0;
        for (const FCWSkillFxTarget& Target : Event.Targets)
        {
            if (TargetMarkerCount >= 8)
            {
                break;
            }
            const FVector TargetLocation = MakeWorldLocation(Target.X, Target.Y, ProjectileFxZ + 18.0f);
            SpawnBeamFx(Origin + FVector(0.0f, 0.0f, 34.0f), TargetLocation, BeamWidth, Primary, Life, 5.8f);
            SpawnBurstFx(TargetLocation, BurstRadius, Secondary, Life * 1.15f, 9);
            ++TargetMarkerCount;
        }
    };

    if (CastType.Contains(TEXT("shockwave"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("stomp"), ESearchCase::IgnoreCase))
    {
        SpawnBurstFx(Origin, Radius * 0.82f, Primary, 0.9f, 32);
        SpawnRingFx(Origin + FVector(0.0f, 0.0f, 8.0f), Radius * 1.22f, Secondary, 1.05f, 7.0f, 5.0f);
        SpawnRingFx(Origin + FVector(0.0f, 0.0f, 16.0f), Radius * 0.62f, Primary, 0.66f, 9.0f, 6.0f);
        SpawnTargetMarkers(6.0f, 38.0f, 0.34f);
        return;
    }

    if (CastType.Contains(TEXT("psi"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("wave"), ESearchCase::IgnoreCase))
    {
        SpawnBurstFx(Origin + FVector(0.0f, 0.0f, 42.0f), Radius * 0.62f, Primary, 0.86f, 28);
        SpawnRingFx(Origin, Radius * 1.35f, Secondary, 1.1f, 6.0f, 5.2f);
        SpawnRingFx(Origin + FVector(0.0f, 0.0f, 22.0f), Radius * 0.95f, Primary, 0.82f, 5.0f, 5.4f);
        SpawnTargetMarkers(8.0f, 46.0f, 0.42f);
        return;
    }

    if (CastType.Contains(TEXT("chain"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("lightning"), ESearchCase::IgnoreCase))
    {
        FVector Previous = Origin + FVector(0.0f, 0.0f, 70.0f);
        int32 LinkIndex = 0;
        for (const FCWSkillFxTarget& Target : Event.Targets)
        {
            const FVector TargetLocation = MakeWorldLocation(Target.X, Target.Y, ProjectileFxZ + 26.0f);
            SpawnBeamFx(Previous, TargetLocation, 11.0f, LinkIndex % 2 == 0 ? Primary : Secondary, 0.42f, 7.5f);
            SpawnBurstFx(TargetLocation, 56.0f, LinkIndex % 2 == 0 ? Secondary : Primary, 0.48f, 10);
            Previous = TargetLocation + FVector(0.0f, 0.0f, 18.0f);
            ++LinkIndex;
        }
        SpawnBurstFx(Origin, 115.0f, Primary, 0.5f, 16);
        return;
    }

    if (CastType.Contains(TEXT("laser"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("lance"), ESearchCase::IgnoreCase))
    {
        if (Event.Targets.Num() <= 0)
        {
            SpawnBeamFx(Origin, Origin + FVector(FMath::Cos(AimAngle), FMath::Sin(AimAngle), 0.0f) * Radius + FVector(0.0f, 0.0f, 70.0f), 18.0f, Primary, 0.38f, 7.5f);
        }
        for (const FCWSkillFxTarget& Target : Event.Targets)
        {
            const FVector TargetLocation = MakeWorldLocation(Target.X, Target.Y, ProjectileFxZ);
            SpawnBeamFx(TargetLocation + FVector(0.0f, 0.0f, 720.0f), TargetLocation + FVector(0.0f, 0.0f, 12.0f), 22.0f, Primary, 0.36f, 8.5f);
            SpawnBeamFx(Origin + FVector(0.0f, 0.0f, 42.0f), TargetLocation + FVector(0.0f, 0.0f, 26.0f), 9.0f, Secondary, 0.32f, 6.2f);
            SpawnBurstFx(TargetLocation, 82.0f, Secondary, 0.46f, 12);
        }
        return;
    }

    if (CastType.Contains(TEXT("missile"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("rocket"), ESearchCase::IgnoreCase))
    {
        const int32 Count = FMath::Clamp(Event.ProjectileCount > 0 ? Event.ProjectileCount : FMath::Max(3, Event.Targets.Num()), 3, 6);
        SpawnBurstFx(Origin, 132.0f, Primary, 0.48f, 16);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float A = AimAngle + ((static_cast<float>(Index) - (Count - 1) * 0.5f) * 0.22f);
            SpawnBeamFx(Origin, Origin + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * 125.0f + FVector(0.0f, 0.0f, 44.0f), 7.0f, Primary, 0.32f, 5.8f);
        }
        SpawnTargetMarkers(5.0f, 42.0f, 0.38f);
        return;
    }

    if (CastType.Contains(TEXT("blade"), ESearchCase::IgnoreCase) || CastType.Contains(TEXT("orbit"), ESearchCase::IgnoreCase))
    {
        const int32 Count = FMath::Clamp(FMath::Max(5, Event.HitCount), 5, 10);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            SpawnArcFx(Origin, AimAngle + Index * TWO_PI / Count, FMath::Max(125.0f, Radius * 0.36f), 72.0f, 52.0f, Primary, Secondary, 0.56f, 8);
        }
        SpawnTargetMarkers(5.0f, 40.0f, 0.32f);
        return;
    }

    SpawnBurstFx(Origin, FMath::Max(140.0f, Radius * 0.45f), Primary, 0.55f, 18);
    SpawnTargetMarkers(6.0f, 38.0f, 0.34f);
}

void ACWNativeWorldActor::SpawnMeleeStyleFx(const FCWMeleeFxEvent& Event)
{
    const FLinearColor Primary = ResolveFxColor(Event.Color, FLinearColor(1.0f, 0.42f, 0.58f, 1.0f));
    const FLinearColor Secondary = ResolveFxColor(Event.SecondaryColor, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
    const FVector Origin = MakeWorldLocation(Event.X, Event.Y, GroundFxZ);
    const FVector Impact = MakeWorldLocation(Event.ImpactX, Event.ImpactY, GroundFxZ + 16.0f);
    const FString Style = Event.Style.ToLower();

    if (Style.Contains(TEXT("hammer")))
    {
        SpawnBeamFx(Origin + FVector(0.0f, 0.0f, 60.0f), Impact + FVector(0.0f, 0.0f, 72.0f), FMath::Max(12.0f, Event.Width * 0.12f), Primary, 0.38f, 4.0f);
        SpawnBurstFx(Impact, FMath::Max(110.0f, Event.Width), Primary, 0.62f, 18);
        SpawnRingFx(Impact, FMath::Max(160.0f, Event.Width * 1.45f), Secondary, 0.72f, 5.0f, 3.2f);
        return;
    }

    if (Style.Contains(TEXT("whip")))
    {
        const FVector Dir(FMath::Cos(Event.Angle), FMath::Sin(Event.Angle), 0.0f);
        const FVector Side(-Dir.Y, Dir.X, 0.0f);
        const FVector Mid = Origin + Dir * Event.Range * 0.55f + Side * Event.Width * 0.55f + FVector(0.0f, 0.0f, ProjectileFxZ);
        const FVector End = Origin + Dir * Event.Range + FVector(0.0f, 0.0f, ProjectileFxZ);
        SpawnBeamFx(Origin + FVector(0.0f, 0.0f, ProjectileFxZ), Mid, FMath::Max(4.0f, Event.Width * 0.08f), Primary, 0.34f, 5.0f);
        SpawnBeamFx(Mid, End, FMath::Max(3.0f, Event.Width * 0.06f), Secondary, 0.34f, 5.5f);
        SpawnBurstFx(End, 44.0f, Primary, 0.26f, 6);
        return;
    }

    const float ArcDeg = Style.Contains(TEXT("cryo")) ? FMath::Max(Event.ArcDeg, 92.0f) : Event.ArcDeg;
    SpawnArcFx(Origin, Event.Angle, FMath::Max(70.0f, Event.Range), FMath::Max(24.0f, ArcDeg), FMath::Max(32.0f, Event.Width), Primary, Secondary, Style.Contains(TEXT("chainsaw")) ? 0.42f : 0.32f, Style.Contains(TEXT("chainsaw")) ? 18 : 12);

    if (Style.Contains(TEXT("chainsaw")))
    {
        SpawnBurstFx(Impact, 60.0f + Event.HitCount * 8.0f, Secondary, 0.28f, 12);
    }
    else if (Style.Contains(TEXT("cryo")))
    {
        SpawnRingFx(Impact, 72.0f, Secondary, 0.45f, 3.0f, 4.5f);
    }
    else if (Style.Contains(TEXT("scythe")) || Style.Contains(TEXT("glaive")))
    {
        SpawnRingFx(Impact, 92.0f, Secondary, 0.38f, 2.5f, 4.0f);
    }
    else if (Event.HitCount > 0)
    {
        SpawnBurstFx(Impact, 54.0f, Primary, 0.24f, 7);
    }
}

void ACWNativeWorldActor::AddRocketTrailFx(const FCWBulletSnapshot& Bullet)
{
    const FLinearColor Color = ResolveFxColor(Bullet.Color, FLinearColor(1.0f, 0.45f, 0.1f, 1.0f));
    const FVector Dir = FVector(Bullet.Vx, Bullet.Vy, 0.0f).GetSafeNormal();
    const FVector Tail = MakeWorldLocation(Bullet.X, Bullet.Y, ProjectileFxZ) - Dir * FMath::Max(16.0f, Bullet.Radius * 2.5f);
    SpawnParticleSprayFx(Tail, -Dir, FVector(-Dir.Y, Dir.X, 0.0f), Color, 4, 70.0f, 210.0f, 0.22f, 0.016f, 0.04f, 22.0f, -90.0f);
}

void ACWNativeWorldActor::AddProjectileImpactClip(const FString& BulletId, const FVector2D& Position, const FVector2D& Direction, float Radius, float LifeSeconds)
{
    const double TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    FCWProjectileImpactClip Clip;
    Clip.BulletId = BulletId;
    Clip.Position = Position;
    Clip.Direction = Direction.GetSafeNormal();
    Clip.Radius = FMath::Clamp(Radius, 40.0f, 520.0f);
    Clip.ExpireTime = TimeSeconds + FMath::Clamp(LifeSeconds, 0.08f, 1.2f);
    RecentProjectileImpactClips.Add(Clip);

    if (!BulletId.IsEmpty())
    {
        ProjectileVisualStates.Remove(BulletId);
        RetiredProjectileVisualUntil.Add(BulletId, Clip.ExpireTime);
    }

    if (RecentProjectileImpactClips.Num() > 128)
    {
        RecentProjectileImpactClips.RemoveAt(0, RecentProjectileImpactClips.Num() - 128, EAllowShrinking::No);
    }
}

void ACWNativeWorldActor::PruneProjectileImpactClips(double TimeSeconds)
{
    for (int32 Index = RecentProjectileImpactClips.Num() - 1; Index >= 0; --Index)
    {
        if (RecentProjectileImpactClips[Index].ExpireTime <= TimeSeconds)
        {
            RecentProjectileImpactClips.RemoveAtSwap(Index, 1, EAllowShrinking::No);
        }
    }
}

void ACWNativeWorldActor::SyncProjectileVisualTargets(const FCWRoomSnapshot& State)
{
    const double TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    PruneProjectileImpactClips(TimeSeconds);
    TArray<FString> ExpiredRetiredProjectileIds;
    for (const TPair<FString, double>& Pair : RetiredProjectileVisualUntil)
    {
        if (Pair.Value <= TimeSeconds)
        {
            ExpiredRetiredProjectileIds.Add(Pair.Key);
        }
    }
    for (const FString& ExpiredId : ExpiredRetiredProjectileIds)
    {
        RetiredProjectileVisualUntil.Remove(ExpiredId);
    }

    TArray<FCWProjectileImpactClip> CurrentImpactClips = RecentProjectileImpactClips;
    CurrentImpactClips.Reserve(CurrentImpactClips.Num() + State.ObjectImpactEvents.Num());
    for (const FCWObjectImpactEventSnapshot& Event : State.ObjectImpactEvents)
    {
        FCWProjectileImpactClip Clip;
        Clip.BulletId = Event.BulletId;
        Clip.Position = FVector2D(Event.X, Event.Y);
        Clip.Direction = FVector2D(Event.DirX, Event.DirY).GetSafeNormal();
        Clip.Radius = FMath::Clamp(Event.Damage * 0.12f + 120.0f, 92.0f, 420.0f);
        Clip.ExpireTime = TimeSeconds + 0.5;
        CurrentImpactClips.Add(Clip);

        if (!Clip.BulletId.IsEmpty())
        {
            ProjectileVisualStates.Remove(Clip.BulletId);
            RetiredProjectileVisualUntil.Add(Clip.BulletId, TimeSeconds + 0.8);
        }
    }

    int32 BulletIndex = 0;
    for (const FCWBulletSnapshot& Bullet : State.Bullets)
    {
        const bool bRocket = Bullet.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase)
            || Bullet.FxKey.Equals(TEXT("projectile.rocket"), ESearchCase::IgnoreCase);
        const FString Key = Bullet.Id.IsEmpty()
            ? FString::Printf(
                TEXT("%s:%s:%s:%s:%d"),
                bRocket ? TEXT("rocket") : TEXT("bullet"),
                *Bullet.OwnerId,
                *Bullet.OwnerPlayerId,
                *Bullet.WeaponKey,
                BulletIndex)
            : Bullet.Id;
        ++BulletIndex;

        const FVector2D ServerPosition(Bullet.X, Bullet.Y);
        const FVector2D ServerVelocity(Bullet.Vx, Bullet.Vy);

        if (!Key.IsEmpty())
        {
            if (const double* RetiredUntil = RetiredProjectileVisualUntil.Find(Key))
            {
                if (*RetiredUntil > TimeSeconds)
                {
                    continue;
                }
                RetiredProjectileVisualUntil.Remove(Key);
            }
        }

        bool bClipProjectileAtImpact = false;
        const FVector2D BulletDir = ServerVelocity.GetSafeNormal();
        for (const FCWProjectileImpactClip& Clip : CurrentImpactClips)
        {
            if (!Clip.BulletId.IsEmpty() && Clip.BulletId == Key)
            {
                bClipProjectileAtImpact = true;
                break;
            }

            if (BulletDir.IsNearlyZero())
            {
                continue;
            }

            const FVector2D ImpactDir = Clip.Direction.IsNearlyZero() ? BulletDir : Clip.Direction;
            if (FVector2D::DotProduct(BulletDir, ImpactDir) < 0.68f)
            {
                continue;
            }

            const FVector2D ImpactToBullet = ServerPosition - Clip.Position;
            const float Along = FVector2D::DotProduct(ImpactToBullet, BulletDir);
            const float Lateral = FMath::Abs(ImpactToBullet.X * BulletDir.Y - ImpactToBullet.Y * BulletDir.X);
            const float Speed = ServerVelocity.Size();
            const float MaxPastImpactDistance = FMath::Clamp(Speed * 0.18f, 140.0f, 950.0f);
            const float MaxLateralDistance = FMath::Max(Clip.Radius, Bullet.Radius * 7.0f);
            if (Along >= -80.0f && Along <= MaxPastImpactDistance && Lateral <= MaxLateralDistance)
            {
                bClipProjectileAtImpact = true;
                break;
            }
        }

        if (bClipProjectileAtImpact)
        {
            ProjectileVisualStates.Remove(Key);
            if (!Key.IsEmpty())
            {
                RetiredProjectileVisualUntil.Add(Key, TimeSeconds + 0.8);
            }
            continue;
        }

        FCWProjectileVisualState& VisualState = ProjectileVisualStates.FindOrAdd(Key);

        if (!VisualState.bInitialized)
        {
            VisualState.Position = ServerPosition;
            VisualState.Velocity = ServerVelocity;
            VisualState.bInitialized = true;
        }
        else
        {
            const float VelocityBlend = bRocket ? 0.45f : 0.65f;
            VisualState.Velocity = FMath::Lerp(VisualState.Velocity, ServerVelocity, VelocityBlend);
        }

        VisualState.ServerPosition = ServerPosition;
        VisualState.ServerVelocity = ServerVelocity;
        VisualState.Kind = Bullet.Kind;
        VisualState.FxKey = Bullet.FxKey;
        VisualState.Color = Bullet.Color;
        VisualState.Radius = Bullet.Radius;
        VisualState.LastServerTime = TimeSeconds;
        VisualState.LastSeenTime = TimeSeconds;
    }
}

void ACWNativeWorldActor::RebuildProjectileInstances(float DeltaSeconds)
{
    if (BulletInstances)
    {
        BulletInstances->ClearInstances();
    }
    if (BulletCoreInstances)
    {
        BulletCoreInstances->ClearInstances();
    }
    if (RocketInstances)
    {
        RocketInstances->ClearInstances();
    }
    if (RocketNoseInstances)
    {
        RocketNoseInstances->ClearInstances();
    }
    if (RocketFlameInstances)
    {
        RocketFlameInstances->ClearInstances();
    }

    const double TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const float SafeDelta = FMath::Clamp(DeltaSeconds, 0.0f, 0.05f);
    TArray<FString> RetiredProjectileKeys;

    int32 VisualIndex = 0;
    for (TPair<FString, FCWProjectileVisualState>& Pair : ProjectileVisualStates)
    {
        FCWProjectileVisualState& VisualState = Pair.Value;
        if (!VisualState.bInitialized)
        {
            RetiredProjectileKeys.Add(Pair.Key);
            continue;
        }

        const bool bRocket = VisualState.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase)
            || VisualState.FxKey.Equals(TEXT("projectile.rocket"), ESearchCase::IgnoreCase);
        const float TimeSinceSeen = static_cast<float>(TimeSeconds - VisualState.LastSeenTime);
        const float RetireSeconds = bRocket ? 0.34f : 0.22f;
        if (TimeSinceSeen > RetireSeconds)
        {
            RetiredProjectileKeys.Add(Pair.Key);
            continue;
        }

        const float PredictionAge = FMath::Clamp(static_cast<float>(TimeSeconds - VisualState.LastServerTime), 0.0f, 0.22f);
        const FVector2D ServerPredictedPosition = VisualState.ServerPosition + VisualState.ServerVelocity * PredictionAge;
        const float SnapDistance = bRocket ? 2800.0f : 1800.0f;
        if (SafeDelta <= KINDA_SMALL_NUMBER || FVector2D::DistSquared(VisualState.Position, ServerPredictedPosition) > FMath::Square(SnapDistance))
        {
            VisualState.Position = ServerPredictedPosition;
        }
        else
        {
            VisualState.Position += VisualState.Velocity * SafeDelta;

            const FVector2D DeltaToTarget = ServerPredictedPosition - VisualState.Position;
            const float DistanceToTarget = DeltaToTarget.Size();
            if (DistanceToTarget > 0.001f)
            {
                const float Speed = VisualState.Velocity.Size();
                const float MaxStep = FMath::Max(bRocket ? 5.0f : 8.0f, Speed * SafeDelta * (bRocket ? 1.05f : 1.4f));
                const float Correction = FMath::Min(DistanceToTarget, MaxStep);
                const float CorrectionRate = 18.0f * (bRocket ? 0.58f : 1.0f);
                const float CorrectionAlpha = (Correction / DistanceToTarget) * FMath::Clamp(CorrectionRate * SafeDelta, 0.0f, 1.0f);
                VisualState.Position += DeltaToTarget * CorrectionAlpha;
            }
        }

        const FVector2D Position = VisualState.Position;

        FVector Dir(VisualState.Velocity.X, VisualState.Velocity.Y, 0.0f);
        if (Dir.SizeSquared2D() <= KINDA_SMALL_NUMBER)
        {
            const FVector2D Delta = ServerPredictedPosition - Position;
            Dir = FVector(Delta.X, Delta.Y, 0.0f);
        }
        Dir = Dir.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
        const float YawDeg = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
        const FVector Location(Position.X, Position.Y, ProjectileFxZ);

        if (bRocket)
        {
            const float Radius = FMath::Max(6.0f, VisualState.Radius);
            const float BodyLength = FMath::Clamp(Radius * 4.8f, 42.0f, 96.0f);
            const float BodyWidth = FMath::Clamp(Radius * 0.9f, 8.0f, 18.0f);
            const float FlamePulse = 1.0f + 0.18f * FMath::Sin(static_cast<float>(TimeSeconds) * 22.0f + VisualIndex);
            const FVector Side(-Dir.Y, Dir.X, 0.0f);
            if (RocketInstances)
            {
                const bool bUseCylinderBody = RocketInstances->GetStaticMesh() == CylinderMeshAsset;
                RocketInstances->AddInstance(FTransform(
                    bUseCylinderBody ? FRotationMatrix::MakeFromZ(Dir).Rotator() : FRotator(0.0f, YawDeg, 0.0f),
                    Location,
                    bUseCylinderBody
                        ? FVector(BodyWidth / BasicMeshSizeCm, BodyWidth / BasicMeshSizeCm, BodyLength / BasicMeshSizeCm)
                        : FVector(BodyLength / BasicMeshSizeCm, BodyWidth / BasicMeshSizeCm, BodyWidth / BasicMeshSizeCm)));
            }
            if (RocketNoseInstances)
            {
                RocketNoseInstances->AddInstance(FTransform(
                    FRotator(0.0f, YawDeg, 0.0f),
                    Location + Dir * (BodyLength * 0.5f),
                    FVector(BodyWidth * 0.95f / BasicMeshSizeCm, BodyWidth * 0.95f / BasicMeshSizeCm, BodyWidth * 0.95f / BasicMeshSizeCm)));
            }
            if (RocketFlameInstances)
            {
                RocketFlameInstances->AddInstance(FTransform(
                    FRotator(0.0f, YawDeg, 0.0f),
                    Location - Dir * (BodyLength * 0.62f),
                    FVector(BodyWidth * 1.15f * FlamePulse / BasicMeshSizeCm, BodyWidth * 0.92f / BasicMeshSizeCm, BodyWidth * 1.35f / BasicMeshSizeCm)));
                RocketFlameInstances->AddInstance(FTransform(
                    FRotator(0.0f, YawDeg, 0.0f),
                    Location - Dir * (BodyLength * 0.48f) + Side * BodyWidth * 0.58f,
                    FVector(BodyWidth * 0.45f * FlamePulse / BasicMeshSizeCm, BodyWidth * 0.45f / BasicMeshSizeCm, BodyWidth * 0.7f / BasicMeshSizeCm)));
                RocketFlameInstances->AddInstance(FTransform(
                    FRotator(0.0f, YawDeg, 0.0f),
                    Location - Dir * (BodyLength * 0.48f) - Side * BodyWidth * 0.58f,
                    FVector(BodyWidth * 0.45f * FlamePulse / BasicMeshSizeCm, BodyWidth * 0.45f / BasicMeshSizeCm, BodyWidth * 0.7f / BasicMeshSizeCm)));
            }
            ++VisualIndex;
            continue;
        }

        const float Radius = FMath::Max(3.0f, VisualState.Radius);
        const float Speed = FVector(VisualState.Velocity.X, VisualState.Velocity.Y, 0.0f).Size2D();
        const float Length = FMath::Clamp(12.0f + Radius * 1.8f + Speed * 0.004f, 14.0f, 34.0f);
        const float Width = FMath::Clamp(1.1f + Radius * 0.16f, 1.4f, 3.0f);
        if (BulletInstances)
        {
            const bool bUseCylinderBody = BulletInstances->GetStaticMesh() == CylinderMeshAsset;
            BulletInstances->AddInstance(FTransform(
                bUseCylinderBody ? FRotationMatrix::MakeFromZ(Dir).Rotator() : FRotator(0.0f, YawDeg, 0.0f),
                Location - Dir * (Length * 0.30f),
                bUseCylinderBody
                    ? FVector(Width / BasicMeshSizeCm, Width / BasicMeshSizeCm, Length / BasicMeshSizeCm)
                    : FVector(Length / BasicMeshSizeCm, Width / BasicMeshSizeCm, Width / BasicMeshSizeCm)));
        }
        if (BulletCoreInstances)
        {
            BulletCoreInstances->AddInstance(FTransform(
                FRotator(0.0f, YawDeg, 0.0f),
                Location + Dir * (Length * 0.32f),
                FVector(Width * 1.25f / BasicMeshSizeCm)));
            BulletCoreInstances->AddInstance(FTransform(
                FRotator(0.0f, YawDeg, 0.0f),
                Location - Dir * (Length * 0.62f),
                FVector(Width * 0.55f / BasicMeshSizeCm)));
        }
        ++VisualIndex;
    }

    for (const FString& Key : RetiredProjectileKeys)
    {
        ProjectileVisualStates.Remove(Key);
    }
}

void ACWNativeWorldActor::UpdateMapObjectStaticMeshVisuals(const FCWRoomSnapshot& State)
{
    TSet<FString> LiveObjectIds;
    int32 RenderedCount = 0;
    int32 ObjectIndex = 0;

    for (const FCWMapObjectSnapshot& Object : State.MapObjects)
    {
        const FString ObjectId = Object.Id.IsEmpty()
            ? FString::Printf(TEXT("map_object_%d"), ObjectIndex)
            : Object.Id;
        ++ObjectIndex;

        if (ObjectId.IsEmpty() || (Object.bDestroyed && Object.bHideAfterDestroyed))
        {
            continue;
        }
        if (MaxStaticMeshMapObjects > 0 && RenderedCount >= MaxStaticMeshMapObjects)
        {
            continue;
        }

        const FString MeshKey = ResolveMapObjectMeshKey(Object.SpriteKey);
        UStaticMesh* Mesh = LoadMapObjectStaticMesh(MeshKey);
        if (!Mesh)
        {
            continue;
        }

        ++RenderedCount;
        LiveObjectIds.Add(ObjectId);

        UStaticMeshComponent*& Component = MapObjectStaticMeshComponents.FindOrAdd(ObjectId);
        if (!Component)
        {
            Component = NewObject<UStaticMeshComponent>(this);
            if (!Component)
            {
                MapObjectStaticMeshComponents.Remove(ObjectId);
                continue;
            }

            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetMobility(EComponentMobility::Movable);
            Component->SetCastShadow(true);
            Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
            Component->RegisterComponent();
            AddInstanceComponent(Component);
        }

        if (MapObjectStaticMeshKeys.FindRef(ObjectId) != MeshKey || Component->GetStaticMesh() != Mesh)
        {
            Component->SetStaticMesh(Mesh);
            Component->EmptyOverrideMaterials();
            MapObjectStaticMeshKeys.Add(ObjectId, MeshKey);
        }

        if (Mesh == PlaneMeshAsset)
        {
            if (UMaterialInterface* TextureMaterial = GetMapObjectTextureMaterial(MeshKey))
            {
                Component->SetMaterial(0, TextureMaterial);
            }
        }
        else if (MeshKey == TEXT("car_red") && MapObjectCarRedMaterial)
        {
            Component->SetMaterial(0, MapObjectCarRedMaterial);
        }
        else if (MeshKey == TEXT("car_blue") && MapObjectCarBlueMaterial)
        {
            Component->SetMaterial(0, MapObjectCarBlueMaterial);
        }

        Component->SetWorldTransform(BuildMapObjectTransform(Object, Mesh));
        Component->SetVisibility(true, true);
        Component->SetCastShadow(!Object.bDestroyed);
    }

    TArray<FString> RetiredObjectIds;
    for (const TPair<FString, UStaticMeshComponent*>& Pair : MapObjectStaticMeshComponents)
    {
        if (!LiveObjectIds.Contains(Pair.Key))
        {
            RetiredObjectIds.Add(Pair.Key);
        }
    }

    for (const FString& ObjectId : RetiredObjectIds)
    {
        UStaticMeshComponent* Component = nullptr;
        MapObjectStaticMeshComponents.RemoveAndCopyValue(ObjectId, Component);
        MapObjectStaticMeshKeys.Remove(ObjectId);
        if (Component)
        {
            Component->DestroyComponent();
        }
    }
}

void ACWNativeWorldActor::UpdateTreeStaticMeshVisuals(const FCWRoomSnapshot& State)
{
    if (!TreeStaticMeshInstances)
    {
        return;
    }

    UStaticMesh* TreeMesh = LoadMapObjectStaticMesh(TEXT("tree"));
    if (!TreeMesh)
    {
        TreeStaticMeshInstances->ClearInstances();
        return;
    }

    if (TreeStaticMeshInstances->GetStaticMesh() != TreeMesh)
    {
        TreeStaticMeshInstances->SetStaticMesh(TreeMesh);
    }

    TreeStaticMeshInstances->ClearInstances();
    const FBoxSphereBounds Bounds = TreeMesh->GetBounds();
    const FVector BoundsMin = Bounds.Origin - Bounds.BoxExtent;
    const float MeshHeight = FMath::Max(1.0f, Bounds.BoxExtent.Z * 2.0f);

    for (const FCWTreeSnapshot& Tree : State.Trees)
    {
        const float TreeScale = FMath::Clamp(Tree.Scale, 0.35f, 2.1f);
        const float TargetHeight = 360.0f * TreeScale;
        const float UniformScale = FMath::Clamp(
            (TargetHeight / MeshHeight) * MapObjectStaticMeshVisualScaleMultiplier(TEXT("tree")),
            0.035f,
            36.0f);
        const int32 Hash = HashCombine(::GetTypeHash(FMath::RoundToInt(Tree.X * 3.0f)), ::GetTypeHash(FMath::RoundToInt(Tree.Y * 3.0f)));
        const float Yaw = static_cast<float>((Hash & 0x3ff) % 360);
        const FVector Location(Tree.X, Tree.Y, GroundZ - BoundsMin.Z * UniformScale);
        TreeStaticMeshInstances->AddInstance(FTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector(UniformScale)));
    }
}

void ACWNativeWorldActor::DestroyMapObjectStaticMeshVisuals()
{
    if (TreeStaticMeshInstances)
    {
        TreeStaticMeshInstances->ClearInstances();
    }

    for (TPair<FString, UStaticMeshComponent*>& Pair : MapObjectStaticMeshComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->DestroyComponent();
        }
    }
    MapObjectStaticMeshComponents.Reset();
    MapObjectStaticMeshKeys.Reset();
    MapObjectStaticMeshCache.Reset();
    MapObjectTextureCache.Reset();
    MapObjectTextureMaterialCache.Reset();
    MissingMapObjectAssetWarnings.Reset();
}

FString ACWNativeWorldActor::ResolveMapObjectMeshKey(const FString& SpriteKey) const
{
    FString Key = SpriteKey.TrimStartAndEnd().ToLower();
    Key.ReplaceInline(TEXT("-"), TEXT("_"));

    if (Key == TEXT("yellow_bus"))
    {
        return TEXT("bus_yellow");
    }
    if (Key == TEXT("ambulance_van"))
    {
        return TEXT("ambulance");
    }
    if (Key == TEXT("concrete_barrier"))
    {
        return TEXT("barrier");
    }
    if (Key == TEXT("red_hatchback"))
    {
        return TEXT("car_red");
    }
    if (Key == TEXT("burnt_sedan"))
    {
        return TEXT("car_blue");
    }
    if (Key == TEXT("road_shack") || Key == TEXT("shack"))
    {
        return TEXT("shack");
    }
    if (Key == TEXT("build_1") || Key == TEXT("bullet_bistro_bunker")) return TEXT("build_1");
    if (Key == TEXT("build_2") || Key == TEXT("fuel_hell_checkpoint")) return TEXT("build_2");
    if (Key == TEXT("build_3") || Key == TEXT("haven_nope_tower")) return TEXT("build_3");
    if (Key == TEXT("build_4") || Key == TEXT("fix_or_die_garage")) return TEXT("build_4");
    if (Key == TEXT("build_5") || Key == TEXT("almost_alive_clinic")) return TEXT("build_5");
    if (Key == TEXT("build_6") || Key == TEXT("hellmart_24_7")) return TEXT("build_6");
    if (Key == TEXT("build_7") || Key == TEXT("dead_signal_station")) return TEXT("build_7");

    if (Key == TEXT("tree")
        || Key == TEXT("bus_yellow")
        || Key == TEXT("ambulance")
        || Key == TEXT("barrier")
        || Key == TEXT("shopping_cart_barricade")
        || Key == TEXT("car_red")
        || Key == TEXT("car_blue")
        || Key == TEXT("mall_block")
        || Key == TEXT("clinic_block")
        || Key == TEXT("industrial_tank")
        || Key == TEXT("reactor_block")
        || Key == TEXT("build_1")
        || Key == TEXT("build_2")
        || Key == TEXT("build_3")
        || Key == TEXT("build_4")
        || Key == TEXT("build_5")
        || Key == TEXT("build_6")
        || Key == TEXT("build_7")
        || Key == TEXT("abandoned_building")
        || Key == TEXT("abandoned_building_1")
        || Key == TEXT("abandoned_building_2")
        || Key == TEXT("abandoned_bus")
        || Key == TEXT("abandoned_gas_station")
        || Key == TEXT("clinic_building")
        || Key == TEXT("cyberpunk_storefront")
        || Key == TEXT("futuristic_barrier")
        || Key == TEXT("futuristic_police_vehicle")
        || Key == TEXT("futuristic_vending_machine")
        || Key == TEXT("glowing_twisted_tree")
        || Key == TEXT("gnarled_tree")
        || Key == TEXT("haunted_house")
        || Key == TEXT("industrial_facility")
        || Key == TEXT("industrial_factory")
        || Key == TEXT("military_ambulance")
        || Key == TEXT("post_apocalyptic_car")
        || Key == TEXT("ruined_sci_fi_facility")
        || Key == TEXT("ruined_storefront")
        || Key == TEXT("rusty_gas_station")
        || Key == TEXT("rusty_shopping_cart")
        || Key == TEXT("steampunk_reactor")
        || Key == TEXT("toxic_waste_barrels")
        || Key == TEXT("wrecked_police_car"))
    {
        return Key;
    }

    return FString();
}

UStaticMesh* ACWNativeWorldActor::LoadMapObjectStaticMesh(const FString& MeshKey)
{
    if (MeshKey.IsEmpty())
    {
        return nullptr;
    }
    if (UStaticMesh* const* Cached = MapObjectStaticMeshCache.Find(MeshKey))
    {
        return *Cached;
    }

    const TArray<FString> CandidatePaths = MapObjectStaticMeshCandidatePaths(MeshKey);
    for (const FString& MeshPath : CandidatePaths)
    {
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath))
        {
            MapObjectStaticMeshCache.Add(MeshKey, Mesh);
            return Mesh;
        }
    }

    if (PlaneMeshAsset && !MapObjectTextureRelativePath(MeshKey).IsEmpty())
    {
        MapObjectStaticMeshCache.Add(MeshKey, PlaneMeshAsset);
        return PlaneMeshAsset;
    }

    const FString WarningKey = FString::Printf(TEXT("map_object_mesh:%s"), *MeshKey);
    if (!MissingMapObjectAssetWarnings.Contains(WarningKey))
    {
        MissingMapObjectAssetWarnings.Add(WarningKey);
        UE_LOG(LogTemp, Warning, TEXT("Missing map object static mesh for key %s. Tried %d candidate path(s)."), *MeshKey, CandidatePaths.Num());
    }
    return nullptr;
}

UTexture2D* ACWNativeWorldActor::LoadMapObjectTexture(const FString& MeshKey)
{
    const FString Key = MeshKey.TrimStartAndEnd().ToLower();
    if (Key.IsEmpty())
    {
        return nullptr;
    }
    if (UTexture2D* const* Cached = MapObjectTextureCache.Find(Key))
    {
        return *Cached;
    }

    const FString RelativePath = MapObjectTextureRelativePath(Key);
    if (RelativePath.IsEmpty())
    {
        MapObjectTextureCache.Add(Key, nullptr);
        return nullptr;
    }

    UTexture2D* Texture = FCWNativeAssetLibrary::LoadTexture(this, AssetsRoot, RelativePath);
    MapObjectTextureCache.Add(Key, Texture);
    return Texture;
}

UMaterialInterface* ACWNativeWorldActor::GetMapObjectTextureMaterial(const FString& MeshKey)
{
    const FString Key = MeshKey.TrimStartAndEnd().ToLower();
    if (Key.IsEmpty())
    {
        return nullptr;
    }
    if (UMaterialInstanceDynamic* const* Cached = MapObjectTextureMaterialCache.Find(Key))
    {
        return *Cached;
    }

    UTexture2D* Texture = LoadMapObjectTexture(Key);
    if (!Texture)
    {
        MapObjectTextureMaterialCache.Add(Key, nullptr);
        return nullptr;
    }

    UMaterialInstanceDynamic* Material = MakeTexturedFxMaterial(TEXT("MapObjectTexture_") + Key, Texture, FLinearColor::White, 0.18f);
    MapObjectTextureMaterialCache.Add(Key, Material);
    return Material;
}

UStaticMesh* ACWNativeWorldActor::LoadXpCrystalStaticMesh()
{
    if (XpCrystalMeshAsset)
    {
        return XpCrystalMeshAsset;
    }
    if (bXpCrystalMeshLoadAttempted)
    {
        return nullptr;
    }
    bXpCrystalMeshLoadAttempted = true;

    const FString CandidatePaths[] = {
        TEXT("/Game/Other/exp-crystal/SM_exp_crystal.SM_exp_crystal"),
        TEXT("/Game/Other/exp-crystal/exp_crystal.exp_crystal"),
        TEXT("/Game/Other/exp-crystal/tripo_convert_58c378b5_22ff_4a85_8542_44672cfac51a.tripo_convert_58c378b5_22ff_4a85_8542_44672cfac51a"),
        TEXT("/Game/Other/exp-crystal/tripo_convert_58c378b5-22ff-4a85-8542-44672cfac51a.tripo_convert_58c378b5-22ff-4a85-8542-44672cfac51a")
    };

    for (const FString& MeshPath : CandidatePaths)
    {
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath))
        {
            XpCrystalMeshAsset = Mesh;
            return XpCrystalMeshAsset;
        }
    }

    const FString WarningKey(TEXT("xp_crystal_mesh"));
    if (!MissingMapObjectAssetWarnings.Contains(WarningKey))
    {
        MissingMapObjectAssetWarnings.Add(WarningKey);
        UE_LOG(LogTemp, Warning, TEXT("XP crystal mesh is not imported as a UStaticMesh yet. Import the FBX from /Game/Other/exp-crystal or name it SM_exp_crystal to use the new model."));
    }
    return nullptr;
}

FTransform ACWNativeWorldActor::BuildMapObjectTransform(const FCWMapObjectSnapshot& Object, UStaticMesh* Mesh) const
{
    if (!Mesh)
    {
        return FTransform::Identity;
    }

    const FBoxSphereBounds Bounds = Mesh->GetBounds();
    const FVector BoundsMin = Bounds.Origin - Bounds.BoxExtent;
    const float AngleAbs = FMath::Abs(Object.Angle);
    const float YawDeg = AngleAbs > (2.0f * UE_PI) + 0.01f ? Object.Angle : FMath::RadiansToDegrees(Object.Angle);
    const FString MeshKey = ResolveMapObjectMeshKey(Object.SpriteKey);
    if (Mesh == PlaneMeshAsset && !MapObjectTextureRelativePath(MeshKey).IsEmpty())
    {
        const float DesiredFootprintX = FMath::Max(24.0f, Object.W * MapObjectFootprintWorldScale(MeshKey));
        const float DesiredFootprintY = FMath::Max(18.0f, Object.H * MapObjectFootprintWorldScale(MeshKey));
        const FVector Scale(DesiredFootprintX / BasicMeshSizeCm, DesiredFootprintY / BasicMeshSizeCm, 1.0f);
        const FVector Location(Object.X, Object.Y, GroundZ + 4.0f);
        return FTransform(FRotator(0.0f, YawDeg, 0.0f), Location, Scale);
    }

    const float MeshFootprintX = FMath::Max(1.0f, Bounds.BoxExtent.X * 2.0f);
    const float MeshFootprintY = FMath::Max(1.0f, Bounds.BoxExtent.Y * 2.0f);
    const float DesiredFootprintX = FMath::Max(24.0f, Object.W * MapObjectFootprintWorldScale(MeshKey));
    const float DesiredFootprintY = FMath::Max(18.0f, Object.H * MapObjectFootprintWorldScale(MeshKey));
    const bool bHasExplicitYawOffset = FMath::Abs(MapObjectStaticMeshYawOffsetDeg(MeshKey)) > KINDA_SMALL_NUMBER;
    const bool bNeedsQuarterTurn = !bHasExplicitYawOffset && (
        (DesiredFootprintX >= DesiredFootprintY && MeshFootprintX < MeshFootprintY)
        || (DesiredFootprintX < DesiredFootprintY && MeshFootprintX >= MeshFootprintY));
    const bool bSwapFootprintAxes = bHasExplicitYawOffset || bNeedsQuarterTurn;
    const float ImportScale = MapObjectStaticMeshVisualScaleMultiplier(MeshKey);
    const float ScaleX = FMath::Clamp(
        (bSwapFootprintAxes ? DesiredFootprintY : DesiredFootprintX) / MeshFootprintX,
        0.035f,
        60.0f) * ImportScale;
    const float ScaleY = FMath::Clamp(
        (bSwapFootprintAxes ? DesiredFootprintX : DesiredFootprintY) / MeshFootprintY,
        0.035f,
        60.0f) * ImportScale;
    FVector Scale(ScaleX, ScaleY, FMath::Min(ScaleX, ScaleY));
    if (Object.bDestroyed && Object.bDestructible)
    {
        Scale.Z *= 0.42f;
    }

    const FVector Location(Object.X, Object.Y, GroundZ - BoundsMin.Z * Scale.Z);
    const float NativeYawOffset = bHasExplicitYawOffset ? MapObjectStaticMeshYawOffsetDeg(MeshKey) : (bNeedsQuarterTurn ? 90.0f : 0.0f);
    return FTransform(FRotator(0.0f, YawDeg + NativeYawOffset, 0.0f), Location, Scale);
}

void ACWNativeWorldActor::MarkPlayerAttackVisual(const FString& PlayerId, const FVector2D& Origin, const FVector2D& Aim, float Duration)
{
    const FString PlayerKey = PlayerId.TrimStartAndEnd();
    if (PlayerKey.IsEmpty())
    {
        return;
    }

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    PlayerAttackVisualUntil.Add(PlayerKey, Now + FMath::Max(0.08f, Duration));

    const FVector2D AimDelta = Aim - Origin;
    if (AimDelta.SizeSquared() > 9.0f)
    {
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(AimDelta.Y, AimDelta.X));
        PlayerAttackVisualRotations.Add(PlayerKey, FRotator(0.0f, Yaw, 0.0f));
    }
}

bool ACWNativeWorldActor::UpdatePlayerSkeletalVisual(const FCWPlayerSnapshot& Player)
{
    const FString PlayerKey = Player.Id.IsEmpty()
        ? FString::Printf(TEXT("player_%s"), *ResolveHeroId(Player))
        : Player.Id;
    if (PlayerKey.IsEmpty())
    {
        return false;
    }

    const FString HeroId = ResolveHeroId(Player);
    USkeletalMesh* Mesh = LoadHeroSkeletalMesh(HeroId);
    if (!Mesh)
    {
        return false;
    }

    USkeletalMeshComponent*& Component = PlayerSkeletalComponents.FindOrAdd(PlayerKey);
    const bool bNewComponent = Component == nullptr;
    if (!Component)
    {
        Component = NewObject<USkeletalMeshComponent>(this);
        if (!Component)
        {
            PlayerSkeletalComponents.Remove(PlayerKey);
            return false;
        }

        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetCastShadow(true);
        Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Component->bPauseAnims = false;
        Component->GlobalAnimRateScale = 1.15f;
        Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        Component->RegisterComponent();
        AddInstanceComponent(Component);
    }

    const bool bHeroChanged = PlayerSkeletalHeroIds.FindRef(PlayerKey) != HeroId || Component->GetSkeletalMeshAsset() != Mesh;
    if (bHeroChanged)
    {
        Component->SetSkeletalMesh(Mesh);
        PlayerSkeletalHeroIds.Add(PlayerKey, HeroId);
        ApplyReadableHeroMaterials(Component, HeroId);
    }

    const FVector2D CurrentLocation2D(Player.X, Player.Y);
    FVector2D PreviousLocation2D = CurrentLocation2D;
    bool bHasPreviousLocation = false;
    if (const FVector2D* PreviousLocation = PlayerPreviousLocations.Find(PlayerKey))
    {
        PreviousLocation2D = *PreviousLocation;
        bHasPreviousLocation = true;
    }

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    double PreviousSampleTime = Now;
    if (const double* SampleTime = PlayerPreviousSampleTimes.Find(PlayerKey))
    {
        PreviousSampleTime = *SampleTime;
    }

    const FVector2D MoveDelta = CurrentLocation2D - PreviousLocation2D;
    const double SampleDelta = FMath::Max(0.0, Now - PreviousSampleTime);
    const float MoveSpeed = (bHasPreviousLocation && SampleDelta > KINDA_SMALL_NUMBER)
        ? MoveDelta.Size() / static_cast<float>(SampleDelta)
        : 0.0f;

    PlayerPreviousLocations.Add(PlayerKey, CurrentLocation2D);
    PlayerPreviousSampleTimes.Add(PlayerKey, Now);

    const bool bMoving = MoveSpeed > 18.0f;
    const double AttackUntil = PlayerAttackVisualUntil.FindRef(PlayerKey);
    const bool bAttackWindow = Player.bAlive && Now < AttackUntil;
    UAnimSequence* DesiredAnim = nullptr;
    bool bLoopAnim = Player.bAlive;
    float DesiredPlayRate = 1.0f;

    if (!Player.bAlive)
    {
        DesiredAnim = LoadHeroDeathAnimation(HeroId);
        bLoopAnim = false;
    }
    else if (bAttackWindow)
    {
        DesiredAnim = LoadHeroAttackAnimation(HeroId);
        bLoopAnim = false;
        DesiredPlayRate = 1.18f;
    }
    if (!DesiredAnim && Player.bAlive && !bMoving)
    {
        DesiredAnim = LoadHeroIdleAnimation(HeroId);
        bLoopAnim = true;
        DesiredPlayRate = 1.0f;
    }
    if (!DesiredAnim && Player.bAlive)
    {
        DesiredAnim = LoadHeroWalkAnimation(HeroId);
        bLoopAnim = true;
        DesiredPlayRate = FMath::Clamp(MoveSpeed / 260.0f, bMoving ? 0.62f : 0.08f, 1.32f);
    }
    if (DesiredAnim)
    {
        const float AnimPhase = StableUnitFromString(PlayerKey + HeroId);
        Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        if (UAnimSingleNodeInstance* SingleNode = Component->GetSingleNodeInstance())
        {
            const bool bAnimChanged = SingleNode->GetAnimationAsset() != DesiredAnim;
            if (bAnimChanged)
            {
                SingleNode->SetAnimationAsset(DesiredAnim, bLoopAnim, DesiredPlayRate);
                if (bLoopAnim)
                {
                    const float AnimLength = DesiredAnim->GetPlayLength();
                    if (AnimLength > KINDA_SMALL_NUMBER)
                    {
                        SingleNode->SetPosition(AnimLength * AnimPhase, false);
                    }
                }
            }
            SingleNode->SetLooping(bLoopAnim);
            SingleNode->SetPlayRate(DesiredPlayRate);
            if (Player.bAlive || bAnimChanged)
            {
                SingleNode->SetPlaying(true);
            }
        }
        else if (bNewComponent || bHeroChanged)
        {
            Component->PlayAnimation(DesiredAnim, bLoopAnim);
            Component->SetPlayRate(DesiredPlayRate);
        }
    }

    const FBoxSphereBounds MeshBounds = Component->GetLocalBounds();
    const FVector BoundsMin = MeshBounds.Origin - MeshBounds.BoxExtent;
    const FVector BoundsMax = MeshBounds.Origin + MeshBounds.BoxExtent;
    const float MeshHeight = FMath::Max(1.0f, BoundsMax.Z - BoundsMin.Z);
    const float PlayerVisualScale = Player.bIsCompanion ? SkeletalVisualScale : SkeletalVisualScale / LocalPlayerVisualScaleDivisor;
    const float TargetHeight = (Player.bIsCompanion ? 180.0f : 260.0f) * PlayerVisualScale;
    const float MeshScale = TargetHeight / MeshHeight;

    float TargetYaw = Component->GetComponentRotation().Yaw;
    const FVector2D AimDelta(Player.AimX - Player.X, Player.AimY - Player.Y);
    if (Player.bAlive && AimDelta.SizeSquared() > 16.0f)
    {
        TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(AimDelta.Y, AimDelta.X));
    }
    else if (Player.bAlive && MoveDelta.SizeSquared() > 4.0f)
    {
        TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(MoveDelta.Y, MoveDelta.X));
    }
    if (Player.bAlive && bAttackWindow)
    {
        if (const FRotator* AttackRotation = PlayerAttackVisualRotations.Find(PlayerKey))
        {
            TargetYaw = AttackRotation->Yaw;
        }
    }

    FCWSkeletalVisualTarget Target;
    Target.Location = FVector(Player.X, Player.Y, GroundZ - BoundsMin.Z * MeshScale);
    Target.Rotation = FRotator(0.0f, TargetYaw, 0.0f);
    Target.Scale = FVector(MeshScale);
    Target.bSnap = bNewComponent || !bHasPreviousLocation || FVector::DistSquared2D(Component->GetComponentLocation(), Target.Location) > FMath::Square(900.0f);
    PlayerSkeletalTargets.Add(PlayerKey, Target);

    if (Target.bSnap)
    {
        Component->SetWorldLocationAndRotation(Target.Location, Target.Rotation);
        Component->SetWorldScale3D(Target.Scale);
    }
    Component->SetVisibility(true, true);
    return true;
}

void ACWNativeWorldActor::RetireMissingPlayerSkeletalVisuals(const FCWRoomSnapshot& State)
{
    TSet<FString> LivePlayerIds;
    for (const FCWPlayerSnapshot& Player : State.Players)
    {
        const FString PlayerKey = Player.Id.IsEmpty()
            ? FString::Printf(TEXT("player_%s"), *ResolveHeroId(Player))
            : Player.Id;
        if (!PlayerKey.IsEmpty())
        {
            LivePlayerIds.Add(PlayerKey);
        }
    }

    TArray<FString> RetiredIds;
    for (const TPair<FString, USkeletalMeshComponent*>& Pair : PlayerSkeletalComponents)
    {
        if (!LivePlayerIds.Contains(Pair.Key))
        {
            RetiredIds.Add(Pair.Key);
        }
    }

    for (const FString& PlayerId : RetiredIds)
    {
        USkeletalMeshComponent* Component = nullptr;
        PlayerSkeletalComponents.RemoveAndCopyValue(PlayerId, Component);
        PlayerSkeletalHeroIds.Remove(PlayerId);
        PlayerPreviousLocations.Remove(PlayerId);
        PlayerPreviousSampleTimes.Remove(PlayerId);
        PlayerAttackVisualUntil.Remove(PlayerId);
        PlayerAttackVisualRotations.Remove(PlayerId);
        PlayerSkeletalTargets.Remove(PlayerId);
        if (Component)
        {
            Component->DestroyComponent();
        }
    }
}

void ACWNativeWorldActor::DestroyPlayerSkeletalVisuals()
{
    for (TPair<FString, USkeletalMeshComponent*>& Pair : PlayerSkeletalComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->DestroyComponent();
        }
    }
    PlayerSkeletalComponents.Reset();
    PlayerSkeletalHeroIds.Reset();
    PlayerPreviousLocations.Reset();
    PlayerPreviousSampleTimes.Reset();
    PlayerAttackVisualUntil.Reset();
    PlayerAttackVisualRotations.Reset();
    PlayerSkeletalTargets.Reset();
}

FString ACWNativeWorldActor::ResolveHeroId(const FCWPlayerSnapshot& Player) const
{
    FString HeroId = Player.VisualLoadout.HeroId.IsEmpty() ? Player.PlayerClass : Player.VisualLoadout.HeroId;
    HeroId = HeroId.TrimStartAndEnd().ToLower();
    HeroId.ReplaceInline(TEXT("-"), TEXT("_"));
    HeroId.ReplaceInline(TEXT(" "), TEXT("_"));
    if (HeroId.IsEmpty())
    {
        HeroId = TEXT("cyber");
    }
    if (HeroId == TEXT("medic"))
    {
        HeroId = TEXT("medis");
    }
    return HeroId;
}

USkeletalMesh* ACWNativeWorldActor::LoadHeroSkeletalMesh(const FString& HeroId)
{
    if (USkeletalMesh* const* Cached = HeroSkeletalMeshCache.Find(HeroId))
    {
        return *Cached;
    }

    const FString PreferredPath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_walk_death_animfix.SK_%s_walk_death_animfix"), *HeroId, *HeroId, *HeroId);
    const FString FallbackPath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_walk_death.SK_%s_walk_death"), *HeroId, *HeroId, *HeroId);
    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *PreferredPath);
    const FString Path = Mesh ? PreferredPath : FallbackPath;
    if (!Mesh)
    {
        Mesh = LoadObject<USkeletalMesh>(nullptr, *FallbackPath);
    }
    HeroSkeletalMeshCache.Add(HeroId, Mesh);
    if (!Mesh)
    {
        const FString WarningKey = FString::Printf(TEXT("mesh:%s"), *HeroId);
        if (!MissingHeroAssetWarnings.Contains(WarningKey))
        {
            MissingHeroAssetWarnings.Add(WarningKey);
            UE_LOG(LogTemp, Warning, TEXT("Missing Tripo hero skeletal mesh: %s"), *Path);
        }
    }
    return Mesh;
}

UAnimSequence* ACWNativeWorldActor::LoadHeroIdleAnimation(const FString& HeroId)
{
    if (UAnimSequence* const* Cached = HeroIdleAnimCache.Find(HeroId))
    {
        return *Cached;
    }

    UAnimSequence* Anim = LoadFirstAnimSequencePath({
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/Anims/A_%s_Idle.A_%s_Idle"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/Anims/A_%s_idle.A_%s_idle"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_Idle.SK_%s_Idle"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_idle.SK_%s_idle"), *HeroId, *HeroId, *HeroId)
    });
    HeroIdleAnimCache.Add(HeroId, Anim);
    return Anim;
}

UAnimSequence* ACWNativeWorldActor::LoadHeroWalkAnimation(const FString& HeroId)
{
    if (UAnimSequence* const* Cached = HeroWalkAnimCache.Find(HeroId))
    {
        return *Cached;
    }

    const FString PreferredPath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_walk_death_animfixSK_%s_M_%s_Walk.SK_%s_walk_death_animfixSK_%s_M_%s_Walk"), *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId);
    const FString FallbackPath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_walk_deathSK_%s_M_%s_Walk.SK_%s_walk_deathSK_%s_M_%s_Walk"), *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId);
    UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *PreferredPath);
    const FString Path = Anim ? PreferredPath : FallbackPath;
    if (!Anim)
    {
        Anim = LoadObject<UAnimSequence>(nullptr, *FallbackPath);
    }
    HeroWalkAnimCache.Add(HeroId, Anim);
    if (!Anim)
    {
        const FString WarningKey = FString::Printf(TEXT("walk:%s"), *HeroId);
        if (!MissingHeroAssetWarnings.Contains(WarningKey))
        {
            MissingHeroAssetWarnings.Add(WarningKey);
            UE_LOG(LogTemp, Warning, TEXT("Missing Tripo hero walk animation: %s"), *Path);
        }
    }
    return Anim;
}

UAnimSequence* ACWNativeWorldActor::LoadHeroAttackAnimation(const FString& HeroId)
{
    if (UAnimSequence* const* Cached = HeroAttackAnimCache.Find(HeroId))
    {
        return *Cached;
    }

    UAnimSequence* Anim = LoadFirstAnimSequencePath({
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/Anims/A_%s_Attack.A_%s_Attack"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/Anims/A_%s_Attack_01.A_%s_Attack_01"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/Anims/A_%s_Melee.A_%s_Melee"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_Attack.SK_%s_Attack"), *HeroId, *HeroId, *HeroId),
        FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_Melee.SK_%s_Melee"), *HeroId, *HeroId, *HeroId)
    });
    HeroAttackAnimCache.Add(HeroId, Anim);
    return Anim;
}

UAnimSequence* ACWNativeWorldActor::LoadHeroDeathAnimation(const FString& HeroId)
{
    if (UAnimSequence* const* Cached = HeroDeathAnimCache.Find(HeroId))
    {
        return *Cached;
    }

    const FString PreferredPath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_walk_death_animfixSK_%s_M_%s_Death.SK_%s_walk_death_animfixSK_%s_M_%s_Death"), *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId);
    const FString FallbackPath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/SK_%s_walk_deathSK_%s_M_%s_Death.SK_%s_walk_deathSK_%s_M_%s_Death"), *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId, *HeroId);
    UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *PreferredPath);
    if (!Anim)
    {
        Anim = LoadObject<UAnimSequence>(nullptr, *FallbackPath);
    }
    HeroDeathAnimCache.Add(HeroId, Anim);
    return Anim;
}

UTexture2D* ACWNativeWorldActor::LoadHeroBaseColorTexture(const FString& HeroId)
{
    const FString CleanHeroId = HeroId.TrimStartAndEnd().ToLower();
    if (UTexture2D* const* Cached = HeroBaseColorTextureCache.Find(CleanHeroId))
    {
        return *Cached;
    }

    FString AssetName;
    if (CleanHeroId == TEXT("cyber"))
    {
        AssetName = TEXT("futuristicarmor3dmodel_basecolor");
    }
    else if (CleanHeroId == TEXT("shadow"))
    {
        AssetName = TEXT("futuristichoodedwarrior3dmodel_basecolor");
    }
    else if (CleanHeroId == TEXT("medis") || CleanHeroId == TEXT("medic"))
    {
        AssetName = TEXT("futuristicmedic3dmodel_basecolor");
    }
    else if (CleanHeroId == TEXT("scout") || CleanHeroId == TEXT("raider"))
    {
        AssetName = TEXT("futuristicsoldier3dmodel_basecolor");
    }

    UTexture2D* Texture = nullptr;
    if (!AssetName.IsEmpty())
    {
        const FString TexturePath = FString::Printf(TEXT("/Game/Characters/Heroes/%s/%s.%s"), *CleanHeroId, *AssetName, *AssetName);
        Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
    }

    HeroBaseColorTextureCache.Add(CleanHeroId, Texture);
    return Texture;
}

void ACWNativeWorldActor::ApplyReadableHeroMaterials(USkeletalMeshComponent* Component, const FString& HeroId)
{
    if (!Component)
    {
        return;
    }

    UTexture2D* BaseColorTexture = LoadHeroBaseColorTexture(HeroId);
    UMaterialInterface* TextureParent = SkeletalTextureMaterialParent ? SkeletalTextureMaterialParent : EmissiveFallbackMaterial;
    if (!BaseColorTexture || !TextureParent)
    {
        return;
    }

    const FString CacheKey = FString::Printf(TEXT("hero_textured_v1:%s"), *HeroId);
    UMaterialInstanceDynamic* TextureMaterial = nullptr;
    if (UMaterialInstanceDynamic* const* CachedTextureMaterial = ReadableEnemyTextureMaterialCache.Find(CacheKey))
    {
        TextureMaterial = *CachedTextureMaterial;
    }
    else
    {
        TextureMaterial = UMaterialInstanceDynamic::Create(TextureParent, this);
        if (TextureMaterial)
        {
            TextureMaterial->SetTextureParameterValue(TEXT("SpriteTexture"), BaseColorTexture);
            TextureMaterial->SetTextureParameterValue(TEXT("LinearColor"), BaseColorTexture);
            TextureMaterial->SetTextureParameterValue(TEXT("Texture"), BaseColorTexture);
            TextureMaterial->SetTextureParameterValue(TEXT("EmissiveTexture"), BaseColorTexture);
            TextureMaterial->SetTextureParameterValue(TEXT("BaseTexture"), BaseColorTexture);
            TextureMaterial->SetTextureParameterValue(TEXT("BaseColorMap"), BaseColorTexture);
            TextureMaterial->SetTextureParameterValue(TEXT("DiffuseColorMap"), BaseColorTexture);
            TextureMaterial->SetVectorParameterValue(TEXT("SpriteColor"), FLinearColor(0.96f, 0.94f, 0.90f, 1.0f));
            TextureMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.96f, 0.94f, 0.90f, 1.0f));
            TextureMaterial->SetVectorParameterValue(TEXT("TintColor"), FLinearColor(0.96f, 0.94f, 0.90f, 1.0f));
            TextureMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.24f, 0.24f, 0.22f, 1.0f));
            TextureMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
            TextureMaterial->SetScalarParameterValue(TEXT("Alpha"), 1.0f);
            TextureMaterial->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.0f);
            ReadableEnemyTextureMaterialCache.Add(CacheKey, TextureMaterial);
        }
    }

    if (TextureMaterial)
    {
        const int32 MaterialCount = Component->GetNumMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
        {
            Component->SetMaterial(MaterialIndex, TextureMaterial);
        }
    }
}

bool ACWNativeWorldActor::UpdateEnemySkeletalVisual(const FCWEnemySnapshot& Enemy, const FCWRoomSnapshot& State)
{
    if (Enemy.Id.IsEmpty())
    {
        return false;
    }

    const FString MobId = ResolveEnemyMobId(Enemy);
    USkeletalMesh* Mesh = LoadEnemySkeletalMesh(MobId);
    if (!Mesh)
    {
        return false;
    }

    UAnimSequence* WalkAnim = LoadEnemyWalkAnimation(MobId);
    USkeletalMeshComponent*& Component = EnemySkeletalComponents.FindOrAdd(Enemy.Id);
    if (!Component)
    {
        Component = NewObject<USkeletalMeshComponent>(this);
        if (!Component)
        {
            EnemySkeletalComponents.Remove(Enemy.Id);
            return false;
        }

        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetCastShadow(true);
        Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Component->bPauseAnims = false;
        Component->GlobalAnimRateScale = 1.0f;
        Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        Component->RegisterComponent();
        AddInstanceComponent(Component);
    }

    const bool bMobChanged = EnemySkeletalMobIds.FindRef(Enemy.Id) != MobId || Component->GetSkeletalMeshAsset() != Mesh;
    if (bMobChanged)
    {
        Component->SetSkeletalMesh(Mesh);
        EnemySkeletalMobIds.Add(Enemy.Id, MobId);
        if (bUseImportedEnemyMaterials)
        {
            ApplyReadableEnemyMaterials(Component, MobId);
        }
        else if (SkeletalEnemyPreviewMaterial)
        {
            const int32 MaterialCount = Component->GetNumMaterials();
            for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
            {
                Component->SetMaterial(MaterialIndex, SkeletalEnemyPreviewMaterial);
            }
        }
    }
    const FVector2D CurrentLocation2D(Enemy.X, Enemy.Y);
    FVector2D PreviousLocation2D = CurrentLocation2D;
    bool bHasPreviousLocation = false;
    if (const FVector2D* PreviousLocation = EnemyPreviousLocations.Find(Enemy.Id))
    {
        PreviousLocation2D = *PreviousLocation;
        bHasPreviousLocation = true;
    }

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    double PreviousSampleTime = Now;
    if (const double* SampleTime = EnemyPreviousSampleTimes.Find(Enemy.Id))
    {
        PreviousSampleTime = *SampleTime;
    }

    const FVector2D MoveDelta = CurrentLocation2D - PreviousLocation2D;
    const double SampleDelta = FMath::Max(0.0, Now - PreviousSampleTime);
    const float MoveSpeed = (bHasPreviousLocation && SampleDelta > KINDA_SMALL_NUMBER)
        ? MoveDelta.Size() / static_cast<float>(SampleDelta)
        : 0.0f;

    EnemyPreviousLocations.Add(Enemy.Id, CurrentLocation2D);
    EnemyPreviousSampleTimes.Add(Enemy.Id, Now);

    const bool bMoving = MoveSpeed > 14.0f;
    UAnimSequence* DesiredEnemyAnim = (!bMoving) ? LoadEnemyIdleAnimation(MobId) : nullptr;
    if (!DesiredEnemyAnim)
    {
        DesiredEnemyAnim = WalkAnim;
    }

    if (DesiredEnemyAnim)
    {
        const float AnimPhase = StableUnitFromString(Enemy.Id + MobId);
        const float RateVariation = FMath::Lerp(0.88f, 1.08f, AnimPhase);
        const float WalkPlayRate = FMath::Clamp(MoveSpeed / 260.0f, bMoving ? 0.52f : 0.06f, 1.28f) * RateVariation;
        Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        if (UAnimSingleNodeInstance* SingleNode = Component->GetSingleNodeInstance())
        {
            if (SingleNode->GetAnimationAsset() != DesiredEnemyAnim)
            {
                SingleNode->SetAnimationAsset(DesiredEnemyAnim, true, WalkPlayRate);
                const float AnimLength = DesiredEnemyAnim->GetPlayLength();
                if (AnimLength > KINDA_SMALL_NUMBER)
                {
                    SingleNode->SetPosition(AnimLength * AnimPhase, false);
                }
            }
            SingleNode->SetLooping(true);
            SingleNode->SetPlayRate(WalkPlayRate);
            SingleNode->SetPlaying(true);
        }
        else if (bMobChanged)
        {
            Component->PlayAnimation(DesiredEnemyAnim, true);
            Component->SetPlayRate(WalkPlayRate);
        }
    }

    const FBoxSphereBounds MeshBounds = Component->GetLocalBounds();
    const FVector BoundsMin = MeshBounds.Origin - MeshBounds.BoxExtent;
    const FVector BoundsMax = MeshBounds.Origin + MeshBounds.BoxExtent;
    const float MeshHeight = FMath::Max(1.0f, BoundsMax.Z - BoundsMin.Z);
    const float VisualRadius = FMath::Max(8.0f, Enemy.Radius);
    const float VisualScale = FMath::Clamp(Enemy.SpriteScale <= KINDA_SMALL_NUMBER ? 1.0f : Enemy.SpriteScale, 0.65f, 2.6f);
    const float TargetHeight = FMath::Clamp(VisualRadius * 9.0f * VisualScale * SkeletalVisualScale, 65.0f, 280.0f);
    const float MeshScale = TargetHeight / MeshHeight;
    const FVector TargetLocation(Enemy.X, Enemy.Y, GroundZ - BoundsMin.Z * MeshScale);

    float TargetYaw = Enemy.bFaceLeft ? 180.0f : 0.0f;
    const FCWPlayerSnapshot* FacingPlayer = nullptr;
    float BestFacingDistSq = TNumericLimits<float>::Max();
    const FVector2D EnemyLocation2D(Enemy.X, Enemy.Y);
    for (const FCWPlayerSnapshot& Player : State.Players)
    {
        if (!Player.bAlive || Player.bIsCompanion)
        {
            continue;
        }

        const FVector2D PlayerLocation2D(Player.X, Player.Y);
        const float DistSq = FVector2D::DistSquared(EnemyLocation2D, PlayerLocation2D);
        if (DistSq < BestFacingDistSq)
        {
            BestFacingDistSq = DistSq;
            FacingPlayer = &Player;
        }
    }

    if (FacingPlayer)
    {
        const FVector2D ToPlayer(FacingPlayer->X - Enemy.X, FacingPlayer->Y - Enemy.Y);
        if (ToPlayer.SizeSquared() > 1.0f)
        {
            TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X));
        }
    }

    FCWSkeletalVisualTarget Target;
    Target.Location = TargetLocation;
    Target.Rotation = FRotator(0.0f, TargetYaw, 0.0f);
    Target.Scale = FVector(MeshScale);
    Target.bSnap = !bHasPreviousLocation || FVector::DistSquared2D(Component->GetComponentLocation(), TargetLocation) > FMath::Square(FMath::Max(900.0f, VisualRadius * 16.0f));
    EnemySkeletalTargets.Add(Enemy.Id, Target);

    if (Target.bSnap)
    {
        Component->SetWorldLocationAndRotation(Target.Location, Target.Rotation);
        Component->SetWorldScale3D(Target.Scale);
    }
    Component->SetVisibility(true, true);
    return true;
}

void ACWNativeWorldActor::RetireMissingEnemySkeletalVisuals(const FCWRoomSnapshot& State)
{
    TSet<FString> LiveEnemyIds;
    for (const FCWEnemySnapshot& Enemy : State.Enemies)
    {
        if (!Enemy.Id.IsEmpty())
        {
            LiveEnemyIds.Add(Enemy.Id);
        }
    }

    TMap<FString, FCWEnemySnapshot> PreviousEnemiesById;
    if (bHasPreviousState)
    {
        for (const FCWEnemySnapshot& Enemy : PreviousState.Enemies)
        {
            if (!Enemy.Id.IsEmpty())
            {
                PreviousEnemiesById.Add(Enemy.Id, Enemy);
            }
        }
    }

    TArray<FString> RetiredIds;
    for (const TPair<FString, USkeletalMeshComponent*>& Pair : EnemySkeletalComponents)
    {
        if (!LiveEnemyIds.Contains(Pair.Key))
        {
            RetiredIds.Add(Pair.Key);
        }
    }

    for (const FString& EnemyId : RetiredIds)
    {
        USkeletalMeshComponent* Component = nullptr;
        EnemySkeletalComponents.RemoveAndCopyValue(EnemyId, Component);
        EnemySkeletalMobIds.Remove(EnemyId);
        EnemyPreviousLocations.Remove(EnemyId);
        EnemyPreviousSampleTimes.Remove(EnemyId);
        EnemySkeletalTargets.Remove(EnemyId);

        if (const FCWEnemySnapshot* PreviousEnemy = PreviousEnemiesById.Find(EnemyId))
        {
            SpawnEnemyDeathVisual(*PreviousEnemy, Component);
        }
        else if (Component)
        {
            Component->DestroyComponent();
        }
    }
}

void ACWNativeWorldActor::SpawnEnemyDeathVisual(const FCWEnemySnapshot& Enemy, USkeletalMeshComponent* ExistingComponent)
{
    if (!Enemy.Id.IsEmpty() && SeenEnemyDeathVisualIds.Contains(Enemy.Id))
    {
        if (ExistingComponent)
        {
            ExistingComponent->DestroyComponent();
        }
        return;
    }

    const FString MobId = ResolveEnemyMobId(Enemy);
    if (!ExistingComponent)
    {
        USkeletalMesh* Mesh = LoadEnemySkeletalMesh(MobId);
        if (!Mesh)
        {
            return;
        }

        ExistingComponent = NewObject<USkeletalMeshComponent>(this);
        if (!ExistingComponent)
        {
            return;
        }

        ExistingComponent->SetSkeletalMesh(Mesh);
        ExistingComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ExistingComponent->SetMobility(EComponentMobility::Movable);
        ExistingComponent->SetCastShadow(true);
        ExistingComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        ExistingComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        ExistingComponent->bPauseAnims = false;
        ExistingComponent->GlobalAnimRateScale = 1.0f;
        ExistingComponent->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        ExistingComponent->RegisterComponent();
        AddInstanceComponent(ExistingComponent);
    }

    if (!Enemy.Id.IsEmpty())
    {
        SeenEnemyDeathVisualIds.Add(Enemy.Id);
        if (SeenEnemyDeathVisualIds.Num() > 1024)
        {
            SeenEnemyDeathVisualIds.Reset();
        }
    }

    if (bUseImportedEnemyMaterials)
    {
        ApplyReadableEnemyMaterials(ExistingComponent, MobId);
    }

    const FBoxSphereBounds MeshBounds = ExistingComponent->GetLocalBounds();
    const FVector BoundsMin = MeshBounds.Origin - MeshBounds.BoxExtent;
    const FVector BoundsMax = MeshBounds.Origin + MeshBounds.BoxExtent;
    const float MeshHeight = FMath::Max(1.0f, BoundsMax.Z - BoundsMin.Z);
    const float VisualRadius = FMath::Max(8.0f, Enemy.Radius);
    const float VisualScale = FMath::Clamp(Enemy.SpriteScale <= KINDA_SMALL_NUMBER ? 1.0f : Enemy.SpriteScale, 0.65f, 2.6f);
    const float TargetHeight = FMath::Clamp(VisualRadius * 9.0f * VisualScale * SkeletalVisualScale, 65.0f, 280.0f);
    const float MeshScale = TargetHeight / MeshHeight;

    ExistingComponent->SetWorldLocationAndRotation(
        FVector(Enemy.X, Enemy.Y, GroundZ - BoundsMin.Z * MeshScale),
        FRotator(0.0f, Enemy.bFaceLeft ? 180.0f : 0.0f, 0.0f));
    ExistingComponent->SetWorldScale3D(FVector(MeshScale));

    UPhysicsAsset* PhysicsAsset = ExistingComponent->GetPhysicsAsset();
    if (!PhysicsAsset)
    {
        const FString PreferredPhysicsPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_death_animfix_PhysicsAsset.SK_%s_walk_death_animfix_PhysicsAsset"), *MobId, *MobId, *MobId);
        const FString FallbackPhysicsPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_death_PhysicsAsset.SK_%s_walk_death_PhysicsAsset"), *MobId, *MobId, *MobId);
        PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, *PreferredPhysicsPath);
        if (!PhysicsAsset)
        {
            PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, *FallbackPhysicsPath);
        }
        if (PhysicsAsset)
        {
            ExistingComponent->SetPhysicsAsset(PhysicsAsset);
        }
    }

    const bool bCanUseRagdoll = ExistingComponent->GetSkeletalMeshAsset() && PhysicsAsset;
    if (bCanUseRagdoll)
    {
        FVector2D HitDirection(Enemy.bFaceLeft ? -1.0f : 1.0f, 0.0f);
        float BestPlayerDistSq = TNumericLimits<float>::Max();
        if (bHasPreviousState)
        {
            for (const FCWPlayerSnapshot& Player : PreviousState.Players)
            {
                const FVector2D Candidate(Enemy.X - Player.X, Enemy.Y - Player.Y);
                const float DistSq = Candidate.SizeSquared();
                if (DistSq > 1.0f && DistSq < BestPlayerDistSq)
                {
                    BestPlayerDistSq = DistSq;
                    HitDirection = Candidate.GetSafeNormal();
                }
            }
        }

        ExistingComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        ExistingComponent->Stop();
        ExistingComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ExistingComponent->SetCollisionObjectType(ECC_PhysicsBody);
        ExistingComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
        ExistingComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        ExistingComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        ExistingComponent->SetGenerateOverlapEvents(false);
        ExistingComponent->SetAllBodiesSimulatePhysics(true);
        ExistingComponent->SetEnablePhysicsBlending(true);
        ExistingComponent->SetAllBodiesPhysicsBlendWeight(0.92f);
        ExistingComponent->SetSimulatePhysics(true);
        ExistingComponent->SetAllPhysicsLinearVelocity(FVector::ZeroVector, false);
        ExistingComponent->SetLinearDamping(1.65f);
        ExistingComponent->SetAngularDamping(4.25f);
        ExistingComponent->SetMaxDepenetrationVelocity(NAME_None, 260.0f);
        ExistingComponent->SetAllAllowPartialIslandSleep(true);
        ExistingComponent->WakeAllRigidBodies();

        const float RagdollRadius = FMath::Max(Enemy.Radius, 16.0f);
        const float HitHeightSeed = StableUnitFromString(Enemy.Id + TEXT("_ragdoll_hit_height"));
        const float HitSideSeed = StableUnitFromString(Enemy.Id + TEXT("_ragdoll_hit_side"));
        const float HitPowerSeed = StableUnitFromString(Enemy.Id + TEXT("_ragdoll_hit_power"));
        const float TwistSeed = StableUnitFromString(Enemy.Id + TEXT("_ragdoll_hit_twist"));
        const FVector2D SideDirection(-HitDirection.Y, HitDirection.X);
        const float HitHeightAlpha = FMath::Lerp(0.22f, 0.92f, HitHeightSeed);
        const float HitSideOffset = FMath::Lerp(-0.85f, 0.85f, HitSideSeed) * RagdollRadius;
        const float HitFrontOffset = FMath::Lerp(0.32f, 0.68f, StableUnitFromString(Enemy.Id + TEXT("_ragdoll_hit_depth"))) * RagdollRadius;
        const FVector ImpactLocation(
            Enemy.X - HitDirection.X * HitFrontOffset + SideDirection.X * HitSideOffset,
            Enemy.Y - HitDirection.Y * HitFrontOffset + SideDirection.Y * HitSideOffset,
            GroundZ + TargetHeight * HitHeightAlpha);

        const float LowHitKick = FMath::GetMappedRangeValueClamped(FVector2D(0.22f, 0.92f), FVector2D(1.10f, 0.86f), HitHeightAlpha);
        const float ImpulseStrength = FMath::Clamp(RagdollRadius * FMath::Lerp(8.0f, 14.0f, HitPowerSeed) * LowHitKick, 180.0f, 1250.0f);
        const FVector Impulse(
            HitDirection.X * ImpulseStrength,
            HitDirection.Y * ImpulseStrength,
            ImpulseStrength * FMath::Lerp(0.055f, 0.20f, HitHeightSeed));
        ExistingComponent->AddImpulseAtLocation(Impulse, ImpactLocation);

        const float SideSign = HitSideOffset < 0.0f ? -1.0f : 1.0f;
        const FVector AngularImpulse(
            SideSign * FMath::Lerp(240.0f, 820.0f, HitHeightSeed),
            (Enemy.bFaceLeft ? -1.0f : 1.0f) * FMath::Lerp(180.0f, 720.0f, HitPowerSeed),
            FMath::Lerp(-860.0f, 860.0f, TwistSeed) + HitSideOffset * 8.0f);
        ExistingComponent->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
        SpawnBloodHitFx(ImpactLocation, FVector(HitDirection.X, HitDirection.Y, 0.0f), FMath::Lerp(1.0f, 1.8f, HitPowerSeed), RagdollRadius);

        FCWActiveEnemyDeathVisual DeathVisual;
        DeathVisual.Component = ExistingComponent;
        DeathVisual.Age = 0.0f;
        DeathVisual.Life = 8.0f;
        DeathVisual.bRagdoll = true;
        DeathVisual.SinkDepth = FMath::Clamp(TargetHeight * 0.58f, 70.0f, 180.0f);
        DeathVisual.SinkDuration = FMath::Clamp(1.25f + RagdollRadius * 0.012f, 1.25f, 2.2f);
        ActiveEnemyDeathVisuals.Add(DeathVisual);
        return;
    }

    ExistingComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExistingComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    ExistingComponent->Stop();

    FCWActiveEnemyDeathVisual DeathVisual;
    DeathVisual.Component = ExistingComponent;
    DeathVisual.Age = 0.0f;
    DeathVisual.Life = 6.5f;
    DeathVisual.bKinematicFall = true;
    DeathVisual.FallStartLocation = ExistingComponent->GetComponentLocation();
    DeathVisual.FallStartRotation = ExistingComponent->GetComponentRotation();
    const float FallSide = StableUnitFromString(Enemy.Id + TEXT("_fall_side")) < 0.5f ? -1.0f : 1.0f;
    DeathVisual.FallEndRotation = FRotator(-88.0f, ExistingComponent->GetComponentRotation().Yaw, FallSide * FMath::Lerp(4.0f, 15.0f, StableUnitFromString(Enemy.Id + TEXT("_fall_roll"))));
    DeathVisual.FallDuration = FMath::Clamp(0.58f + VisualRadius * 0.0035f, 0.58f, 0.9f);
    DeathVisual.LieDuration = FMath::Clamp(2.0f + VisualRadius * 0.014f, 2.0f, 3.4f);
    DeathVisual.SinkDepth = FMath::Clamp(TargetHeight * 0.58f, 70.0f, 180.0f);
    DeathVisual.SinkDuration = FMath::Clamp(1.25f + VisualRadius * 0.012f, 1.25f, 2.2f);
    ActiveEnemyDeathVisuals.Add(DeathVisual);
}

void ACWNativeWorldActor::DestroyEnemySkeletalVisuals()
{
    for (TPair<FString, USkeletalMeshComponent*>& Pair : EnemySkeletalComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->DestroyComponent();
        }
    }
    EnemySkeletalComponents.Reset();
    EnemySkeletalMobIds.Reset();
    EnemyPreviousLocations.Reset();
    EnemyPreviousSampleTimes.Reset();
    EnemySkeletalTargets.Reset();

    for (FCWActiveEnemyDeathVisual& Visual : ActiveEnemyDeathVisuals)
    {
        if (Visual.Component)
        {
            Visual.Component->DestroyComponent();
        }
    }
    ActiveEnemyDeathVisuals.Reset();
    SeenEnemyDeathVisualIds.Reset();
}

FString ACWNativeWorldActor::ResolveEnemyMobId(const FCWEnemySnapshot& Enemy) const
{
    FString MobId = Enemy.MobId.IsEmpty() ? Enemy.Type : Enemy.MobId;
    MobId = MobId.TrimStartAndEnd().ToLower();
    MobId.ReplaceInline(TEXT("-"), TEXT("_"));
    MobId.ReplaceInline(TEXT(" "), TEXT("_"));

    if (MobId.IsEmpty())
    {
        return TEXT("normal");
    }
    if (MobId == TEXT("medis"))
    {
        return TEXT("medic");
    }
    if (MobId.Contains(TEXT("chief")) || MobId.Contains(TEXT("surgeon")) || MobId.Contains(TEXT("hazmat")))
    {
        return TEXT("boss_chief_surgeon");
    }
    if (MobId.Contains(TEXT("hellmart")) || MobId.Contains(TEXT("hell_mart")))
    {
        return TEXT("boss_hellmart");
    }
    if (MobId.Contains(TEXT("reactor")) || MobId.Contains(TEXT("apostle")) || MobId.Contains(TEXT("biohazard")))
    {
        return TEXT("boss_reactor_apostle");
    }
    if (MobId.Contains(TEXT("road")) || MobId.Contains(TEXT("titan")))
    {
        return TEXT("boss_road_titan");
    }
    if (MobId.StartsWith(TEXT("boss_")))
    {
        return MobId;
    }
    if (MobId.Contains(TEXT("boss")))
    {
        return TEXT("boss");
    }
    return MobId;
}

USkeletalMesh* ACWNativeWorldActor::LoadEnemySkeletalMesh(const FString& MobId)
{
    if (USkeletalMesh* const* Cached = EnemySkeletalMeshCache.Find(MobId))
    {
        return *Cached;
    }

    const FString PreferredPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_death_animfix.SK_%s_walk_death_animfix"), *MobId, *MobId, *MobId);
    const FString FallbackPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_death.SK_%s_walk_death"), *MobId, *MobId, *MobId);
    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *PreferredPath);
    const FString Path = Mesh ? PreferredPath : FallbackPath;
    if (!Mesh)
    {
        Mesh = LoadObject<USkeletalMesh>(nullptr, *FallbackPath);
    }
    EnemySkeletalMeshCache.Add(MobId, Mesh);

    if (!Mesh)
    {
        const FString WarningKey = FString::Printf(TEXT("mesh:%s"), *MobId);
        if (!MissingEnemyAssetWarnings.Contains(WarningKey))
        {
            MissingEnemyAssetWarnings.Add(WarningKey);
            UE_LOG(LogTemp, Warning, TEXT("Missing Tripo enemy skeletal mesh: %s"), *Path);
        }
    }
    return Mesh;
}

UAnimSequence* ACWNativeWorldActor::LoadEnemyIdleAnimation(const FString& MobId)
{
    if (UAnimSequence* const* Cached = EnemyIdleAnimCache.Find(MobId))
    {
        return *Cached;
    }

    UAnimSequence* Anim = LoadFirstAnimSequencePath({
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/Anims/A_%s_Idle.A_%s_Idle"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/Anims/A_%s_idle.A_%s_idle"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_Idle.SK_%s_Idle"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_idle.SK_%s_idle"), *MobId, *MobId, *MobId)
    });
    EnemyIdleAnimCache.Add(MobId, Anim);
    return Anim;
}

UAnimSequence* ACWNativeWorldActor::LoadEnemyWalkAnimation(const FString& MobId)
{
    if (UAnimSequence* const* Cached = EnemyWalkAnimCache.Find(MobId))
    {
        return *Cached;
    }

    const FString PreferredPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_death_animfixSK_%s_M_%s_Walk.SK_%s_walk_death_animfixSK_%s_M_%s_Walk"), *MobId, *MobId, *MobId, *MobId, *MobId, *MobId, *MobId);
    const FString FallbackPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_deathSK_%s_M_%s_Walk.SK_%s_walk_deathSK_%s_M_%s_Walk"), *MobId, *MobId, *MobId, *MobId, *MobId, *MobId, *MobId);
    UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *PreferredPath);
    const FString Path = Anim ? PreferredPath : FallbackPath;
    if (!Anim)
    {
        Anim = LoadObject<UAnimSequence>(nullptr, *FallbackPath);
    }
    EnemyWalkAnimCache.Add(MobId, Anim);

    if (!Anim)
    {
        const FString WarningKey = FString::Printf(TEXT("walk:%s"), *MobId);
        if (!MissingEnemyAssetWarnings.Contains(WarningKey))
        {
            MissingEnemyAssetWarnings.Add(WarningKey);
            UE_LOG(LogTemp, Warning, TEXT("Missing Tripo enemy walk animation: %s"), *Path);
        }
    }
    return Anim;
}

UAnimSequence* ACWNativeWorldActor::LoadEnemyAttackAnimation(const FString& MobId)
{
    if (UAnimSequence* const* Cached = EnemyAttackAnimCache.Find(MobId))
    {
        return *Cached;
    }

    UAnimSequence* Anim = LoadFirstAnimSequencePath({
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/Anims/A_%s_Attack.A_%s_Attack"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/Anims/A_%s_Attack_01.A_%s_Attack_01"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/Anims/A_%s_Melee.A_%s_Melee"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_Attack.SK_%s_Attack"), *MobId, *MobId, *MobId),
        FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_Melee.SK_%s_Melee"), *MobId, *MobId, *MobId)
    });
    EnemyAttackAnimCache.Add(MobId, Anim);
    return Anim;
}

UAnimSequence* ACWNativeWorldActor::LoadEnemyDeathAnimation(const FString& MobId)
{
    if (UAnimSequence* const* Cached = EnemyDeathAnimCache.Find(MobId))
    {
        return *Cached;
    }

    const FString PreferredPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_death_animfixSK_%s_M_%s_Death.SK_%s_walk_death_animfixSK_%s_M_%s_Death"), *MobId, *MobId, *MobId, *MobId, *MobId, *MobId, *MobId);
    const FString FallbackPath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/SK_%s_walk_deathSK_%s_M_%s_Death.SK_%s_walk_deathSK_%s_M_%s_Death"), *MobId, *MobId, *MobId, *MobId, *MobId, *MobId, *MobId);
    UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *PreferredPath);
    const FString Path = Anim ? PreferredPath : FallbackPath;
    if (!Anim)
    {
        Anim = LoadObject<UAnimSequence>(nullptr, *FallbackPath);
    }
    EnemyDeathAnimCache.Add(MobId, Anim);

    if (!Anim)
    {
        const FString WarningKey = FString::Printf(TEXT("death:%s"), *MobId);
        if (!MissingEnemyAssetWarnings.Contains(WarningKey))
        {
            MissingEnemyAssetWarnings.Add(WarningKey);
            UE_LOG(LogTemp, Warning, TEXT("Missing Tripo enemy death animation: %s"), *Path);
        }
    }
    return Anim;
}

void ACWNativeWorldActor::UpdateSkeletalVisualTargets(float DeltaSeconds)
{
    if (DeltaSeconds <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    auto UpdateSet = [DeltaSeconds](TMap<FString, USkeletalMeshComponent*>& Components, TMap<FString, FCWSkeletalVisualTarget>& Targets)
    {
        for (TPair<FString, USkeletalMeshComponent*>& Pair : Components)
        {
            USkeletalMeshComponent* Component = Pair.Value;
            FCWSkeletalVisualTarget* Target = Targets.Find(Pair.Key);
            if (!Component || !Target)
            {
                continue;
            }

            if (Target->bSnap)
            {
                Component->SetWorldLocationAndRotation(Target->Location, Target->Rotation);
                Component->SetWorldScale3D(Target->Scale);
                Target->bSnap = false;
                continue;
            }

            const FVector CurrentLocation = Component->GetComponentLocation();
            const float DistanceSq = FVector::DistSquared2D(CurrentLocation, Target->Location);
            const FVector NewLocation = DistanceSq > FMath::Square(1400.0f)
                ? Target->Location
                : FMath::VInterpTo(CurrentLocation, Target->Location, DeltaSeconds, 10.0f);
            const FRotator NewRotation = FMath::RInterpTo(Component->GetComponentRotation(), Target->Rotation, DeltaSeconds, 9.0f);
            const FVector NewScale = FMath::VInterpTo(Component->GetComponentScale(), Target->Scale, DeltaSeconds, 8.0f);

            Component->SetWorldLocationAndRotation(NewLocation, NewRotation);
            Component->SetWorldScale3D(NewScale);
        }
    };

    UpdateSet(PlayerSkeletalComponents, PlayerSkeletalTargets);
    UpdateSet(EnemySkeletalComponents, EnemySkeletalTargets);
}

FVector2D ACWNativeWorldActor::GetSmoothedPickupPosition(TMap<FString, FVector2D>& PositionCache, const FString& Key, const FVector2D& Target, float DeltaSeconds, float InterpSpeed, float SnapDistance)
{
    FVector2D* Existing = PositionCache.Find(Key);
    if (!Existing || DeltaSeconds <= KINDA_SMALL_NUMBER)
    {
        PositionCache.Add(Key, Target);
        return Target;
    }

    if (FVector2D::DistSquared(*Existing, Target) > FMath::Square(FMath::Max(1.0f, SnapDistance)))
    {
        *Existing = Target;
    }
    else
    {
        const FVector NewPosition = FMath::VInterpTo(FVector(Existing->X, Existing->Y, 0.0f), FVector(Target.X, Target.Y, 0.0f), DeltaSeconds, FMath::Max(0.1f, InterpSpeed));
        *Existing = FVector2D(NewPosition.X, NewPosition.Y);
    }
    return *Existing;
}

void ACWNativeWorldActor::RebuildPickupInstances(const FCWRoomSnapshot& State, float DeltaSeconds)
{
    if (DropInstances)
    {
        DropInstances->ClearInstances();
    }
    if (XpVacuumInstances)
    {
        XpVacuumInstances->ClearInstances();
    }
    if (XpOrbInstances)
    {
        XpOrbInstances->ClearInstances();
    }
    if (XpOrbHaloInstances)
    {
        XpOrbHaloInstances->ClearInstances();
    }
    if (XpOrbSparkInstances)
    {
        XpOrbSparkInstances->ClearInstances();
    }
    if (XpVacuumHaloInstances)
    {
        XpVacuumHaloInstances->ClearInstances();
    }
    if (XpVacuumSparkInstances)
    {
        XpVacuumSparkInstances->ClearInstances();
    }

    const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    UStaticMesh* XpCrystalMesh = LoadXpCrystalStaticMesh();
    const bool bUseXpCrystalMesh = XpCrystalMesh
        && XpOrbInstances
        && XpVacuumInstances
        && XpOrbInstances->GetStaticMesh() == XpCrystalMesh
        && XpVacuumInstances->GetStaticMesh() == XpCrystalMesh;
    TSet<FString> LiveDrops;
    TSet<FString> LiveXpOrbs;
    TSet<FString> LiveVacuumDrops;

    auto MakePickupComponent = [&](UStaticMesh* Mesh, UMaterialInterface* Material, const FString& DebugName) -> UStaticMeshComponent*
    {
        if (!Mesh || !Material)
        {
            return nullptr;
        }

        UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, NAME_None);
        if (!Component)
        {
            return nullptr;
        }
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(false);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetMaterial(0, Material);
        Component->ComponentTags.Add(FName(*DebugName));
        Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        Component->RegisterComponent();
        AddInstanceComponent(Component);
        return Component;
    };

    int32 DropIndex = 0;
    for (const FCWPickupSnapshot& Drop : State.Drops)
    {
        const bool bVacuum = Drop.Kind.Equals(TEXT("xp_vacuum"), ESearchCase::IgnoreCase);
        const FString Key = Drop.Id.IsEmpty()
            ? FString::Printf(TEXT("%s_%d"), bVacuum ? TEXT("vacuum") : TEXT("drop"), DropIndex)
            : Drop.Id;
        ++DropIndex;

        const FVector2D Position = GetSmoothedPickupPosition(bVacuum ? XpVacuumRenderPositions : DropRenderPositions, Key, FVector2D(Drop.X, Drop.Y), DeltaSeconds);
        if (bVacuum)
        {
            LiveVacuumDrops.Add(Key);
            const float DropSeed = StableUnitFromString(Key);
            const float Phase = DropSeed * TWO_PI;
            const float PulseYaw = TimeSeconds * FMath::Lerp(52.0f, 74.0f, DropSeed) + DropSeed * 360.0f;
            const float AuraPulse = 1.0f + FMath::Sin(TimeSeconds * 2.8f + Phase) * 0.075f;
            const float Bob = FMath::Sin(TimeSeconds * 2.15f + Phase) * 4.2f;
            const FVector CrystalLocation(Position.X, Position.Y, GroundZ + 42.0f + Bob);
            if (XpVacuumInstances)
            {
                if (bUseXpCrystalMesh)
                {
                    const float TargetHeight = 68.0f;
                    const float CrystalScale = StaticMeshScaleForHeight(XpCrystalMesh, TargetHeight, 0.13f);
                    XpVacuumInstances->AddInstance(FTransform(
                        FRotator(10.0f + FMath::Sin(TimeSeconds * 1.25f + Phase) * 3.0f, PulseYaw, -8.0f),
                        FVector(Position.X, Position.Y, GroundZ + TargetHeight * 0.52f + Bob),
                        FVector(CrystalScale)));
                }
                else
                {
                    XpVacuumInstances->AddInstance(FTransform(FRotator(45.0f, PulseYaw, 45.0f), CrystalLocation, FVector(0.112f, 0.112f, 0.30f)));
                    XpVacuumInstances->AddInstance(FTransform(FRotator(-32.0f, PulseYaw + 71.0f, 28.0f), CrystalLocation + FVector(9.0f, -6.0f, -4.0f), FVector(0.044f, 0.044f, 0.12f)));
                    XpVacuumInstances->AddInstance(FTransform(FRotator(24.0f, PulseYaw - 57.0f, -34.0f), CrystalLocation + FVector(-9.0f, 7.0f, -5.0f), FVector(0.038f, 0.038f, 0.11f)));
                }
            }
            if (XpVacuumSparkInstances)
            {
                for (int32 Spark = 0; Spark < 10; ++Spark)
                {
                    const float SparkSeed = StableUnitFromString(FString::Printf(TEXT("%s_vacuum_spark_%d"), *Key, Spark));
                    const float Angle = TimeSeconds * (1.35f + SparkSeed * 0.72f) + Phase + Spark * TWO_PI / 10.0f;
                    const float Orbit = FMath::Lerp(32.0f, 52.0f, SparkSeed);
                    const float SparkBob = FMath::Sin(TimeSeconds * 3.6f + SparkSeed * 8.0f + Phase) * 7.0f;
                    const float SparkSize = FMath::Lerp(0.009f, 0.021f, SparkSeed) * AuraPulse;
                    XpVacuumSparkInstances->AddInstance(FTransform(
                        FRotator::ZeroRotator,
                        FVector(Position.X + FMath::Cos(Angle) * Orbit, Position.Y + FMath::Sin(Angle) * Orbit, GroundZ + 44.0f + SparkBob),
                        FVector(SparkSize)));
                }
            }
        }
        else
        {
            LiveDrops.Add(Key);
            const FString WeaponKey = NormalizeWeaponPickupKey(Drop.ItemKey);
            const FLinearColor Accent = WeaponPickupAccentColor(WeaponKey);
            const float Seed = StableUnitFromString(Key + WeaponKey);
            const float Phase = Seed * TWO_PI;
            const float Pulse = 1.0f + FMath::Sin(TimeSeconds * 2.5f + Phase) * 0.055f;
            const float Bob = FMath::Sin(TimeSeconds * 2.0f + Phase) * 3.2f;

            FCWPickupWorldVisual* Visual = WeaponPickupWorldVisuals.Find(Key);
            const bool bNeedsIconRefresh = Visual && Visual->IconKey != WeaponKey;
            if (!Visual)
            {
                FCWPickupWorldVisual NewVisual;
                WeaponPickupWorldVisuals.Add(Key, NewVisual);
                Visual = WeaponPickupWorldVisuals.Find(Key);
            }
            if (!Visual)
            {
                continue;
            }
            if (bNeedsIconRefresh)
            {
                if (Visual->Icon)
                {
                    Visual->Icon->DestroyComponent();
                    Visual->Icon = nullptr;
                }
                Visual->IconKey.Reset();
            }
            if (!Visual->Aura)
            {
                Visual->Aura = MakePickupComponent(
                    SphereMeshAsset ? SphereMeshAsset : CubeMeshAsset,
                    MakeFxMaterial(TEXT("WeaponPickupAura_") + Key, FLinearColor(Accent.R, Accent.G, Accent.B, 0.32f), 1.7f),
                    TEXT("WeaponPickupAura"));
            }
            if (!Visual->Icon)
            {
                UTexture2D* WeaponTexture = LoadWeaponPickupTexture(WeaponKey);
                UMaterialInterface* IconMaterial = GetSkillIconMaterial(TEXT("weapon_") + WeaponKey, WeaponTexture, Accent);
                Visual->Icon = MakePickupComponent(
                    PlaneMeshAsset ? PlaneMeshAsset : CubeMeshAsset,
                    IconMaterial ? IconMaterial : MakeFxMaterial(TEXT("WeaponPickupIconFallback_") + Key, FLinearColor(Accent.R, Accent.G, Accent.B, 0.92f), 1.5f),
                    TEXT("WeaponPickupIcon"));
                Visual->IconKey = WeaponKey;
            }
            if (!Visual->Label)
            {
                Visual->Label = MakePickupLabelComponent(TEXT("WeaponPickupLabel"));
            }

            if (Visual->Aura)
            {
                Visual->Aura->SetWorldLocation(FVector(Position.X, Position.Y, GroundZ + 38.0f + Bob * 0.25f));
                Visual->Aura->SetWorldRotation(FRotator::ZeroRotator);
                Visual->Aura->SetWorldScale3D(FVector(0.28f * Pulse, 0.28f * Pulse, 0.045f * Pulse));
            }
            if (Visual->Icon)
            {
                Visual->Icon->SetWorldLocation(FVector(Position.X, Position.Y, GroundZ + 58.0f + Bob));
                Visual->Icon->SetWorldRotation(FRotator::ZeroRotator);
                Visual->Icon->SetWorldScale3D(FVector(0.44f * Pulse, 0.44f * Pulse, 0.44f * Pulse));
            }
            UpdatePickupLabel(Visual->Label, WeaponPickupLabel(Drop.ItemKey), FVector(Position.X, Position.Y - 46.0f, GroundZ + 96.0f + Bob), Accent, 13.0f);
        }
    }

    int32 OrbIndex = 0;
    for (const FCWPickupSnapshot& Orb : State.XpOrbs)
    {
        const FString Key = Orb.Id.IsEmpty() ? FString::Printf(TEXT("xp_%d"), OrbIndex) : Orb.Id;
        ++OrbIndex;
        LiveXpOrbs.Add(Key);
        const FVector2D Position = GetSmoothedPickupPosition(XpOrbRenderPositions, Key, FVector2D(Orb.X, Orb.Y), DeltaSeconds);
        const float OrbSeed = StableUnitFromString(Key);
        const float Phase = OrbSeed * TWO_PI;
        const float Spin = TimeSeconds * FMath::Lerp(64.0f, 92.0f, OrbSeed) + OrbSeed * 360.0f;
        const float Bob = FMath::Sin(TimeSeconds * 2.35f + Phase) * 4.0f;
        const float Size = FMath::Clamp(0.085f + Orb.Xp * 0.002f, 0.085f, 0.16f);
        const float AuraPulse = 1.0f + FMath::Sin(TimeSeconds * 2.9f + Phase * 1.7f) * 0.09f;
        if (XpOrbInstances)
        {
            if (bUseXpCrystalMesh)
            {
                const float SizeT = FMath::Clamp((Size - 0.085f) / 0.075f, 0.0f, 1.0f);
                const float TargetHeight = FMath::Lerp(34.0f, 56.0f, SizeT);
                const float CrystalScale = StaticMeshScaleForHeight(XpCrystalMesh, TargetHeight, Size * 1.18f);
                XpOrbInstances->AddInstance(FTransform(
                    FRotator(8.0f + FMath::Sin(TimeSeconds * 1.15f + Phase) * 2.8f, Spin, -7.0f),
                    FVector(Position.X, Position.Y, GroundZ + TargetHeight * 0.52f + Bob),
                    FVector(CrystalScale)));
            }
            else
            {
                XpOrbInstances->AddInstance(FTransform(FRotator(45.0f, Spin, 45.0f), FVector(Position.X, Position.Y, GroundZ + 25.0f + Bob), FVector(Size * 0.52f, Size * 0.52f, Size * 1.40f)));
            }
        }
        if (XpOrbSparkInstances)
        {
            const int32 SparkCount = FMath::Clamp(5 + FMath::RoundToInt(Orb.Xp * 0.08f), 5, 8);
            for (int32 Spark = 0; Spark < SparkCount; ++Spark)
            {
                const float SparkSeed = StableUnitFromString(FString::Printf(TEXT("%s_xp_spark_%d"), *Key, Spark));
                const float Angle = TimeSeconds * (1.55f + SparkSeed * 0.95f) + Phase + Spark * TWO_PI / FMath::Max(1, SparkCount);
                const float Orbit = FMath::Lerp(18.0f, 34.0f, SparkSeed) + Size * 48.0f;
                const float SparkSize = FMath::Lerp(0.007f, 0.018f, SparkSeed) * AuraPulse;
                XpOrbSparkInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    FVector(Position.X + FMath::Cos(Angle) * Orbit, Position.Y + FMath::Sin(Angle) * Orbit, GroundZ + 27.0f + Bob + FMath::Sin(TimeSeconds * 3.8f + Spark + Phase) * 5.0f),
                    FVector(SparkSize)));
                if (Spark < 5)
                {
                    const float GroundAngle = -TimeSeconds * (0.82f + SparkSeed * 0.28f) + Phase + Spark * TWO_PI / 5.0f;
                    const float GroundOrbit = FMath::Lerp(14.0f, 24.0f, SparkSeed) + Size * 34.0f;
                    XpOrbSparkInstances->AddInstance(FTransform(
                        FRotator::ZeroRotator,
                        FVector(Position.X + FMath::Cos(GroundAngle) * GroundOrbit, Position.Y + FMath::Sin(GroundAngle) * GroundOrbit, GroundZ + 7.0f + FMath::Sin(TimeSeconds * 2.9f + SparkSeed * 5.0f) * 2.5f),
                        FVector(SparkSize * 0.62f)));
                }
            }
        }
    }

    UpdateSkillOrbWorldVisuals(State, DeltaSeconds);

    auto PrunePositions = [](TMap<FString, FVector2D>& Positions, const TSet<FString>& LiveKeys)
    {
        TArray<FString> DeadKeys;
        for (const TPair<FString, FVector2D>& Pair : Positions)
        {
            if (!LiveKeys.Contains(Pair.Key))
            {
                DeadKeys.Add(Pair.Key);
            }
        }
        for (const FString& Key : DeadKeys)
        {
            Positions.Remove(Key);
        }
    };

    PrunePositions(DropRenderPositions, LiveDrops);
    PrunePositions(XpOrbRenderPositions, LiveXpOrbs);
    PrunePositions(XpVacuumRenderPositions, LiveVacuumDrops);

    TArray<FString> DeadWeaponVisualKeys;
    for (const TPair<FString, FCWPickupWorldVisual>& Pair : WeaponPickupWorldVisuals)
    {
        if (!LiveDrops.Contains(Pair.Key))
        {
            DeadWeaponVisualKeys.Add(Pair.Key);
        }
    }
    for (const FString& Key : DeadWeaponVisualKeys)
    {
        FCWPickupWorldVisual Visual;
        WeaponPickupWorldVisuals.RemoveAndCopyValue(Key, Visual);
        DestroyPickupWorldVisual(Visual);
    }
}

FLinearColor ACWNativeWorldActor::SkillOrbAccentColor(const FString& SkillId) const
{
    const FString Key = SkillId.ToLower();
    if (Key.Contains(TEXT("regen")) || Key.Contains(TEXT("heal")) || Key.Contains(TEXT("vital")) || Key.Contains(TEXT("shield")) || Key.Contains(TEXT("barrier")) || Key.Contains(TEXT("plating")))
    {
        return FLinearColor(0.22f, 0.92f, 0.56f, 1.0f);
    }
    if (Key.Contains(TEXT("lightning")) || Key.Contains(TEXT("arc")) || Key.Contains(TEXT("electric")) || Key.Contains(TEXT("storm")) || Key.Contains(TEXT("chain")))
    {
        return FLinearColor(0.20f, 0.88f, 1.0f, 1.0f);
    }
    if (Key.Contains(TEXT("laser")) || Key.Contains(TEXT("void")) || Key.Contains(TEXT("shadow")) || Key.Contains(TEXT("night")) || Key.Contains(TEXT("eclipse")))
    {
        return FLinearColor(0.74f, 0.42f, 1.0f, 1.0f);
    }
    if (Key.Contains(TEXT("rocket")) || Key.Contains(TEXT("missile")) || Key.Contains(TEXT("barrage")) || Key.Contains(TEXT("stomp")) || Key.Contains(TEXT("shrapnel")))
    {
        return FLinearColor(1.0f, 0.48f, 0.16f, 1.0f);
    }
    if (Key.Contains(TEXT("blood")) || Key.Contains(TEXT("rage")) || Key.Contains(TEXT("blade")) || Key.Contains(TEXT("slash")) || Key.Contains(TEXT("fang")))
    {
        return FLinearColor(1.0f, 0.34f, 0.50f, 1.0f);
    }
    if (Key.Contains(TEXT("haste")) || Key.Contains(TEXT("speed")) || Key.Contains(TEXT("reload")) || Key.Contains(TEXT("stride")) || Key.Contains(TEXT("drink")))
    {
        return FLinearColor(1.0f, 0.78f, 0.12f, 1.0f);
    }
    if (Key.Contains(TEXT("magnet")) || Key.Contains(TEXT("gps")) || Key.Contains(TEXT("seeker")) || Key.Contains(TEXT("homing")) || Key.Contains(TEXT("mark")) || Key.Contains(TEXT("sync")) || Key.Contains(TEXT("buddy")) || Key.Contains(TEXT("drone")))
    {
        return FLinearColor(0.36f, 0.64f, 1.0f, 1.0f);
    }
    return FLinearColor(0.22f, 0.90f, 1.0f, 1.0f);
}

FString ACWNativeWorldActor::ResolveSkillIconKey(const FString& SkillId) const
{
    FString Id = SkillId.ToLower();
    Id.TrimStartAndEndInline();
    if (Id.StartsWith(TEXT("native_")))
    {
        Id.RightChopInline(7);
    }
    if (Id.IsEmpty())
    {
        return FString();
    }

    if (Id == TEXT("shockwave")) return TEXT("raider_war_stomp");
    if (Id == TEXT("psi_blast")) return TEXT("shadow_void_burst");
    if (Id == TEXT("blade_orbit")) return TEXT("scout_razor_wind");
    if (Id == TEXT("chain_lightning")) return TEXT("cyber_arc_matrix");
    if (Id == TEXT("laser_strike")) return TEXT("cyber_ion_lance");
    if (Id == TEXT("homing_missiles")) return TEXT("cyber_seeker_protocol");
    if (Id == TEXT("weapon_mastery")) return TEXT("cyber_combat_firmware");
    if (Id == TEXT("rapid_reload")) return TEXT("shadow_action_reload");
    if (Id == TEXT("tactical_slap")) return TEXT("raider_anger_management");
    if (Id == TEXT("shilo_rm")) return TEXT("cyber_pulse_wave");
    if (Id == TEXT("bullet_gps")) return TEXT("scout_hunter_mark");
    if (Id == TEXT("vitality")) return TEXT("medic_vital_plating");
    if (Id == TEXT("haste")) return TEXT("scout_long_stride");
    if (Id == TEXT("magnetism")) return TEXT("scout_vital_sight");
    if (Id == TEXT("bloodlust")) return TEXT("raider_battle_rage");
    if (Id == TEXT("regeneration")) return TEXT("medic_field_aid");
    if (Id == TEXT("force_shield")) return TEXT("cyber_adaptive_frame");
    if (Id == TEXT("dodge_instinct")) return TEXT("shadow_ghost_step");
    if (Id == TEXT("pistol_buddy")) return TEXT("cyber_sync_link");
    if (Id == TEXT("smg_buddy")) return TEXT("cyber_pulse_wave");
    if (Id == TEXT("shotgun_buddy")) return TEXT("raider_shrapnel_burst");
    if (Id == TEXT("sniper_buddy")) return TEXT("scout_hunter_mark");
    if (Id == TEXT("pulse_wave")) return TEXT("cyber_pulse_wave");
    if (Id == TEXT("ion_lance")) return TEXT("cyber_ion_lance");
    if (Id == TEXT("arc_matrix")) return TEXT("cyber_arc_matrix");
    if (Id == TEXT("seeker_protocol")) return TEXT("cyber_seeker_protocol");
    if (Id == TEXT("adaptive_frame")) return TEXT("cyber_adaptive_frame");
    if (Id == TEXT("combat_firmware")) return TEXT("cyber_combat_firmware");
    if (Id == TEXT("sync_link")) return TEXT("cyber_sync_link");
    if (Id == TEXT("cache_goblin")) return TEXT("cyber_cache_goblin");
    if (Id == TEXT("razor_wind")) return TEXT("scout_razor_wind");
    if (Id == TEXT("hunter_mark")) return TEXT("scout_hunter_mark");
    if (Id == TEXT("storm_net")) return TEXT("scout_storm_net");
    if (Id == TEXT("sky_chasers")) return TEXT("scout_sky_chasers");
    if (Id == TEXT("long_stride")) return TEXT("scout_long_stride");
    if (Id == TEXT("vital_sight")) return TEXT("scout_vital_sight");
    if (Id == TEXT("trailblazer")) return TEXT("scout_trailblazer");
    if (Id == TEXT("energy_drink_iv")) return TEXT("scout_energy_drink_iv");
    if (Id == TEXT("void_burst")) return TEXT("shadow_void_burst");
    if (Id == TEXT("ghost_step")) return TEXT("shadow_ghost_step");
    if (Id == TEXT("black_comets")) return TEXT("shadow_black_comets");
    if (Id == TEXT("eclipse_chain")) return TEXT("shadow_eclipse_chain");
    if (Id == TEXT("assassin_instinct")) return TEXT("shadow_assassin_instinct");
    if (Id == TEXT("night_fangs")) return TEXT("shadow_night_fangs");
    if (Id == TEXT("action_reload")) return TEXT("shadow_action_reload");
    if (Id == TEXT("umbral_doctrine")) return TEXT("shadow_umbral_doctrine");
    if (Id == TEXT("triage_beam")) return TEXT("medic_triage_beam");
    if (Id == TEXT("sterile_wave")) return TEXT("medic_sterile_wave");
    if (Id == TEXT("toxin_arc")) return TEXT("medic_toxin_arc");
    if (Id == TEXT("rescue_rockets")) return TEXT("medic_rescue_rockets");
    if (Id == TEXT("field_aid")) return TEXT("medic_field_aid");
    if (Id == TEXT("vital_plating")) return TEXT("medic_vital_plating");
    if (Id == TEXT("support_protocol")) return TEXT("medic_support_protocol");
    if (Id == TEXT("oops_all_bandages")) return TEXT("medic_oops_all_bandages");
    if (Id == TEXT("war_stomp")) return TEXT("raider_war_stomp");
    if (Id == TEXT("shrapnel_burst")) return TEXT("raider_shrapnel_burst");
    if (Id == TEXT("berserk_arc")) return TEXT("raider_berserk_arc");
    if (Id == TEXT("siege_barrage")) return TEXT("raider_siege_barrage");
    if (Id == TEXT("battle_rage")) return TEXT("raider_battle_rage");
    if (Id == TEXT("iron_hide")) return TEXT("raider_iron_hide");
    if (Id == TEXT("war_banner")) return TEXT("raider_war_banner");
    if (Id == TEXT("anger_management")) return TEXT("raider_anger_management");
    if (Id.StartsWith(TEXT("cyber_")) || Id.StartsWith(TEXT("scout_")) || Id.StartsWith(TEXT("shadow_")) || Id.StartsWith(TEXT("medic_")) || Id.StartsWith(TEXT("raider_")))
    {
        return Id;
    }
    return Id;
}

UTexture2D* ACWNativeWorldActor::LoadSkillIconTexture(const FString& SkillId)
{
    const FString IconKey = ResolveSkillIconKey(SkillId);
    if (IconKey.IsEmpty())
    {
        return nullptr;
    }
    if (UTexture2D* const* Cached = SkillIconTextureCache.Find(IconKey))
    {
        return *Cached;
    }

    FString Root = AssetsRoot;
    if (Root.IsEmpty())
    {
        Root = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
    }

    UTexture2D* Texture = FCWNativeAssetLibrary::LoadTexture(this, Root, FString::Printf(TEXT("hero-skills/%s.png"), *IconKey));
    SkillIconTextureCache.Add(IconKey, Texture);
    return Texture;
}

FString ACWNativeWorldActor::ResolveSkillOfferLabel(const FString& SkillId, const FCWRoomSnapshot& State, const FCWPickupSnapshot& Orb) const
{
    const auto FindSkillName = [&](bool bRequireOwner) -> FString
    {
        for (const FCWPlayerSnapshot& Player : State.Players)
        {
            if (bRequireOwner && !Orb.OwnerId.IsEmpty() && Player.Id != Orb.OwnerId)
            {
                continue;
            }
            for (const FCWSkillSnapshot& Skill : Player.Skills)
            {
                if (Skill.Id.Equals(SkillId, ESearchCase::IgnoreCase))
                {
                    return Skill.Name.IsEmpty() ? PrettyLabelFromKey(Skill.Id) : Skill.Name;
                }
            }
        }
        return FString();
    };

    FString Label = Orb.OwnerId.IsEmpty() ? FString() : FindSkillName(true);
    if (Label.IsEmpty())
    {
        Label = FindSkillName(false);
    }
    return Label.IsEmpty() ? PrettyLabelFromKey(SkillId) : Label;
}

int32 ACWNativeWorldActor::ResolveSkillOfferLevel(const FString& SkillId, const FCWRoomSnapshot& State, const FCWPickupSnapshot& Orb) const
{
    int32 BestLevel = INDEX_NONE;
    for (const FCWPlayerSnapshot& Player : State.Players)
    {
        if (!Orb.OwnerId.IsEmpty() && Player.Id != Orb.OwnerId)
        {
            continue;
        }
        for (const FCWSkillSnapshot& Skill : Player.Skills)
        {
            if (Skill.Id.Equals(SkillId, ESearchCase::IgnoreCase))
            {
                BestLevel = FMath::Max(BestLevel, Skill.Level + 1);
            }
        }
    }
    if (BestLevel == INDEX_NONE && !Orb.OwnerId.IsEmpty())
    {
        for (const FCWPlayerSnapshot& Player : State.Players)
        {
            for (const FCWSkillSnapshot& Skill : Player.Skills)
            {
                if (Skill.Id.Equals(SkillId, ESearchCase::IgnoreCase))
                {
                    BestLevel = FMath::Max(BestLevel, Skill.Level + 1);
                }
            }
        }
    }
    return FMath::Max(1, BestLevel == INDEX_NONE ? 1 : BestLevel);
}

FString ACWNativeWorldActor::NormalizeWeaponPickupKey(const FString& WeaponKey) const
{
    const FString Key = WeaponKey.TrimStartAndEnd().ToLower();
    if (Key.Contains(TEXT("sniper")) || Key.Contains(TEXT("rifle")))
    {
        return TEXT("sniper");
    }
    if (Key.Contains(TEXT("shotgun")) || Key.Contains(TEXT("spread")))
    {
        return TEXT("shotgun");
    }
    if (Key.Contains(TEXT("smg")) || Key.Contains(TEXT("submachine")) || Key.Contains(TEXT("rapid")))
    {
        return TEXT("smg");
    }
    return TEXT("pistol");
}

FString ACWNativeWorldActor::WeaponPickupLabel(const FString& WeaponKey) const
{
    const FString Key = NormalizeWeaponPickupKey(WeaponKey);
    if (Key == TEXT("sniper")) return TEXT("Sniper");
    if (Key == TEXT("shotgun")) return TEXT("Shotgun");
    if (Key == TEXT("smg")) return TEXT("SMG");
    if (Key == TEXT("pistol")) return TEXT("Pistol");
    return PrettyLabelFromKey(WeaponKey);
}

FLinearColor ACWNativeWorldActor::WeaponPickupAccentColor(const FString& WeaponKey) const
{
    const FString Key = NormalizeWeaponPickupKey(WeaponKey);
    if (Key == TEXT("sniper")) return FLinearColor(0.72f, 0.92f, 1.0f, 1.0f);
    if (Key == TEXT("shotgun")) return FLinearColor(1.0f, 0.48f, 0.16f, 1.0f);
    if (Key == TEXT("smg")) return FLinearColor(0.18f, 0.92f, 1.0f, 1.0f);
    return FLinearColor(0.42f, 1.0f, 0.62f, 1.0f);
}

UTexture2D* ACWNativeWorldActor::LoadWeaponPickupTexture(const FString& WeaponKey)
{
    const FString Key = NormalizeWeaponPickupKey(WeaponKey);
    if (UTexture2D* const* Cached = WeaponPickupTextureCache.Find(Key))
    {
        return *Cached;
    }

    FString Root = AssetsRoot;
    if (Root.IsEmpty())
    {
        Root = TEXT("C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets");
    }

    UTexture2D* Texture = FCWNativeAssetLibrary::LoadTexture(this, Root, FString::Printf(TEXT("weapon-pickups/%s.png"), *Key));
    WeaponPickupTextureCache.Add(Key, Texture);
    return Texture;
}

UTextRenderComponent* ACWNativeWorldActor::MakePickupLabelComponent(const FString& DebugName)
{
    UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this, NAME_None);
    if (!Label)
    {
        return nullptr;
    }

    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCastShadow(false);
    Label->SetMobility(EComponentMobility::Movable);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetText(FText::GetEmpty());
    Label->SetTextRenderColor(FColor::White);
    Label->SetWorldSize(14.0f);
    Label->ComponentTags.Add(FName(*DebugName));
    Label->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
    Label->RegisterComponent();
    AddInstanceComponent(Label);
    return Label;
}

void ACWNativeWorldActor::UpdatePickupLabel(UTextRenderComponent* Label, const FString& Text, const FVector& Location, const FLinearColor& Color, float WorldSize)
{
    if (!Label)
    {
        return;
    }

    const FLinearColor LabelColor(
        FMath::Min(1.0f, Color.R + 0.20f),
        FMath::Min(1.0f, Color.G + 0.20f),
        FMath::Min(1.0f, Color.B + 0.20f),
        1.0f);
    Label->SetText(FText::FromString(Text));
    Label->SetTextRenderColor(LabelColor.ToFColor(true));
    Label->SetWorldLocation(Location);
    Label->SetWorldRotation(FRotator::ZeroRotator);
    Label->SetWorldSize(FMath::Max(8.0f, WorldSize));
    Label->SetVisibility(!Text.IsEmpty(), true);
}

void ACWNativeWorldActor::UpdateSkillOrbWorldVisuals(const FCWRoomSnapshot& State, float DeltaSeconds)
{
    TSet<FString> LiveSkillOrbKeys;
    LiveSkillOrbKeys.Reserve(State.SkillOrbs.Num());
    const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    auto MakeComponent = [&](UStaticMesh* Mesh, UMaterialInterface* Material, const FString& DebugName) -> UStaticMeshComponent*
    {
        if (!Mesh || !Material)
        {
            return nullptr;
        }

        UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, NAME_None);
        if (!Component)
        {
            return nullptr;
        }
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(false);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetMaterial(0, Material);
        Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        Component->RegisterComponent();
        AddInstanceComponent(Component);
        return Component;
    };

    int32 OrbIndex = 0;
    for (const FCWPickupSnapshot& Orb : State.SkillOrbs)
    {
        const FString SkillId = Orb.ItemKey.IsEmpty() ? TEXT("skill") : Orb.ItemKey;
        const FString Key = Orb.Id.IsEmpty()
            ? FString::Printf(TEXT("skill_%s_%d"), *SkillId, OrbIndex)
            : Orb.Id;
        ++OrbIndex;
        LiveSkillOrbKeys.Add(Key);

        const FVector2D Position = GetSmoothedPickupPosition(SkillOrbRenderPositions, Key, FVector2D(Orb.X, Orb.Y), DeltaSeconds, 12.0f, 1100.0f);
        const FLinearColor Accent = SkillOrbAccentColor(SkillId);
        const FString IconKey = ResolveSkillIconKey(SkillId);
        UTexture2D* IconTexture = LoadSkillIconTexture(SkillId);

        FCWSkillOrbWorldVisual* Visual = SkillOrbWorldVisuals.Find(Key);
        const bool bNeedsIconRefresh = Visual && Visual->IconKey != IconKey;
        const bool bNeedsCreateOrRefresh = !Visual || bNeedsIconRefresh || (Visual && !Visual->Icon && IconTexture);
        if (bNeedsCreateOrRefresh)
        {
            if (Visual && bNeedsIconRefresh)
            {
                if (Visual->Icon)
                {
                    Visual->Icon->DestroyComponent();
                    Visual->Icon = nullptr;
                }
                Visual->IconKey = FString();
            }
            if (!Visual)
            {
                FCWSkillOrbWorldVisual NewVisual;
                SkillOrbWorldVisuals.Add(Key, NewVisual);
                Visual = SkillOrbWorldVisuals.Find(Key);
            }
            if (!Visual)
            {
                continue;
            }

            const FString MaterialStemBase = IconKey.IsEmpty() ? TEXT("skill") : IconKey;
            const FString MaterialStem = FString::Printf(TEXT("%s_%08x"), *MaterialStemBase, GetTypeHash(Key));
            auto DestroyComponentRef = [](UStaticMeshComponent*& Component)
            {
                if (Component)
                {
                    Component->DestroyComponent();
                    Component = nullptr;
                }
            };
            DestroyComponentRef(Visual->Halo);
            DestroyComponentRef(Visual->RingA);
            DestroyComponentRef(Visual->RingB);
            if (!Visual->Shell)
            {
                Visual->Shell = MakeComponent(SphereMeshAsset ? SphereMeshAsset : CubeMeshAsset, MakeFxMaterial(TEXT("SkillOrbAura_") + MaterialStem, FLinearColor(Accent.R, Accent.G, Accent.B, 0.55f), 1.85f), TEXT("SkillOrbAura"));
            }
            if (!Visual->Icon)
            {
                UMaterialInterface* IconMaterial = GetSkillIconMaterial(IconKey, IconTexture, Accent);
                Visual->Icon = MakeComponent(PlaneMeshAsset ? PlaneMeshAsset : CubeMeshAsset, IconMaterial ? IconMaterial : MakeFxMaterial(TEXT("SkillOrbIconFallback_") + MaterialStem, FLinearColor(0.88f, 0.96f, 1.0f, 1.0f), 1.2f), TEXT("SkillOrbIcon"));
                Visual->IconKey = IconKey;
            }
            if (!Visual->Label)
            {
                Visual->Label = MakePickupLabelComponent(TEXT("SkillOrbLabel"));
            }

            SpawnParticleSprayFx(MakeWorldLocation(Position.X, Position.Y, 82.0f), FVector::UpVector, FVector::RightVector, Accent, 12, 90.0f, 260.0f, 0.42f, 0.014f, 0.034f, 110.0f, -260.0f);
            SpawnRingFx(MakeWorldLocation(Position.X, Position.Y, 8.0f), 86.0f, Accent, 0.42f, 2.0f, 3.0f);
        }

        Visual = SkillOrbWorldVisuals.Find(Key);
        if (!Visual)
        {
            continue;
        }
        if (Visual->Halo)
        {
            Visual->Halo->DestroyComponent();
            Visual->Halo = nullptr;
        }
        if (Visual->RingA)
        {
            Visual->RingA->DestroyComponent();
            Visual->RingA = nullptr;
        }
        if (Visual->RingB)
        {
            Visual->RingB->DestroyComponent();
            Visual->RingB = nullptr;
        }
        if (!Visual->Label)
        {
            Visual->Label = MakePickupLabelComponent(TEXT("SkillOrbLabel"));
        }

        const float Seed = StableUnitFromString(Key + SkillId);
        const float Pulse = 1.0f + FMath::Sin(TimeSeconds * 2.7f + Seed * TWO_PI) * 0.06f;
        const float Bob = FMath::Sin(TimeSeconds * 1.9f + Seed * TWO_PI) * 5.0f;

        if (Visual->Shell)
        {
            Visual->Shell->SetWorldLocation(FVector(Position.X, Position.Y, GroundZ + 68.0f + Bob * 0.55f));
            Visual->Shell->SetWorldRotation(FRotator::ZeroRotator);
            Visual->Shell->SetWorldScale3D(FVector(0.32f * Pulse, 0.32f * Pulse, 0.13f * Pulse));
        }
        if (Visual->Icon)
        {
            Visual->Icon->SetWorldLocation(FVector(Position.X, Position.Y, GroundZ + 88.0f + Bob));
            Visual->Icon->SetWorldRotation(FRotator::ZeroRotator);
            Visual->Icon->SetWorldScale3D(FVector(0.48f * Pulse, 0.48f * Pulse, 0.48f * Pulse));
        }
        const FString OfferName = ResolveSkillOfferLabel(SkillId, State, Orb).Left(24);
        const int32 OfferLevel = ResolveSkillOfferLevel(SkillId, State, Orb);
        UpdatePickupLabel(
            Visual->Label,
            FString::Printf(TEXT("%s Lv %d"), *OfferName, OfferLevel),
            FVector(Position.X, Position.Y - 54.0f, GroundZ + 128.0f + Bob),
            Accent,
            13.5f);

        double& NextAuraPulseTime = SkillOrbAuraPulseTimes.FindOrAdd(Key);
        if (NextAuraPulseTime <= 0.0)
        {
            NextAuraPulseTime = TimeSeconds + Seed * 0.92;
        }
        if (TimeSeconds >= NextAuraPulseTime)
        {
            SpawnRingFx(MakeWorldLocation(Position.X, Position.Y, 32.0f), 74.0f, Accent, 0.34f, 2.0f, 2.8f);
            SpawnParticleSprayFx(MakeWorldLocation(Position.X, Position.Y, 78.0f), FVector::UpVector, FVector::RightVector, Accent, 5, 42.0f, 130.0f, 0.36f, 0.010f, 0.022f, 54.0f, -150.0f);
            NextAuraPulseTime = TimeSeconds + FMath::Lerp(1.08f, 1.34f, Seed);
        }
    }

    TArray<FString> DeadKeys;
    for (const TPair<FString, FCWSkillOrbWorldVisual>& Pair : SkillOrbWorldVisuals)
    {
        if (!LiveSkillOrbKeys.Contains(Pair.Key))
        {
            DeadKeys.Add(Pair.Key);
        }
    }

    for (const FString& Key : DeadKeys)
    {
        FCWSkillOrbWorldVisual Visual;
        SkillOrbWorldVisuals.RemoveAndCopyValue(Key, Visual);
        if (Visual.Halo) Visual.Halo->DestroyComponent();
        if (Visual.Shell) Visual.Shell->DestroyComponent();
        if (Visual.Icon) Visual.Icon->DestroyComponent();
        if (Visual.RingA) Visual.RingA->DestroyComponent();
        if (Visual.RingB) Visual.RingB->DestroyComponent();
        if (Visual.Label) Visual.Label->DestroyComponent();
        SkillOrbRenderPositions.Remove(Key);
        SkillOrbAuraPulseTimes.Remove(Key);
    }
}

void ACWNativeWorldActor::DestroySkillOrbWorldVisuals()
{
    for (TPair<FString, FCWSkillOrbWorldVisual>& Pair : SkillOrbWorldVisuals)
    {
        FCWSkillOrbWorldVisual& Visual = Pair.Value;
        if (Visual.Halo) Visual.Halo->DestroyComponent();
        if (Visual.Shell) Visual.Shell->DestroyComponent();
        if (Visual.Icon) Visual.Icon->DestroyComponent();
        if (Visual.RingA) Visual.RingA->DestroyComponent();
        if (Visual.RingB) Visual.RingB->DestroyComponent();
        if (Visual.Label) Visual.Label->DestroyComponent();
    }
    SkillOrbWorldVisuals.Reset();
    SkillOrbRenderPositions.Reset();
    SkillOrbAuraPulseTimes.Reset();
}

const FCWNativeFxProfile* ACWNativeWorldActor::FindFxProfile(const FString& FxKey) const
{
    return FxKey.IsEmpty() ? nullptr : FxProfiles.Find(FxKey);
}

FLinearColor ACWNativeWorldActor::ResolveFxColor(const FString& Hex, const FLinearColor& Fallback) const
{
    FString Clean = Hex.TrimStartAndEnd();
    if (Clean.IsEmpty())
    {
        return Fallback;
    }
    Clean.RemoveFromStart(TEXT("#"));
    if (Clean.Len() != 6 && Clean.Len() != 8)
    {
        return Fallback;
    }
    return FLinearColor::FromSRGBColor(FColor::FromHex(Clean));
}

FVector ACWNativeWorldActor::MakeWorldLocation(float X, float Y, float ZBias) const
{
    return FVector(X, Y, GroundZ + ZBias);
}

void ACWNativeWorldActor::SetupArenaVisuals()
{
    HideTemplateFloorActors();

    EnsureGroundMaterial();
    GridMaterial = MakeColorMaterial(TEXT("ArenaGridMaterial"), FLinearColor(0.08f, 0.62f, 0.74f, 1.0f), 1.2f);
    PlayerMaterial = MakeColorMaterial(TEXT("PlayerMaterial"), FLinearColor(0.05f, 0.82f, 1.0f, 1.0f), 0.7f);
    EnemyMaterial = MakeColorMaterial(TEXT("EnemyMaterial"), FLinearColor(1.0f, 0.12f, 0.08f, 1.0f), 0.35f);
    BulletMaterial = MakeColorMaterial(TEXT("BulletMaterial"), FLinearColor(1.0f, 0.78f, 0.24f, 1.0f), 2.2f);
    BulletCoreMaterial = MakeColorMaterial(TEXT("BulletCoreMaterial"), FLinearColor(1.0f, 0.96f, 0.64f, 1.0f), 4.0f);
    RocketMaterial = MakeColorMaterial(TEXT("RocketMaterial"), FLinearColor(0.88f, 0.92f, 0.88f, 1.0f), 0.18f);
    RocketFlameMaterial = MakeColorMaterial(TEXT("RocketFlameMaterial"), FLinearColor(1.0f, 0.34f, 0.06f, 1.0f), 4.4f);
    DropMaterial = MakeColorMaterial(TEXT("DropMaterial"), FLinearColor(0.95f, 0.34f, 1.0f, 1.0f), 1.25f);
    XpVacuumMaterial = MakeColorMaterial(TEXT("XpVacuumMaterial"), FLinearColor(0.08f, 0.50f, 1.0f, 1.0f), 3.4f);
    XpOrbMaterial = MakeColorMaterial(TEXT("XpOrbMaterial"), FLinearColor(0.04f, 0.74f, 1.0f, 1.0f), 3.8f);
    XpOrbHaloMaterial = MakeColorMaterial(TEXT("XpOrbHaloMaterial"), FLinearColor(0.00f, 0.42f, 0.92f, 1.0f), 1.8f);
    XpOrbSparkMaterial = MakeColorMaterial(TEXT("XpOrbSparkMaterial"), FLinearColor(0.50f, 0.96f, 1.0f, 1.0f), 5.0f);
    XpVacuumHaloMaterial = MakeColorMaterial(TEXT("XpVacuumHaloMaterial"), FLinearColor(0.04f, 0.36f, 1.0f, 1.0f), 2.2f);
    XpVacuumSparkMaterial = MakeColorMaterial(TEXT("XpVacuumSparkMaterial"), FLinearColor(0.42f, 0.92f, 1.0f, 1.0f), 5.0f);
    MapObjectCarRedMaterial = MakeColorMaterial(TEXT("MapObjectCarRedMaterial"), FLinearColor(0.55f, 0.05f, 0.035f, 1.0f), 0.12f);
    MapObjectCarBlueMaterial = MakeColorMaterial(TEXT("MapObjectCarBlueMaterial"), FLinearColor(0.055f, 0.09f, 0.13f, 1.0f), 0.08f);

    if (UStaticMesh* XpCrystalMesh = LoadXpCrystalStaticMesh())
    {
        if (XpOrbInstances)
        {
            XpOrbInstances->SetStaticMesh(XpCrystalMesh);
        }
        if (XpVacuumInstances)
        {
            XpVacuumInstances->SetStaticMesh(XpCrystalMesh);
        }
    }
    if (GroundMesh && (BlackMaterial || GroundMaterial))
    {
        GroundMesh->SetMaterial(0, GroundMaterial ? GroundMaterial : BlackMaterial);
    }
    if (GridLineInstances && (PlayerFallbackMaterial || GridMaterial))
    {
        GridLineInstances->SetMaterial(0, GridMaterial ? GridMaterial : PlayerFallbackMaterial);
        GridLineInstances->SetVisibility(bShowDebugGrid, true);
    }
    if (PlayerInstances && PlayerMaterial)
    {
        PlayerInstances->SetMaterial(0, PlayerFallbackMaterial ? PlayerFallbackMaterial : PlayerMaterial);
    }
    if (EnemyInstances && EnemyMaterial)
    {
        EnemyInstances->SetMaterial(0, EnemyFallbackMaterial ? EnemyFallbackMaterial : EnemyMaterial);
    }
    if (BulletInstances && BulletMaterial)
    {
        BulletInstances->SetMaterial(0, BulletMaterial);
    }
    if (BulletCoreInstances && BulletCoreMaterial)
    {
        BulletCoreInstances->SetMaterial(0, BulletCoreMaterial);
    }
    if (RocketInstances && RocketMaterial)
    {
        RocketInstances->SetMaterial(0, RocketMaterial);
    }
    if (RocketNoseInstances && BulletCoreMaterial)
    {
        RocketNoseInstances->SetMaterial(0, BulletCoreMaterial);
    }
    if (RocketFlameInstances && RocketFlameMaterial)
    {
        RocketFlameInstances->SetMaterial(0, RocketFlameMaterial);
    }
    if (DropInstances && DropMaterial)
    {
        DropInstances->SetMaterial(0, DropMaterial);
    }
    if (XpVacuumInstances && XpVacuumMaterial)
    {
        const int32 SlotCount = FMath::Max(1, XpVacuumInstances->GetNumMaterials());
        for (int32 Slot = 0; Slot < SlotCount; ++Slot)
        {
            XpVacuumInstances->SetMaterial(Slot, XpVacuumMaterial);
        }
    }
    if (XpOrbInstances && XpOrbMaterial)
    {
        const int32 SlotCount = FMath::Max(1, XpOrbInstances->GetNumMaterials());
        for (int32 Slot = 0; Slot < SlotCount; ++Slot)
        {
            XpOrbInstances->SetMaterial(Slot, XpOrbMaterial);
        }
    }
    if (XpOrbHaloInstances && XpOrbHaloMaterial)
    {
        XpOrbHaloInstances->SetMaterial(0, XpOrbHaloMaterial);
    }
    if (XpOrbSparkInstances && XpOrbSparkMaterial)
    {
        XpOrbSparkInstances->SetMaterial(0, XpOrbSparkMaterial);
    }
    if (XpVacuumHaloInstances && XpVacuumHaloMaterial)
    {
        XpVacuumHaloInstances->SetMaterial(0, XpVacuumHaloMaterial);
    }
    if (XpVacuumSparkInstances && XpVacuumSparkMaterial)
    {
        XpVacuumSparkInstances->SetMaterial(0, XpVacuumSparkMaterial);
    }

    FCWRoomSnapshot DefaultState;
    DefaultState.World.Width = DefaultArenaWidth;
    DefaultState.World.Height = DefaultArenaHeight;
    DefaultState.MapId = TEXT("mall_night");
    ApplyGroundTextureForState(DefaultState);
    UpdateArenaSurface(DefaultState);
}

void ACWNativeWorldActor::HideTemplateFloorActors()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor == this || Actor->IsA<ACWNativeWorldActor>())
        {
            continue;
        }

        const FString ActorName = Actor->GetName();
        const FString ClassName = Actor->GetClass() ? Actor->GetClass()->GetName() : FString();
        const bool bTemplateLight = Actor->FindComponentByClass<ULightComponent>() != nullptr;
        const bool bTemplateEnvironment = ActorName.Contains(TEXT("Fog"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("SkyAtmosphere"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("SkySphere"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("PostProcess"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("Sun"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("Fog"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("SkyAtmosphere"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("SkySphere"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("VolumetricCloud"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("PostProcess"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("Sun"), ESearchCase::IgnoreCase);
        const bool bTemplateVisual = Actor->IsA<AStaticMeshActor>()
            || ActorName.Contains(TEXT("Floor"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("Plane"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("Landscape"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("Landscape"), ESearchCase::IgnoreCase)
            || bTemplateLight
            || bTemplateEnvironment;
        if (!bTemplateVisual)
        {
            continue;
        }

        Actor->SetActorHiddenInGame(true);
        Actor->SetActorEnableCollision(false);
        if (UExponentialHeightFogComponent* FogComponent = Actor->FindComponentByClass<UExponentialHeightFogComponent>())
        {
            FogComponent->SetVisibility(false, true);
            FogComponent->SetHiddenInGame(true, true);
            FogComponent->SetFogDensity(0.0f);
            FogComponent->SetVolumetricFog(false);
        }
        if (UPostProcessComponent* PostProcessComponent = Actor->FindComponentByClass<UPostProcessComponent>())
        {
            PostProcessComponent->bEnabled = false;
            PostProcessComponent->SetVisibility(false, true);
            PostProcessComponent->SetHiddenInGame(true, true);
        }
        if (USkyAtmosphereComponent* SkyAtmosphereComponent = Actor->FindComponentByClass<USkyAtmosphereComponent>())
        {
            SkyAtmosphereComponent->SetVisibility(false, true);
            SkyAtmosphereComponent->SetHiddenInGame(true, true);
        }
        if (UVolumetricCloudComponent* VolumetricCloudComponent = Actor->FindComponentByClass<UVolumetricCloudComponent>())
        {
            VolumetricCloudComponent->SetVisibility(false, true);
            VolumetricCloudComponent->SetHiddenInGame(true, true);
        }
        if (ULightComponent* LightComponent = Actor->FindComponentByClass<ULightComponent>())
        {
            LightComponent->SetVisibility(false, true);
            LightComponent->SetHiddenInGame(true, true);
            LightComponent->SetIntensity(0.0f);
        }
        if (AStaticMeshActor* StaticActor = Cast<AStaticMeshActor>(Actor))
        {
            if (UStaticMeshComponent* MeshComponent = StaticActor->GetStaticMeshComponent())
            {
                MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                if (BlackMaterial)
                {
                    MeshComponent->SetMaterial(0, BlackMaterial);
                }
            }
        }
    }

    bTemplateVisualActorsHidden = true;
}

void ACWNativeWorldActor::UpdateArenaSurface(const FCWRoomSnapshot& State)
{
    const float Width = FMath::Max(DefaultArenaWidth, State.World.Width);
    const float Height = FMath::Max(DefaultArenaHeight, State.World.Height);
    const FVector Center(Width * 0.5f, Height * 0.5f, GroundZ - 10.0f);

    if (GroundMesh)
    {
        GroundMesh->SetRelativeLocation(Center);
        const float GroundPad = FMath::Max(2600.0f, FMath::Max(Width, Height) * 0.65f);
        GroundMesh->SetRelativeScale3D(FVector((Width + GroundPad * 2.0f) / BasicMeshSizeCm, (Height + GroundPad * 2.0f) / BasicMeshSizeCm, 1.0f));
    }

    if (!bShowDebugGrid)
    {
        if (GridLineInstances)
        {
            GridLineInstances->SetVisibility(false, true);
            if (GridLineInstances->GetInstanceCount() > 0)
            {
                GridLineInstances->ClearInstances();
            }
        }
        LastGridSize = FVector2D::ZeroVector;
        return;
    }

    const FVector2D GridSize(Width, Height);
    if (!LastGridSize.Equals(GridSize, 1.0f) || !GridLineInstances || GridLineInstances->GetInstanceCount() <= 0)
    {
        RebuildGrid(Width, Height);
        LastGridSize = GridSize;
    }
}

void ACWNativeWorldActor::RebuildGrid(float Width, float Height)
{
    if (!GridLineInstances)
    {
        return;
    }

    GridLineInstances->SetVisibility(bShowDebugGrid, true);
    GridLineInstances->ClearInstances();
    if (!bShowDebugGrid)
    {
        return;
    }

    const float MinX = 0.0f;
    const float MaxX = Width;
    const float MinY = 0.0f;
    const float MaxY = Height;
    const float CenterX = Width * 0.5f;
    const float CenterY = Height * 0.5f;
    const float Z = GroundZ + 6.0f;

    for (float X = MinX; X <= MaxX + 1.0f; X += GridStep)
    {
        const FVector Scale(0.010f, Height / BasicMeshSizeCm, 0.010f);
        GridLineInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, CenterY, Z), Scale));
    }

    for (float Y = MinY; Y <= MaxY + 1.0f; Y += GridStep)
    {
        const FVector Scale(Width / BasicMeshSizeCm, 0.010f, 0.010f);
        GridLineInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(CenterX, Y, Z), Scale));
    }
}

void ACWNativeWorldActor::EnsureGroundMaterial()
{
    constexpr TCHAR GroundMaterialPath[] = TEXT("/Game/CrimsonWars/Materials/M_NativeGroundField.M_NativeGroundField");
    if (!GroundTextureMaterialParent || GroundTextureMaterialParent->GetPathName() != GroundMaterialPath)
    {
        GroundTextureMaterialParent = LoadObject<UMaterialInterface>(
            nullptr,
            GroundMaterialPath);
        GroundMaterial = nullptr;
    }

    if (!GroundMaterial && GroundTextureMaterialParent)
    {
        GroundMaterial = UMaterialInstanceDynamic::Create(GroundTextureMaterialParent, this, TEXT("ArenaGroundTextureRuntime"));
    }

    if (GroundMaterial)
    {
        GroundMaterial->SetVectorParameterValue(TEXT("TintColorAndOpacity"), FLinearColor(0.76f, 0.82f, 0.62f, 1.0f));
        GroundMaterial->SetVectorParameterValue(TEXT("BackColor"), FLinearColor(0.090f, 0.105f, 0.070f, 1.0f));
        GroundMaterial->SetScalarParameterValue(TEXT("OpacityFromTexture"), 1.0f);
        GroundMaterial->SetScalarParameterValue(TEXT("TextureStrength"), 1.0f);
        GroundMaterial->SetScalarParameterValue(TEXT("TextureWeight"), 1.0f);
        GroundMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.92f);
    }
}

void ACWNativeWorldActor::ApplyGroundTextureForState(const FCWRoomSnapshot& State)
{
    EnsureGroundMaterial();
    if (!GroundMaterial)
    {
        return;
    }

    UTexture2D* GroundTexture = LoadGroundTextureForMap(State.MapId);
    if (!GroundTexture)
    {
        return;
    }

    CurrentGroundTexture = GroundTexture;
    GroundMaterial->SetTextureParameterValue(TEXT("SlateUI"), GroundTexture);
    GroundMaterial->SetTextureParameterValue(TEXT("GroundTexture"), GroundTexture);
    GroundMaterial->SetTextureParameterValue(TEXT("Texture"), GroundTexture);
}

void ACWNativeWorldActor::ApplyReadableEnemyMaterials(USkeletalMeshComponent* Component, const FString& MobId)
{
    if (!Component)
    {
        return;
    }

    USkeletalMesh* Mesh = Component->GetSkeletalMeshAsset();
    const int32 MaterialCount = Component->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
    {
        UMaterialInterface* SourceMaterial = Component->GetMaterial(MaterialIndex);
        if (!SourceMaterial)
        {
            if (UMaterialInterface* ReadableMaterial = GetReadableEnemyMaterial(nullptr, MobId))
            {
                Component->SetMaterial(MaterialIndex, ReadableMaterial);
            }
            continue;
        }

        if (UMaterialInstanceDynamic* ExistingDynamic = Cast<UMaterialInstanceDynamic>(SourceMaterial))
        {
            if (ExistingDynamic->GetOuter() == this)
            {
                const FString DesiredCacheKey = FString::Printf(TEXT("textured_v2:%s"), *MobId);
                if (UMaterialInstanceDynamic* const* DesiredMaterial = ReadableEnemyTextureMaterialCache.Find(DesiredCacheKey))
                {
                    if (*DesiredMaterial == ExistingDynamic)
                    {
                        continue;
                    }
                }

                SourceMaterial = nullptr;
                if (Mesh && Mesh->GetMaterials().IsValidIndex(MaterialIndex))
                {
                    SourceMaterial = Mesh->GetMaterials()[MaterialIndex].MaterialInterface;
                }
                Component->SetMaterial(MaterialIndex, SourceMaterial);
            }
        }

        if (UMaterialInterface* ReadableMaterial = GetReadableEnemyMaterial(SourceMaterial, MobId))
        {
            Component->SetMaterial(MaterialIndex, ReadableMaterial);
        }
    }
}

UMaterialInterface* ACWNativeWorldActor::GetReadableEnemyMaterial(UMaterialInterface* SourceMaterial, const FString& MobId)
{
    if (UTexture2D* BaseColorTexture = LoadEnemyBaseColorTexture(MobId))
    {
        const FString CacheKey = FString::Printf(TEXT("textured_v2:%s"), *MobId);
        if (UMaterialInstanceDynamic* const* CachedTextureMaterial = ReadableEnemyTextureMaterialCache.Find(CacheKey))
        {
            return *CachedTextureMaterial;
        }

        UMaterialInterface* TextureParent = SkeletalTextureMaterialParent ? SkeletalTextureMaterialParent : EmissiveFallbackMaterial;
        if (TextureParent)
        {
            UMaterialInstanceDynamic* TextureMaterial = UMaterialInstanceDynamic::Create(TextureParent, this);
            if (TextureMaterial)
            {
                TextureMaterial->SetTextureParameterValue(TEXT("SpriteTexture"), BaseColorTexture);
                TextureMaterial->SetTextureParameterValue(TEXT("LinearColor"), BaseColorTexture);
                TextureMaterial->SetTextureParameterValue(TEXT("Texture"), BaseColorTexture);
                TextureMaterial->SetTextureParameterValue(TEXT("EmissiveTexture"), BaseColorTexture);
                TextureMaterial->SetTextureParameterValue(TEXT("BaseTexture"), BaseColorTexture);
                TextureMaterial->SetTextureParameterValue(TEXT("BaseColorMap"), BaseColorTexture);
                TextureMaterial->SetTextureParameterValue(TEXT("DiffuseColorMap"), BaseColorTexture);
                TextureMaterial->SetVectorParameterValue(TEXT("SpriteColor"), FLinearColor(0.92f, 0.90f, 0.84f, 1.0f));
                TextureMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.92f, 0.90f, 0.84f, 1.0f));
                TextureMaterial->SetVectorParameterValue(TEXT("TintColor"), FLinearColor(0.92f, 0.90f, 0.84f, 1.0f));
                TextureMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.28f, 0.27f, 0.24f, 1.0f));
                TextureMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
                TextureMaterial->SetScalarParameterValue(TEXT("Alpha"), 1.0f);
                TextureMaterial->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.0f);
                ReadableEnemyTextureMaterialCache.Add(CacheKey, TextureMaterial);
                return TextureMaterial;
            }
        }
    }

    if (!SourceMaterial)
    {
        return nullptr;
    }

    if (UMaterialInstanceDynamic* const* Cached = ReadableEnemyMaterialCache.Find(SourceMaterial))
    {
        return *Cached;
    }

    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SourceMaterial, this);
    if (!Material)
    {
        return SourceMaterial;
    }

    Material->SetScalarParameterValue(TEXT("DiffuseColorMapWeight"), 1.0f);
    Material->SetScalarParameterValue(TEXT("AmbientColorMapWeight"), 0.15f);
    Material->SetScalarParameterValue(TEXT("SpecularColorMapWeight"), 0.0f);
    Material->SetScalarParameterValue(TEXT("ShininessMapWeight"), 0.0f);
    Material->SetScalarParameterValue(TEXT("Shininess"), 0.08f);
    Material->SetScalarParameterValue(TEXT("EmissiveColorMapWeight"), 0.0f);
    Material->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
    Material->SetVectorParameterValue(TEXT("DiffuseColor"), FLinearColor(0.92f, 0.88f, 0.78f, 1.0f));
    Material->SetVectorParameterValue(TEXT("AmbientColor"), FLinearColor(0.28f, 0.27f, 0.24f, 1.0f));
    Material->SetVectorParameterValue(TEXT("SpecularColor"), FLinearColor(0.02f, 0.02f, 0.018f, 1.0f));
    Material->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));

    ReadableEnemyMaterialCache.Add(SourceMaterial, Material);
    return Material;
}

UTexture2D* ACWNativeWorldActor::LoadEnemyBaseColorTexture(const FString& MobId)
{
    const FString CleanMobId = MobId.TrimStartAndEnd().ToLower();
    if (UTexture2D* const* Cached = EnemyBaseColorTextureCache.Find(CleanMobId))
    {
        return *Cached;
    }

    FString AssetName;
    if (CleanMobId == TEXT("boss"))
    {
        AssetName = TEXT("armoreddemon3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("boss_chief_surgeon"))
    {
        AssetName = TEXT("hazmatsuit3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("boss_hellmart"))
    {
        AssetName = TEXT("fierydemon3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("boss_reactor_apostle"))
    {
        AssetName = TEXT("biohazardarmoredmonster3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("boss_road_titan"))
    {
        AssetName = TEXT("lavademon3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("brute"))
    {
        AssetName = TEXT("armoredhumanoid3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("charger"))
    {
        AssetName = TEXT("lavagolem3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("exploder") || CleanMobId == TEXT("runner"))
    {
        AssetName = TEXT("zombiecreature3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("medic"))
    {
        AssetName = TEXT("zombie3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("normal"))
    {
        AssetName = TEXT("undeadzombie3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("ranged"))
    {
        AssetName = TEXT("zombiemonster3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("shield"))
    {
        AssetName = TEXT("fantasycreature3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("sniper"))
    {
        AssetName = TEXT("icemonster3dmodel_basecolor");
    }
    else if (CleanMobId == TEXT("splitter"))
    {
        AssetName = TEXT("monster3dmodel_basecolor");
    }

    UTexture2D* Texture = nullptr;
    if (!AssetName.IsEmpty())
    {
        const FString TexturePath = FString::Printf(TEXT("/Game/Characters/Monsters/%s/%s.%s"), *CleanMobId, *AssetName, *AssetName);
        Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
        if (!Texture)
        {
            UE_LOG(LogTemp, Warning, TEXT("Missing Tripo enemy base color texture: %s"), *TexturePath);
        }
    }

    EnemyBaseColorTextureCache.Add(CleanMobId, Texture);
    return Texture;
}

UTexture2D* ACWNativeWorldActor::LoadGroundTextureForMap(const FString& MapId)
{
    FString MapKey = MapId.TrimStartAndEnd().ToLower();
    if (MapKey.IsEmpty())
    {
        MapKey = TEXT("default");
    }

    const FString ProceduralAssetName = FString::Printf(TEXT("procedural_ground_v4_%s"), *MapKey);
    if (UTexture2D* const* CachedProcedural = GroundTextureCache.Find(ProceduralAssetName))
    {
        return *CachedProcedural;
    }

    if (UTexture2D* ProceduralTexture = CreateProceduralGroundTexture(this, MapKey))
    {
        GroundTextureCache.Add(ProceduralAssetName, ProceduralTexture);
        return ProceduralTexture;
    }

    FString AssetName = TEXT("night-mall");
    FString Folder = TEXT("maps");

    if (MapKey.Contains(TEXT("clinic")) || MapKey.Contains(TEXT("yard")))
    {
        AssetName = TEXT("clinic-yard");
    }
    else if (MapKey.Contains(TEXT("reactor")))
    {
        AssetName = TEXT("reactor-sprawl");
    }
    else if (MapKey.Contains(TEXT("ring")) || MapKey.Contains(TEXT("road")) || MapKey.Contains(TEXT("bbq")))
    {
        AssetName = TEXT("ringroad-bbq");
    }
    else if (MapKey.Contains(TEXT("mall")) || MapKey.Contains(TEXT("night")))
    {
        AssetName = TEXT("night-mall");
    }

    if (UTexture2D* const* Cached = GroundTextureCache.Find(AssetName))
    {
        return *Cached;
    }

    const FString TexturePath = FString::Printf(TEXT("/Game/CrimsonWars/RawAssets/%s/%s.%s"), *Folder, *AssetName, *AssetName);
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
    ConfigureGroundTextureFor3D(Texture);
    GroundTextureCache.Add(AssetName, Texture);

    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("Missing Crimson Wars ground texture: %s for map id %s"), *TexturePath, *MapId);
    }
    return Texture;
}

UMaterialInstanceDynamic* ACWNativeWorldActor::MakeColorMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength)
{
    if (!BaseShapeMaterial)
    {
        return nullptr;
    }

    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this, FName(*Name));
    if (!Material)
    {
        return nullptr;
    }

    Material->SetVectorParameterValue(TEXT("Color"), Color);
    Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
    Material->SetVectorParameterValue(TEXT("EmissiveColor"), Color * EmissiveStrength);
    Material->SetScalarParameterValue(TEXT("Roughness"), 0.82f);
    return Material;
}

FTransform ACWNativeWorldActor::MakeSphereTransform(float X, float Y, float Radius, float ZBias)
{
    const float ScaleValue = (Radius * 2.0f) / BasicMeshSizeCm;
    return FTransform(FRotator::ZeroRotator, FVector(X, Y, Radius + ZBias), FVector(ScaleValue));
}

FTransform ACWNativeWorldActor::MakeCylinderTransform(float X, float Y, float Radius, float Height)
{
    const FVector Scale((Radius * 2.0f) / BasicMeshSizeCm, (Radius * 2.0f) / BasicMeshSizeCm, Height / BasicMeshSizeCm);
    return FTransform(FRotator::ZeroRotator, FVector(X, Y, Height * 0.5f), Scale);
}
