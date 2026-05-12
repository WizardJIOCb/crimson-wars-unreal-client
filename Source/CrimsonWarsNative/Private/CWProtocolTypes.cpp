#include "CWProtocolTypes.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace CWJson
{
    FString ValueAsString(const TSharedPtr<FJsonValue>& Value, const FString& DefaultValue = TEXT(""))
    {
        if (!Value.IsValid())
        {
            return DefaultValue;
        }

        if (Value->Type == EJson::String)
        {
            return Value->AsString();
        }

        if (Value->Type == EJson::Number)
        {
            return FString::SanitizeFloat(Value->AsNumber());
        }

        if (Value->Type == EJson::Boolean)
        {
            return Value->AsBool() ? TEXT("true") : TEXT("false");
        }

        return DefaultValue;
    }

    double ValueAsNumber(const TSharedPtr<FJsonValue>& Value, double DefaultValue = 0.0)
    {
        if (!Value.IsValid())
        {
            return DefaultValue;
        }

        if (Value->Type == EJson::Number)
        {
            return Value->AsNumber();
        }

        if (Value->Type == EJson::String)
        {
            return FCString::Atod(*Value->AsString());
        }

        if (Value->Type == EJson::Boolean)
        {
            return Value->AsBool() ? 1.0 : 0.0;
        }

        return DefaultValue;
    }

    bool ValueAsBool(const TSharedPtr<FJsonValue>& Value, bool bDefaultValue = false)
    {
        if (!Value.IsValid())
        {
            return bDefaultValue;
        }

        if (Value->Type == EJson::Boolean)
        {
            return Value->AsBool();
        }

        if (Value->Type == EJson::Number)
        {
            return FMath::Abs(Value->AsNumber()) > KINDA_SMALL_NUMBER;
        }

        if (Value->Type == EJson::String)
        {
            const FString Text = Value->AsString().ToLower();
            return Text == TEXT("true") || Text == TEXT("1") || Text == TEXT("yes");
        }

        return bDefaultValue;
    }

    FString GetString(const TSharedPtr<FJsonObject>& Obj, const FString& Field, const FString& DefaultValue = TEXT(""))
    {
        if (!Obj.IsValid())
        {
            return DefaultValue;
        }

        const TSharedPtr<FJsonValue>* Value = Obj->Values.Find(Field);
        return Value ? ValueAsString(*Value, DefaultValue) : DefaultValue;
    }

    double GetNumber(const TSharedPtr<FJsonObject>& Obj, const FString& Field, double DefaultValue = 0.0)
    {
        if (!Obj.IsValid())
        {
            return DefaultValue;
        }

        const TSharedPtr<FJsonValue>* Value = Obj->Values.Find(Field);
        return Value ? ValueAsNumber(*Value, DefaultValue) : DefaultValue;
    }

    bool GetBool(const TSharedPtr<FJsonObject>& Obj, const FString& Field, bool bDefaultValue = false)
    {
        if (!Obj.IsValid())
        {
            return bDefaultValue;
        }

        const TSharedPtr<FJsonValue>* Value = Obj->Values.Find(Field);
        return Value ? ValueAsBool(*Value, bDefaultValue) : bDefaultValue;
    }

    TSharedPtr<FJsonObject> GetObject(const TSharedPtr<FJsonObject>& Obj, const FString& Field)
    {
        if (!Obj.IsValid())
        {
            return nullptr;
        }

        const TSharedPtr<FJsonValue>* Value = Obj->Values.Find(Field);
        if (!Value || !Value->IsValid() || (*Value)->Type != EJson::Object)
        {
            return nullptr;
        }

        return (*Value)->AsObject();
    }

    const TArray<TSharedPtr<FJsonValue>>* GetArray(const TSharedPtr<FJsonObject>& Obj, const FString& Field)
    {
        if (!Obj.IsValid())
        {
            return nullptr;
        }

        const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
        if (Obj->TryGetArrayField(Field, Array))
        {
            return Array;
        }

        return nullptr;
    }

    FString ArrayString(const TArray<TSharedPtr<FJsonValue>>& Array, int32 Index, const FString& DefaultValue = TEXT(""))
    {
        return Array.IsValidIndex(Index) ? ValueAsString(Array[Index], DefaultValue) : DefaultValue;
    }

    double ArrayNumber(const TArray<TSharedPtr<FJsonValue>>& Array, int32 Index, double DefaultValue = 0.0)
    {
        return Array.IsValidIndex(Index) ? ValueAsNumber(Array[Index], DefaultValue) : DefaultValue;
    }

    bool ArrayBool(const TArray<TSharedPtr<FJsonValue>>& Array, int32 Index, bool bDefaultValue = false)
    {
        return Array.IsValidIndex(Index) ? ValueAsBool(Array[Index], bDefaultValue) : bDefaultValue;
    }
}

namespace
{
    FString ReadFxKey(const TSharedPtr<FJsonObject>& Obj)
    {
        FString FxKey = CWJson::GetString(Obj, TEXT("fxKey"));
        if (!FxKey.IsEmpty())
        {
            return FxKey;
        }

        const TSharedPtr<FJsonObject> FxObj = CWJson::GetObject(Obj, TEXT("fx"));
        return CWJson::GetString(FxObj, TEXT("key"));
    }
}

bool FCWProtocolParser::ParseStatePayload(const TSharedPtr<FJsonObject>& Payload, FCWRoomSnapshot& OutState)
{
    if (!Payload.IsValid())
    {
        return false;
    }

    OutState = FCWRoomSnapshot();
    OutState.RoomCode = CWJson::GetString(Payload, TEXT("roomCode"));
    OutState.MapId = CWJson::GetString(Payload, TEXT("mapId"));
    OutState.GameMode = CWJson::GetString(Payload, TEXT("gameMode"));
    OutState.RoomStartedAt = CWJson::GetNumber(Payload, TEXT("roomStartedAt"));
    OutState.MatchEndsAt = CWJson::GetNumber(Payload, TEXT("matchEndsAt"));
    OutState.ServerNowMs = CWJson::GetNumber(Payload, TEXT("now"));
    OutState.SpectatorCount = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Payload, TEXT("spectatorCount"), CWJson::GetNumber(Payload, TEXT("spectators")))));
    OutState.TotalEnemyKills = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Payload, TEXT("totalEnemyKills"))));
    OutState.NextBossAtKills = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Payload, TEXT("nextBossAtKills"))));
    OutState.NextBossSpawnAt = CWJson::GetNumber(Payload, TEXT("nextBossSpawnAt"));
    OutState.bBossAlive = CWJson::GetBool(Payload, TEXT("bossAlive"));

    if (const TSharedPtr<FJsonObject> DifficultyObj = CWJson::GetObject(Payload, TEXT("roomDifficulty")))
    {
        OutState.RoomDifficulty.Level = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(DifficultyObj, TEXT("level"), 1.0)));
        OutState.RoomDifficulty.HpMul = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(DifficultyObj, TEXT("hpMul"), 1.0)));
        OutState.RoomDifficulty.SpeedMul = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(DifficultyObj, TEXT("speedMul"), 1.0)));
        OutState.RoomDifficulty.DamageMul = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(DifficultyObj, TEXT("damageMul"), 1.0)));
        OutState.RoomDifficulty.AttackRateMul = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(DifficultyObj, TEXT("attackRateMul"), 1.0)));
        OutState.RoomDifficulty.SpawnIntervalMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(DifficultyObj, TEXT("spawnIntervalMs"))));
    }

    const TSharedPtr<FJsonObject> WorldObj = CWJson::GetObject(Payload, TEXT("world"));
    OutState.World.Width = static_cast<float>(CWJson::GetNumber(WorldObj, TEXT("width")));
    OutState.World.Height = static_cast<float>(CWJson::GetNumber(WorldObj, TEXT("height")));

    if (const TArray<TSharedPtr<FJsonValue>>* Players = CWJson::GetArray(Payload, TEXT("players")))
    {
        OutState.Players.Reserve(Players->Num());
        for (const TSharedPtr<FJsonValue>& Value : *Players)
        {
            if (Value.IsValid() && Value->Type == EJson::Object)
            {
                OutState.Players.Add(ParsePlayerObject(Value->AsObject()));
            }
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* Enemies = CWJson::GetArray(Payload, TEXT("enemies")))
    {
        OutState.Enemies.Reserve(Enemies->Num());
        for (const TSharedPtr<FJsonValue>& Value : *Enemies)
        {
            OutState.Enemies.Add(ParseEnemyValue(Value));
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* Bullets = CWJson::GetArray(Payload, TEXT("bullets")))
    {
        OutState.Bullets.Reserve(Bullets->Num());
        for (const TSharedPtr<FJsonValue>& Value : *Bullets)
        {
            OutState.Bullets.Add(ParseBulletValue(Value));
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* ShotEvents = CWJson::GetArray(Payload, TEXT("shotEvents")))
    {
        OutState.ShotEvents.Reserve(ShotEvents->Num());
        for (const TSharedPtr<FJsonValue>& Value : *ShotEvents)
        {
            if (Value.IsValid() && Value->Type == EJson::Object)
            {
                OutState.ShotEvents.Add(ParseShotEventObject(Value->AsObject()));
            }
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* ObjectImpactEvents = CWJson::GetArray(Payload, TEXT("objectImpactEvents")))
    {
        OutState.ObjectImpactEvents.Reserve(ObjectImpactEvents->Num());
        for (const TSharedPtr<FJsonValue>& Value : *ObjectImpactEvents)
        {
            if (Value.IsValid() && Value->Type == EJson::Object)
            {
                OutState.ObjectImpactEvents.Add(ParseObjectImpactEventObject(Value->AsObject()));
            }
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* BossPortals = CWJson::GetArray(Payload, TEXT("bossPortals")))
    {
        OutState.BossPortals.Reserve(BossPortals->Num());
        for (const TSharedPtr<FJsonValue>& Value : *BossPortals)
        {
            if (Value.IsValid() && Value->Type == EJson::Object)
            {
                OutState.BossPortals.Add(ParseBossPortalObject(Value->AsObject()));
            }
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* Drops = CWJson::GetArray(Payload, TEXT("drops")))
    {
        OutState.bHasDropsPayload = true;
        OutState.Drops.Reserve(Drops->Num());
        for (const TSharedPtr<FJsonValue>& Value : *Drops)
        {
            OutState.Drops.Add(ParseDropValue(Value));
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* XpOrbs = CWJson::GetArray(Payload, TEXT("xpOrbs")))
    {
        OutState.bHasXpOrbsPayload = true;
        OutState.XpOrbs.Reserve(XpOrbs->Num());
        for (const TSharedPtr<FJsonValue>& Value : *XpOrbs)
        {
            OutState.XpOrbs.Add(ParseXpOrbValue(Value));
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* SkillOrbs = CWJson::GetArray(Payload, TEXT("skillOrbs")))
    {
        OutState.bHasSkillOrbsPayload = true;
        OutState.SkillOrbs.Reserve(SkillOrbs->Num());
        for (const TSharedPtr<FJsonValue>& Value : *SkillOrbs)
        {
            OutState.SkillOrbs.Add(ParseSkillOrbValue(Value));
        }
    }

    if (const TSharedPtr<FJsonObject> RealtimeObj = CWJson::GetObject(Payload, TEXT("realtime")))
    {
        OutState.DropsVersion = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(RealtimeObj, TEXT("dropsVersion"))));
        OutState.XpOrbsVersion = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(RealtimeObj, TEXT("xpOrbsVersion"))));
        OutState.SkillOrbsVersion = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(RealtimeObj, TEXT("skillOrbsVersion"))));
    }

    const TSharedPtr<FJsonObject> DecorObj = CWJson::GetObject(Payload, TEXT("decor"));
    if (DecorObj.IsValid())
    {
        OutState.MapObjectsVersion = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(DecorObj, TEXT("objectsVersion"))));

        if (const TSharedPtr<FJsonObject> ThemeObj = CWJson::GetObject(DecorObj, TEXT("theme")))
        {
            OutState.SceneTheme.ThemeId = CWJson::GetString(ThemeObj, TEXT("themeId"));
            OutState.SceneTheme.BaseMaterial = CWJson::GetString(ThemeObj, TEXT("baseMaterial"), TEXT("asphalt_wet"));
            OutState.SceneTheme.Accent = CWJson::GetString(ThemeObj, TEXT("accent"), TEXT("#22c55e"));
        }

        if (const TArray<TSharedPtr<FJsonValue>>* Trees = CWJson::GetArray(DecorObj, TEXT("trees")))
        {
            OutState.Trees.Reserve(Trees->Num());
            for (const TSharedPtr<FJsonValue>& Value : *Trees)
            {
                OutState.Trees.Add(ParseTreeValue(Value));
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* TerrainZones = CWJson::GetArray(DecorObj, TEXT("terrainZones")))
        {
            OutState.TerrainZones.Reserve(TerrainZones->Num());
            for (const TSharedPtr<FJsonValue>& Value : *TerrainZones)
            {
                if (Value.IsValid() && Value->Type == EJson::Object)
                {
                    OutState.TerrainZones.Add(ParseTerrainZoneObject(Value->AsObject()));
                }
            }
        }

        if (const TArray<TSharedPtr<FJsonValue>>* Objects = CWJson::GetArray(DecorObj, TEXT("objects")))
        {
            OutState.MapObjects.Reserve(Objects->Num());
            for (const TSharedPtr<FJsonValue>& Value : *Objects)
            {
                if (Value.IsValid() && Value->Type == EJson::Object)
                {
                    OutState.MapObjects.Add(ParseMapObjectObject(Value->AsObject()));
                }
            }
        }
    }

    return true;
}

FCWPlayerSnapshot FCWProtocolParser::ParsePlayerObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWPlayerSnapshot Player;
    Player.Id = CWJson::GetString(Obj, TEXT("id"));
    Player.Name = CWJson::GetString(Obj, TEXT("name"));
    Player.PlayerClass = CWJson::GetString(Obj, TEXT("playerClass"), TEXT("cyber"));
    Player.WeaponKey = CWJson::GetString(Obj, TEXT("weaponKey"), TEXT("pistol"));
    Player.WeaponLabel = CWJson::GetString(Obj, TEXT("weaponLabel"), Player.WeaponKey);
    Player.VisualLoadout = ParseVisualLoadoutObject(CWJson::GetObject(Obj, TEXT("visualLoadout")), Player.PlayerClass, Player.WeaponKey);
    Player.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
    Player.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
    Player.AimX = static_cast<float>(CWJson::GetNumber(Obj, TEXT("aimX"), Player.X));
    Player.AimY = static_cast<float>(CWJson::GetNumber(Obj, TEXT("aimY"), Player.Y));
    Player.Hp = static_cast<float>(CWJson::GetNumber(Obj, TEXT("hp")));
    Player.MaxHp = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("maxHp"), 1.0)));
    Player.Level = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("level"), 1.0)));
    Player.Xp = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("xp"))));
    Player.XpToNext = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("xpToNext"), 1.0)));
    Player.PickupRadius = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("pickupRadius"))));
    Player.MagazineAmmo = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("magazineAmmo"))));
    Player.MagazineSize = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("magazineSize"), 1.0)));
    Player.ReserveAmmo = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("reserveAmmo"), CWJson::GetNumber(Obj, TEXT("ammo")))));
    Player.ReloadLeftMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("reloadLeftMs"))));
    Player.ReloadTotalMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("reloadTotalMs"))));
    Player.DodgeCharges = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("dodgeCharges"))));
    Player.DodgeChargesMax = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("dodgeChargesMax"), 1.0)));
    Player.DodgeRechargeMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("dodgeRechargeMs"))));
    Player.DodgeRechargeTotalMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("dodgeRechargeTotalMs"))));
    Player.DodgeInvulnUntil = CWJson::GetNumber(Obj, TEXT("dodgeInvulnUntil"));
    Player.NetPingMs = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("netPingMs"))));
    Player.LastProcessedInputSeq = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("lastProcessedInputSeq"))));
    Player.bAlive = CWJson::GetBool(Obj, TEXT("alive"), true);
    Player.bShooting = CWJson::GetBool(Obj, TEXT("shooting"));
    Player.bIsCompanion = CWJson::GetBool(Obj, TEXT("isCompanion"));
    if (const TSharedPtr<FJsonObject> MeleeObj = CWJson::GetObject(Obj, TEXT("melee")))
    {
        Player.Melee = ParseMeleeObject(MeleeObj);
        Player.bHasMelee = !Player.Melee.ItemId.IsEmpty() || !Player.Melee.Style.IsEmpty();
    }

    if (const TArray<TSharedPtr<FJsonValue>>* Skills = CWJson::GetArray(Obj, TEXT("skills")))
    {
        Player.Skills.Reserve(Skills->Num());
        for (const TSharedPtr<FJsonValue>& Value : *Skills)
        {
            const FCWSkillSnapshot Skill = ParseSkillValue(Value);
            if (!Skill.Id.IsEmpty() && Skill.Level > 0)
            {
                Player.Skills.Add(Skill);
            }
        }
    }

    if (const TArray<TSharedPtr<FJsonValue>>* QuickSlots = CWJson::GetArray(Obj, TEXT("quickSlots")))
    {
        Player.QuickSlots.Reserve(QuickSlots->Num());
        for (const TSharedPtr<FJsonValue>& Value : *QuickSlots)
        {
            if (Value.IsValid() && Value->Type == EJson::Object)
            {
                Player.QuickSlots.Add(ParseQuickSlotObject(Value->AsObject()));
            }
        }
    }

    return Player;
}

FCWVisualLoadoutSnapshot FCWProtocolParser::ParseVisualLoadoutObject(const TSharedPtr<FJsonObject>& Obj, const FString& FallbackHeroId, const FString& FallbackWeaponKey)
{
    FCWVisualLoadoutSnapshot Loadout;
    Loadout.HeroId = FallbackHeroId.IsEmpty() ? TEXT("cyber") : FallbackHeroId;
    Loadout.BodyMesh = FString::Printf(TEXT("heroes/%s/body"), *Loadout.HeroId);
    Loadout.WeaponKey = FallbackWeaponKey.IsEmpty() ? TEXT("pistol") : FallbackWeaponKey;

    if (!Obj.IsValid())
    {
        return Loadout;
    }

    Loadout.HeroId = CWJson::GetString(Obj, TEXT("heroId"), Loadout.HeroId);
    Loadout.BodyMesh = CWJson::GetString(Obj, TEXT("bodyMesh"), Loadout.BodyMesh);
    Loadout.Skin = CWJson::GetString(Obj, TEXT("skin"), TEXT("default"));
    Loadout.WeaponKey = CWJson::GetString(Obj, TEXT("weaponKey"), Loadout.WeaponKey);

    const TSharedPtr<FJsonObject> SlotsObj = CWJson::GetObject(Obj, TEXT("slots"));
    if (!SlotsObj.IsValid())
    {
        return Loadout;
    }

    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : SlotsObj->Values)
    {
        if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Object)
        {
            continue;
        }

        const TSharedPtr<FJsonObject> SlotObj = Pair.Value->AsObject();
        FCWVisualSlotSnapshot Slot;
        Slot.ItemId = CWJson::GetString(SlotObj, TEXT("itemId"));
        Slot.AssetId = CWJson::GetString(SlotObj, TEXT("assetId"));
        Slot.SlotCategory = CWJson::GetString(SlotObj, TEXT("slotCategory"));
        Slot.Rarity = CWJson::GetString(SlotObj, TEXT("rarity"));
        Slot.Icon = CWJson::GetString(SlotObj, TEXT("icon"));
        Slot.Level = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(SlotObj, TEXT("level"), 1.0)));
        Loadout.Slots.Add(Pair.Key, Slot);
    }

    return Loadout;
}

FCWMeleeSnapshot FCWProtocolParser::ParseMeleeObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWMeleeSnapshot Melee;
    if (!Obj.IsValid())
    {
        return Melee;
    }

    Melee.ItemId = CWJson::GetString(Obj, TEXT("itemId"));
    Melee.Name = CWJson::GetString(Obj, TEXT("name"), Melee.ItemId);
    Melee.SkillName = CWJson::GetString(Obj, TEXT("skillName"));
    Melee.Style = CWJson::GetString(Obj, TEXT("style"), TEXT("slash"));
    Melee.FxKey = ReadFxKey(Obj);
    Melee.Color = CWJson::GetString(Obj, TEXT("color"));
    Melee.SecondaryColor = CWJson::GetString(Obj, TEXT("secondaryColor"));
    Melee.Level = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("level"), 1.0)));
    Melee.Damage = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("damage"))));
    Melee.Range = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("range"))));
    Melee.Width = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("width"))));
    Melee.ArcDeg = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("arcDeg"))));
    Melee.CooldownMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("cooldownMs"))));
    Melee.CooldownLeftMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("cooldownLeftMs"))));
    return Melee;
}

FCWSkillSnapshot FCWProtocolParser::ParseSkillValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWSkillSnapshot Skill;

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Skill.Id = CWJson::ArrayString(A, 0);
        Skill.Level = FMath::Max(0, static_cast<int32>(CWJson::ArrayNumber(A, 1)));
        Skill.CooldownMs = static_cast<float>(FMath::Max(0.0, CWJson::ArrayNumber(A, 2)));
        Skill.MaxCooldownMs = static_cast<float>(FMath::Max(0.0, CWJson::ArrayNumber(A, 3)));
        Skill.Kind = CWJson::ArrayString(A, 4, TEXT("passive"));
        Skill.Rarity = CWJson::ArrayString(A, 5, TEXT("common"));
        Skill.Name = CWJson::ArrayString(A, 6, Skill.Id);
        Skill.CastType = CWJson::ArrayString(A, 7);
        Skill.FxKey = CWJson::ArrayString(A, 8);
        return Skill;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Skill.Id = CWJson::GetString(Obj, TEXT("id"));
        Skill.Name = CWJson::GetString(Obj, TEXT("name"), Skill.Id);
        Skill.Kind = CWJson::GetString(Obj, TEXT("kind"), TEXT("passive"));
        Skill.CastType = CWJson::GetString(Obj, TEXT("castType"));
        Skill.FxKey = ReadFxKey(Obj);
        Skill.Rarity = CWJson::GetString(Obj, TEXT("rarity"), TEXT("common"));
        Skill.Desc = CWJson::GetString(Obj, TEXT("desc"));
        Skill.Level = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("level"))));
        Skill.CooldownMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("cooldownMs"))));
        Skill.MaxCooldownMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("maxCooldownMs"))));
    }

    return Skill;
}

FCWQuickSlotSnapshot FCWProtocolParser::ParseQuickSlotObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWQuickSlotSnapshot Slot;
    Slot.SlotKey = CWJson::GetString(Obj, TEXT("slotKey"));
    Slot.ItemId = CWJson::GetString(Obj, TEXT("itemId"));
    Slot.Name = CWJson::GetString(Obj, TEXT("name"), Slot.ItemId);
    Slot.Rarity = CWJson::GetString(Obj, TEXT("rarity"), TEXT("common"));
    Slot.Hotkey = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("hotkey"))));
    Slot.Quantity = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("quantity"))));
    Slot.Level = FMath::Max(1, static_cast<int32>(CWJson::GetNumber(Obj, TEXT("level"), 1.0)));
    Slot.bEmpty = CWJson::GetBool(Obj, TEXT("empty"), Slot.ItemId.IsEmpty());
    return Slot;
}

FCWEnemySnapshot FCWProtocolParser::ParseEnemyValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWEnemySnapshot Enemy;

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Enemy.Id = CWJson::ArrayString(A, 0);
        Enemy.Type = CWJson::ArrayString(A, 1, TEXT("normal"));
        Enemy.X = static_cast<float>(CWJson::ArrayNumber(A, 2));
        Enemy.Y = static_cast<float>(CWJson::ArrayNumber(A, 3));
        Enemy.Hp = static_cast<float>(CWJson::ArrayNumber(A, 4));
        Enemy.MaxHp = static_cast<float>(FMath::Max(1.0, CWJson::ArrayNumber(A, 5, 1.0)));
        Enemy.Radius = static_cast<float>(FMath::Max(1.0, CWJson::ArrayNumber(A, 6, 18.0)));
        Enemy.bFaceLeft = CWJson::ArrayBool(A, 7);
        Enemy.MobId = CWJson::ArrayString(A, 8, Enemy.Type);
        Enemy.Behavior = CWJson::ArrayString(A, 9);
        Enemy.Color = CWJson::ArrayString(A, 10);
        Enemy.SpriteScale = static_cast<float>(CWJson::ArrayNumber(A, 11, 1.0));
        Enemy.Name = CWJson::ArrayString(A, 12);
        Enemy.ExplosionRadius = static_cast<float>(FMath::Max(0.0, CWJson::ArrayNumber(A, 13)));
        return Enemy;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Enemy.Id = CWJson::GetString(Obj, TEXT("id"));
        Enemy.Type = CWJson::GetString(Obj, TEXT("type"), TEXT("normal"));
        Enemy.MobId = CWJson::GetString(Obj, TEXT("mobId"), Enemy.Type);
        Enemy.Behavior = CWJson::GetString(Obj, TEXT("behavior"));
        Enemy.Name = CWJson::GetString(Obj, TEXT("name"));
        Enemy.Color = CWJson::GetString(Obj, TEXT("color"));
        Enemy.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
        Enemy.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
        Enemy.Hp = static_cast<float>(CWJson::GetNumber(Obj, TEXT("hp")));
        Enemy.MaxHp = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("maxHp"), 1.0)));
        Enemy.Radius = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("radius"), 18.0)));
        Enemy.SpriteScale = static_cast<float>(CWJson::GetNumber(Obj, TEXT("spriteScale"), 1.0));
        Enemy.ExplosionRadius = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("explosionRadius"))));
        Enemy.bFaceLeft = CWJson::GetBool(Obj, TEXT("faceLeft"));
    }

    return Enemy;
}

FCWBulletSnapshot FCWProtocolParser::ParseBulletValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWBulletSnapshot Bullet;

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Bullet.Id = CWJson::ArrayString(A, 0);
        Bullet.OwnerId = CWJson::ArrayString(A, 1);
        Bullet.OwnerPlayerId = CWJson::ArrayString(A, 2);
        Bullet.WeaponKey = CWJson::ArrayString(A, 3);
        Bullet.X = static_cast<float>(CWJson::ArrayNumber(A, 4));
        Bullet.Y = static_cast<float>(CWJson::ArrayNumber(A, 5));
        Bullet.Vx = static_cast<float>(CWJson::ArrayNumber(A, 6));
        Bullet.Vy = static_cast<float>(CWJson::ArrayNumber(A, 7));
        Bullet.Color = CWJson::ArrayString(A, 8);
        Bullet.Kind = CWJson::ArrayString(A, 9, TEXT("bullet"));
        Bullet.Radius = static_cast<float>(FMath::Max(1.0, CWJson::ArrayNumber(A, 10, 4.0)));
        Bullet.bFromEnemy = CWJson::ArrayBool(A, 11);
        Bullet.ShooterType = CWJson::ArrayString(A, 12);
        Bullet.ExplosionRadius = static_cast<float>(FMath::Max(0.0, CWJson::ArrayNumber(A, 13)));
        return Bullet;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Bullet.Id = CWJson::GetString(Obj, TEXT("id"));
        Bullet.OwnerId = CWJson::GetString(Obj, TEXT("ownerId"));
        Bullet.OwnerPlayerId = CWJson::GetString(Obj, TEXT("ownerPlayerId"));
        Bullet.WeaponKey = CWJson::GetString(Obj, TEXT("weaponKey"));
        Bullet.Kind = CWJson::GetString(Obj, TEXT("kind"), TEXT("bullet"));
        Bullet.ShooterType = CWJson::GetString(Obj, TEXT("shooterType"));
        Bullet.Color = CWJson::GetString(Obj, TEXT("color"));
        Bullet.FxKey = ReadFxKey(Obj);
        Bullet.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
        Bullet.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
        Bullet.Vx = static_cast<float>(CWJson::GetNumber(Obj, TEXT("vx")));
        Bullet.Vy = static_cast<float>(CWJson::GetNumber(Obj, TEXT("vy")));
        Bullet.Radius = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("radius"), 4.0)));
        Bullet.ExplosionRadius = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("explosionRadius"))));
        Bullet.bFromEnemy = CWJson::GetBool(Obj, TEXT("fromEnemy"));
    }

    return Bullet;
}

FCWShotEventSnapshot FCWProtocolParser::ParseShotEventObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWShotEventSnapshot Event;
    Event.Id = CWJson::GetString(Obj, TEXT("id"));
    Event.BulletId = CWJson::GetString(Obj, TEXT("bulletId"));
    Event.OwnerId = CWJson::GetString(Obj, TEXT("ownerId"));
    Event.OwnerPlayerId = CWJson::GetString(Obj, TEXT("ownerPlayerId"));
    Event.ShooterType = CWJson::GetString(Obj, TEXT("shooterType"), TEXT("player"));
    Event.WeaponKey = CWJson::GetString(Obj, TEXT("weaponKey"), TEXT("pistol"));
    Event.Kind = CWJson::GetString(Obj, TEXT("kind"), TEXT("bullet"));
    Event.Color = CWJson::GetString(Obj, TEXT("color"), TEXT("#facc15"));
    Event.FxKey = ReadFxKey(Obj);
    Event.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
    Event.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
    Event.Vx = static_cast<float>(CWJson::GetNumber(Obj, TEXT("vx")));
    Event.Vy = static_cast<float>(CWJson::GetNumber(Obj, TEXT("vy")));
    Event.Radius = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("radius"), 3.0)));
    Event.ExplosionRadius = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("explosionRadius"))));
    Event.At = CWJson::GetNumber(Obj, TEXT("at"));
    return Event;
}

FCWObjectImpactEventSnapshot FCWProtocolParser::ParseObjectImpactEventObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWObjectImpactEventSnapshot Event;
    Event.Id = CWJson::GetString(Obj, TEXT("id"));
    Event.ObjectId = CWJson::GetString(Obj, TEXT("objectId"));
    Event.BulletId = CWJson::GetString(Obj, TEXT("bulletId"));
    Event.Kind = CWJson::GetString(Obj, TEXT("kind"));
    Event.SpriteKey = CWJson::GetString(Obj, TEXT("spriteKey"));
    Event.Material = CWJson::GetString(Obj, TEXT("material"), TEXT("concrete"));
    Event.BulletKind = CWJson::GetString(Obj, TEXT("bulletKind"), TEXT("bullet"));
    Event.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
    Event.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
    Event.DirX = static_cast<float>(CWJson::GetNumber(Obj, TEXT("dirX")));
    Event.DirY = static_cast<float>(CWJson::GetNumber(Obj, TEXT("dirY")));
    Event.NormalX = static_cast<float>(CWJson::GetNumber(Obj, TEXT("nx")));
    Event.NormalY = static_cast<float>(CWJson::GetNumber(Obj, TEXT("ny")));
    Event.Damage = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("damage"))));
    Event.At = CWJson::GetNumber(Obj, TEXT("at"));
    return Event;
}

FCWBossPortalSnapshot FCWProtocolParser::ParseBossPortalObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWBossPortalSnapshot Portal;
    Portal.Id = CWJson::GetString(Obj, TEXT("id"));
    Portal.MobId = CWJson::GetString(Obj, TEXT("mobId"));
    Portal.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
    Portal.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
    Portal.SpawnAt = CWJson::GetNumber(Obj, TEXT("spawnAt"));
    Portal.TtlMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("ttlMs"))));
    return Portal;
}

FCWPickupSnapshot FCWProtocolParser::ParseDropValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWPickupSnapshot Drop;

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Drop.Id = CWJson::ArrayString(A, 0);
        Drop.X = static_cast<float>(CWJson::ArrayNumber(A, 1));
        Drop.Y = static_cast<float>(CWJson::ArrayNumber(A, 2));
        Drop.Kind = CWJson::ArrayString(A, 3, TEXT("weapon"));
        Drop.ItemKey = CWJson::ArrayString(A, 4);
        Drop.TtlMs = static_cast<float>(CWJson::ArrayNumber(A, 5));
        return Drop;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Drop.Id = CWJson::GetString(Obj, TEXT("id"));
        Drop.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
        Drop.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
        Drop.Kind = CWJson::GetString(Obj, TEXT("kind"), TEXT("weapon"));
        Drop.ItemKey = CWJson::GetString(Obj, TEXT("weaponKey"));
        Drop.FxKey = ReadFxKey(Obj);
        Drop.TtlMs = static_cast<float>(CWJson::GetNumber(Obj, TEXT("ttlMs")));
    }

    return Drop;
}

FCWPickupSnapshot FCWProtocolParser::ParseXpOrbValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWPickupSnapshot Orb;
    Orb.Kind = TEXT("xp");

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Orb.Id = CWJson::ArrayString(A, 0);
        Orb.X = static_cast<float>(CWJson::ArrayNumber(A, 1));
        Orb.Y = static_cast<float>(CWJson::ArrayNumber(A, 2));
        Orb.TtlMs = static_cast<float>(CWJson::ArrayNumber(A, 3));
        return Orb;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Orb.Id = CWJson::GetString(Obj, TEXT("id"));
        Orb.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
        Orb.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
        Orb.Xp = static_cast<float>(CWJson::GetNumber(Obj, TEXT("xp")));
        Orb.TtlMs = static_cast<float>(CWJson::GetNumber(Obj, TEXT("ttlMs")));
        Orb.TtlMaxMs = static_cast<float>(CWJson::GetNumber(Obj, TEXT("ttlMaxMs")));
    }

    return Orb;
}

FCWPickupSnapshot FCWProtocolParser::ParseSkillOrbValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWPickupSnapshot Orb;
    Orb.Kind = TEXT("skill");

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Orb.Id = CWJson::ArrayString(A, 0);
        Orb.OwnerId = CWJson::ArrayString(A, 1);
        Orb.ItemKey = CWJson::ArrayString(A, 2);
        Orb.X = static_cast<float>(CWJson::ArrayNumber(A, 3));
        Orb.Y = static_cast<float>(CWJson::ArrayNumber(A, 4));
        Orb.TtlMs = static_cast<float>(CWJson::ArrayNumber(A, 5));
        Orb.TtlMaxMs = static_cast<float>(CWJson::ArrayNumber(A, 6));
        return Orb;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Orb.Id = CWJson::GetString(Obj, TEXT("id"));
        Orb.OwnerId = CWJson::GetString(Obj, TEXT("ownerId"));
        Orb.ItemKey = CWJson::GetString(Obj, TEXT("skillId"));
        Orb.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
        Orb.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
        Orb.TtlMs = static_cast<float>(CWJson::GetNumber(Obj, TEXT("ttlMs")));
        Orb.TtlMaxMs = static_cast<float>(CWJson::GetNumber(Obj, TEXT("ttlMaxMs")));
    }

    return Orb;
}

FCWTreeSnapshot FCWProtocolParser::ParseTreeValue(const TSharedPtr<FJsonValue>& Value)
{
    FCWTreeSnapshot Tree;

    if (Value.IsValid() && Value->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
        Tree.X = static_cast<float>(CWJson::ArrayNumber(A, 0));
        Tree.Y = static_cast<float>(CWJson::ArrayNumber(A, 1));
        Tree.Scale = static_cast<float>(FMath::Max(0.1, CWJson::ArrayNumber(A, 2, 1.0)));
        return Tree;
    }

    if (Value.IsValid() && Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject> Obj = Value->AsObject();
        Tree.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
        Tree.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
        Tree.Scale = static_cast<float>(FMath::Max(0.1, CWJson::GetNumber(Obj, TEXT("scale"), 1.0)));
    }

    return Tree;
}

FCWTerrainZoneSnapshot FCWProtocolParser::ParseTerrainZoneObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWTerrainZoneSnapshot Zone;
    Zone.Id = CWJson::GetString(Obj, TEXT("id"));
    Zone.Material = CWJson::GetString(Obj, TEXT("material"), TEXT("asphalt_wet"));
    Zone.Shape = CWJson::GetString(Obj, TEXT("shape"), TEXT("ellipse"));
    Zone.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
    Zone.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
    Zone.W = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("w"), 1.0)));
    Zone.H = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("h"), 1.0)));
    Zone.Alpha = static_cast<float>(FMath::Clamp(CWJson::GetNumber(Obj, TEXT("alpha"), 0.65), 0.0, 1.0));
    Zone.Feather = static_cast<float>(FMath::Clamp(CWJson::GetNumber(Obj, TEXT("feather"), 0.18), 0.0, 1.0));
    Zone.Angle = static_cast<float>(CWJson::GetNumber(Obj, TEXT("angle")));
    Zone.bCenterStripe = CWJson::GetBool(Obj, TEXT("centerStripe"));
    return Zone;
}

FCWMapObjectSnapshot FCWProtocolParser::ParseMapObjectObject(const TSharedPtr<FJsonObject>& Obj)
{
    FCWMapObjectSnapshot MapObject;
    MapObject.Id = CWJson::GetString(Obj, TEXT("id"));
    MapObject.Kind = CWJson::GetString(Obj, TEXT("kind"));
    MapObject.SpriteKey = CWJson::GetString(Obj, TEXT("spriteKey"));
    MapObject.CollisionShape = CWJson::GetString(Obj, TEXT("collisionShape"), TEXT("rect"));
    MapObject.X = static_cast<float>(CWJson::GetNumber(Obj, TEXT("x")));
    MapObject.Y = static_cast<float>(CWJson::GetNumber(Obj, TEXT("y")));
    MapObject.W = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("w"), 1.0)));
    MapObject.H = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("h"), 1.0)));
    MapObject.CollisionW = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("collisionW"), MapObject.W)));
    MapObject.CollisionH = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("collisionH"), MapObject.H)));
    MapObject.CollisionOffsetY = static_cast<float>(CWJson::GetNumber(Obj, TEXT("collisionOffsetY")));
    MapObject.Angle = static_cast<float>(CWJson::GetNumber(Obj, TEXT("angle")));
    MapObject.AnchorY = static_cast<float>(FMath::Clamp(CWJson::GetNumber(Obj, TEXT("anchorY"), 0.56), 0.0, 1.0));
    MapObject.ShadowScale = static_cast<float>(FMath::Max(0.05, CWJson::GetNumber(Obj, TEXT("shadowScale"), 1.0)));
    MapObject.Hp = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("hp"))));
    MapObject.MaxHp = static_cast<float>(FMath::Max(1.0, CWJson::GetNumber(Obj, TEXT("maxHp"), 1.0)));
    MapObject.LastHitAt = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(Obj, TEXT("lastHitAt"))));
    MapObject.bSolid = CWJson::GetBool(Obj, TEXT("solid"));
    MapObject.bDestructible = CWJson::GetBool(Obj, TEXT("destructible"));
    MapObject.bZombieBreakable = CWJson::GetBool(Obj, TEXT("zombieBreakable"));
    MapObject.bHideAfterDestroyed = CWJson::GetBool(Obj, TEXT("hideAfterDestroyed"));
    MapObject.bExplosive = CWJson::GetBool(Obj, TEXT("explosive"));
    MapObject.bDestroyed = CWJson::GetBool(Obj, TEXT("destroyed"));
    return MapObject;
}

bool FCWProtocolParser::ParseMeleeFxEvent(const TSharedPtr<FJsonObject>& EventObj, FCWMeleeFxEvent& OutEvent)
{
    if (!EventObj.IsValid())
    {
        return false;
    }

    OutEvent = FCWMeleeFxEvent();
    OutEvent.Id = CWJson::GetString(EventObj, TEXT("id"));
    OutEvent.PlayerId = CWJson::GetString(EventObj, TEXT("playerId"));
    OutEvent.ItemId = CWJson::GetString(EventObj, TEXT("itemId"));
    OutEvent.ItemName = CWJson::GetString(EventObj, TEXT("itemName"));
    OutEvent.SkillName = CWJson::GetString(EventObj, TEXT("skillName"));
    OutEvent.Style = CWJson::GetString(EventObj, TEXT("style"), TEXT("slash"));
    OutEvent.FxKey = ReadFxKey(EventObj);
    OutEvent.Color = CWJson::GetString(EventObj, TEXT("color"));
    OutEvent.SecondaryColor = CWJson::GetString(EventObj, TEXT("secondaryColor"));
    OutEvent.X = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("x")));
    OutEvent.Y = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("y")));
    OutEvent.ImpactX = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("impactX"), OutEvent.X));
    OutEvent.ImpactY = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("impactY"), OutEvent.Y));
    OutEvent.Angle = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("angle")));
    OutEvent.Range = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(EventObj, TEXT("range"))));
    OutEvent.Width = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(EventObj, TEXT("width"))));
    OutEvent.ArcDeg = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(EventObj, TEXT("arcDeg"))));
    OutEvent.Damage = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(EventObj, TEXT("damage"))));
    OutEvent.HitCount = FMath::Max(0, static_cast<int32>(CWJson::GetNumber(EventObj, TEXT("hitCount"))));
    return !OutEvent.Id.IsEmpty() || !OutEvent.PlayerId.IsEmpty();
}

bool FCWProtocolParser::ParseWorldFxEvent(const TSharedPtr<FJsonObject>& EventObj, FCWWorldFxEvent& OutEvent)
{
    if (!EventObj.IsValid())
    {
        return false;
    }

    OutEvent = FCWWorldFxEvent();
    OutEvent.Id = CWJson::GetString(EventObj, TEXT("id"));
    OutEvent.Kind = CWJson::GetString(EventObj, TEXT("kind"));
    OutEvent.PlayerId = CWJson::GetString(EventObj, TEXT("playerId"));
    OutEvent.FxKey = ReadFxKey(EventObj);
    OutEvent.Color = CWJson::GetString(EventObj, TEXT("color"));
    OutEvent.SecondaryColor = CWJson::GetString(EventObj, TEXT("secondaryColor"));
    OutEvent.X = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("x")));
    OutEvent.Y = static_cast<float>(CWJson::GetNumber(EventObj, TEXT("y")));
    OutEvent.Radius = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(EventObj, TEXT("radius"))));
    OutEvent.DurationMs = static_cast<float>(FMath::Max(0.0, CWJson::GetNumber(EventObj, TEXT("durationMs"))));
    return !OutEvent.Kind.IsEmpty() || !OutEvent.FxKey.IsEmpty();
}
