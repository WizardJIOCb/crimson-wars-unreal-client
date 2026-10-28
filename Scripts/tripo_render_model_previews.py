import math
import sys
from pathlib import Path

import bpy
import mathutils


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()


def look_at(obj, target):
    direction = mathutils.Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


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


def setup_render():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    if hasattr(scene, "eevee"):
        scene.eevee.taa_render_samples = 32
    scene.render.resolution_x = 512
    scene.render.resolution_y = 512
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("PreviewWorld") if not scene.world else scene.world
    scene.world.color = (0.78, 0.78, 0.78)

    light_data = bpy.data.lights.new("KeyLight", "AREA")
    light_data.energy = 550
    light_data.size = 4.0
    light = bpy.data.objects.new("KeyLight", light_data)
    bpy.context.collection.objects.link(light)
    light.location = (2.5, -4.0, 5.5)

    fill_data = bpy.data.lights.new("FillLight", "POINT")
    fill_data.energy = 90
    fill = bpy.data.objects.new("FillLight", fill_data)
    bpy.context.collection.objects.link(fill)
    fill.location = (-3.0, 3.0, 3.0)

    cam_data = bpy.data.cameras.new("Camera")
    cam = bpy.data.objects.new("Camera", cam_data)
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    cam.data.lens = 55
    return cam


def render_one(fbx_path, out_path):
    reset_scene()
    bpy.ops.import_scene.fbx(filepath=str(fbx_path), automatic_bone_orientation=True)

    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        return False

    for obj in meshes:
        obj.rotation_euler[2] += math.radians(18)

    cam = setup_render()
    mins, maxs = scene_bounds(meshes)
    center = (mins + maxs) / 2
    size = max((maxs - mins).x, (maxs - mins).y, (maxs - mins).z)
    height = (maxs - mins).z
    dist = max(size * 2.1, 3.0)
    cam.location = (center.x, center.y - dist, center.z + height * 0.06)
    look_at(cam, center)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.context.scene.render.filepath = str(out_path)
    bpy.ops.render.render(write_still=True)
    return True


def main():
    args = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else sys.argv[1:]
    models_dir = Path(args[0])
    out_dir = Path(args[1])

    for fbx in sorted(models_dir.glob("*/*.fbx")):
        out_path = out_dir / f"{fbx.parent.name}.png"
        try:
            render_one(fbx, out_path)
        except Exception as exc:
            print(f"FAILED {fbx}: {exc}")


if __name__ == "__main__":
    main()
