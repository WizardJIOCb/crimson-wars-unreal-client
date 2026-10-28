import math
import sys
from pathlib import Path

import bpy
import mathutils


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


def setup_render(meshes):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    if hasattr(scene, "eevee"):
        scene.eevee.taa_render_samples = 32
    scene.render.resolution_x = 420
    scene.render.resolution_y = 420
    scene.world = bpy.data.worlds.new("PreviewWorld") if not scene.world else scene.world
    scene.world.color = (0.78, 0.78, 0.78)

    light_data = bpy.data.lights.new("KeyLight", "AREA")
    light_data.energy = 600
    light_data.size = 4.0
    light = bpy.data.objects.new("KeyLight", light_data)
    bpy.context.collection.objects.link(light)
    light.location = (2.5, -4.0, 5.5)

    cam_data = bpy.data.cameras.new("Camera")
    cam = bpy.data.objects.new("Camera", cam_data)
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    cam.data.lens = 58

    mins, maxs = scene_bounds(meshes)
    center = (mins + maxs) / 2
    size = max((maxs - mins).x, (maxs - mins).y, (maxs - mins).z)
    dist = max(size * 2.4, 3.0)
    cam.location = (center.x + size * 0.24, center.y - dist, center.z + size * 0.05)
    look_at(cam, center)


def apply_action(armature, action_keyword, fallback_frame):
    if not action_keyword:
        bpy.context.scene.frame_set(1)
        return
    action = next((a for a in bpy.data.actions if action_keyword in a.name), None)
    if not action:
        bpy.context.scene.frame_set(1)
        return
    armature.animation_data_create()
    armature.animation_data.action = action
    start, end = action.frame_range
    frame = fallback_frame if action_keyword == "Walk" else int(end)
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()


def render_pose(fbx, out_path, action_keyword, frame):
    reset_scene()
    bpy.ops.import_scene.fbx(filepath=str(fbx), automatic_bone_orientation=True)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    if not meshes or not armatures:
        return

    armature = armatures[0]
    apply_action(armature, action_keyword, frame)
    setup_render(meshes)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.context.scene.render.filepath = str(out_path)
    bpy.ops.render.render(write_still=True)


def main():
    args = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else sys.argv[1:]
    rigged_dir = Path(args[0])
    out_dir = Path(args[1])
    for fbx in sorted(rigged_dir.glob("*/*.fbx")):
        mob_id = fbx.parent.name
        render_pose(fbx, out_dir / f"{mob_id}_rest.png", None, 1)
        render_pose(fbx, out_dir / f"{mob_id}_walk.png", "Walk", 15)
        render_pose(fbx, out_dir / f"{mob_id}_death.png", "Death", 45)


if __name__ == "__main__":
    main()
