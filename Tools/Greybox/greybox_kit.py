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
        # set_collision_enabled() alone was not saved with the map (the Seine slab and Tokyo lanterns kept full
        # collision, so Recast built a walkable navmesh island on the river): use the NoCollision profile, which is
        # serialized, and keep the slab out of navigation explicitly.
        smc.modify()
        smc.set_collision_profile_name("NoCollision")
        smc.set_editor_property("can_ever_affect_navigation", False)
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
    """Render triangles as collision. Also drops the simple (convex) collision: the mesh's cached nav/physics data was
    built from the import-time convex hull and kept blocking the navmesh until it was removed (SM_Tokyo_Torii)."""
    m = mesh_asset(mesh_path)
    if not m:
        return
    unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).remove_collisions(m)
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


# ---------------------------------------------------------------------------------------------------------------
# Visual polish: per-city look (time of day + grading + toon params) applied to an EXISTING map, no rebuild.
# build_city_map() destroys and respawns every actor; apply_look() only edits the Lighting actors in place, so it
# is safe to re-run while tuning. Values come from cfg["look"] in cities.py (the single source of truth).
# ---------------------------------------------------------------------------------------------------------------

LOOKS = f"{MAT}/Looks"


def _vec4(v):
    """1.0 -> (1,1,1,1); (r,g,b,a) -> as is; "HEX" -> sRGB hex as a linear colour with w = 1."""
    if isinstance(v, (int, float)):
        return unreal.Vector4(v, v, v, v)
    if isinstance(v, str):
        c = lin(v)
        return unreal.Vector4(c.r, c.g, c.b, 1.0)
    return unreal.Vector4(*v)


def _actor(cls):
    for a in EAS.get_all_level_actors():
        if isinstance(a, cls):
            return a
    return None


def toon_look_instance(city, params):
    """MI_PP_Toon_<City>: a constant instance of PP_Toon holding this city's toon parameters."""
    name = f"MI_PP_Toon_{city}"
    path = f"{LOOKS}/{name}"
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = AT.create_asset(name, LOOKS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", EAL.load_asset(f"{MAT}/PP_Toon"))
    for key, value in params.items():
        if isinstance(value, str):
            MEL.set_material_instance_vector_parameter_value(mi, key, lin(value))
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, key, float(value))
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi, False)
    return mi


def city_tint(hex_color, city, roof_hex, roof_amount=1.0):
    """MI_Tint_<HEX>_<City>: child of MI_Tint_<HEX> with the city's roof colour (M_ToonMaster v2 RoofTint)."""
    hex_color = hex_color.upper()
    name = f"MI_Tint_{hex_color}_{city}"
    path = f"{MAT}/Tints/{name}"
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = AT.create_asset(name, f"{MAT}/Tints", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", tint(hex_color))
    MEL.set_material_instance_vector_parameter_value(mi, "RoofTint", lin(roof_hex))
    MEL.set_material_instance_scalar_parameter_value(mi, "RoofAmount", roof_amount)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi, False)
    return mi


def apply_look(cfg, save=True):
    """Time of day, sky/fog, grading, tilt-shift DOF and toon parameters for the open city map (cfg["look"])."""
    look = cfg["look"]
    city = cfg["map"].rsplit("_", 1)[-1]

    sun = _actor(unreal.DirectionalLight)
    sun.set_actor_rotation(unreal.Rotator(0, look["sun_pitch"], look["sun_yaw"]), False)
    lc = sun.light_component
    lc.set_intensity(look["sun_intensity"])
    lc.set_light_color(lin(look["sun_color"]))
    lc.set_editor_property("light_source_angle", look.get("sun_angle", 0.5))

    sky = _actor(unreal.SkyLight).light_component
    sky.set_light_color(lin(look["sky_color"]))
    sky.set_intensity(look.get("sky_intensity", 1.0))

    fog = _actor(unreal.ExponentialHeightFog).component
    fog.set_editor_property("fog_inscattering_luminance", lin(look["fog_color"]))
    fog.set_editor_property("fog_density", look.get("fog_density", 0.02))
    fog.set_editor_property("start_distance", look.get("fog_start", 3000.0))
    fog.set_editor_property("fog_height_falloff", look.get("fog_falloff", 0.2))

    ppv = next(a for a in EAS.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume) and a.get_editor_property("unbound"))
    s = ppv.get_editor_property("settings")
    g = look.get("grading", {})
    dof = look.get("dof", {})
    values = {
        "white_temp": g.get("white_temp", 6500.0),
        "color_saturation": _vec4(g.get("saturation", 1.0)),
        "color_contrast": _vec4(g.get("contrast", 1.0)),
        "color_gain_shadows": _vec4(g.get("gain_shadows", 1.0)),
        "color_gain_highlights": _vec4(g.get("gain_highlights", 1.0)),
        "bloom_intensity": g.get("bloom", 0.3),
        "bloom_threshold": g.get("bloom_threshold", 1.5),
        "vignette_intensity": g.get("vignette", 0.3),
        "ambient_occlusion_intensity": g.get("ao", 0.5),
        "ambient_occlusion_radius": g.get("ao_radius", 80.0),
        "depth_of_field_focal_distance": dof.get("focal", 2500.0),
        "depth_of_field_fstop": dof.get("fstop", 2.0),
        "depth_of_field_sensor_width": dof.get("sensor", 36.0),
    }
    for prop, value in values.items():
        s.set_editor_property("override_" + prop, True)
        s.set_editor_property(prop, value)
    toon = toon_look_instance(city, look.get("toon", {}))
    wb = unreal.WeightedBlendables()
    blend = unreal.WeightedBlendable()
    blend.set_editor_property("weight", 1.0)
    blend.set_editor_property("object", toon)
    wb.set_editor_property("array", [blend])
    s.set_editor_property("weighted_blendables", wb)
    ppv.set_editor_property("settings", s)

    # Roof colour for this city's buildings (tinted MI -> per-city child with RoofTint)
    if look.get("roof"):
        for a in EAS.get_all_level_actors():
            if not str(a.get_folder_path()).startswith("City/Buildings") or not isinstance(a, unreal.StaticMeshActor):
                continue
            smc = a.static_mesh_component
            for i in range(smc.get_num_materials()):
                mat = smc.get_material(i)
                name = mat.get_name() if mat else ""
                if name.startswith("MI_Tint_"):
                    hex_color = name.split("_")[2]
                    smc.set_material(i, city_tint(hex_color, city, look["roof"]))

    # Glow: emissive props (lanterns, vending machines...): mesh-name fragment -> Emissive strength. Each actor keeps
    # its tint: MI_Glow_<HEX>_<strength x10> (child of M_ToonMaster with Tint + Emissive).
    for mesh_fragment, strength in look.get("glow", {}).items():
        for a in EAS.get_all_level_actors():
            if not isinstance(a, unreal.StaticMeshActor):
                continue
            smc = a.static_mesh_component
            if not smc.static_mesh or mesh_fragment not in smc.static_mesh.get_name():
                continue
            for i in range(max(1, smc.get_num_materials())):
                mat = smc.get_material(i)
                name = mat.get_name() if mat else ""
                hex_color = name.split("_")[2] if name.startswith(("MI_Tint_", "MI_Glow_")) else "FFFFFF"
                mi_name = f"MI_Glow_{hex_color}_{int(round(strength * 10))}"
                path = f"{MAT}/Tints/{mi_name}"
                if EAL.does_asset_exist(path):
                    glow = EAL.load_asset(path)
                else:
                    glow = AT.create_asset(mi_name, f"{MAT}/Tints", unreal.MaterialInstanceConstant,
                                           unreal.MaterialInstanceConstantFactoryNew())
                    glow.set_editor_property("parent", EAL.load_asset(f"{MAT}/M_ToonMaster"))
                    MEL.set_material_instance_vector_parameter_value(glow, "Tint", lin(hex_color))
                    MEL.set_material_instance_scalar_parameter_value(glow, "Emissive", float(strength))
                    MEL.update_material_instance(glow)
                    EAL.save_loaded_asset(glow, False)
                smc.set_material(i, glow)
    if save:
        LES.save_current_level()
    return city


def _paved_tint(hex_color, scale, joint, strength):
    """MI_Tint_<HEX>_Pave<scale>: MI_Tint child with the M_ToonMaster v2 running-bond paving pattern switched on."""
    hex_color = hex_color.upper()
    name = f"MI_Tint_{hex_color}_Pave{int(scale)}"
    path = f"{MAT}/Tints/{name}"
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = AT.create_asset(name, f"{MAT}/Tints", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", tint(hex_color))
    MEL.set_material_instance_scalar_parameter_value(mi, "PavingScale", float(scale))
    MEL.set_material_instance_scalar_parameter_value(mi, "PavingJoint", float(joint))
    MEL.set_material_instance_scalar_parameter_value(mi, "PavingStrength", float(strength))
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi, False)
    return mi


def apply_street_detail(cfg, save=True):
    """Q6: road colour, paving pattern, centre dashes, crosswalks and kerbs for the open map (cfg["street"]).
    Markings are thin no-collision slabs a few cm above the road (stacked slabs at the same height z-fight).
    Re-runnable: everything in City/Markings is replaced."""
    st = cfg["street"]
    L = cfg["length"]
    road = cfg.get("road_width", 800)
    walk = cfg.get("sidewalk", 350)
    for a in EAS.get_all_level_actors():
        if a.get_actor_label() in ("Dash", "Zebra", "Kerb") or str(a.get_folder_path()).startswith("City/Markings"):
            EAS.destroy_actor(a)

    by_label = {}
    for a in EAS.get_all_level_actors():
        by_label.setdefault(a.get_actor_label(), []).append(a)

    def paint(label, hex_color, pave=None):
        for a in by_label.get(label, []) + [x for k, v in by_label.items() if k.startswith(label + "_") or k.startswith(label + "2") for x in v]:
            if isinstance(a, unreal.StaticMeshActor):
                mi = _paved_tint(hex_color, *pave) if pave else tint(hex_color)
                a.static_mesh_component.set_material(0, mi)

    paint("Road", st["road"], st.get("road_paving"))
    paint("Sidewalk", st["sidewalk"], st.get("sidewalk_paving"))
    for label, (hex_color, pave) in st.get("plazas", {}).items():
        paint(label, hex_color, pave)

    folder = "City/Markings"
    half = (L + 6000) / 2
    z = 3.0  # road top is at 2 cm
    if st.get("dashes", True):
        x = -half
        while x < half:
            if not any(c - 400 <= x <= c + 400 for c in st.get("crosswalks", [])):
                slab("Dash", (x, 0, z), (300, 14, 1), st["marking"], folder=folder, collide=False)
            x += 700
    for cx in st.get("crosswalks", []):
        y = -road / 2 + 60
        while y <= road / 2 - 60:
            slab("Zebra", (cx, y, z), (320, 45, 1), st["marking"], folder=folder, collide=False)
            y += 90
    for s in (1, -1):
        slab("Kerb", (0, s * (road / 2 + 10), 3.5), (L + 6000, 22, 7), st["kerb"], folder=folder, collide=False)
    if save:
        LES.save_current_level()
    return len([a for a in EAS.get_all_level_actors() if a.get_actor_label() in ("Dash", "Zebra", "Kerb")])


def apply_nav_fixes(cfg, save=True):
    """Playtest nav fixes for the open map (re-runnable):
    - cfg["complex_collision"] meshes use their render triangles as collision (a torii's auto convex hull is one
      solid 7 m slab: it walled off the gate and put a nav hole exactly where the Shrine marker stands);
    - cfg["nav_blockers"]: NavModifierVolumes with NavArea_Null (no navmesh): the Seine slab is walkable collision,
      so Recast built an island on the water between the quais that pawns could end up on but never path out of."""
    for mesh_path in cfg.get("complex_collision", []):
        set_complex_collision(mesh_path)
    for a in EAS.get_all_level_actors():
        if a.get_actor_label().startswith("NavBlock_"):
            EAS.destroy_actor(a)
    null_area = unreal.load_class(None, "/Script/NavigationSystem.NavArea_Null")
    for b in cfg.get("nav_blockers", []):
        (x0, y0, z0), (x1, y1, z1) = b["min"], b["max"]
        v = EAS.spawn_actor_from_class(unreal.NavModifierVolume, V((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))
        v.set_actor_scale3d(V((x1 - x0) / 200.0, (y1 - y0) / 200.0, (z1 - z0) / 200.0))  # default brush is 200 cm
        v.set_editor_property("area_class", null_area)
        v.set_actor_label("NavBlock_" + b["label"])
        v.set_folder_path("Gameplay")
    nav = unreal.NavigationSystemV1.get_navigation_system(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world())
    if save:
        LES.save_current_level()
    return [a.get_actor_label() for a in EAS.get_all_level_actors() if a.get_actor_label().startswith("NavBlock_")]


MENU_LOOK = dict(
    sun_pitch=-38, sun_yaw=50, sun_color="FFE4C4", sun_intensity=6.0,
    sky_color="CFDDF5", sky_intensity=1.2, fog_color="F3DCC4", fog_density=0.06, fog_start=0, fog_falloff=0.05,
    grading=dict(white_temp=6800, saturation=1.05, contrast=1.04, bloom=0.3, vignette=0.45, ao=0.6),
    dof=dict(focal=4300, fstop=2.0, sensor=120),
    toon=dict(LitLevel=1.0, LightNorm=1.0, LightHueKeep=0.55, ShadowTint="C9C0EA", HazeColor="F3DCC4", HazeStart=3500, HazeRange=6000, HazeMax=0.4),
    table="EADFC5", table_edge="7A5238",
    roofs={(-1, -1): "7C8AA0", (1, -1): "C46A4A", (-1, 1): "4A4F63", (1, 1): "B8613F"},  # Paris, Madrid, Tokyo, Rome
)


def apply_menu_look(look=MENU_LOOK, save=True):
    """Q7: the main-menu diorama as a lit tabletop in warm haze instead of a grey slab in a navy void (open Lvl_MainMenu)."""
    apply_look(dict(map="/Game/LinguaSpy/Maps/Lvl_MainMenu", look=look), save=False)  # look -> MI_PP_Toon_MainMenu
    ground = next(a for a in EAS.get_all_level_actors() if a.get_actor_label() == "Cube")
    ground.static_mesh_component.set_material(0, tint(look["table"]))
    for a in EAS.get_all_level_actors():
        if a.get_actor_label() == "TableEdge":
            EAS.destroy_actor(a)
    edge = slab("TableEdge", (0, 0, -75), (8300, 8300, 60), look["table_edge"], folder="Diorama")
    for a in EAS.get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        smc = a.static_mesh_component
        mat = smc.get_material(0)
        if mat and mat.get_name() in ("MI_Roof",) or (mat and mat.get_name().startswith("MI_Tint_") and a.get_actor_label().startswith("Cube") and a.get_actor_location().z > 400 and abs(a.get_actor_scale3d().z - 0.8) < 0.01):
            loc = a.get_actor_location()
            key = (1 if loc.x > 0 else -1, 1 if loc.y > 0 else -1)
            smc.set_material(0, tint(look["roofs"][key]))
    if save:
        LES.save_current_level()
    return edge.get_actor_label()
