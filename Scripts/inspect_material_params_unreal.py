import unreal


MATERIALS = [
    "/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque",
    "/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial",
    "/Engine/EngineMaterials/EmissiveTexturedMaterial.EmissiveTexturedMaterial",
    "/Engine/EngineMaterials/BaseFlattenDiffuseMap.BaseFlattenDiffuseMap",
    "/Engine/EngineMaterials/PaperDiffuse.PaperDiffuse",
    "/Engine/EngineMaterials/DefaultDiffuse.DefaultDiffuse",
    "/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial",
    "/Engine/EngineDebugMaterials/TextureColorViewMode.TextureColorViewMode",
    "/Engine/EngineDebugMaterials/TexturePaintingMaskMaterial.TexturePaintingMaskMaterial",
    "/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial",
    "/Engine/EngineDebugMaterials/M_SimpleOpaque.M_SimpleOpaque",
    "/Engine/EngineDebugMaterials/M_GeoInsp_Clay.M_GeoInsp_Clay",
    "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial",
    "/Game/CrimsonWars/Materials/M_NativeGroundTextured.M_NativeGroundTextured",
]


def names(values):
    return [str(value) for value in values] if values else []


for path in MATERIALS:
    material = unreal.load_asset(path)
    unreal.log(f"MAT {path} => {material}")
    if not material:
        continue

    for label, getter in (
        ("scalar", unreal.MaterialEditingLibrary.get_scalar_parameter_names),
        ("vector", unreal.MaterialEditingLibrary.get_vector_parameter_names),
        ("texture", unreal.MaterialEditingLibrary.get_texture_parameter_names),
    ):
        try:
            unreal.log(f"  {label}: {names(getter(material))}")
        except Exception as exc:
            unreal.log_warning(f"  {label}: {exc}")

    try:
        expressions = material.get_editor_property("expressions")
        for expr in expressions:
            try:
                param = expr.get_editor_property("parameter_name")
            except Exception:
                param = ""
            unreal.log(f"  expr {expr.get_class().get_name()} param={param}")
    except Exception as exc:
        unreal.log_warning(f"  expressions: {exc}")
