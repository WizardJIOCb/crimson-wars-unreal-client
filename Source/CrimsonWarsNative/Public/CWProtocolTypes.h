#pragma once

#include "CoreMinimal.h"
#include "CWProtocolTypes.generated.h"

class FJsonObject;
class FJsonValue;

USTRUCT(BlueprintType)
struct FCWWorldSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Width = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Height = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWVisualSlotSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ItemId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString AssetId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString SlotCategory;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Rarity;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Level = 1;
};

USTRUCT(BlueprintType)
struct FCWVisualLoadoutSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString HeroId = TEXT("cyber");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString BodyMesh;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Skin = TEXT("default");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString WeaponKey = TEXT("pistol");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TMap<FString, FCWVisualSlotSnapshot> Slots;
};

USTRUCT(BlueprintType)
struct FCWSkillSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Kind = TEXT("passive");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString CastType;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Rarity = TEXT("common");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Desc;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Level = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float CooldownMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float MaxCooldownMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWMeleeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ItemId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString SkillName;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Style;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Color;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString SecondaryColor;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Level = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Damage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Range = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Width = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ArcDeg = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float CooldownMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float CooldownLeftMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWQuickSlotSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString SlotKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ItemId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Rarity = TEXT("common");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Hotkey = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Level = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bEmpty = true;
};

USTRUCT(BlueprintType)
struct FCWPlayerSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString PlayerClass;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString WeaponKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString WeaponLabel;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FCWVisualLoadoutSnapshot VisualLoadout;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float AimX = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float AimY = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Hp = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float MaxHp = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Level = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Xp = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 XpToNext = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float PickupRadius = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 MagazineAmmo = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 MagazineSize = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 ReserveAmmo = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ReloadLeftMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ReloadTotalMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 DodgeCharges = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 DodgeChargesMax = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float DodgeRechargeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float DodgeRechargeTotalMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double DodgeInvulnUntil = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 NetPingMs = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 LastProcessedInputSeq = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bAlive = true;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bShooting = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bIsCompanion = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bHasMelee = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FCWMeleeSnapshot Melee;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWSkillSnapshot> Skills;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWQuickSlotSnapshot> QuickSlots;
};

USTRUCT(BlueprintType)
struct FCWEnemySnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Type;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString MobId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Behavior;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Color;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Hp = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float MaxHp = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Radius = 18.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float SpriteScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ExplosionRadius = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bFaceLeft = false;
};

USTRUCT(BlueprintType)
struct FCWBulletSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString OwnerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString OwnerPlayerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString WeaponKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ShooterType;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Color;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Vx = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Vy = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Radius = 4.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ExplosionRadius = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bFromEnemy = false;
};

USTRUCT(BlueprintType)
struct FCWShotEventSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString BulletId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString OwnerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString OwnerPlayerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ShooterType = TEXT("player");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString WeaponKey = TEXT("pistol");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Kind = TEXT("bullet");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Color = TEXT("#facc15");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Vx = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Vy = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Radius = 3.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ExplosionRadius = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double At = 0.0;
};

USTRUCT(BlueprintType)
struct FCWObjectImpactEventSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ObjectId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString BulletId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString SpriteKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Material = TEXT("concrete");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString BulletKind = TEXT("bullet");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float DirX = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float DirY = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float NormalX = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float NormalY = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Damage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double At = 0.0;
};

USTRUCT(BlueprintType)
struct FCWBossPortalSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString MobId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double SpawnAt = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float TtlMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWPickupSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ItemKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString OwnerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float TtlMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float TtlMaxMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Xp = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWNativeFxProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Key;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Style;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Trigger;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString PrimaryColor;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString SecondaryColor;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString AccentColor;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString NiagaraSystem;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString GameplayCue;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString MaterialPreset;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString SpawnMode;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    TArray<FString> Layers;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Intensity = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Radius = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Range = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Width = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float ArcDeg = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float DurationMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWMeleeFxEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString PlayerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString ItemId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString ItemName;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString SkillName;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Style;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Color;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString SecondaryColor;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float ImpactX = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float ImpactY = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Angle = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Range = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Width = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float ArcDeg = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Damage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    int32 HitCount = 0;
};

USTRUCT(BlueprintType)
struct FCWWorldFxEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString PlayerId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString FxKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString Color;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    FString SecondaryColor;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float Radius = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars|FX")
    float DurationMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWTreeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Scale = 1.0f;
};

USTRUCT(BlueprintType)
struct FCWTerrainZoneSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Material;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Shape;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float W = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float H = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Alpha = 0.65f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Feather = 0.18f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Angle = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bCenterStripe = false;
};

USTRUCT(BlueprintType)
struct FCWMapObjectSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Kind;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString SpriteKey;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString CollisionShape;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float X = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Y = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float W = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float H = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float CollisionW = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float CollisionH = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float CollisionOffsetY = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Angle = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float AnchorY = 0.56f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float ShadowScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float Hp = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float MaxHp = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float LastHitAt = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bSolid = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bDestructible = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bZombieBreakable = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bHideAfterDestroyed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bExplosive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bDestroyed = false;
};

USTRUCT(BlueprintType)
struct FCWSceneThemeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString ThemeId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString BaseMaterial = TEXT("asphalt_wet");

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString Accent = TEXT("#22c55e");
};

USTRUCT(BlueprintType)
struct FCWRoomDifficultySnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 Level = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float HpMul = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float SpeedMul = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float DamageMul = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float AttackRateMul = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    float SpawnIntervalMs = 0.0f;
};

USTRUCT(BlueprintType)
struct FCWRoomSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString RoomCode;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString MapId;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FString GameMode;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double RoomStartedAt = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double MatchEndsAt = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double ServerNowMs = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FCWWorldSnapshot World;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWPlayerSnapshot> Players;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWEnemySnapshot> Enemies;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWBulletSnapshot> Bullets;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWShotEventSnapshot> ShotEvents;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWObjectImpactEventSnapshot> ObjectImpactEvents;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWBossPortalSnapshot> BossPortals;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWPickupSnapshot> Drops;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWPickupSnapshot> XpOrbs;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWPickupSnapshot> SkillOrbs;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 DropsVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 XpOrbsVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 SkillOrbsVersion = 0;

    bool bHasDropsPayload = false;
    bool bHasXpOrbsPayload = false;
    bool bHasSkillOrbsPayload = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWTreeSnapshot> Trees;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWTerrainZoneSnapshot> TerrainZones;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    TArray<FCWMapObjectSnapshot> MapObjects;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 MapObjectsVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 SpectatorCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 TotalEnemyKills = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    int32 NextBossAtKills = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    double NextBossSpawnAt = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    bool bBossAlive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FCWRoomDifficultySnapshot RoomDifficulty;

    UPROPERTY(BlueprintReadOnly, Category = "Crimson Wars")
    FCWSceneThemeSnapshot SceneTheme;
};

class CRIMSONWARSNATIVE_API FCWProtocolParser
{
public:
    static bool ParseStatePayload(const TSharedPtr<FJsonObject>& Payload, FCWRoomSnapshot& OutState);
    static bool ParseMeleeFxEvent(const TSharedPtr<FJsonObject>& EventObj, FCWMeleeFxEvent& OutEvent);
    static bool ParseWorldFxEvent(const TSharedPtr<FJsonObject>& EventObj, FCWWorldFxEvent& OutEvent);

private:
    static FCWPlayerSnapshot ParsePlayerObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWVisualLoadoutSnapshot ParseVisualLoadoutObject(const TSharedPtr<FJsonObject>& Obj, const FString& FallbackHeroId, const FString& FallbackWeaponKey);
    static FCWSkillSnapshot ParseSkillValue(const TSharedPtr<FJsonValue>& Value);
    static FCWMeleeSnapshot ParseMeleeObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWQuickSlotSnapshot ParseQuickSlotObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWEnemySnapshot ParseEnemyValue(const TSharedPtr<FJsonValue>& Value);
    static FCWBulletSnapshot ParseBulletValue(const TSharedPtr<FJsonValue>& Value);
    static FCWShotEventSnapshot ParseShotEventObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWObjectImpactEventSnapshot ParseObjectImpactEventObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWBossPortalSnapshot ParseBossPortalObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWPickupSnapshot ParseDropValue(const TSharedPtr<FJsonValue>& Value);
    static FCWPickupSnapshot ParseXpOrbValue(const TSharedPtr<FJsonValue>& Value);
    static FCWPickupSnapshot ParseSkillOrbValue(const TSharedPtr<FJsonValue>& Value);
    static FCWTreeSnapshot ParseTreeValue(const TSharedPtr<FJsonValue>& Value);
    static FCWTerrainZoneSnapshot ParseTerrainZoneObject(const TSharedPtr<FJsonObject>& Obj);
    static FCWMapObjectSnapshot ParseMapObjectObject(const TSharedPtr<FJsonObject>& Obj);
};
