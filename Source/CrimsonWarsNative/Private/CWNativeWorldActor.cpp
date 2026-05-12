#include "CWNativeWorldActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CWNativeGameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float BasicMeshSizeCm = 100.0f;
    constexpr float DefaultArenaWidth = 4800.0f;
    constexpr float DefaultArenaHeight = 2800.0f;
    constexpr float GridStep = 400.0f;
    constexpr float ProjectileFxZ = 64.0f;
    constexpr float GroundFxZ = 10.0f;

    void SetupInstanceComponent(UInstancedStaticMeshComponent* Component)
    {
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetMobility(EComponentMobility::Movable);
        Component->NumCustomDataFloats = 0;
        Component->SetCastShadow(false);
    }
}

ACWNativeWorldActor::ACWNativeWorldActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArenaGround"));
    GroundMesh->SetupAttachment(SceneRoot);
    GroundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GroundMesh->SetMobility(EComponentMobility::Movable);
    GroundMesh->SetCastShadow(false);

    GridLineInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArenaGrid"));
    GridLineInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(GridLineInstances);

    PlayerInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Players"));
    PlayerInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(PlayerInstances);

    EnemyInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Enemies"));
    EnemyInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(EnemyInstances);

    BulletInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bullets"));
    BulletInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(BulletInstances);

    RocketInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rockets"));
    RocketInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(RocketInstances);

    DropInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Drops"));
    DropInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(DropInstances);

    XpVacuumInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpVacuumDrops"));
    XpVacuumInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpVacuumInstances);

    XpOrbInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("XpOrbs"));
    XpOrbInstances->SetupAttachment(SceneRoot);
    SetupInstanceComponent(XpOrbInstances);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlackUnlitMaterial(TEXT("/Engine/EngineDebugMaterials/BlackUnlitMaterial.BlackUnlitMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialError(TEXT("/Engine/EngineDebugMaterials/MaterialError_Mat.MaterialError_Mat"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlueTemplateMaterial(TEXT("/Engine/TemplateResources/MI_Template_BaseBlue.MI_Template_BaseBlue"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMeshMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));

    if (CylinderMesh.Succeeded())
    {
        CylinderMeshAsset = CylinderMesh.Object;
        PlayerInstances->SetStaticMesh(CylinderMesh.Object);
        EnemyInstances->SetStaticMesh(CylinderMesh.Object);
    }

    if (SphereMesh.Succeeded())
    {
        SphereMeshAsset = SphereMesh.Object;
        BulletInstances->SetStaticMesh(SphereMesh.Object);
        XpOrbInstances->SetStaticMesh(SphereMesh.Object);
    }

    if (CubeMesh.Succeeded())
    {
        CubeMeshAsset = CubeMesh.Object;
        GroundMesh->SetStaticMesh(CubeMesh.Object);
        GridLineInstances->SetStaticMesh(CubeMesh.Object);
        DropInstances->SetStaticMesh(CubeMesh.Object);
        RocketInstances->SetStaticMesh(CubeMesh.Object);
        XpVacuumInstances->SetStaticMesh(CubeMesh.Object);
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
}

void ACWNativeWorldActor::BeginPlay()
{
    Super::BeginPlay();

    SetupArenaVisuals();

    if (UCWNativeGameInstance* GI = GetGameInstance<UCWNativeGameInstance>())
    {
        StateDelegateHandle = GI->OnStateReceived.AddUObject(this, &ACWNativeWorldActor::HandleStateReceived);
        BootstrapDelegateHandle = GI->OnBootstrapReceived.AddUObject(this, &ACWNativeWorldActor::HandleBootstrapReceived);
        SkillFxDelegateHandle = GI->OnSkillFxReceived.AddUObject(this, &ACWNativeWorldActor::HandleSkillFxReceived);
        MeleeFxDelegateHandle = GI->OnMeleeFxReceived.AddUObject(this, &ACWNativeWorldActor::HandleMeleeFxReceived);
        WorldFxDelegateHandle = GI->OnWorldFxReceived.AddUObject(this, &ACWNativeWorldActor::HandleWorldFxReceived);
        HandleBootstrapReceived(GI->Bootstrap);
        RenderState(GI->LatestState);
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

    Super::EndPlay(EndPlayReason);
}

void ACWNativeWorldActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    for (int32 Index = ActiveFxComponents.Num() - 1; Index >= 0; --Index)
    {
        FCWActiveFxComponent& Fx = ActiveFxComponents[Index];
        if (!Fx.Component)
        {
            ActiveFxComponents.RemoveAtSwap(Index);
            continue;
        }

        Fx.Age += DeltaSeconds;
        const float T = Fx.Life > KINDA_SMALL_NUMBER ? FMath::Clamp(Fx.Age / Fx.Life, 0.0f, 1.0f) : 1.0f;
        const float Ease = 1.0f - FMath::Pow(1.0f - T, 2.0f);
        Fx.Component->SetWorldScale3D(FMath::Lerp(Fx.StartScale, Fx.EndScale, Ease));
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
}

void ACWNativeWorldActor::HandleStateReceived(const FCWRoomSnapshot& State)
{
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
    SpawnSkillEventFx(Event);
}

void ACWNativeWorldActor::HandleMeleeFxReceived(const FCWMeleeFxEvent& Event)
{
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

void ACWNativeWorldActor::RenderState(const FCWRoomSnapshot& State)
{
    EmitStateDrivenFx(State);
    UpdateArenaSurface(State);

    PlayerInstances->ClearInstances();
    EnemyInstances->ClearInstances();
    BulletInstances->ClearInstances();
    RocketInstances->ClearInstances();
    DropInstances->ClearInstances();
    XpVacuumInstances->ClearInstances();
    XpOrbInstances->ClearInstances();

    for (const FCWPlayerSnapshot& Player : State.Players)
    {
        const float Radius = Player.bIsCompanion ? 20.0f : 28.0f;
        PlayerInstances->AddInstance(MakeCylinderTransform(Player.X, Player.Y, Radius, Player.bAlive ? 120.0f : 28.0f));
    }

    for (const FCWEnemySnapshot& Enemy : State.Enemies)
    {
        EnemyInstances->AddInstance(MakeCylinderTransform(Enemy.X, Enemy.Y, FMath::Max(8.0f, Enemy.Radius), FMath::Max(40.0f, Enemy.Radius * 3.0f)));
    }

    for (const FCWBulletSnapshot& Bullet : State.Bullets)
    {
        const bool bRocket = Bullet.Kind.Equals(TEXT("rocket"), ESearchCase::IgnoreCase) || Bullet.FxKey.Equals(TEXT("projectile.rocket"), ESearchCase::IgnoreCase);
        if (bRocket && RocketInstances)
        {
            const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Bullet.Vy, Bullet.Vx));
            const FVector Scale(FMath::Max(0.22f, Bullet.Radius / 16.0f), 0.08f, 0.08f);
            RocketInstances->AddInstance(FTransform(FRotator(0.0f, AngleDeg, 0.0f), FVector(Bullet.X, Bullet.Y, ProjectileFxZ), Scale));
        }
        else
        {
            BulletInstances->AddInstance(MakeSphereTransform(Bullet.X, Bullet.Y, FMath::Max(3.0f, Bullet.Radius), 42.0f));
        }
    }

    for (const FCWPickupSnapshot& Drop : State.Drops)
    {
        if (Drop.Kind.Equals(TEXT("xp_vacuum"), ESearchCase::IgnoreCase))
        {
            const float PulseYaw = FMath::Fmod(GetWorld() ? GetWorld()->GetTimeSeconds() * 80.0f : 0.0f, 360.0f);
            XpVacuumInstances->AddInstance(FTransform(FRotator(0.0f, PulseYaw, 45.0f), FVector(Drop.X, Drop.Y, 42.0f), FVector(0.2f, 0.2f, 0.34f)));
        }
        else
        {
            const FVector Scale(0.22f, 0.22f, 0.08f);
            DropInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Drop.X, Drop.Y, 18.0f), Scale));
        }
    }

    for (const FCWPickupSnapshot& Orb : State.XpOrbs)
    {
        XpOrbInstances->AddInstance(MakeSphereTransform(Orb.X, Orb.Y, 9.0f, 24.0f));
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

    PreviousState = State;
    bHasPreviousState = true;
}

void ACWNativeWorldActor::EmitStateDrivenFx(const FCWRoomSnapshot& State)
{
    TMap<FString, float> NextSkillCooldowns;
    TMap<FString, FCWBulletSnapshot> CurrentRocketBullets;

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
                SpawnRocketExplosionFx(Pair.Value);
            }
        }
    }

    PreviousSkillCooldowns = MoveTemp(NextSkillCooldowns);
    LastRocketBullets = MoveTemp(CurrentRocketBullets);
}

UMaterialInstanceDynamic* ACWNativeWorldActor::MakeFxMaterial(const FString& Name, const FLinearColor& Color, float EmissiveStrength)
{
    return MakeColorMaterial(Name, Color, EmissiveStrength);
}

UStaticMeshComponent* ACWNativeWorldActor::SpawnFxMesh(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& StartScale, const FVector& EndScale, const FLinearColor& Color, float Life, float EmissiveStrength, float SpinYawDegPerSec)
{
    if (!Mesh || Life <= KINDA_SMALL_NUMBER)
    {
        return nullptr;
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

    if (UMaterialInstanceDynamic* Material = MakeFxMaterial(FString::Printf(TEXT("RuntimeFx_%d"), ActiveFxComponents.Num()), Color, EmissiveStrength))
    {
        Component->SetMaterial(0, Material);
    }

    FCWActiveFxComponent Fx;
    Fx.Component = Component;
    Fx.Age = 0.0f;
    Fx.Life = Life;
    Fx.StartScale = StartScale;
    Fx.EndScale = EndScale;
    Fx.SpinYawDegPerSec = SpinYawDegPerSec;
    ActiveFxComponents.Add(Fx);
    return Component;
}

void ACWNativeWorldActor::SpawnRingFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, float Height, float EmissiveStrength)
{
    const float SafeRadius = FMath::Max(4.0f, Radius);
    const FVector StartScale((SafeRadius * 0.3f * 2.0f) / BasicMeshSizeCm, (SafeRadius * 0.3f * 2.0f) / BasicMeshSizeCm, FMath::Max(1.0f, Height) / BasicMeshSizeCm);
    const FVector EndScale((SafeRadius * 2.0f) / BasicMeshSizeCm, (SafeRadius * 2.0f) / BasicMeshSizeCm, FMath::Max(1.0f, Height) / BasicMeshSizeCm);
    SpawnFxMesh(CylinderMeshAsset, Location, FRotator::ZeroRotator, StartScale, EndScale, Color, Life, EmissiveStrength, 0.0f);
}

void ACWNativeWorldActor::SpawnBurstFx(const FVector& Location, float Radius, const FLinearColor& Color, float Life, int32 SparkCount)
{
    const float SafeRadius = FMath::Max(8.0f, Radius);
    SpawnRingFx(Location, SafeRadius, Color, Life, 3.0f, 3.4f);
    SpawnFxMesh(SphereMeshAsset, Location + FVector(0.0f, 0.0f, SafeRadius * 0.18f), FRotator::ZeroRotator, FVector(0.08f), FVector(SafeRadius / 48.0f), Color, Life * 0.72f, 4.0f, 0.0f);

    const int32 Count = FMath::Clamp(SparkCount, 0, 32);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Angle = (TWO_PI * static_cast<float>(Index)) / FMath::Max(1, Count);
        const FVector Start = Location + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * SafeRadius * 0.18f + FVector(0.0f, 0.0f, 20.0f);
        const FVector End = Location + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * SafeRadius * (0.72f + (Index % 3) * 0.1f) + FVector(0.0f, 0.0f, 24.0f);
        SpawnBeamFx(Start, End, 6.0f + (Index % 3) * 2.0f, Color, Life * 0.62f, 3.0f);
    }
}

void ACWNativeWorldActor::SpawnBeamFx(const FVector& Start, const FVector& End, float Width, const FLinearColor& Color, float Life, float EmissiveStrength)
{
    const FVector Delta = End - Start;
    const float Length = Delta.Size2D();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Mid = (Start + End) * 0.5f;
    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    const float SafeWidth = FMath::Max(1.5f, Width);
    const FVector Scale(Length / BasicMeshSizeCm, SafeWidth / BasicMeshSizeCm, SafeWidth / BasicMeshSizeCm);
    SpawnFxMesh(CubeMeshAsset, Mid, FRotator(0.0f, Yaw, 0.0f), Scale, Scale * FVector(1.0f, 0.55f, 0.55f), Color, Life, EmissiveStrength, 0.0f);
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
            if (Links >= 7)
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
            if (Beams >= 6)
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
        for (int32 Index = 0; Index < 5; ++Index)
        {
            const float A = Player.AimX != Player.X || Player.AimY != Player.Y
                ? FMath::Atan2(Player.AimY - Player.Y, Player.AimX - Player.X) + (Index - 2) * 0.18f
                : Index * TWO_PI / 5.0f;
            SpawnBeamFx(Origin, Origin + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * 120.0f + FVector(0.0f, 0.0f, 30.0f), 9.0f, Primary, 0.28f, 4.2f);
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
        SpawnBurstFx(Start, 58.0f, Color, 0.32f, 9);
        SpawnBeamFx(Start - Dir * 44.0f + FVector(0.0f, 0.0f, 4.0f), Start + Dir * 20.0f, 14.0f, Color, 0.22f, 4.8f);
    }
    else
    {
        SpawnBeamFx(Start - Dir * 38.0f, Start + Dir * 66.0f, 5.0f, Color, 0.16f, 3.5f);
    }
}

void ACWNativeWorldActor::SpawnRocketExplosionFx(const FCWBulletSnapshot& Bullet)
{
    const FLinearColor Color = ResolveFxColor(Bullet.Color, FLinearColor(1.0f, 0.36f, 0.08f, 1.0f));
    const float Radius = FMath::Max(90.0f, Bullet.ExplosionRadius);
    const FVector Origin = MakeWorldLocation(Bullet.X, Bullet.Y, GroundFxZ + 24.0f);
    SpawnBurstFx(Origin, Radius, Color, 0.62f, 22);
    SpawnRingFx(Origin + FVector(0.0f, 0.0f, 8.0f), Radius * 1.25f, FLinearColor(1.0f, 0.78f, 0.2f, 1.0f), 0.72f, 5.0f, 4.0f);
}

void ACWNativeWorldActor::SpawnObjectImpactFx(const FCWObjectImpactEventSnapshot& Event)
{
    const FLinearColor Color = Event.Material.Equals(TEXT("metal"), ESearchCase::IgnoreCase)
        ? FLinearColor(1.0f, 0.82f, 0.18f, 1.0f)
        : FLinearColor(0.76f, 0.82f, 0.9f, 1.0f);
    const FVector Origin = MakeWorldLocation(Event.X, Event.Y, GroundFxZ + 24.0f);
    SpawnBurstFx(Origin, FMath::Clamp(Event.Damage * 0.18f + 34.0f, 34.0f, 120.0f), Color, 0.28f, 8);
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
        if (Links >= 28)
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
        for (const FCWSkillFxTarget& Target : Event.Targets)
        {
            const FVector TargetLocation = MakeWorldLocation(Target.X, Target.Y, ProjectileFxZ + 18.0f);
            SpawnBeamFx(Origin + FVector(0.0f, 0.0f, 34.0f), TargetLocation, BeamWidth, Primary, Life, 5.8f);
            SpawnBurstFx(TargetLocation, BurstRadius, Secondary, Life * 1.15f, 9);
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
        const int32 Count = FMath::Clamp(Event.ProjectileCount > 0 ? Event.ProjectileCount : FMath::Max(3, Event.Targets.Num()), 3, 12);
        SpawnBurstFx(Origin, 132.0f, Primary, 0.48f, 16);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float A = AimAngle + ((static_cast<float>(Index) - (Count - 1) * 0.5f) * 0.22f);
            SpawnBeamFx(Origin, Origin + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * 190.0f + FVector(0.0f, 0.0f, 54.0f), 13.0f, Primary, 0.44f, 5.8f);
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
    SpawnFxMesh(SphereMeshAsset, Tail, FRotator::ZeroRotator, FVector(0.06f), FVector(0.18f), Color, 0.42f, 2.2f, 0.0f);
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

    GroundMaterial = MakeColorMaterial(TEXT("ArenaGroundMaterial"), FLinearColor(0.006f, 0.011f, 0.017f, 1.0f));
    GridMaterial = MakeColorMaterial(TEXT("ArenaGridMaterial"), FLinearColor(0.04f, 0.22f, 0.30f, 1.0f), 0.18f);
    PlayerMaterial = MakeColorMaterial(TEXT("PlayerMaterial"), FLinearColor(0.05f, 0.82f, 1.0f, 1.0f), 0.7f);
    EnemyMaterial = MakeColorMaterial(TEXT("EnemyMaterial"), FLinearColor(1.0f, 0.12f, 0.08f, 1.0f), 0.35f);
    BulletMaterial = MakeColorMaterial(TEXT("BulletMaterial"), FLinearColor(1.0f, 0.78f, 0.24f, 1.0f), 0.8f);
    RocketMaterial = MakeColorMaterial(TEXT("RocketMaterial"), FLinearColor(1.0f, 0.44f, 0.12f, 1.0f), 1.8f);
    DropMaterial = MakeColorMaterial(TEXT("DropMaterial"), FLinearColor(0.78f, 0.28f, 1.0f, 1.0f), 0.35f);
    XpVacuumMaterial = MakeColorMaterial(TEXT("XpVacuumMaterial"), FLinearColor(0.65f, 0.18f, 1.0f, 1.0f), 2.2f);
    XpOrbMaterial = MakeColorMaterial(TEXT("XpOrbMaterial"), FLinearColor(0.28f, 1.0f, 0.48f, 1.0f), 0.75f);

    if (GroundMesh && GroundMaterial)
    {
        GroundMesh->SetMaterial(0, BlackMaterial ? BlackMaterial : GroundMaterial);
    }
    if (GridLineInstances && GridMaterial)
    {
        GridLineInstances->SetMaterial(0, GridMaterial);
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
        BulletInstances->SetMaterial(0, EmissiveFallbackMaterial ? EmissiveFallbackMaterial : BulletMaterial);
    }
    if (RocketInstances && RocketMaterial)
    {
        RocketInstances->SetMaterial(0, RocketMaterial);
    }
    if (DropInstances && DropMaterial)
    {
        DropInstances->SetMaterial(0, DropMaterial);
    }
    if (XpVacuumInstances && XpVacuumMaterial)
    {
        XpVacuumInstances->SetMaterial(0, XpVacuumMaterial);
    }
    if (XpOrbInstances && XpOrbMaterial)
    {
        XpOrbInstances->SetMaterial(0, XpOrbMaterial);
    }

    FCWRoomSnapshot DefaultState;
    DefaultState.World.Width = DefaultArenaWidth;
    DefaultState.World.Height = DefaultArenaHeight;
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
        const bool bTemplateVisual = Actor->IsA<AStaticMeshActor>()
            || ActorName.Contains(TEXT("Floor"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("Plane"), ESearchCase::IgnoreCase)
            || ActorName.Contains(TEXT("Landscape"), ESearchCase::IgnoreCase)
            || ClassName.Contains(TEXT("Landscape"), ESearchCase::IgnoreCase);
        if (!bTemplateVisual)
        {
            continue;
        }

        Actor->SetActorHiddenInGame(true);
        Actor->SetActorEnableCollision(false);
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
}

void ACWNativeWorldActor::UpdateArenaSurface(const FCWRoomSnapshot& State)
{
    const float Width = FMath::Max(DefaultArenaWidth, State.World.Width);
    const float Height = FMath::Max(DefaultArenaHeight, State.World.Height);
    const FVector Center(Width * 0.5f, Height * 0.5f, GroundZ + 4.0f);

    if (GroundMesh)
    {
        GroundMesh->SetRelativeLocation(Center);
        GroundMesh->SetRelativeScale3D(FVector((Width + 1000.0f) / BasicMeshSizeCm, (Height + 1000.0f) / BasicMeshSizeCm, 0.08f));
    }

    const FVector2D GridSize(Width, Height);
    if (!LastGridSize.Equals(GridSize, 1.0f))
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

    GridLineInstances->ClearInstances();

    const float MinX = 0.0f;
    const float MaxX = Width;
    const float MinY = 0.0f;
    const float MaxY = Height;
    const float CenterX = Width * 0.5f;
    const float CenterY = Height * 0.5f;
    const float Z = GroundZ + 2.0f;

    for (float X = MinX; X <= MaxX + 1.0f; X += GridStep)
    {
        const FVector Scale(0.018f, Height / BasicMeshSizeCm, 0.018f);
        GridLineInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, CenterY, Z), Scale));
    }

    for (float Y = MinY; Y <= MaxY + 1.0f; Y += GridStep)
    {
        const FVector Scale(Width / BasicMeshSizeCm, 0.018f, 0.018f);
        GridLineInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(CenterX, Y, Z), Scale));
    }
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
