#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CWProtocolTypes.h"
#include "CWNativeWorldActor.generated.h"

class UInstancedStaticMeshComponent;
class UAnimSequence;
class UDirectionalLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkyLightComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTexture2D;
struct FCWNativeBootstrapSnapshot;

struct FCWActiveFxComponent
{
    UStaticMeshComponent* Component = nullptr;
    float Age = 0.0f;
    float Life = 0.0f;
    FVector StartScale = FVector::OneVector;
    FVector EndScale = FVector::OneVector;
    FVector Velocity = FVector::ZeroVector;
    FVector Acceleration = FVector::ZeroVector;
    float SpinYawDegPerSec = 0.0f;
    bool bAlignToVelocity = false;
    bool bBloodDroplet = false;
    bool bLeavesBloodSplat = false;
    bool bGroundBloodPool = false;
    float BloodSplatRadius = 18.0f;
    float FadeOutSeconds = 0.45f;
    FLinearColor BaseColor = FLinearColor::White;
    float EmissiveStrength = 0.0f;
};

struct FCWActiveEnemyDeathVisual
{
    USkeletalMeshComponent* Component = nullptr;
    float Age = 0.0f;
    float Life = 1.45f;
    bool bRagdoll = false;
    bool bKinematicFall = false;
    bool bSinkStarted = false;
    bool bBloodCleanupSpawned = false;
    float StillAge = 0.0f;
    float SinkStartAge = 0.0f;
    float SinkDuration = 1.45f;
    float SinkDepth = 95.0f;
    FVector SinkStartLocation = FVector::ZeroVector;
    FVector SinkStartScale = FVector::OneVector;
    FVector FallStartLocation = FVector::ZeroVector;
    FRotator FallStartRotation = FRotator::ZeroRotator;
    FRotator FallEndRotation = FRotator::ZeroRotator;
    float FallDuration = 0.75f;
    float LieDuration = 2.35f;
};

struct FCWSkeletalVisualTarget
{
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Scale = FVector::OneVector;
    bool bSnap = true;
};

struct FCWProjectileVisualState
{
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Velocity = FVector2D::ZeroVector;
    FVector2D ServerPosition = FVector2D::ZeroVector;
    FVector2D ServerVelocity = FVector2D::ZeroVector;
    FString Kind;
    FString FxKey;
    FString Color;
    float Radius = 3.0f;
    double LastServerTime = 0.0;
    double LastSeenTime = 0.0;
    bool bInitialized = false;
};

struct FCWProjectileImpactClip
{
    FString BulletId;
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Direction = FVector2D::ZeroVector;
    float Radius = 90.0f;
    double ExpireTime = 0.0;
};

struct FCWSkillOrbWorldVisual
{
    UStaticMeshComponent* Halo = nullptr;
    UStaticMeshComponent* Shell = nullptr;
    UStaticMeshComponent* Icon = nullptr;
    UStaticMeshComponent* RingA = nullptr;
    UStaticMeshComponent* RingB = nullptr;
    UTextRenderComponent* Label = nullptr;
    FString IconKey;
};

struct FCWPickupWorldVisual
{
    UStaticMeshComponent* Aura = nullptr;
    UStaticMeshComponent* Icon = nullptr;
    UTextRenderComponent* Label = nullptr;
    FString IconKey;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    bool bUseSkeletalEnemyVisuals = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    bool bUseSkeletalPlayerVisuals = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    bool bUseStaticMeshMapObjects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    int32 MaxStaticMeshMapObjects = 220;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    bool bEnableRuntimeFx = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    bool bUseImportedEnemyMaterials = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crimson Wars|World")
    bool bShowDebugGrid = false;

private:
    void HandleStateReceived(const FCWRoomSnapshot& State);
    void HandleBootstrapReceived(const FCWNativeBootstrapSnapshot& Bootstrap);
    void HandleSkillFxReceived(const FCWSkillFxEvent& Event);
    void HandleMeleeFxReceived(const FCWMeleeFxEvent& Event);
    void HandleWorldFxReceived(const FCWWorldFxEvent& Event);
    void RenderState(const FCWRoomSnapshot& State);
    FString MakeRunIdentity(const FCWRoomSnapshot& State) const;
    bool IsEmptyRoomState(const FCWRoomSnapshot& State) const;
    void ResetTransientRunVisualState();
    void SyncRunIdentity(const FCWRoomSnapshot& State);
    void EmitStateDrivenFx(const FCWRoomSnapshot& State);
    void SpawnStateDrivenEnemyBloodFx(const FCWRoomSnapshot& State);
    void SetupArenaVisuals();
    void HideTemplateFloorActors();
    void UpdateArenaSurface(const FCWRoomSnapshot& State);
    void RebuildGrid(float Width, float Height);
    void EnsureGroundMaterial();
    void ApplyGroundTextureForState(const FCWRoomSnapshot& State);
    UTexture2D* LoadGroundTextureForMap(const FString& MapId);
    void ApplyReadableEnemyMaterials(USkeletalMeshComponent* Component, const FString& MobId);
    UMaterialInterface* GetReadableEnemyMaterial(UMaterialInterface* SourceMaterial, const FString& MobId);
    UTexture2D* LoadEnemyBaseColorTexture(const FString& MobId);
    UMaterialInstanceDynamic* MakeColorMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength = 0.0f);
    UMaterialInstanceDynamic* MakeFxMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength = 2.5f);
    UMaterialInstanceDynamic* MakeTexturedFxMaterial(const FString& Name, UTexture2D* Texture, const FLinearColor& Tint, float EmissiveStrength = 1.8f);
    UMaterialInstanceDynamic* GetSkillIconMaterial(const FString& IconKey, UTexture2D* Texture, const FLinearColor& Accent);
    UStaticMeshComponent* SpawnFxMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& StartScale, const FVector& EndScale, const FLinearColor& Color, float Life, float EmissiveStrength = 3.0f, float SpinYawDegPerSec = 0.0f, const FVector& Velocity = FVector::ZeroVector, const FVector& Acceleration = FVector::ZeroVector, bool bAlignToVelocity = false, UMaterialInterface* OverrideMaterial = nullptr);
    void SpawnParticleSprayFx(const FVector& Location, const FVector& BaseDirection, const FVector& SideDirection, const FLinearColor& Color, int32 Count, float SpeedMin, float SpeedMax, float Life, float SizeMin, float SizeMax, float UpBias = 120.0f, float Gravity = -520.0f);
    void SpawnRingFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, float Height = 3.0f, float EmissiveStrength = 3.0f);
    void SpawnBurstFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, int32 SparkCount = 8);
    void SpawnBeamFx(const FVector& Start, const FVector& End, float Width, const FLinearColor& Color, float Life, float EmissiveStrength = 3.0f);
    void SpawnSurfaceImpactFx(const FVector& Location, const FVector& Direction, const FVector& Normal, const FString& Material, const FString& BulletKind, float Damage);
    void SpawnBloodHitFx(const FVector& Location, const FVector& Direction, float DamageScale, float VisualRadius);
    void SpawnBloodDeathFx(const FCWEnemySnapshot& Enemy);
    void SpawnBloodPoolFx(const FVector& Location, float Radius, float Life, float Seed = 0.0f);
    void AddProjectileImpactClip(const FString& BulletId, const FVector2D& Position, const FVector2D& Direction, float Radius, float LifeSeconds);
    void PruneProjectileImpactClips(double TimeSeconds);
    void SpawnArcFx(const FVector& Center, float AngleRad, float Range, float ArcDeg, float Width, const FLinearColor& Color, const FLinearColor& SecondaryColor, float Life, int32 Segments = 12);
    void SpawnSkillCastFx(const FCWPlayerSnapshot& Player, const FCWSkillSnapshot& Skill, const FCWRoomSnapshot& State);
    void SpawnProjectileShotFx(const FCWShotEventSnapshot& Event);
    FString ResolveSurfaceMaterialAt(const FCWRoomSnapshot& State, const FVector2D& Position) const;
    void SpawnRocketExplosionFx(const FCWBulletSnapshot& Bullet, const FCWRoomSnapshot& State);
    void SpawnRocketExplosionAt(const FVector& Location, const FVector& Direction, const FString& SurfaceMaterial, float Radius, const FLinearColor& Color);
    void SpawnObjectImpactFx(const FCWObjectImpactEventSnapshot& Event);
    void SpawnXpSurgePullFx(const FCWWorldFxEvent& Event);
    void SpawnGenericWorldFx(const FCWWorldFxEvent& Event);
    void SpawnSkillEventFx(const FCWSkillFxEvent& Event);
    void SpawnMeleeStyleFx(const FCWMeleeFxEvent& Event);
    void AddRocketTrailFx(const FCWBulletSnapshot& Bullet);
    void SyncProjectileVisualTargets(const FCWRoomSnapshot& State);
    void RebuildProjectileInstances(float DeltaSeconds);
    void UpdateMapObjectStaticMeshVisuals(const FCWRoomSnapshot& State);
    void UpdateTreeStaticMeshVisuals(const FCWRoomSnapshot& State);
    void DestroyMapObjectStaticMeshVisuals();
    FString ResolveMapObjectMeshKey(const FString& SpriteKey) const;
    UStaticMesh* LoadMapObjectStaticMesh(const FString& MeshKey);
    UTexture2D* LoadMapObjectTexture(const FString& MeshKey);
    UMaterialInterface* GetMapObjectTextureMaterial(const FString& MeshKey);
    FTransform BuildMapObjectTransform(const FCWMapObjectSnapshot& Object, UStaticMesh* Mesh) const;
    UStaticMesh* LoadXpCrystalStaticMesh();
    void MarkPlayerAttackVisual(const FString& PlayerId, const FVector2D& Origin, const FVector2D& Aim, float Duration = 0.46f);
    bool UpdatePlayerSkeletalVisual(const FCWPlayerSnapshot& Player);
    void RetireMissingPlayerSkeletalVisuals(const FCWRoomSnapshot& State);
    void DestroyPlayerSkeletalVisuals();
    FString ResolveHeroId(const FCWPlayerSnapshot& Player) const;
    USkeletalMesh* LoadHeroSkeletalMesh(const FString& HeroId);
    UAnimSequence* LoadHeroIdleAnimation(const FString& HeroId);
    UAnimSequence* LoadHeroWalkAnimation(const FString& HeroId);
    UAnimSequence* LoadHeroAttackAnimation(const FString& HeroId);
    UAnimSequence* LoadHeroDeathAnimation(const FString& HeroId);
    UTexture2D* LoadHeroBaseColorTexture(const FString& HeroId);
    void ApplyReadableHeroMaterials(USkeletalMeshComponent* Component, const FString& HeroId);
    bool UpdateEnemySkeletalVisual(const FCWEnemySnapshot& Enemy, const FCWRoomSnapshot& State);
    void RetireMissingEnemySkeletalVisuals(const FCWRoomSnapshot& State);
    void SpawnEnemyDeathVisual(const FCWEnemySnapshot& Enemy, USkeletalMeshComponent* ExistingComponent);
    void DestroyEnemySkeletalVisuals();
    FString ResolveEnemyMobId(const FCWEnemySnapshot& Enemy) const;
    USkeletalMesh* LoadEnemySkeletalMesh(const FString& MobId);
    UAnimSequence* LoadEnemyIdleAnimation(const FString& MobId);
    UAnimSequence* LoadEnemyWalkAnimation(const FString& MobId);
    UAnimSequence* LoadEnemyAttackAnimation(const FString& MobId);
    UAnimSequence* LoadEnemyDeathAnimation(const FString& MobId);
    void UpdateSkeletalVisualTargets(float DeltaSeconds);
    FVector2D GetSmoothedPickupPosition(TMap<FString, FVector2D>& PositionCache, const FString& Key, const FVector2D& Target, float DeltaSeconds, float InterpSpeed = 14.0f, float SnapDistance = 1000.0f);
    void RebuildPickupInstances(const FCWRoomSnapshot& State, float DeltaSeconds);
    void UpdateSkillOrbWorldVisuals(const FCWRoomSnapshot& State, float DeltaSeconds);
    void DestroySkillOrbWorldVisuals();
    const FCWNativeFxProfile* FindFxProfile(const FString& FxKey) const;
    FLinearColor ResolveFxColor(const FString& Hex, const FLinearColor& Fallback) const;
    FLinearColor SkillOrbAccentColor(const FString& SkillId) const;
    FString ResolveSkillIconKey(const FString& SkillId) const;
    FString ResolveSkillOfferLabel(const FString& SkillId, const FCWRoomSnapshot& State, const FCWPickupSnapshot& Orb) const;
    int32 ResolveSkillOfferLevel(const FString& SkillId, const FCWRoomSnapshot& State, const FCWPickupSnapshot& Orb) const;
    UTexture2D* LoadSkillIconTexture(const FString& SkillId);
    FString NormalizeWeaponPickupKey(const FString& WeaponKey) const;
    FString WeaponPickupLabel(const FString& WeaponKey) const;
    FLinearColor WeaponPickupAccentColor(const FString& WeaponKey) const;
    UTexture2D* LoadWeaponPickupTexture(const FString& WeaponKey);
    UTextRenderComponent* MakePickupLabelComponent(const FString& DebugName);
    void UpdatePickupLabel(UTextRenderComponent* Label, const FString& Text, const FVector& Location, const FLinearColor& Color, float WorldSize = 20.0f);
    FVector MakeWorldLocation(float X, float Y, float ZBias = 0.0f) const;
    static FTransform MakeSphereTransform(float X, float Y, float Radius, float ZBias = 0.0f);
    static FTransform MakeCylinderTransform(float X, float Y, float Radius, float Height);

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    USceneComponent* SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UDirectionalLightComponent* KeyLight = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    USkyLightComponent* SkyLight = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UStaticMeshComponent* GroundMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* GridLineInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* TreeStaticMeshInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* PlayerInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* EnemyInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* BulletInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* BulletCoreInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* RocketInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* RocketNoseInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* RocketFlameInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* DropInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpVacuumInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpOrbInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpOrbHaloInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpOrbSparkInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpVacuumHaloInstances = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Crimson Wars")
    UInstancedStaticMeshComponent* XpVacuumSparkInstances = nullptr;

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
    UMaterialInterface* TranslucentFxMaterialParent = nullptr;

    UPROPERTY()
    UMaterialInterface* SkeletalEnemyPreviewMaterial = nullptr;

    UPROPERTY()
    UMaterialInterface* SkeletalTextureMaterialParent = nullptr;

    UPROPERTY()
    UMaterialInterface* WorldIconTextureMaterialParent = nullptr;

    UPROPERTY()
    UMaterialInterface* GroundTextureMaterialParent = nullptr;

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
    UMaterialInstanceDynamic* BulletCoreMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* RocketMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* RocketFlameMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* DropMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpVacuumMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpOrbMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpOrbHaloMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpOrbSparkMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpVacuumHaloMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* XpVacuumSparkMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* MapObjectCarRedMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* MapObjectCarBlueMaterial = nullptr;

    UPROPERTY()
    UStaticMesh* CylinderMeshAsset = nullptr;

    UPROPERTY()
    UStaticMesh* SphereMeshAsset = nullptr;

    UPROPERTY()
    UStaticMesh* CubeMeshAsset = nullptr;

    UPROPERTY()
    UStaticMesh* PlaneMeshAsset = nullptr;

    UPROPERTY()
    UStaticMesh* XpCrystalMeshAsset = nullptr;

    bool bXpCrystalMeshLoadAttempted = false;

    FString AssetsRoot;
    FDelegateHandle StateDelegateHandle;
    FDelegateHandle BootstrapDelegateHandle;
    FDelegateHandle SkillFxDelegateHandle;
    FDelegateHandle MeleeFxDelegateHandle;
    FDelegateHandle WorldFxDelegateHandle;
    FVector2D LastGridSize = FVector2D::ZeroVector;
    bool bLoggedFirstState = false;
    bool bTemplateVisualActorsHidden = false;
    FCWRoomSnapshot PreviousState;
    bool bHasPreviousState = false;
    FString LastRunIdentity;
    double LatestStateReceivedTime = 0.0;
    TMap<FString, FCWNativeFxProfile> FxProfiles;
    TSet<FString> SeenSkillFxEventIds;
    TSet<FString> SeenShotEventIds;
    TSet<FString> SeenObjectImpactEventIds;
    TMap<FString, float> PreviousSkillCooldowns;
    TMap<FString, FCWBulletSnapshot> LastRocketBullets;
    TMap<FString, double> RetiredProjectileVisualUntil;
    TArray<FCWProjectileImpactClip> RecentProjectileImpactClips;
    TSet<FString> SeenEnemyDeathBloodIds;
    TArray<FCWActiveFxComponent> ActiveFxComponents;

    UPROPERTY()
    TMap<FString, UStaticMeshComponent*> MapObjectStaticMeshComponents;

    UPROPERTY()
    TMap<FString, UStaticMesh*> MapObjectStaticMeshCache;

    UPROPERTY()
    TMap<FString, UTexture2D*> MapObjectTextureCache;

    UPROPERTY()
    TMap<FString, UMaterialInstanceDynamic*> MapObjectTextureMaterialCache;

    TMap<FString, FString> MapObjectStaticMeshKeys;
    TSet<FString> MissingMapObjectAssetWarnings;

    UPROPERTY()
    TMap<FString, UTexture2D*> GroundTextureCache;

    UPROPERTY()
    UTexture2D* CurrentGroundTexture = nullptr;

    UPROPERTY()
    TMap<FString, USkeletalMeshComponent*> PlayerSkeletalComponents;

    UPROPERTY()
    TMap<FString, FString> PlayerSkeletalHeroIds;

    UPROPERTY()
    TMap<FString, USkeletalMesh*> HeroSkeletalMeshCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> HeroIdleAnimCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> HeroWalkAnimCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> HeroAttackAnimCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> HeroDeathAnimCache;

    UPROPERTY()
    TMap<FString, UTexture2D*> HeroBaseColorTextureCache;

    UPROPERTY()
    TMap<FString, USkeletalMeshComponent*> EnemySkeletalComponents;

    UPROPERTY()
    TMap<FString, FString> EnemySkeletalMobIds;

    UPROPERTY()
    TMap<FString, USkeletalMesh*> EnemySkeletalMeshCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> EnemyIdleAnimCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> EnemyWalkAnimCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> EnemyAttackAnimCache;

    UPROPERTY()
    TMap<FString, UAnimSequence*> EnemyDeathAnimCache;

    UPROPERTY()
    TMap<FString, UTexture2D*> EnemyBaseColorTextureCache;

    UPROPERTY()
    TMap<FString, UMaterialInstanceDynamic*> ReadableEnemyTextureMaterialCache;

    TMap<UMaterialInterface*, UMaterialInstanceDynamic*> ReadableEnemyMaterialCache;

    TMap<FString, FVector2D> EnemyPreviousLocations;
    TMap<FString, double> EnemyPreviousSampleTimes;
    TMap<FString, FVector2D> PlayerPreviousLocations;
    TMap<FString, double> PlayerPreviousSampleTimes;
    TMap<FString, double> PlayerAttackVisualUntil;
    TMap<FString, FRotator> PlayerAttackVisualRotations;
    TMap<FString, FCWSkeletalVisualTarget> PlayerSkeletalTargets;
    TMap<FString, FCWSkeletalVisualTarget> EnemySkeletalTargets;
    TMap<FString, FVector2D> DropRenderPositions;
    TMap<FString, FVector2D> XpOrbRenderPositions;
    TMap<FString, FVector2D> XpVacuumRenderPositions;
    TMap<FString, FVector2D> SkillOrbRenderPositions;
    TMap<FString, double> SkillOrbAuraPulseTimes;
    TMap<FString, FCWProjectileVisualState> ProjectileVisualStates;
    TMap<FString, FCWSkillOrbWorldVisual> SkillOrbWorldVisuals;
    TMap<FString, FCWPickupWorldVisual> WeaponPickupWorldVisuals;

    UPROPERTY()
    TMap<FString, UTexture2D*> SkillIconTextureCache;

    UPROPERTY()
    TMap<FString, UMaterialInstanceDynamic*> SkillIconMaterialCache;

    UPROPERTY()
    TMap<FString, UTexture2D*> WeaponPickupTextureCache;

    TArray<FCWActiveEnemyDeathVisual> ActiveEnemyDeathVisuals;
    TSet<FString> SeenEnemyDeathVisualIds;
    TSet<FString> MissingEnemyAssetWarnings;
    TSet<FString> MissingHeroAssetWarnings;
};
