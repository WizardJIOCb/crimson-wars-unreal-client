import pathlib

import unreal


def safe_set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception:
        return False


def make_static_mesh_import_options():
    options = unreal.FbxImportUI()
    safe_set(options, "automated_import_should_detect_type", False)
    safe_set(options, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    safe_set(options, "import_mesh", True)
    safe_set(options, "import_as_skeletal", False)
    safe_set(options, "import_materials", True)
    safe_set(options, "import_textures", True)
    safe_set(options, "create_physics_asset", False)

    static_data = options.static_mesh_import_data
    safe_set(static_data, "combine_meshes", True)
    safe_set(static_data, "auto_generate_collision", False)
    safe_set(static_data, "generate_lightmap_u_vs", True)
    safe_set(static_data, "convert_scene", True)
    safe_set(static_data, "convert_scene_unit", True)
    return options


def main():
    content_dir = pathlib.Path(unreal.Paths.project_content_dir()).resolve()
    source_dir = content_dir / "Other" / "exp-crystal"
    fbx_matches = sorted(source_dir.glob("*.fbx"))
    if not fbx_matches:
        raise RuntimeError(f"XP crystal FBX missing: {source_dir}")

    destination_path = "/Game/Other/exp-crystal"
    unreal.EditorAssetLibrary.make_directory(destination_path)

    task = unreal.AssetImportTask()
    safe_set(task, "filename", str(fbx_matches[0]))
    safe_set(task, "destination_path", destination_path)
    safe_set(task, "destination_name", "SM_exp_crystal")
    safe_set(task, "automated", True)
    safe_set(task, "replace_existing", True)
    safe_set(task, "save", True)
    safe_set(task, "options", make_static_mesh_import_options())

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)

    imported = ", ".join(task.imported_object_paths) if task.imported_object_paths else f"{destination_path}/SM_exp_crystal"
    unreal.log(f"XP crystal import complete: {imported}")


if __name__ == "__main__":
    main()
