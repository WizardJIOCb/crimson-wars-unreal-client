import unreal
import os


MATERIAL_DIR = "/Game/CrimsonWars/Materials"
MATERIAL_NAME = "M_NativeGroundField"
GROUND_TEXTURE_DIR = "/Game/CrimsonWars/RawAssets/tiles"
GROUND_TEXTURE_NAME = "ground_grass"
DEFAULT_MAP_TEXTURE_ASSET = f"{GROUND_TEXTURE_DIR}/{GROUND_TEXTURE_NAME}"
DEFAULT_MAP_TEXTURE = f"{DEFAULT_MAP_TEXTURE_ASSET}.{GROUND_TEXTURE_NAME}"
GROUND_TEXTURE_SOURCE_RELATIVE_PATHS = (
    "CrimsonWars/RawAssets/tiles/ground_grass.jpg",
    "CrimsonWars/RawAssets/tiles/ground_grass.png",
)
GROUND_TEXTURE_TILING = 8.0


def try_get_enum_value(enum_type, *names):
    for name in names:
        try:
            return getattr(enum_type, name)
        except Exception:
            pass
    return None


def try_set_editor_property(obj, prop_name, value):
    if value is None:
        return
    try:
        obj.set_editor_property(prop_name, value)
    except Exception:
        pass


def configure_ground_texture(texture):
    if not texture:
        return

    try_set_editor_property(texture, "filter", try_get_enum_value(unreal.TextureFilter, "TF_TRILINEAR", "TF_Trilinear"))
    try_set_editor_property(texture, "lod_group", try_get_enum_value(unreal.TextureGroup, "TEXTUREGROUP_WORLD", "TEXTUREGROUP_World"))
    try_set_editor_property(texture, "mip_gen_settings", try_get_enum_value(unreal.TextureMipGenSettings, "TMGS_FROM_TEXTURE_GROUP", "TMGS_FromTextureGroup"))
    try_set_editor_property(texture, "lod_bias", 1)
    try_set_editor_property(texture, "never_stream", False)
    unreal.EditorAssetLibrary.save_asset(DEFAULT_MAP_TEXTURE_ASSET, only_if_is_dirty=False)


def find_ground_texture_source_path():
    content_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
    for relative_path in GROUND_TEXTURE_SOURCE_RELATIVE_PATHS:
        source_path = os.path.join(content_dir, relative_path).replace("\\", "/")
        if os.path.exists(source_path):
            return source_path
    return ""


def load_or_import_ground_texture():
    texture = unreal.EditorAssetLibrary.load_asset(DEFAULT_MAP_TEXTURE)
    if not texture:
        texture = unreal.EditorAssetLibrary.load_asset(DEFAULT_MAP_TEXTURE_ASSET)
    if texture:
        configure_ground_texture(texture)
        return texture

    source_path = find_ground_texture_source_path()
    if not source_path:
        raise RuntimeError(f"Missing source ground texture. Tried: {GROUND_TEXTURE_SOURCE_RELATIVE_PATHS}")

    unreal.EditorAssetLibrary.make_directory(GROUND_TEXTURE_DIR)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source_path)
    task.set_editor_property("destination_path", GROUND_TEXTURE_DIR)
    task.set_editor_property("destination_name", GROUND_TEXTURE_NAME)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = task.get_editor_property("imported_object_paths")
    for imported_path in imported_paths:
        texture = unreal.EditorAssetLibrary.load_asset(imported_path)
        if texture:
            configure_ground_texture(texture)
            return texture

    texture = unreal.EditorAssetLibrary.load_asset(DEFAULT_MAP_TEXTURE_ASSET)
    if not texture:
        raise RuntimeError(f"Failed to import default ground texture from {source_path}")
    configure_ground_texture(texture)
    return texture


def get_material_expressions(material):
    try:
        return list(unreal.MaterialEditingLibrary.get_material_expressions(material))
    except Exception:
        try:
            expressions = material.get_editor_property("expressions")
            return list(expressions) if expressions else []
        except Exception:
            return []


def main():
    unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
    material_path = f"{MATERIAL_DIR}/{MATERIAL_NAME}"
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if not material:
        factory = unreal.MaterialFactoryNew()
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            MATERIAL_NAME,
            MATERIAL_DIR,
            unreal.Material,
            factory,
        )
    if not material:
        raise RuntimeError(f"Could not create material {material_path}")

    texture = load_or_import_ground_texture()

    expressions = get_material_expressions(material)
    coord = None
    sample = None
    roughness = None
    for expr in expressions:
        class_name = expr.get_class().get_name()
        if class_name == "MaterialExpressionTextureCoordinate" and coord is None:
            coord = expr
        elif class_name == "MaterialExpressionTextureSampleParameter2D":
            try:
                parameter_name = str(expr.get_editor_property("parameter_name"))
            except Exception:
                parameter_name = ""
            if parameter_name in ("GroundTexture", "None", "") and sample is None:
                sample = expr
        elif class_name == "MaterialExpressionConstant" and roughness is None:
            roughness = expr

    if not coord:
        coord = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureCoordinate,
            -520,
            0,
        )
    coord.set_editor_property("u_tiling", GROUND_TEXTURE_TILING)
    coord.set_editor_property("v_tiling", GROUND_TEXTURE_TILING)

    if not sample:
        sample = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureSampleParameter2D,
            -300,
            0,
        )
    sample.set_editor_property("parameter_name", "GroundTexture")
    sample.set_editor_property("texture", texture)
    try:
        sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    except Exception:
        pass

    if not roughness:
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionConstant,
            -300,
            220,
        )
    roughness.set_editor_property("r", 0.92)

    try:
        unreal.MaterialEditingLibrary.connect_material_expressions(coord, "", sample, "UVs")
    except Exception:
        try:
            unreal.MaterialEditingLibrary.connect_material_expressions(coord, "", sample, "Coordinates")
        except Exception:
            pass

    for output_name in ("RGB", ""):
        try:
            unreal.MaterialEditingLibrary.connect_material_property(
                sample,
                output_name,
                unreal.MaterialProperty.MP_BASE_COLOR,
            )
            break
        except Exception:
            pass

    for output_name in ("RGB", ""):
        try:
            unreal.MaterialEditingLibrary.connect_material_property(
                sample,
                output_name,
                unreal.MaterialProperty.MP_EMISSIVE_COLOR,
            )
            break
        except Exception:
            pass

    try:
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness,
            "",
            unreal.MaterialProperty.MP_ROUGHNESS,
        )
    except Exception:
        pass

    try:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    except Exception:
        pass
    try:
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception:
        pass
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material_path, only_if_is_dirty=False)
    unreal.log(f"Created/updated native ground material: {material_path}")


if __name__ == "__main__":
    main()
