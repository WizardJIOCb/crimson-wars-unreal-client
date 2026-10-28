import unreal


MATERIAL_PATH = "/Game/CrimsonWars/Materials/M_NativeGroundTextured"


def main():
    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
    if not material:
        raise RuntimeError(f"Missing material {MATERIAL_PATH}")

    unreal.log(f"Material {MATERIAL_PATH} blend={material.get_editor_property('blend_mode')}")
    for expr in unreal.MaterialEditingLibrary.get_material_expressions(material):
        unreal.log(f"Expression {expr.get_name()} class={expr.get_class().get_name()}")
        for prop_name in ("parameter_name", "texture", "u_tiling", "v_tiling"):
            try:
                unreal.log(f"  {prop_name}={expr.get_editor_property(prop_name)}")
            except Exception:
                pass


if __name__ == "__main__":
    main()
