import json
import math
import os
import sys
from pathlib import Path

import bpy


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


def import_fbx(path):
    bpy.ops.import_scene.fbx(filepath=str(path), automatic_bone_orientation=True)


def bounds_for_objects(objects):
    mins = [math.inf, math.inf, math.inf]
    maxs = [-math.inf, -math.inf, -math.inf]
    for obj in objects:
        for corner in obj.bound_box:
            world = obj.matrix_world @ mathutils.Vector(corner)
            for i in range(3):
                mins[i] = min(mins[i], world[i])
                maxs[i] = max(maxs[i], world[i])
    return mins, maxs


def inspect_file(path):
    reset_scene()
    import_fbx(path)

    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    actions = list(bpy.data.actions)

    vertex_count = sum(len(obj.data.vertices) for obj in meshes)
    polygon_count = sum(len(obj.data.polygons) for obj in meshes)
    bone_count = sum(len(arm.data.bones) for arm in armatures)
    modifier_types = sorted({mod.type for obj in meshes for mod in obj.modifiers})
    mins, maxs = bounds_for_objects(meshes) if meshes else ([0, 0, 0], [0, 0, 0])
    dimensions = [round(maxs[i] - mins[i], 4) for i in range(3)]

    return {
        "folder": path.parent.name,
        "fbx": str(path),
        "meshes": len(meshes),
        "armatures": len(armatures),
        "bones": bone_count,
        "actions": [action.name for action in actions],
        "vertices": vertex_count,
        "polygons": polygon_count,
        "mesh_names": [obj.name for obj in meshes],
        "armature_names": [obj.name for obj in armatures],
        "modifier_types": modifier_types,
        "bounds_min": [round(v, 4) for v in mins],
        "bounds_max": [round(v, 4) for v in maxs],
        "dimensions": dimensions,
    }


def main():
    if len(sys.argv) < 2:
        raise SystemExit("Usage: blender --background --python tripo_inspect_models.py -- <models_dir> <out_json>")

    args = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else sys.argv[1:]
    models_dir = Path(args[0])
    out_json = Path(args[1])

    results = []
    for fbx in sorted(models_dir.glob("*/*.fbx")):
        try:
            results.append(inspect_file(fbx))
        except Exception as exc:
            results.append({"folder": fbx.parent.name, "fbx": str(fbx), "error": str(exc)})

    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    import mathutils

    main()
