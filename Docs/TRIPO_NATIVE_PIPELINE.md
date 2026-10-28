# Tripo3D Native Unreal Pipeline

This project renders gameplay in Unreal with real meshes for characters and map props. Web sprites can still exist as fallback/UI assets, but native gameplay should prefer:

- `USkeletalMeshComponent` for heroes and monsters.
- `UStaticMeshComponent` for map objects and environment props.
- `UAnimSequence`/future montages for idle, walk, attack, hit-react, cast, and death.

## Import Tripo Map Objects

Source FBX exports live in:

```powershell
C:\Projects\crimson-wars-native\Content\Objects
```

Import them into stable Unreal asset paths with:

```powershell
.\Scripts\import-tripo-map-objects.ps1
```

Or explicitly:

```powershell
.\Scripts\import-tripo-map-objects.ps1 -EngineRoot "C:\UE_5.7"
```

The runtime loader expects these assets:

```text
/Game/CrimsonWars/MapObjects/Tripo3D/bus_yellow/SM_bus_yellow
/Game/CrimsonWars/MapObjects/Tripo3D/clinic_block/SM_clinic_block
/Game/CrimsonWars/MapObjects/Tripo3D/barrier/SM_barrier
/Game/CrimsonWars/MapObjects/Tripo3D/tree/SM_tree
/Game/CrimsonWars/MapObjects/Tripo3D/industrial_tank/SM_industrial_tank
/Game/CrimsonWars/MapObjects/Tripo3D/ambulance/SM_ambulance
/Game/CrimsonWars/MapObjects/Tripo3D/mall_block/SM_mall_block
/Game/CrimsonWars/MapObjects/Tripo3D/build_2/SM_build_2
/Game/CrimsonWars/MapObjects/Tripo3D/build_5/SM_build_5
/Game/CrimsonWars/MapObjects/Tripo3D/shopping_cart_barricade/SM_shopping_cart_barricade
/Game/CrimsonWars/MapObjects/Tripo3D/reactor_block/SM_reactor_block
```

`ACWNativeWorldActor` maps server `spriteKey` values to those meshes and spawns them as native `UStaticMeshComponent`s. Trees are rendered with a native instanced static mesh component.

## Character Animation Slots

Existing Tripo character imports already provide walk/death sequences. The native runtime now also looks for optional idle and attack sequences. Add new animations at these paths and they will be picked up automatically:

```text
/Game/Characters/Heroes/<hero>/Anims/A_<hero>_Idle
/Game/Characters/Heroes/<hero>/Anims/A_<hero>_Attack
/Game/Characters/Heroes/<hero>/Anims/A_<hero>_Attack_01
/Game/Characters/Heroes/<hero>/Anims/A_<hero>_Melee

/Game/Characters/Monsters/<mob>/Anims/A_<mob>_Idle
/Game/Characters/Monsters/<mob>/Anims/A_<mob>_Attack
/Game/Characters/Monsters/<mob>/Anims/A_<mob>_Attack_01
/Game/Characters/Monsters/<mob>/Anims/A_<mob>_Melee
```

Recommended production flow:

1. Generate or rig the model in Tripo3D/3DAI Studio.
2. Clean and validate skeleton orientation in Blender.
3. Use Mixamo or Unreal IK Retargeter for humanoid idle/walk/run/attack/death cycles.
4. Import the animation FBX onto the existing hero/monster skeleton.
5. Name the imported sequences with the paths above.

For punchier top-down combat, use short attack clips with exaggerated anticipation, clear weapon arcs, and `AnimNotify` markers for VFX/audio timing once we move from single-node playback to montages.
