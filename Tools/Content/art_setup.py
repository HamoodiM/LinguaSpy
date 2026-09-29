"""LinguaSpy art setup inside Unreal: imports Blender exports and builds the toon materials.

Run in the editor:  exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/art_setup.py", encoding="utf-8").read())
Then call the functions you need, e.g. import_characters(); import_kits(); build_toon_master(); build_pp_toon()
"""
import unreal, os, glob

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EXPORT = "C:/UnrealEngineProjects/Personal/Saved/Export"
MAT = "/Game/LinguaSpy/Materials"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"


# ---------------------------------------------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------------------------------------------

def _task(filename, dest, options):
    t = unreal.AssetImportTask()
    t.filename = filename
    t.destination_path = dest
    t.automated = True
    t.replace_existing = True
    t.save = True
    t.options = options
    return t


def import_characters():
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    ui.skeleton = EAL.load_asset(SKELETON)
    ui.import_materials = False
    ui.import_textures = False
    ui.import_animations = False
    ui.create_physics_asset = False
    ui.skeletal_mesh_import_data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
    tasks = [_task(f, "/Game/LinguaSpy/Art/Characters", ui) for f in sorted(glob.glob(EXPORT + "/Characters/SK_*.fbx"))]
    AT.import_asset_tasks(tasks)
    return [p for t in tasks for p in t.imported_object_paths]


def import_kits(cities=("Shared", "Paris", "Madrid", "Tokyo", "Rome")):
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    ui.import_materials = False
    ui.import_textures = False
    ui.static_mesh_import_data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    ui.static_mesh_import_data.set_editor_property("combine_meshes", True)
    ui.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    ui.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", False)
    out = []
    for city in cities:
        tasks = [_task(f, "/Game/LinguaSpy/Art/" + city, ui) for f in sorted(glob.glob(f"{EXPORT}/Kits/{city}/SM_*.fbx"))]
        AT.import_asset_tasks(tasks)
        out += [p for t in tasks for p in t.imported_object_paths]
    return out


def assign_material(paths, material_path):
    mat = EAL.load_asset(material_path)
    for p in paths:
        asset = EAL.load_asset(p.split(".")[0])
        if isinstance(asset, unreal.StaticMesh):
            for i in range(asset.get_num_sections(0)):
                asset.set_material(i, mat)
        elif isinstance(asset, unreal.SkeletalMesh):
            mats = asset.get_editor_property("materials")
            for m in mats:
                m.set_editor_property("material_interface", mat)
            asset.set_editor_property("materials", mats)
        EAL.save_loaded_asset(asset, False)


# ---------------------------------------------------------------------------------------------------------------
# Materials
# ---------------------------------------------------------------------------------------------------------------

def _new_material(name):
    path = f"{MAT}/{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return AT.create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())


def build_toon_master():
    """Vertex color * per-instance Tint (where VertexColor.a = 1), flat roughness, dithered CPD[0] occlusion fade."""
    m = _new_material("M_ToonMaster")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("opacity_mask_clip_value", 0.5)

    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -900, 0)
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 200)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, 150)
    MEL.connect_material_expressions(vc, "", mul, "A")
    MEL.connect_material_expressions(tint, "", mul, "B")
    lerp = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -350, 50)
    MEL.connect_material_expressions(vc, "", lerp, "A")
    MEL.connect_material_expressions(mul, "", lerp, "B")
    MEL.connect_material_expressions(vc, "A", lerp, "Alpha")
    MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -350, 300)
    rough.set_editor_property("r", 1.0)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -350, 380)
    spec.set_editor_property("r", 0.0)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    # Occlusion fade: CPD[0] (0 solid .. 1 gone) -> dithered opacity mask
    fade = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 500)
    fade.set_editor_property("parameter_name", "Fade")
    fade.set_editor_property("use_custom_primitive_data", True)
    fade.set_editor_property("primitive_data_index", 0)
    fade.set_editor_property("default_value", 0.0)
    inv = MEL.create_material_expression(m, unreal.MaterialExpressionOneMinus, -650, 500)
    MEL.connect_material_expressions(fade, "", inv, "")
    dither = MEL.create_material_expression(m, unreal.MaterialExpressionMaterialFunctionCall, -400, 500)
    dither.set_editor_property("material_function", EAL.load_asset("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA"))
    MEL.connect_material_expressions(inv, "", dither, "Alpha Threshold")
    MEL.connect_material_property(dither, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    m.set_editor_property("dither_opacity_mask", False)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    return m


PP_CODE = r"""
float2 uv = GetDefaultSceneTextureUV(Parameters, 14);
float3 col = SceneTextureLookup(uv, 14, false).rgb;
float depth = SceneTextureLookup(uv, 1, false).r;
if (depth > SkyDepth) { return col; }

// Cel bands: quantize the light ratio (scene / albedo) into shadow / mid / lit
float3 albedo = max(SceneTextureLookup(uv, 2, false).rgb, 0.03);
float light = dot(col / albedo, float3(0.3333, 0.3333, 0.3333)) / max(LitLevel, 0.0001);
float band = light < T1 ? ShadowLevel : (light < T2 ? MidLevel : 1.0);
float3 toon = albedo * band * LitLevel;
toon = lerp(toon, toon * ShadowTint, (1.0 - band) * 0.8);

// Ink outlines from depth + normal discontinuities
float2 px = View.BufferSizeAndInvSize.zw * Thickness;
float dL = SceneTextureLookup(uv - float2(px.x, 0), 1, false).r;
float dR = SceneTextureLookup(uv + float2(px.x, 0), 1, false).r;
float dU = SceneTextureLookup(uv - float2(0, px.y), 1, false).r;
float dD = SceneTextureLookup(uv + float2(0, px.y), 1, false).r;
float edgeD = saturate((abs(dL + dR - 2 * depth) + abs(dU + dD - 2 * depth)) / (depth * DepthSensitivity));
float3 n0 = SceneTextureLookup(uv, 8, false).rgb;
float3 nR = SceneTextureLookup(uv + float2(px.x, 0), 8, false).rgb;
float3 nD = SceneTextureLookup(uv + float2(0, px.y), 8, false).rgb;
float edgeN = saturate(((1 - dot(n0, nR)) + (1 - dot(n0, nD)) - NormalThreshold) * 4.0);
float edge = saturate(max(edgeD, edgeN)) * saturate(1 - depth / OutlineFadeDistance);
return lerp(toon, OutlineColor * LitLevel, edge);
"""


def build_pp_toon(lit_level=1.0):
    m = _new_material("PP_Toon")
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)

    custom = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
    custom.set_editor_property("code", PP_CODE)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)

    params = [("LitLevel", lit_level), ("T1", 0.35), ("T2", 0.75), ("ShadowLevel", 0.45), ("MidLevel", 0.75),
              ("Thickness", 1.5), ("DepthSensitivity", 0.02), ("NormalThreshold", 0.35),
              ("OutlineFadeDistance", 20000.0), ("SkyDepth", 60000.0)]
    inputs = []
    y = 0
    exprs = []
    for name, value in params:
        p = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", value)
        exprs.append((name, p)); y += 80
    for name, vec in [("ShadowTint", unreal.LinearColor(0.75, 0.78, 1.0, 1)), ("OutlineColor", unreal.LinearColor(0.08, 0.07, 0.12, 1))]:
        p = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", vec)
        exprs.append((name, p)); y += 120

    # A SceneTexture node makes SceneTextureLookup available to the custom node
    st = MEL.create_material_expression(m, unreal.MaterialExpressionSceneTexture, -900, y)
    st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    exprs.append(("Dummy", st))

    for name, _ in exprs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    for name, p in exprs:
        MEL.connect_material_expressions(p, "Color" if name == "Dummy" else "", custom, name)
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    return m


def retarget_flat_instances():
    """Moves the greybox MI_* instances from M_Flat onto M_ToonMaster (Color -> Tint)."""
    master = EAL.load_asset(f"{MAT}/M_ToonMaster")
    changed = []
    for path in EAL.list_assets(MAT, recursive=False):
        asset = EAL.load_asset(path.split(".")[0])
        if isinstance(asset, unreal.MaterialInstanceConstant) and asset.get_name().startswith("MI_"):
            color = MEL.get_material_instance_vector_parameter_value(asset, "Color")
            asset.set_editor_property("parent", master)
            MEL.set_material_instance_vector_parameter_value(asset, "Tint", color)
            MEL.update_material_instance(asset)
            EAL.save_loaded_asset(asset, False)
            changed.append(asset.get_name())
    return changed


def tint_instance(name, hex_color):
    """Creates/updates MI_Tint_<name> on M_ToonMaster."""
    def lin(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    path = f"{MAT}/MI_Tint_{name}"
    mi = EAL.load_asset(path) if EAL.does_asset_exist(path) else AT.create_asset(
        f"MI_Tint_{name}", MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", EAL.load_asset(f"{MAT}/M_ToonMaster"))
    MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(
        lin(int(hex_color[0:2], 16)), lin(int(hex_color[2:4], 16)), lin(int(hex_color[4:6], 16)), 1))
    EAL.save_loaded_asset(mi, False)
    return mi
