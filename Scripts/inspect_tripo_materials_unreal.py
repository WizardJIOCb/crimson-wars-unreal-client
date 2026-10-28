import unreal


MATERIALS = [
    "/Game/Characters/Monsters/normal/tripo_mat_3d64ea66.tripo_mat_3d64ea66",
    "/Game/Characters/Monsters/brute/tripo_mat_0df6e508.tripo_mat_0df6e508",
    "/Game/Characters/Monsters/boss_hellmart/tripo_mat_d4273d5d.tripo_mat_d4273d5d",
]


def show_material(path):
    material = unreal.load_asset(path)
    unreal.log(f"TRIPO MAT {path} => {material}")
    if not material:
        return
    for label, getter in (
        ("scalar", unreal.MaterialEditingLibrary.get_scalar_parameter_names),
        ("vector", unreal.MaterialEditingLibrary.get_vector_parameter_names),
        ("texture", unreal.MaterialEditingLibrary.get_texture_parameter_names),
    ):
        try:
            names = getter(material)
            unreal.log(f"  {label}: {[str(name) for name in names]}")
            for name in names:
                try:
                    if label == "texture":
                        value = unreal.MaterialEditingLibrary.get_material_default_texture_parameter_value(material, name)
                    elif label == "vector":
                        value = unreal.MaterialEditingLibrary.get_material_default_vector_parameter_value(material, name)
                    else:
                        value = unreal.MaterialEditingLibrary.get_material_default_scalar_parameter_value(material, name)
                    unreal.log(f"    {name}: {value}")
                except Exception as exc:
                    unreal.log_warning(f"    {name}: {exc}")
        except Exception as exc:
            unreal.log_warning(f"  {label}: {exc}")


for path in MATERIALS:
    show_material(path)
