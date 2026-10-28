import unreal


ROOTS = (
    "/Game/Characters/Monsters",
    "/Game/Characters/Heroes",
)


def safe_get_physics_asset(mesh):
    try:
        return mesh.get_editor_property("physics_asset")
    except Exception:
        try:
            return mesh.get_physics_asset()
        except Exception:
            return None


def is_primary_character_mesh(asset):
    if not isinstance(asset, unreal.SkeletalMesh):
        return False

    name = asset.get_name()
    return name.startswith("SK_") and (
        name.endswith("_walk_death_animfix") or name.endswith("_walk_death")
    )


def create_physics_asset(mesh):
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    if subsystem:
        return subsystem.create_physics_asset(mesh, True, 0)
    return unreal.SkeletalMeshEditorSubsystem.create_physics_asset(mesh, True, 0)


def main():
    created = []
    skipped = []
    failed = []

    for root in ROOTS:
        for asset_path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if not is_primary_character_mesh(asset):
                continue

            existing_physics_asset = safe_get_physics_asset(asset)
            if existing_physics_asset:
                skipped.append(asset_path)
                continue

            unreal.log(f"Creating PhysicsAsset for {asset_path}")
            try:
                physics_asset = create_physics_asset(asset)
            except Exception as exc:
                unreal.log_error(f"Failed to create PhysicsAsset for {asset_path}: {exc}")
                failed.append(asset_path)
                continue

            if physics_asset:
                created.append(physics_asset.get_path_name())
                unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
                unreal.EditorAssetLibrary.save_loaded_asset(physics_asset, only_if_is_dirty=False)
            else:
                failed.append(asset_path)

    for root in ROOTS:
        unreal.EditorAssetLibrary.save_directory(root, only_if_is_dirty=False, recursive=True)

    unreal.log(
        "Character PhysicsAsset pass complete: "
        f"created={len(created)} skipped={len(skipped)} failed={len(failed)}"
    )
    if failed:
        unreal.log_warning("PhysicsAsset creation failed for:\n" + "\n".join(failed))


if __name__ == "__main__":
    main()
