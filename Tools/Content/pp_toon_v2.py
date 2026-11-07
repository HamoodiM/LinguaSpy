"""PP_Toon v2: edits the existing material's Custom node IN PLACE (run in-editor via unreal-mcp).

Why in place: art_setup.build_pp_toon() deletes and recreates the asset, which breaks the blendable reference held by
every map's ToonPostProcess volume. This script keeps the asset, swaps the HLSL, adds the new parameters and
recompiles. PP_CODE_V1 (the Phase 3 code) stays in art_setup.py for reference / revert.

    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/pp_toon_v2.py", encoding="utf-8").read())
    upgrade_pp_toon()

What changed vs v1 (see docs/Visual_Polish_Log.md):
  1. Light hue is kept: v1 used only the luminance of scene/albedo, so sun and sky colours never reached the screen.
  2. Emissive pass-through: pixels much brighter than the lit level keep their scene colour (lanterns can glow).
  3. Soft band edges (smoothstep) instead of hard steps (less shimmer under TSR).
  4. Outlines use the Laplacian of 1/depth (zero on any plane), not of depth (non-zero on planes seen at an angle,
     which drew hatching on walls and parapets). Width scales with the render height.
  5. Depth haze toward a per-city colour.
"""
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
PP_PATH = "/Game/LinguaSpy/Materials/PP_Toon"

PP_CODE_V2 = r"""
float2 uv = GetDefaultSceneTextureUV(Parameters, 14);
float3 col = SceneTextureLookup(uv, 14, false).rgb;
float depth = SceneTextureLookup(uv, 1, false).r;
if (depth > SkyDepth) { return col; }

// Cel bands from the light's BRIGHTNESS, keeping the light's HUE (v1 kept luminance only: every city looked alike)
float3 albedo = max(SceneTextureLookup(uv, 2, false).rgb, 0.03);
float3 lightRGB = col / albedo;
float lum = dot(lightRGB, float3(0.3333, 0.3333, 0.3333));
float light = lum / max(LightNorm, 0.0001);
float3 toon;
if (light > EmissiveThreshold)
{
	toon = col; // emissive surfaces (lanterns, signs) keep their glow
}
else
{
	float w = max(BandSoftness, 0.0001);
	float band = lerp(ShadowLevel, MidLevel, smoothstep(T1 - w, T1 + w, light));
	band = lerp(band, 1.0, smoothstep(T2 - w, T2 + w, light));
	float3 hue = clamp(lightRGB / max(lum, 0.0001), 0.0, 2.0);
	toon = albedo * band * LitLevel * lerp(float3(1.0, 1.0, 1.0), hue, LightHueKeep);
	toon = lerp(toon, toon * ShadowTint, (1.0 - band) * 0.8);
}

// Ink outlines. Depth term: Laplacian of 1/depth. 1/z is affine in screen space across any plane, so floors and walls
// at grazing angles give ~0 and only real breaks (silhouettes, creases) draw. Width scales with the render height.
float2 px = View.BufferSizeAndInvSize.zw * Thickness * max(View.ViewSizeAndInvSize.y / OutlineRefHeight, 1.0);
float dL = SceneTextureLookup(uv - float2(px.x, 0), 1, false).r;
float dR = SceneTextureLookup(uv + float2(px.x, 0), 1, false).r;
float dU = SceneTextureLookup(uv - float2(0, px.y), 1, false).r;
float dD = SceneTextureLookup(uv + float2(0, px.y), 1, false).r;
float edgeD = saturate((abs(depth / dL + depth / dR - 2.0) + abs(depth / dU + depth / dD - 2.0)) / DepthSensitivity);
float3 n0 = SceneTextureLookup(uv, 8, false).rgb;
float3 nR = SceneTextureLookup(uv + float2(px.x, 0), 8, false).rgb;
float3 nD = SceneTextureLookup(uv + float2(0, px.y), 8, false).rgb;
float edgeN = saturate(((1 - dot(n0, nR)) + (1 - dot(n0, nD)) - NormalThreshold) * 4.0);
float edge = saturate(max(edgeD, edgeN)) * saturate(1 - depth / OutlineFadeDistance);
float3 result = lerp(toon, OutlineColor * LitLevel, edge);

// Depth haze: far things fade toward the city's haze colour (aerial perspective for a top-down camera)
float haze = saturate((depth - HazeStart) / max(HazeRange, 1.0)) * HazeMax;
return lerp(result, HazeColor, haze);
"""

NEW_SCALARS = [("LightNorm", 1.0), ("LightHueKeep", 0.7), ("EmissiveThreshold", 2.5), ("BandSoftness", 0.04),
               ("OutlineRefHeight", 1080.0), ("HazeStart", 3500.0), ("HazeRange", 9000.0), ("HazeMax", 0.3)]
NEW_VECTORS = [("HazeColor", unreal.LinearColor(0.80, 0.82, 0.92, 1.0))]
RETUNE = {"DepthSensitivity": 0.02}   # new scale: relative curvature of 1/depth


def _find_custom(m):
    for e in MEL.get_material_expressions(m):
        if isinstance(e, unreal.MaterialExpressionCustom):
            return e
    return None


def _params(m):
    out = {}
    for e in MEL.get_material_expressions(m):
        if isinstance(e, (unreal.MaterialExpressionScalarParameter, unreal.MaterialExpressionVectorParameter)):
            out[str(e.get_editor_property("parameter_name"))] = e
        elif isinstance(e, unreal.MaterialExpressionSceneTexture):
            out["Dummy"] = e
    return out


def upgrade_pp_toon():
    m = unreal.load_asset(PP_PATH)
    custom = _find_custom(m)
    params = _params(m)
    y = 1200
    for name, value in NEW_SCALARS:
        if name not in params:
            p = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, y)
            p.set_editor_property("parameter_name", name)
            p.set_editor_property("default_value", value)
            params[name] = p
            y += 80
    for name, value in NEW_VECTORS:
        if name not in params:
            p = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, y)
            p.set_editor_property("parameter_name", name)
            p.set_editor_property("default_value", value)
            params[name] = p
            y += 120
    for name, value in RETUNE.items():
        params[name].set_editor_property("default_value", value)

    names = [str(ci.get_editor_property("input_name")) for ci in custom.get_editor_property("inputs")]
    order = [n for n in names] + [n for n in list(params.keys()) if n not in names]
    inputs = []
    for n in order:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    custom.set_editor_property("code", PP_CODE_V2.strip())
    for n in order:
        MEL.connect_material_expressions(params[n], "Color" if n == "Dummy" else "", custom, n)
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    return order
