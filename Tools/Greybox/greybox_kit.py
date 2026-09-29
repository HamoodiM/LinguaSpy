"""LinguaSpy city map builder (run inside the Unreal editor).

A city is a dict (see cities.py): a main street along X with facade rows on both sides, optional plazas/rivers,
special props, NPCs, mission markers/pickups, pedestrians, a landmark, a lighting preset and the toon post-process.

    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Greybox/greybox_kit.py", encoding="utf-8").read())
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Greybox/cities.py", encoding="utf-8").read())
    build_city_map(PARIS)

Kit meshes face +Y in UE (Blender -Y). North-row buildings are rotated 180 degrees so they face the street.
Units: cm.
"""
import unreal, random, math

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
V = unreal.Vector
MAT = "/Game/LinguaSpy/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"


def lin(h):
    def c(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    return unreal.LinearColor(c(int(h[0:2], 16)), c(int(h[2:4], 16)), c(int(h[4:6], 16)), 1.0)


_tint_cache = {}


def tint(hex_color):
    """MI_Tint_<hex> on M_ToonMaster (cached)."""
    hex_color = hex_color.upper()
    if hex_color in _tint_cache:
        return _tint_cache[hex_color]
    path = f"{MAT}/Tints/MI_Tint_{hex_color}"
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = AT.create_asset(f"MI_Tint_{hex_color}", f"{MAT}/Tints", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", EAL.load_asset(f"{MAT}/M_ToonMaster"))
        MEL.set_material_instance_vector_parameter_value(mi, "Tint", lin(hex_color))
        EAL.save_loaded_asset(mi, False)
    _tint_cache[hex_color] = mi
    return mi


def mesh_asset(path):
    return EAL.load_asset(path)


def place(mesh_path, loc, yaw=0.0, tint_hex=None, scale=(1, 1, 1), folder="City", label=None, collide=True):
    mesh = mesh_asset(mesh_path)
    if not mesh:
        unreal.log_warning(f"Missing mesh {mesh_path}")
        return None
    a = EAS.spawn_actor_from_object(mesh, V(*loc), unreal.Rotator(0, 0, yaw))
    a.set_actor_scale3d(V(*scale))
    smc = a.static_mesh_component
    if tint_hex:
        for i in range(max(1, smc.get_num_materials())):
            smc.set_material(i, tint(tint_hex))
    if not collide:
        smc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    a.set_folder_path(folder)
    if label:
        a.set_actor_label(label)
    return a


def slab(label, center, size, hex_color, folder="City/Ground", collide=True):
    """A flat-colored engine cube (ground, road, water...). size in cm."""
    a = place(CUBE, center, 0, hex_color, (size[0] / 100, size[1] / 100, size[2] / 100), folder, label, collide)
    return a


def mesh_width(mesh_path):
    m = mesh_asset(mesh_path)
    return m.get_bounds().box_extent.x * 2 if m else 600


def set_complex_collision(mesh_path):
    m = mesh_asset(mesh_path)
    if not m:
        return
    body = m.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        EAL.save_loaded_asset(m, False)


# ---------------------------------------------------------------------------------------------------------------
# Main entry
# ---------------------------------------------------------------------------------------------------------------

def build_city_map(cfg):
    random.seed(cfg.get("seed", 1))
    path = cfg["map"]
    if EAL.does_asset_exist(path):
        LES.load_level(path)
        for a in EAS.get_all_level_actors():
            if a.get_class().get_name() not in ("WorldSettings", "Brush", "WorldDataLayers"):
                EAS.destroy_actor(a)
    else:
        LES.new_level(path, False)

    L = cfg["length"]                   # street length (cm), centered on 0
    road = cfg.get("road_width", 800)
    walk = cfg.get("sidewalk", 350)
    row_y = road / 2 + walk             # building fronts start here
    colors = cfg["colors"]

    # Ground, road, sidewalks
    slab("Ground", (0, 0, -50), (L + 6000, 7000, 100), colors["ground"])
    slab("Road", (0, 0, 1), (L + 6000, road, 2), colors["road"])
    for s in (1, -1):
        slab("Sidewalk", (0, s * (road / 2 + walk / 2), 2), (L + 6000, walk, 4), colors["sidewalk"])
    for extra in cfg.get("slabs", []):
        slab(extra["label"], extra["center"], extra["size"], extra["color"], collide=extra.get("collide", True))

    # Facade rows (skipping gaps for plazas / special buildings)
    for side, s in (("north", 1), ("south", -1)):
        gaps = cfg.get("gaps", {}).get(side, [])
        specials = {sp["x"]: sp for sp in cfg.get("row_specials", {}).get(side, [])}
        x = -L / 2
        i = 0
        while x < L / 2:
            special = next((sp for sx, sp in specials.items() if abs(sx - x) < 1 or (x <= sx < x + 300)), None)
            if any(g[0] <= x < g[1] for g in gaps):
                x = next(g[1] for g in gaps if g[0] <= x < g[1])
                continue
            mesh_path = special["mesh"] if special else random.choice(cfg["buildings"])
            w = mesh_width(mesh_path)
            depth = mesh_asset(mesh_path).get_bounds().box_extent.y * 2
            cx = x + w / 2
            cy = s * (row_y + depth / 2 - 30)
            color = special.get("tint") if special else random.choice(cfg["wall_tints"])
            place(mesh_path, (cx, cy, 0), 180.0 if s > 0 else 0.0, color, folder=f"City/Buildings/{side}",
                  label=(special.get("label") if special else None))
            if special:
                specials.pop(special["x"], None)
            x += w + cfg.get("building_gap", 20)
            i += 1

    # Props, landmarks, special meshes
    for p in cfg.get("props", []):
        place(p["mesh"], p["loc"], p.get("yaw", 0.0), p.get("tint"), p.get("scale", (1, 1, 1)),
              folder=p.get("folder", "City/Props"), label=p.get("label"), collide=p.get("collide", True))
    for mesh_path in cfg.get("complex_collision", []):
        set_complex_collision(mesh_path)

    # Street furniture along both sidewalks
    fx = cfg.get("furniture", {})
    if fx:
        for s in (1, -1):
            x = -L / 2 + 400
            k = 0
            while x < L / 2 - 200:
                if not any(g[0] - 200 <= x <= g[1] + 200 for g in fx.get("skip", [])):
                    mesh_path = fx["lamp"] if k % 2 == 0 else fx["tree"]
                    place(mesh_path, (x, s * (road / 2 + 60), 4), 0, None, folder="City/Furniture")
                x += fx.get("spacing", 700)
                k += 1

    _lighting(cfg)
    _gameplay(cfg)
    LES.save_current_level()
    return path


def _lighting(cfg):
    li = cfg["lighting"]
    sun = EAS.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 1500), unreal.Rotator(0, li["sun_pitch"], li["sun_yaw"]))
    sun.set_actor_label("Sun"); sun.set_folder_path("Lighting")
    lc = sun.light_component
    lc.set_intensity(li.get("sun_intensity", 6.0))
    lc.set_light_color(lin(li["sun_color"]))
    lc.set_editor_property("atmosphere_sun_light", True)
    sky = EAS.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0)); sky.set_folder_path("Lighting")
    sl = EAS.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 600)); sl.set_folder_path("Lighting")
    sl.light_component.set_editor_property("real_time_capture", True)
    sl.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sl.light_component.set_light_color(lin(li.get("sky_color", "FFFFFF")))
    fog = EAS.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0)); fog.set_folder_path("Lighting")
    fog.component.set_editor_property("fog_inscattering_luminance", lin(li.get("fog_color", "C8D8F0")))
    fog.component.set_editor_property("fog_density", li.get("fog_density", 0.01))

    # Toon post-process, unbound
    ppv = EAS.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0))
    ppv.set_actor_label("ToonPostProcess"); ppv.set_folder_path("Lighting")
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    pp = EAL.load_asset(f"{MAT}/PP_Toon")
    if pp:
        wb = unreal.WeightedBlendables()
        blend = unreal.WeightedBlendable()
        blend.set_editor_property("weight", 1.0)
        blend.set_editor_property("object", pp)
        wb.set_editor_property("array", [blend])
        settings.set_editor_property("weighted_blendables", wb)
    for prop, value in [("auto_exposure_min_brightness", 1.0), ("auto_exposure_max_brightness", 1.0),
                        ("bloom_intensity", 0.2), ("vignette_intensity", 0.25), ("motion_blur_amount", 0.0)]:
        settings.set_editor_property("override_" + prop, True)
        settings.set_editor_property(prop, value)
    ppv.set_editor_property("settings", settings)


def _gameplay(cfg):
    L = cfg["length"]
    ps = EAS.spawn_actor_from_class(unreal.PlayerStart, V(*cfg["player_start"]), unreal.Rotator(0, 0, 0))
    ps.set_folder_path("Gameplay")
    nav = EAS.spawn_actor_from_class(unreal.NavMeshBoundsVolume, V(cfg.get("nav_center_x", 0), 0, 100))
    nav.set_actor_scale3d(V((L + 4000) / 200, cfg.get("nav_depth", 7000) / 200, 5))
    nav.set_folder_path("Gameplay")

    for n in cfg.get("npcs", []):
        a = EAS.spawn_actor_from_class(unreal.NpcCharacter, V(*n["loc"]), unreal.Rotator(0, 0, n.get("yaw", 0)))
        a.set_actor_label("NPC_" + n["id"])
        a.set_folder_path("Gameplay/NPCs")
        a.set_editor_property("display_name", n["name"])
        a.set_editor_property("conversation", EAL.load_asset(n["conversation"]))
        opts = []
        for o in n.get("options", []):
            opt = unreal.NpcConversationOption()
            opt.set_editor_property("conversation", EAL.load_asset(o["conversation"]))
            opt.set_editor_property("required_mission", o.get("mission", ""))
            opt.set_editor_property("required_flags", o.get("flags", []))
            opt.set_editor_property("blocked_flags", o.get("blocked", []))
            opts.append(opt)
        a.set_editor_property("conversation_options", opts)
        a.get_editor_property("mission_target").set_editor_property("target_id", n["id"])
        if n.get("mesh"):
            a.get_editor_property("mesh").set_skeletal_mesh_asset(EAL.load_asset(n["mesh"]))

    for m in cfg.get("markers", []):
        a = EAS.spawn_actor_from_class(unreal.MissionMarker, V(*m["loc"]))
        a.set_actor_label("Marker_" + m["id"]); a.set_folder_path("Gameplay/Mission")
        a.get_editor_property("mission_target").set_editor_property("target_id", m["id"])
    for p in cfg.get("pickups", []):
        a = EAS.spawn_actor_from_class(unreal.MissionPickup, V(*p["loc"]))
        a.set_actor_label("Pickup_" + p["id"]); a.set_folder_path("Gameplay/Mission")
        a.get_editor_property("mission_target").set_editor_property("target_id", p["id"])

    variants = [EAL.load_asset(v) for v in cfg.get("pedestrian_meshes", [])]
    for loc in cfg.get("pedestrians", []):
        a = EAS.spawn_actor_from_class(unreal.PedestrianCharacter, V(*loc), unreal.Rotator(0, 0, random.uniform(0, 360)))
        a.set_folder_path("Gameplay/Pedestrians")
        if variants:
            a.set_editor_property("mesh_variants", variants)
            a.get_editor_property("mesh").set_skeletal_mesh_asset(random.choice(variants))

    ws = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings()
    ws.set_editor_property("default_game_mode", EAL.load_asset("/Game/LinguaSpy/Blueprints/BP_SpyGameMode").generated_class())
