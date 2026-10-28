import pathlib
import os

import unreal


TRIPO_MAP_OBJECTS = [
    ("bus_yellow", "abandoned+bus+3d+model"),
    ("clinic_block", "clinic+building+3d+model"),
    ("barrier", "concrete+barrier+3d+model"),
    ("tree", "gnarled+tree+3d+model"),
    ("industrial_tank", "industrial+facility+3d+model"),
    ("ambulance", "military+ambulance+3d+model"),
    ("mall_block", "ruined+storefront+3d+model"),
    ("build_2", "rusty+gas+station+3d+model"),
    ("build_5", "clinic+building+3d+model"),
    ("shopping_cart_barricade", "rusty+shopping+cart+3d+model"),
    ("reactor_block", "steampunk+reactor+3d+model"),
]


def _set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception:
        pass


def _first_fbx(source_dir):
    matches = sorted(pathlib.Path(source_dir).glob("*.fbx"))
    return matches[0] if matches else None


def _make_static_mesh_import_options():
    options = unreal.FbxImportUI()
    _set(options, "automated_import_should_detect_type", False)
    _set(options, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    _set(options, "import_mesh", True)
    _set(options, "import_as_skeletal", False)
    _set(options, "import_materials", True)
    _set(options, "import_textures", True)
    _set(options, "create_physics_asset", False)

    static_data = options.static_mesh_import_data
    _set(static_data, "combine_meshes", True)
    _set(static_data, "auto_generate_collision", False)
    _set(static_data, "generate_lightmap_u_vs", True)
    _set(static_data, "convert_scene", True)
    _set(static_data, "convert_scene_unit", True)
    return options


def main():
    content_dir = pathlib.Path(unreal.Paths.project_content_dir()).resolve()
    source_root = content_dir / "Objects"
    destination_root = "/Game/CrimsonWars/MapObjects/Tripo3D"
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []
    only_key = os.environ.get("CW_TRIPO_MAP_OBJECT_KEY", "").strip().lower()

    for key, folder_name in TRIPO_MAP_OBJECTS:
        if only_key and key != only_key:
            continue

        fbx_path = _first_fbx(source_root / folder_name)
        if not fbx_path:
            unreal.log_warning(f"Tripo map object source FBX missing: {source_root / folder_name}")
            continue

        destination_path = f"{destination_root}/{key}"
        unreal.EditorAssetLibrary.make_directory(destination_path)

        task = unreal.AssetImportTask()
        task.filename = str(fbx_path)
        task.destination_path = destination_path
        task.destination_name = f"SM_{key}"
        task.automated = True
        task.replace_existing = True
        task.save = False
        task.options = _make_static_mesh_import_options()
        tasks.append(task)

    if not tasks:
        unreal.log_warning("No Tripo map object import tasks were created.")
        return

    asset_tools.import_asset_tasks(tasks)

    for task in tasks:
        if task.imported_object_paths:
            for imported_path in task.imported_object_paths:
                unreal.EditorAssetLibrary.save_asset(imported_path, only_if_is_dirty=False)
        else:
            asset_path = f"{task.destination_path}/{task.destination_name}"
            unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)

    unreal.log(f"Imported {len(tasks)} Tripo map object static meshes into {destination_root}")


if __name__ == "__main__":
    main()
