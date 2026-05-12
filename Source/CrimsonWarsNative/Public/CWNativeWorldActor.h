#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWProtocolTypes.h"
#include "CWNativeWorldActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FCWNativeBootstrapSnapshot;

struct FCWActiveFxComponent
{
    UStaticMeshComponent* Component = nullptr;
    float Age = 0.0f;
    float Life = 0.0f;
    FVector StartScale = FVector::OneVector;
    FVector EndScale = FVector::OneVector;
    float SpinYawDegPerSec = 0.0f;
};

UCLASS()
class CRIMSONWARSNATIVE_API ACWNativeWorldActor : public AActor
{
    GENERATED_BODY()

public:
    ACWNativeWorldActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    float GroundZ = 0.0f;

private:
    void HandleStateReceived(const FCWRoomSnapshot& State);
    void HandleBootstrapReceived(const FCWNativeBootstrapSnapshot& Bootstrap);
    void HandleSkillFxReceived(const FCWSkillFxEvent& Event);
    void HandleMeleeFxReceived(const FCWMeleeFxEvent& Event);
    void HandleWorldFxReceived(const FCWWorldFxEvent& Event);
    void RenderState(const FCWRoomSnapshot& State);
    void EmitStateDrivenFx(const FCWRoomSnapshot& State);
    void SetupArenaVisuals();
    void HideTemplateFloorActors();
    void UpdateArenaSurface(const FCWRoomSnapshot& State);
    void RebuildGrid(float Width, float Height);
    UMaterialInstanceDynamic* MakeColorMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength = 0.0f);
    UMaterialInstanceDynamic* MakeFxMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength = 2.5f);
    UStaticMeshComponent* SpawnFxMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& StartScale, const FVector& EndScale, const FLinearColor& Color, float Life, float EmissiveStrength = 3.0f, float SpinYawDegPerSec = 0.0f);
    void SpawnRingFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, float Height = 3.0f, float EmissiveStrength = 3.0f);
    void SpawnBurstFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, int32 SparkCount = 8);
    void SpawnBeamFx(const FVector& Start, const FVector& End, float Width, const FLinearColor& Color, float Life, float EmissiveStrength = 3.0f);
    void SpawnArcFx(const FVector& Center, float AngleRad, float Range, float ArcDeg, float Width, const FLinearColor& Color, const FLinearColor& SecondaryColor, float Life, int32 Segments = 12);
    void SpawnSkillCastFx(const FCWPlayerSnapshot& Player, const FCWSkillSnapshot& Skill, const FCWRoomSnapshot& State);
    void SpawnProjectileShotFx(const FCWShotEventSnapshot& Event);
    void SpawnRocketExplosionFx(const FCWBulletSnapshot& Bullet);
    void SpawnObjectImpactFx(const FCWObjectImpactEventSnapshot& Event);
    void SpawnXpSurgePullFx(const FCWWorldFxEvent& Event);
    void SpawnGenericWorldFx(const FCWWorldFxEvent& Event);
    void SpawnSkillEventFx(const FCWSkillFxEvent& Event);
    void SpawnMeleeStyleFx(const FCWMeleeFxEvent& Event);
    void AddRocketTrailFx(const FCWBulletSnapshot& Bullet);
    const FCWNativeFxProfile* FindFxProfile(const FString& FxKey) const;
    FLinearColor ResolveFxColor(const FString& Hex, const FLinearColor& Fallback) const;
    FVector MakeWorldLocation(float X, float Y, float ZBias = 0.0f) const;
    static FTransform MakeSphereTransform(float X, float Y, float Radius, float ZBias = 0.0f);
    static FTransform MakeCylinderTransform(float X, float Y, float Radius, float Height);

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    USceneComponent* SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UStaticMeshComponent* GroundMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* GridLineInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* PlayerInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* EnemyInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* BulletInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* RocketInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* DropInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpVacuumInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpOrbInstances = nullptr;

    UPROPERTY()
    UMaterialInterface* BaseShapeMaterial = nullptr;

    UPROPERTY()
    UMaterialInterface* BlackMaterial = nullptr;

    UPROPERTY()
    UMaterialInterface* EnemyFallbackMaterial = nullptr;

    UPROPERTY()
    UMaterialInterface* PlayerFallbackMaterial = nullptr;

    UPROPERTY()
    UMaterialInterface* EmissiveFallbackMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* GroundMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* GridMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* PlayerMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* EnemyMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* BulletMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* RocketMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* DropMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpVacuumMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpOrbMaterial = nullptr;

    UPROPERTY()
    UStaticMesh* CylinderMeshAsset = nullptr;

    UPROPERTY()
    UStaticMesh* SphereMeshAsset = nullptr;

    UPROPERTY()
    UStaticMesh* CubeMeshAsset = nullptr;

    FDelegateHandle StateDelegateHandle;
    FDelegateHandle BootstrapDelegateHandle;
    FDelegateHandle SkillFxDelegateHandle;
    FDelegateHandle MeleeFxDelegateHandle;
    FDelegateHandle WorldFxDelegateHandle;
    FVector2D LastGridSize = FVector2D::ZeroVector;
    bool bLoggedFirstState = false;
    FCWRoomSnapshot PreviousState;
    bool bHasPreviousState = false;
    TMap<FString, FCWNativeFxProfile> FxProfiles;
    TSet<FString> SeenSkillFxEventIds;
    TSet<FString> SeenShotEventIds;
    TSet<FString> SeenObjectImpactEventIds;
    TMap<FString, float> PreviousSkillCooldowns;
    TMap<FString, FCWBulletSnapshot> LastRocketBullets;
    TArray<FCWActiveFxComponent> ActiveFxComponents;
};
