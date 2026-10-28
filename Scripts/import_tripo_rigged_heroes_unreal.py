import json
import os
import shutil

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MANIFEST_PATH = os.path.join(PROJECT_DIR, "Stuff", "Tripo3d", "RiggedHeroes", "hero_rig_manifest.json")
DEST_ROOT = "/Game/Characters/Heroes"
ASSET_SUFFIX = "_animfix"


def safe_set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception:
        return False


def make_import_ui():
    ui = unreal.FbxImportUI()
    safe_set(ui, "import_as_skeletal", True)
    safe_set(ui, "import_mesh", True)
    safe_set(ui, "import_animations", True)
    safe_set(ui, "import_materials", False)
    safe_set(ui, "import_textures", False)
    safe_set(ui, "automated_import_should_detect_type", False)
    safe_set(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

    if ui.skeletal_mesh_import_data:
        safe_set(ui.skeletal_mesh_import_data, "import_meshes_in_bone_hierarchy", True)
        safe_set(ui.skeletal_mesh_import_data, "convert_scene", True)
        safe_set(ui.skeletal_mesh_import_data, "convert_scene_unit", True)
        safe_set(ui.skeletal_mesh_import_data, "update_skeleton_reference_pose", True)

    if ui.anim_sequence_import_data:
        safe_set(ui.anim_sequence_import_data, "import_custom_attribute", True)
        safe_set(ui.anim_sequence_import_data, "animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)

    return ui


def import_one(asset_tools, item):
    hero_id = item["heroId"]
    source_fbx = item["riggedFbx"]
    fbx = os.path.join(os.path.dirname(source_fbx), f"SK_{hero_id}_walk_death{ASSET_SUFFIX}.fbx")
    if not os.path.exists(fbx) or os.path.getmtime(fbx) < os.path.getmtime(source_fbx):
        shutil.copy2(source_fbx, fbx)

    dest = f"{DEST_ROOT}/{hero_id}"
    unreal.EditorAssetLibrary.make_directory(dest)

    task = unreal.AssetImportTask()
    safe_set(task, "filename", fbx)
    safe_set(task, "destination_path", dest)
    safe_set(task, "destination_name", f"SK_{hero_id}_walk_death{ASSET_SUFFIX}")
    safe_set(task, "automated", True)
    safe_set(task, "save", True)
    safe_set(task, "replace_existing", False)
    safe_set(task, "options", make_import_ui())

    unreal.log(f"Importing hero {hero_id}: {fbx} -> {dest}")
    asset_tools.import_asset_tasks([task])
    return task


def main():
    if not os.path.exists(MANIFEST_PATH):
        raise RuntimeError(f"Missing manifest: {MANIFEST_PATH}")

    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        manifest = json.load(f)

    only_hero_id = os.environ.get("CW_TRIPO_HERO_ID", "").strip().lower()
    if only_hero_id:
        manifest = [item for item in manifest if item.get("heroId", "").strip().lower() == only_hero_id]
        if not manifest:
            raise RuntimeError(f"Hero not found in manifest: {only_hero_id}")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = [import_one(asset_tools, item) for item in manifest]
    unreal.EditorAssetLibrary.save_directory(DEST_ROOT, only_if_is_dirty=False, recursive=True)

    imported = sum(1 for task in tasks if task.imported_object_paths)
    unreal.log(f"Tripo hero import complete: {imported}/{len(tasks)} tasks imported.")


if __name__ == "__main__":
    main()
