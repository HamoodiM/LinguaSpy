"""M_ToonMaster v2 (Q4 of the visual polish plan): edits the existing master material IN PLACE (run in-editor).

Adds, between the vertex-colour/tint lerp and Base Color, one Custom node ("ToonSurface") that:
  - darkens the bottom of vertical surfaces (contact shadow: buildings and props sit ON the ground instead of floating;
    floors are left alone, so the street slabs don't darken),
  - jitters brightness / warmth per object (hash of the object position: repeated kit buildings stop looking cloned),
  - tints up-facing, untinted (vertex alpha 0), high surfaces with RoofTint * RoofAmount (per-city roof colour without
    re-exporting the kits; per-city values live in MI_Tint_<HEX>_<City> children, see greybox_kit.apply_look),
and an Emissive scalar (Base Color * Emissive -> Emissive Color) for glowing props (PP_Toon v2 passes it through).

    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/toon_master_v2.py", encoding="utf-8").read())
    upgrade_toon_master()
"""
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MASTER = "/Game/LinguaSpy/Materials/M_ToonMaster"

SURFACE_CODE = r"""
// Per-object variation (stable hash of the object's position)
float h = frac(sin(dot(floor(ObjPos.xy / 10.0), float2(12.9898, 78.233))) * 43758.5453);
float h2 = frac(h * 7.31);
float3 c = Base * (1.0 + (h - 0.5) * 2.0 * Variation);
c *= float3(1.0 + (h2 - 0.5) * Variation, 1.0, 1.0 - (h2 - 0.5) * Variation);

// Contact darkening on vertical surfaces only (|N.z| ~ 0): the bottom ContactHeight cm fade from ContactDark to 1
float aboveGround = WorldPos.z - GroundZ;
float wall = 1.0 - saturate(abs(Normal.z));
c *= lerp(1.0, lerp(ContactDark, 1.0, saturate(aboveGround / ContactHeight)), wall);

// Paving: a quiet world-aligned running-bond grid on up-facing surfaces (cobbles / plaza tiles), PavingStrength 0 = off
float2 cell = WorldPos.xy / max(PavingScale, 1.0);
cell.x += 0.5 * floor(cell.y);
float2 f = frac(cell);
float joint = 1.0 - step(PavingJoint, f.x) * step(PavingJoint, f.y);
c *= 1.0 - PavingStrength * joint * saturate(Normal.z);

// Roof colour: up-facing, untinted (vertex alpha 0) and high above the street
float roof = step(0.9, Normal.z) * step(VertexAlpha, 0.5) * step(RoofMinHeight, aboveGround);
c = lerp(c, RoofTint, roof * RoofAmount);
return c;
"""

SCALARS = [("Variation", 0.05), ("ContactDark", 0.72), ("ContactHeight", 180.0), ("GroundZ", 0.0),
           ("RoofMinHeight", 500.0), ("RoofAmount", 0.0), ("Emissive", 0.0),
           ("PavingScale", 60.0), ("PavingJoint", 0.06), ("PavingStrength", 0.0)]
VECTORS = [("RoofTint", unreal.LinearColor(0.2, 0.24, 0.3, 1.0))]


def update_toon_surface():
    """Re-applies SURFACE_CODE and adds any missing parameters to an already-upgraded M_ToonMaster."""
    m = unreal.load_asset(MASTER)
    exprs = MEL.get_material_expressions(m)
    custom = next(e for e in exprs if isinstance(e, unreal.MaterialExpressionCustom))
    params = {str(e.get_editor_property("parameter_name")): e for e in exprs
              if isinstance(e, (unreal.MaterialExpressionScalarParameter, unreal.MaterialExpressionVectorParameter))}
    links = {}
    for n, node in zip([str(ci.get_editor_property("input_name")) for ci in custom.get_editor_property("inputs")],
                       MEL.get_inputs_for_material_expression(m, custom)):
        links[n] = node
    y = 0
    for name, value in SCALARS:
        if name not in params:
            p = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1000, y)
            p.set_editor_property("parameter_name", name)
            p.set_editor_property("default_value", value)
            params[name] = p
            y += 70
    names = ["Base", "VertexAlpha", "WorldPos", "ObjPos", "Normal", "Variation", "ContactDark", "ContactHeight",
             "GroundZ", "RoofMinHeight", "RoofAmount", "RoofTint", "PavingScale", "PavingJoint", "PavingStrength"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    custom.set_editor_property("code", SURFACE_CODE.strip())
    vc = next(e for e in exprs if isinstance(e, unreal.MaterialExpressionVertexColor))
    for n in names:
        if n == "VertexAlpha":
            MEL.connect_material_expressions(vc, "A", custom, n)
        elif n in params:
            MEL.connect_material_expressions(params[n], "", custom, n)
        elif links.get(n) is not None:
            MEL.connect_material_expressions(links[n], "", custom, n)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    return names


def upgrade_toon_master():
    m = unreal.load_asset(MASTER)
    exprs = MEL.get_material_expressions(m)
    if any(isinstance(e, unreal.MaterialExpressionCustom) for e in exprs):
        return update_toon_surface()
    base_node = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
    vc = next(e for e in exprs if isinstance(e, unreal.MaterialExpressionVertexColor))

    custom = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -150, -250)
    custom.set_editor_property("description", "ToonSurface")
    custom.set_editor_property("code", SURFACE_CODE.strip())
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)

    params = {}
    y = -900
    for name, value in SCALARS:
        p = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", value)
        params[name] = p
        y += 70
    for name, value in VECTORS:
        p = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -700, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", value)
        params[name] = p
        y += 120
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -700, y)
    op = MEL.create_material_expression(m, unreal.MaterialExpressionObjectPositionWS, -700, y + 70)
    nrm = MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -700, y + 140)

    names = ["Base", "VertexAlpha", "WorldPos", "ObjPos", "Normal", "Variation", "ContactDark", "ContactHeight",
             "GroundZ", "RoofMinHeight", "RoofAmount", "RoofTint", "PavingScale", "PavingJoint", "PavingStrength"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    MEL.connect_material_expressions(base_node, "", custom, "Base")
    MEL.connect_material_expressions(vc, "A", custom, "VertexAlpha")
    MEL.connect_material_expressions(wp, "", custom, "WorldPos")
    MEL.connect_material_expressions(op, "", custom, "ObjPos")
    MEL.connect_material_expressions(nrm, "", custom, "Normal")
    for n in names[5:]:
        MEL.connect_material_expressions(params[n], "", custom, n)
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)

    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, 100, -100)
    MEL.connect_material_expressions(custom, "", mul, "A")
    MEL.connect_material_expressions(params["Emissive"], "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    return "upgraded"


def keep_shadows_when_faded():
    """Occlusion fade dithers the opacity mask of whatever stands between the camera and the spy. The shadow depth
    pass used the same mask, so a faded building lost its shadow (bright, grainy holes in the street shadows with the
    25 m camera). A ShadowPassSwitch feeds the shadow pass an opaque mask: occluders fade, their shadows stay."""
    m = unreal.load_asset(MASTER)
    exprs = MEL.get_material_expressions(m)
    if any(e.get_class().get_name() == "MaterialExpressionShadowReplace" for e in exprs):
        return "already"
    switch_cls = unreal.MaterialExpressionShadowReplace  # the "Shadow Pass Switch" node
    mask_node = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_OPACITY_MASK)
    switch = MEL.create_material_expression(m, switch_cls, -150, 600)
    one = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 700)
    one.set_editor_property("r", 1.0)
    MEL.connect_material_expressions(mask_node, "", switch, "Default")
    MEL.connect_material_expressions(one, "", switch, "Shadow")
    MEL.connect_material_property(switch, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    return [n for n in MEL.get_material_expression_input_names(switch)]
