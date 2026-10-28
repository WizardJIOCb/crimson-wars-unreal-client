import json
import math
import shutil
import sys
from pathlib import Path

import bpy
import mathutils


MAPPING = {
    "normal": {
        "name": "Ash Walker",
        "sourceFolder": "undead+zombie+3d+model",
    },
    "runner": {
        "name": "Red Sprinter",
        "sourceFolder": "zombie+creature+3d+model (1)",
    },
    "charger": {
        "name": "Amber Charger",
        "sourceFolder": "lava+golem+3d+model",
    },
    "ranged": {
        "name": "Rose Spitter",
        "sourceFolder": "zombie+monster+3d+model",
    },
    "brute": {
        "name": "Iron Brute",
        "sourceFolder": "armored+humanoid+3d+model",
    },
    "splitter": {
        "name": "Violet Splitter",
        "sourceFolder": "monster+3d+model",
    },
    "medic": {
        "name": "Green Stitcher",
        "sourceFolder": "zombie+3d+model",
    },
    "sniper": {
        "name": "Blue Needle",
        "sourceFolder": "ice+monster+3d+model",
    },
    "exploder": {
        "name": "Gold Burster",
        "sourceFolder": "zombie+creature+3d+model",
    },
    "shield": {
        "name": "Bone Bulwark",
        "sourceFolder": "fantasy+creature+3d+model",
    },
    "boss": {
        "name": "Crimson Behemoth",
        "sourceFolder": "armored+demon+3d+model",
    },
    "boss_hellmart": {
        "name": "Hellmart Butcher",
        "sourceFolder": "fiery+demon+3d+model",
    },
    "boss_chief_surgeon": {
        "name": "Chief Surgeon",
        "sourceFolder": "hazmat+suit+3d+model",
    },
    "boss_road_titan": {
        "name": "Road Rage Titan",
        "sourceFolder": "lava+demon+3d+model",
    },
    "boss_reactor_apostle": {
        "name": "Reactor Apostle",
        "sourceFolder": "biohazard+armored+monster+3d+model",
    },
}

DEFORM_BONES = [
    "pelvis",
    "spine_01",
    "spine_02",
    "neck_01",
    "head",
    "clavicle_l",
    "upperarm_l",
    "lowerarm_l",
    "hand_l",
    "clavicle_r",
    "upperarm_r",
    "lowerarm_r",
    "hand_r",
    "thigh_l",
    "calf_l",
    "foot_l",
    "thigh_r",
    "calf_r",
    "foot_r",
]


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)
    for armature in list(bpy.data.armatures):
        if armature.users == 0:
            bpy.data.armatures.remove(armature)
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for material in list(bpy.data.materials):
        if material.users == 0:
            bpy.data.materials.remove(material)
    for image in list(bpy.data.images):
        if image.users == 0:
            bpy.data.images.remove(image)


def scene_bounds(objects):
    mins = mathutils.Vector((math.inf, math.inf, math.inf))
    maxs = mathutils.Vector((-math.inf, -math.inf, -math.inf))
    for obj in objects:
        for corner in obj.bound_box:
            p = obj.matrix_world @ mathutils.Vector(corner)
            mins.x = min(mins.x, p.x)
            mins.y = min(mins.y, p.y)
            mins.z = min(mins.z, p.z)
            maxs.x = max(maxs.x, p.x)
            maxs.y = max(maxs.y, p.y)
            maxs.z = max(maxs.z, p.z)
    return mins, maxs


def make_bone(edit_bones, name, head, tail, parent=None, connected=False):
    bone = edit_bones.new(name)
    bone.head = head
    bone.tail = tail
    if parent:
        bone.parent = parent
        bone.use_connect = connected
    bone.use_deform = name in DEFORM_BONES
    return bone


def create_armature(asset_id, meshes):
    mins, maxs = scene_bounds(meshes)
    size = maxs - mins
    center = (mins + maxs) / 2
    h = max(size.z, 0.1)
    w = max(size.y, 0.1)
    d = max(size.x, 0.1)

    z = lambda n: mins.z + h * n
    y = lambda n: center.y + w * n
    x = lambda n: center.x + d * n

    arm_data = bpy.data.armatures.new(f"SK_{asset_id}")
    arm_obj = bpy.data.objects.new(f"SK_{asset_id}", arm_data)
    bpy.context.collection.objects.link(arm_obj)
    bpy.context.view_layer.objects.active = arm_obj
    arm_obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm_data.edit_bones

    root = make_bone(eb, "root", (center.x, center.y, mins.z), (center.x, center.y, z(0.08)))
    pelvis = make_bone(eb, "pelvis", (center.x, center.y, z(0.42)), (center.x, center.y, z(0.52)), root)
    spine_01 = make_bone(eb, "spine_01", pelvis.tail, (center.x, center.y, z(0.64)), pelvis, True)
    spine_02 = make_bone(eb, "spine_02", spine_01.tail, (center.x, center.y, z(0.78)), spine_01, True)
    neck = make_bone(eb, "neck_01", spine_02.tail, (center.x, center.y, z(0.88)), spine_02, True)
    make_bone(eb, "head", neck.tail, (center.x, center.y, z(0.98)), neck, True)

    for side, suffix in ((1, "l"), (-1, "r")):
        clavicle = make_bone(
            eb,
            f"clavicle_{suffix}",
            (center.x, center.y, z(0.75)),
            (x(0.02), y(side * 0.23), z(0.74)),
            spine_02,
        )
        upper = make_bone(
            eb,
            f"upperarm_{suffix}",
            clavicle.tail,
            (x(0.02), y(side * 0.38), z(0.58)),
            clavicle,
            True,
        )
        lower = make_bone(
            eb,
            f"lowerarm_{suffix}",
            upper.tail,
            (x(0.01), y(side * 0.36), z(0.40)),
            upper,
            True,
        )
        make_bone(
            eb,
            f"hand_{suffix}",
            lower.tail,
            (x(0.00), y(side * 0.33), z(0.29)),
            lower,
            True,
        )

        thigh = make_bone(
            eb,
            f"thigh_{suffix}",
            (center.x, y(side * 0.12), z(0.44)),
            (center.x, y(side * 0.12), z(0.26)),
            pelvis,
        )
        calf = make_bone(
            eb,
            f"calf_{suffix}",
            thigh.tail,
            (center.x, y(side * 0.11), z(0.09)),
            thigh,
            True,
        )
        make_bone(
            eb,
            f"foot_{suffix}",
            calf.tail,
            (x(0.16), y(side * 0.11), z(0.03)),
            calf,
            True,
        )

    bpy.ops.object.mode_set(mode="OBJECT")
    arm_obj.show_in_front = True
    return arm_obj, mins, maxs


def assign_rigid_weights(meshes, arm_obj, mins, maxs):
    center = (mins + maxs) / 2
    size = maxs - mins
    h = max(size.z, 0.1)
    w = max(size.y, 0.1)

    for obj in meshes:
        obj.select_set(False)
        groups = {name: obj.vertex_groups.new(name=name) for name in DEFORM_BONES}
        for vertex in obj.data.vertices:
            p = obj.matrix_world @ vertex.co
            nz = (p.z - mins.z) / h
            yoff = p.y - center.y
            abs_y = abs(yoff)
            side = "l" if yoff >= 0 else "r"

            if nz > 0.88:
                group_name = "head"
            elif abs_y > w * 0.22 and 0.24 < nz < 0.82:
                if nz < 0.38 or abs_y > w * 0.36:
                    group_name = f"hand_{side}"
                elif nz < 0.56:
                    group_name = f"lowerarm_{side}"
                elif nz < 0.72:
                    group_name = f"upperarm_{side}"
                else:
                    group_name = f"clavicle_{side}"
            elif nz < 0.46:
                if nz < 0.12:
                    group_name = f"foot_{side}"
                elif nz < 0.28:
                    group_name = f"calf_{side}"
                else:
                    group_name = f"thigh_{side}"
            elif nz < 0.60:
                group_name = "pelvis"
            elif nz < 0.76:
                group_name = "spine_01"
            elif nz < 0.86:
                group_name = "spine_02"
            else:
                group_name = "neck_01"

            groups[group_name].add([vertex.index], 1.0, "REPLACE")

        modifier = obj.modifiers.new("Armature", "ARMATURE")
        modifier.object = arm_obj
        obj.parent = arm_obj


def assign_weights(meshes, arm_obj, mins, maxs):
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        obj.select_set(True)
    arm_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm_obj

    try:
        bpy.ops.object.parent_set(type="ARMATURE_AUTO")
        if all(any(mod.type == "ARMATURE" for mod in obj.modifiers) for obj in meshes):
            return "auto"
    except Exception as exc:
        print(f"Auto weights failed, using rigid fallback: {exc}")

    for obj in meshes:
        obj.parent = None
        for mod in list(obj.modifiers):
            if mod.type == "ARMATURE":
                obj.modifiers.remove(mod)
        for group in list(obj.vertex_groups):
            obj.vertex_groups.remove(group)
    assign_rigid_weights(meshes, arm_obj, mins, maxs)
    return "rigid"


def key_pose(arm_obj, frame, rotations=None, locations=None):
    bpy.context.scene.frame_set(frame)
    rotations = rotations or {}
    locations = locations or {}
    for pb in arm_obj.pose.bones:
        pb.rotation_mode = "XYZ"
        pb.rotation_euler = (0.0, 0.0, 0.0)
        pb.location = (0.0, 0.0, 0.0)

    for bone_name, rot in rotations.items():
        if bone_name in arm_obj.pose.bones:
            arm_obj.pose.bones[bone_name].rotation_euler = rot

    for bone_name, loc in locations.items():
        if bone_name in arm_obj.pose.bones:
            arm_obj.pose.bones[bone_name].location = loc

    for pb in arm_obj.pose.bones:
        pb.keyframe_insert(data_path="rotation_euler", frame=frame)
        pb.keyframe_insert(data_path="location", frame=frame)


def make_action(arm_obj, action_name, frame_range, keyframes):
    action = bpy.data.actions.new(action_name)
    arm_obj.animation_data_create()
    arm_obj.animation_data.action = action
    bpy.context.scene.frame_start = frame_range[0]
    bpy.context.scene.frame_end = frame_range[1]

    for frame, rotations, locations in keyframes:
        key_pose(arm_obj, frame, rotations, locations)

    if hasattr(action, "fcurves"):
        for fc in action.fcurves:
            for kp in fc.keyframe_points:
                kp.interpolation = "SINE"
    return action


def create_actions(asset_id, arm_obj, height):
    walk_name = f"M_{asset_id}_Walk"
    death_name = f"M_{asset_id}_Death"
    step = math.radians(36)
    arm = math.radians(30)
    bend = math.radians(20)
    bob = height * 0.04
    sink = height * 0.36

    make_action(
        arm_obj,
        walk_name,
        (1, 24),
        [
            (
                1,
                {
                    "pelvis": (0, 0, math.radians(4)),
                    "spine_01": (math.radians(-5), 0, math.radians(-5)),
                    "thigh_l": (0, step, 0),
                    "calf_l": (0, -bend, 0),
                    "foot_l": (0, math.radians(12), 0),
                    "thigh_r": (0, -step, 0),
                    "calf_r": (0, bend, 0),
                    "foot_r": (0, math.radians(-10), 0),
                    "upperarm_l": (0, -arm, 0),
                    "lowerarm_l": (0, math.radians(12), 0),
                    "upperarm_r": (0, arm, 0),
                    "lowerarm_r": (0, math.radians(-12), 0),
                },
                {"root": (0, 0, 0)},
            ),
            (
                12,
                {
                    "pelvis": (0, 0, math.radians(-4)),
                    "spine_01": (math.radians(-5), 0, math.radians(5)),
                    "thigh_l": (0, -step, 0),
                    "calf_l": (0, bend, 0),
                    "foot_l": (0, math.radians(-10), 0),
                    "thigh_r": (0, step, 0),
                    "calf_r": (0, -bend, 0),
                    "foot_r": (0, math.radians(12), 0),
                    "upperarm_l": (0, arm, 0),
                    "lowerarm_l": (0, math.radians(-12), 0),
                    "upperarm_r": (0, -arm, 0),
                    "lowerarm_r": (0, math.radians(12), 0),
                },
                {"root": (0, 0, bob)},
            ),
            (
                24,
                {
                    "pelvis": (0, 0, math.radians(4)),
                    "spine_01": (math.radians(-5), 0, math.radians(-5)),
                    "thigh_l": (0, step, 0),
                    "calf_l": (0, -bend, 0),
                    "foot_l": (0, math.radians(12), 0),
                    "thigh_r": (0, -step, 0),
                    "calf_r": (0, bend, 0),
                    "foot_r": (0, math.radians(-10), 0),
                    "upperarm_l": (0, -arm, 0),
                    "lowerarm_l": (0, math.radians(12), 0),
                    "upperarm_r": (0, arm, 0),
                    "lowerarm_r": (0, math.radians(-12), 0),
                },
                {"root": (0, 0, 0)},
            ),
        ],
    )

    make_action(
        arm_obj,
        death_name,
        (1, 70),
        [
            (1, {}, {"root": (0, 0, 0)}),
            (
                16,
                {
                    "spine_01": (0, math.radians(8), 0),
                    "spine_02": (0, math.radians(6), 0),
                    "head": (0, math.radians(-8), 0),
                    "thigh_l": (0, math.radians(8), 0),
                    "thigh_r": (0, math.radians(-8), 0),
                },
                {"root": (0, 0, bob * 0.35)},
            ),
            (
                42,
                {
                    "root": (0, math.radians(-88), 0),
                    "pelvis": (0, math.radians(-8), 0),
                    "spine_01": (0, math.radians(-10), 0),
                    "spine_02": (0, math.radians(-6), 0),
                    "head": (0, math.radians(16), 0),
                    "thigh_l": (math.radians(8), math.radians(18), math.radians(10)),
                    "thigh_r": (math.radians(-6), math.radians(10), math.radians(-14)),
                    "calf_l": (0, math.radians(-18), 0),
                    "calf_r": (0, math.radians(-14), 0),
                    "upperarm_l": (math.radians(18), math.radians(6), math.radians(64)),
                    "upperarm_r": (math.radians(-18), math.radians(-4), math.radians(-58)),
                    "lowerarm_l": (0, math.radians(10), math.radians(20)),
                    "lowerarm_r": (0, math.radians(8), math.radians(-18)),
                },
                {"root": (0, 0, -sink)},
            ),
            (
                70,
                {
                    "root": (0, math.radians(-88), 0),
                    "pelvis": (0, math.radians(-8), 0),
                    "spine_01": (0, math.radians(-10), 0),
                    "spine_02": (0, math.radians(-6), 0),
                    "head": (0, math.radians(16), 0),
                    "thigh_l": (math.radians(8), math.radians(18), math.radians(10)),
                    "thigh_r": (math.radians(-6), math.radians(10), math.radians(-14)),
                    "calf_l": (0, math.radians(-18), 0),
                    "calf_r": (0, math.radians(-14), 0),
                    "upperarm_l": (math.radians(18), math.radians(6), math.radians(64)),
                    "upperarm_r": (math.radians(-18), math.radians(-4), math.radians(-58)),
                    "lowerarm_l": (0, math.radians(10), math.radians(20)),
                    "lowerarm_r": (0, math.radians(8), math.radians(-18)),
                },
                {"root": (0, 0, -sink)},
            ),
        ],
    )
    return [walk_name, death_name]


def copy_texture_folder(source_fbx, out_dir):
    fbm = source_fbx.parent / f"{source_fbx.stem}.fbm"
    if fbm.exists():
        target = out_dir / fbm.name
        if target.exists():
            shutil.rmtree(target)
        shutil.copytree(fbm, target)


def process_asset(asset_id, info, models_dir, out_dir):
    source_dir = models_dir / info["sourceFolder"]
    source_fbx = next(source_dir.glob("*.fbx"))

    reset_scene()
    bpy.ops.import_scene.fbx(filepath=str(source_fbx), automatic_bone_orientation=True)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"No mesh in {source_fbx}")

    for obj in meshes:
        obj.name = f"SM_{asset_id}"
        obj.data.name = f"SM_{asset_id}_Mesh"

    arm_obj, mins, maxs = create_armature(asset_id, meshes)
    weight_mode = assign_weights(meshes, arm_obj, mins, maxs)
    action_names = create_actions(asset_id, arm_obj, maxs.z - mins.z)

    asset_out = out_dir / asset_id
    asset_out.mkdir(parents=True, exist_ok=True)
    out_fbx = asset_out / f"SK_{asset_id}_walk_death.fbx"
    copy_texture_folder(source_fbx, asset_out)

    bpy.ops.object.select_all(action="DESELECT")
    arm_obj.select_set(True)
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = arm_obj

    bpy.ops.export_scene.fbx(
        filepath=str(out_fbx),
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        add_leaf_bones=False,
        bake_anim=True,
        bake_anim_use_all_actions=True,
        bake_anim_force_startend_keying=True,
        path_mode="COPY",
        embed_textures=False,
    )

    return {
        "mobId": asset_id,
        "name": info["name"],
        "sourceFolder": info["sourceFolder"],
        "sourceFbx": str(source_fbx),
        "riggedFbx": str(out_fbx),
        "animations": action_names,
        "bones": len(arm_obj.data.bones),
        "meshCount": len(meshes),
        "weightMode": weight_mode,
    }


def main():
    args = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else sys.argv[1:]
    models_dir = Path(args[0])
    out_dir = Path(args[1])
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest = []
    for asset_id, info in MAPPING.items():
        print(f"Rigging {asset_id}: {info['sourceFolder']}")
        manifest.append(process_asset(asset_id, info, models_dir, out_dir))

    (out_dir / "monster_rig_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
