"""City layouts for greybox_kit.build_city_map. Coordinates in cm; the street runs along X, sidewalks at |y| 400..750.
North side = +Y (buildings rotated to face -Y), south side = -Y.
"""
A = "/Game/LinguaSpy/Art"
CH = A + "/Characters"
FR = "/Game/LinguaSpy/Data/French"

PARIS = dict(
    map="/Game/LinguaSpy/Maps/Lvl_Paris",
    seed=7,
    length=7000,
    colors=dict(ground="C9C3B8", road="8A8FA0", sidewalk="E6DFD2"),
    buildings=[f"{A}/Paris/SM_Paris_Building_{i}" for i in range(4)],
    wall_tints=["F3E3C3", "EFC2B8", "CFC3E3", "B8D5E6", "F2DE9B", "F7F4EE"],
    gaps=dict(south=[(-2700, -1500)]),                      # Place du Tertre
    row_specials=dict(north=[dict(x=-1800, mesh=f"{A}/Paris/SM_Paris_Cafe", tint="F3E3C3", label="Cafe")]),
    furniture=dict(lamp=f"{A}/Paris/SM_Paris_Lamp", tree=f"{A}/Paris/SM_Paris_Tree", spacing=800,
                   skip=[(-1900, -800), (-200, 200), (1400, 2300), (700, 1100)]),
    slabs=[
        dict(label="PlaceDuTertre", center=(-2100, -1500, 3), size=(1200, 1500, 6), color="D8CFC0"),
        dict(label="Seine", center=(4100, 0, 4), size=(800, 7000, 6), color="7FA7C9", collide=False),  # top at 7cm, above road/sidewalk (no z-fighting),
        dict(label="Quai_W_N", center=(3700, 1950, 40), size=(60, 3100, 80), color="B8B4AA"),
        dict(label="Quai_W_S", center=(3700, -1950, 40), size=(60, 3100, 80), color="B8B4AA"),
        dict(label="Quai_E_N", center=(4500, 1950, 40), size=(60, 3100, 80), color="B8B4AA"),
        dict(label="Quai_E_S", center=(4500, -1950, 40), size=(60, 3100, 80), color="B8B4AA"),
    ],
    props=[
        # Café terrace
        dict(mesh=f"{A}/Shared/SM_CafeSet", loc=(-1700, 520, 4)),
        dict(mesh=f"{A}/Shared/SM_CafeSet", loc=(-1000, 520, 4)),
        dict(mesh=f"{A}/Shared/SM_Counter", loc=(-1350, 690, 4), yaw=180, tint="A67B5B"),
        # Kiosk
        dict(mesh=f"{A}/Paris/SM_Paris_Kiosk", loc=(0, -600, 4), tint="3E7D5A"),
        # Metro entrance + ticket booth
        dict(mesh=f"{A}/Paris/SM_Paris_MetroArch", loc=(1600, -560, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_TicketBooth", loc=(2100, -620, 4), tint="3D5A80"),
        # Boulangerie front on the north row
        dict(mesh=f"{A}/Paris/SM_Paris_BakeryFront", loc=(900, 735, 4), yaw=180, tint="C98E4A"),
        # Place du Tertre
        dict(mesh=f"{A}/Paris/SM_Paris_Easel", loc=(-2150, -700, 4), yaw=20),
        dict(mesh=f"{A}/Shared/SM_Bench", loc=(-2500, -1200, 4), yaw=90, tint="A67B5B"),
        dict(mesh=f"{A}/Shared/SM_Bench", loc=(-1700, -1200, 4), yaw=-90, tint="A67B5B"),
        dict(mesh=f"{A}/Paris/SM_Paris_Tree", loc=(-2500, -1800, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_Tree", loc=(-1700, -1800, 4)),
        dict(mesh=f"{A}/Shared/SM_Parasol", loc=(-2100, -1500, 4)),
        # Seine: bridge (deck along X) + far bank + Eiffel
        dict(mesh=f"{A}/Paris/SM_Paris_Bridge", loc=(4100, 0, -80), yaw=90, tint="D8CFC0", label="Bridge"),
        dict(mesh=f"{A}/Paris/SM_Paris_Tree", loc=(5200, 900, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_Tree", loc=(5200, -900, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_Eiffel", loc=(7800, 2600, 0), scale=(1.4, 1.4, 1.4), collide=False),
    ],
    complex_collision=[f"{A}/Paris/SM_Paris_Bridge", f"{A}/Paris/SM_Paris_MetroArch"],
    # The Seine slab is walkable collision: keep the navmesh off the water between the quais (bridge deck |y| < 407)
    nav_blockers=[dict(label="Seine_N", min=(3730, 430, -50), max=(4470, 3600, 60)),
                  dict(label="Seine_S", min=(3730, -3600, -50), max=(4470, -430, 60))],
    lighting=dict(sun_pitch=-48, sun_yaw=35, sun_color="FFEAD2", sun_intensity=6.0, sky_color="DDE6FF", fog_color="C8D8F0"),
    player_start=(-3200, 0, 100),
    nav_center_x=500,
    npcs=[
        dict(id="Camille", name="Camille (barista)", mesh=f"{CH}/SK_Barista", loc=(-1350, 600, 98), yaw=-90,
             conversation=f"{FR}/DA_Conv_Cafe",
             options=[dict(conversation=f"{FR}/DA_Conv_Cafe_Croissant", mission="Paris_Croissant")]),
        dict(id="Marcel", name="Marcel (kiosk vendor)", mesh=f"{CH}/SK_Vendor", loc=(0, -460, 98), yaw=90,
             conversation=f"{FR}/DA_Conv_Kiosk",
             options=[dict(conversation=f"{FR}/DA_Conv_Kiosk_Retry", mission="Paris_FirstContact", flags=["LostTail"]),
                      dict(conversation=f"{FR}/DA_Conv_Kiosk_Croissant", mission="Paris_Croissant")]),
        dict(id="Luc", name="Luc (metro booth)", mesh=f"{CH}/SK_Worker", loc=(2100, -470, 98), yaw=90,
             conversation=f"{FR}/DA_Conv_Metro_Luc"),
        dict(id="Sophie", name="Sophie (boulangère)", mesh=f"{CH}/SK_Barista", loc=(900, 600, 98), yaw=-90,
             conversation=f"{FR}/DA_Conv_Bakery_Sophie"),
        dict(id="Hugo", name="Hugo (painter)", mesh=f"{CH}/SK_Young", loc=(-2000, -600, 98), yaw=100,
             conversation=f"{FR}/DA_Conv_Painter_Hugo"),
        dict(id="Elise", name="Élise (contact)", mesh=f"{CH}/SK_Tourist", loc=(4300, 250, 98), yaw=180,
             conversation=f"{FR}/DA_Conv_Bridge_Elise"),
    ],
    markers=[dict(id="Metro", loc=(1600, -450, 100)), dict(id="Bridge", loc=(4000, 0, 100))],
    pickups=[dict(id="Package", loc=(-1000, 430, 20))],
    pedestrian_meshes=[f"{CH}/SK_Elder", f"{CH}/SK_Young", f"{CH}/SK_Tourist", f"{CH}/SK_Worker", f"{CH}/SK_Vendor"],
    pedestrians=[(-2800, 550, 98), (-500, -550, 98), (500, 550, 98), (1200, -550, 98), (2600, 550, 98), (-2200, -1300, 98)],
)

PEDS = [f"{CH}/SK_Elder", f"{CH}/SK_Young", f"{CH}/SK_Tourist", f"{CH}/SK_Worker", f"{CH}/SK_Vendor", f"{CH}/SK_Barista"]
ES = "/Game/LinguaSpy/Data/Spanish"
JA = "/Game/LinguaSpy/Data/Japanese"
IT = "/Game/LinguaSpy/Data/Italian"

# ---------------------------------------------------------------------------------------------------------------
# Madrid: warm terracotta street opening onto an arcaded plaza (Plaza Mayor feel) with a fountain; palace at the end
# ---------------------------------------------------------------------------------------------------------------
MADRID = dict(
    map="/Game/LinguaSpy/Maps/Lvl_Madrid",
    seed=11,
    length=7000,
    colors=dict(ground="E6D5B8", road="B59E86", sidewalk="EFE3CF"),
    buildings=[f"{A}/Madrid/SM_Madrid_Building_{i}" for i in range(4)],
    wall_tints=["F3E3C3", "F2C9A0", "E8B4A0", "F2DE9B", "FFF1D6"],
    gaps=dict(north=[(-1700, 1700)]),
    row_specials=dict(south=[dict(x=-2600, mesh=f"{A}/Madrid/SM_Madrid_TapasBar", tint="F2C9A0", label="TapasBar")]),
    furniture=dict(lamp=f"{A}/Madrid/SM_Madrid_Lamp", tree=f"{A}/Madrid/SM_Madrid_OrangeTree", spacing=900,
                   skip=[(-2700, -1500), (2300, 3000)]),
    slabs=[dict(label="PlazaMayor", center=(0, 2300, 3), size=(3400, 3200, 6), color="E9D2AE")],
    props=[
        dict(mesh=f"{A}/Madrid/SM_Madrid_Arcade", loc=(0, 4450, 4), yaw=180, tint="D98E5F", label="Arcade"),
        dict(mesh=f"{A}/Madrid/SM_Madrid_Arcade", loc=(-2200, 2400, 4), yaw=90, tint="E8B4A0"),
        dict(mesh=f"{A}/Madrid/SM_Madrid_Arcade", loc=(2200, 2400, 4), yaw=-90, tint="E8B4A0"),
        dict(mesh=f"{A}/Madrid/SM_Madrid_Fountain", loc=(0, 2200, 4), tint="E6DFD2"),
        dict(mesh=f"{A}/Madrid/SM_Madrid_OrangeTree", loc=(-1100, 1400, 4)),
        dict(mesh=f"{A}/Madrid/SM_Madrid_OrangeTree", loc=(1100, 1400, 4)),
        dict(mesh=f"{A}/Shared/SM_CafeSet", loc=(-2400, -520, 4)),
        dict(mesh=f"{A}/Shared/SM_CafeSet", loc=(-1750, -520, 4)),
        dict(mesh=f"{A}/Madrid/SM_Madrid_Parasol", loc=(-2400, -520, 4)),
        dict(mesh=f"{A}/Madrid/SM_Madrid_Parasol", loc=(-1750, -520, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_MetroArch", loc=(2600, -560, 4)),
        dict(mesh=f"{A}/Madrid/SM_Madrid_Palace", loc=(6200, 0, 0), yaw=90, tint="F3E3C3", collide=False),
        # Phase 5: taquilla (retinted Paris ticket booth until SM_Madrid_Taquilla exists)
        dict(mesh=f"{A}/Paris/SM_Paris_TicketBooth", loc=(1450, 3450, 4), yaw=-135, tint="D98E5F", label="Taquilla"),
    ],
    complex_collision=[f"{A}/Madrid/SM_Madrid_Arcade", f"{A}/Paris/SM_Paris_MetroArch"],
    lighting=dict(sun_pitch=-55, sun_yaw=20, sun_color="FFE2B8", sun_intensity=7.0, sky_color="FFF0DC", fog_color="F0DCC0"),
    player_start=(-3200, 0, 100),
    nav_center_x=500,
    npcs=[
        dict(id="Javier", name="Javier (camarero)", mesh=f"{CH}/SK_Vendor", loc=(-2050, -620, 98), yaw=90, conversation=f"{ES}/DA_Conv_TapasBar",
             options=[dict(conversation=f"{ES}/DA_Conv_Javier_Palacio", mission="Madrid_Palacio")]),
        dict(id="Carmen", name="Carmen (madrileña)", mesh=f"{CH}/SK_Elder", loc=(400, 1800, 98), yaw=-90, conversation=f"{ES}/DA_Conv_Madrid_Carmen",
             options=[dict(conversation=f"{ES}/DA_Conv_Carmen_Palacio", mission="Madrid_Palacio")]),
        dict(id="Diego", name="Diego (contact)", mesh=f"{CH}/SK_Young", loc=(2900, -480, 98), yaw=90, conversation=f"{ES}/DA_Conv_Madrid_Diego",
             options=[dict(conversation=f"{ES}/DA_Conv_Diego_Palacio", mission="Madrid_Palacio")]),
        # Phase 5: El Palacio
        dict(id="Rosa", name="Rosa (taquilla)", mesh=f"{CH}/SK_Barista", loc=(1250, 3250, 98), yaw=-135, conversation=f"{ES}/DA_Conv_Madrid_Rosa"),
        dict(id="Alvaro", name="Álvaro (guardia)", mesh=f"{CH}/SK_Worker", loc=(350, 3950, 98), yaw=-90, conversation=f"{ES}/DA_Conv_Madrid_Alvaro",
             options=[dict(conversation=f"{ES}/DA_Conv_Madrid_Alvaro_Retry", mission="Madrid_Palacio", flags=["AskedCarmen"])]),
    ],
    markers=[dict(id="Metro", loc=(2600, -420, 100)), dict(id="PalaceGate", loc=(0, 3850, 100))],
    pickups=[dict(id="Folder", loc=(-2050, 2400, 20))],
    nav_depth=9000,  # the PalaceGate marker (y 3850) and Alvaro were outside the default +-3500 nav bounds
    pedestrian_meshes=PEDS,
    pedestrians=[(-2800, 550, 98), (-600, 1200, 98), (600, 2600, 98), (1200, -550, 98), (2900, 550, 98), (-1000, -550, 98)],
)

# ---------------------------------------------------------------------------------------------------------------
# Tokyo: narrow shopping street with signs, konbini, vending machines, lanterns; shrine with torii; Tokyo Tower beyond
# ---------------------------------------------------------------------------------------------------------------
TOKYO = dict(
    map="/Game/LinguaSpy/Maps/Lvl_Tokyo",
    seed=21,
    length=7000,
    road_width=600,
    sidewalk=300,
    colors=dict(ground="CFCBD6", road="6E7385", sidewalk="E3E0EA"),
    buildings=[f"{A}/Tokyo/SM_Tokyo_Building_{i}" for i in range(4)],
    wall_tints=["F7F4EE", "E6E1EC", "F4D6DE", "D6E8EE", "F2E6C9"],
    gaps=dict(south=[(1300, 3200)]),
    row_specials=dict(north=[dict(x=-1500, mesh=f"{A}/Tokyo/SM_Tokyo_Konbini", tint="F7F4EE", label="Konbini")]),
    furniture=dict(lamp=f"{A}/Tokyo/SM_Tokyo_Lamp", tree=f"{A}/Tokyo/SM_Tokyo_Tree", spacing=1000,
                   skip=[(1200, 3300), (-1300, -900)]),  # no lamp post wedging the player against Sato at the konbini
    slabs=[dict(label="ShrineGrounds", center=(2250, -1900, 3), size=(1900, 2600, 6), color="D9CFC0")],
    complex_collision=[f"{A}/Tokyo/SM_Tokyo_Torii"],  # the auto convex hull turned the gate into a solid wall
    props=[
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Shrine", loc=(2250, -2900, 4), tint="F7F4EE", label="Shrine"),
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Torii", loc=(2250, -1000, 4)),
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Tree", loc=(1600, -1700, 4)),
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Tree", loc=(2900, -1700, 4)),
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Vending", loc=(-300, 520, 4), yaw=180, tint="D9534F"),
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Vending", loc=(-180, 520, 4), yaw=180, tint="8FB8DE"),
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Vending", loc=(900, -520, 4), tint="F2C14E"),
    ] + [dict(mesh=f"{A}/Tokyo/SM_Tokyo_Lantern", loc=(x, s * 560, 330), collide=False)
         for x in range(-3000, 3200, 600) for s in (1, -1) if not (s < 0 and 1200 < x < 3300)] + [
        dict(mesh=f"{A}/Tokyo/SM_Tokyo_Tower", loc=(6500, 2600, 0), scale=(1.4, 1.4, 1.4), collide=False),
    ],
    lighting=dict(sun_pitch=-50, sun_yaw=60, sun_color="F2F2FF", sun_intensity=6.0, sky_color="E8E4FF", fog_color="D8D8F0"),
    player_start=(-3200, 0, 100),
    nav_center_x=500,
    npcs=[
        dict(id="Sato", name="Sato-san (konbini)", mesh=f"{CH}/SK_Worker", loc=(-1050, 470, 98), yaw=-90, conversation=f"{JA}/DA_Conv_Konbini"),
        dict(id="Tanaka", name="Tanaka-san", mesh=f"{CH}/SK_Elder", loc=(500, -470, 98), yaw=90, conversation=f"{JA}/DA_Conv_Tokyo_Tanaka"),
        dict(id="Yuki", name="Yuki (contact)", mesh=f"{CH}/SK_Tourist", loc=(2600, -1500, 98), yaw=180, conversation=f"{JA}/DA_Conv_Tokyo_Yuki"),
    ],
    markers=[dict(id="Shrine", loc=(2250, -1000, 100))],
    pedestrian_meshes=PEDS,
    pedestrians=[(-2800, 450, 98), (-1800, -450, 98), (0, 450, 98), (1000, -450, 98), (2800, 450, 98), (-400, -450, 98)],
)

# ---------------------------------------------------------------------------------------------------------------
# Rome: ochre street opening onto a piazza with a fountain; trattoria with parasols; Vespas; Colosseum beyond
# ---------------------------------------------------------------------------------------------------------------
ROME = dict(
    map="/Game/LinguaSpy/Maps/Lvl_Rome",
    seed=31,
    length=7000,
    colors=dict(ground="D8C3A0", road="A89A88", sidewalk="E6D8C0"),
    buildings=[f"{A}/Rome/SM_Rome_Building_{i}" for i in range(4)],
    wall_tints=["E3B778", "D98E5F", "F0D5A8", "E8C39E", "C97B5A"],
    gaps=dict(north=[(-1600, 1600)]),
    row_specials=dict(south=[dict(x=-2600, mesh=f"{A}/Rome/SM_Rome_Trattoria", tint="E3B778", label="Trattoria")]),
    furniture=dict(lamp=f"{A}/Rome/SM_Rome_Lamp", tree=f"{A}/Rome/SM_Rome_Pine", spacing=900, skip=[(-2700, -1500)]),
    slabs=[dict(label="Piazza", center=(0, 2200, 3), size=(3200, 3000, 6), color="E8D6B5")],
    props=[
        dict(mesh=f"{A}/Rome/SM_Rome_Building_1", loc=(-1100, 4200, 4), yaw=180, tint="D98E5F"),
        dict(mesh=f"{A}/Rome/SM_Rome_Building_0", loc=(1100, 4200, 4), yaw=180, tint="F0D5A8"),
        dict(mesh=f"{A}/Rome/SM_Rome_Fountain", loc=(0, 2200, 4), tint="E3C89A"),
        dict(mesh=f"{A}/Rome/SM_Rome_Pine", loc=(-1300, 1300, 4)),
        dict(mesh=f"{A}/Rome/SM_Rome_Pine", loc=(1300, 1300, 4)),
        dict(mesh=f"{A}/Shared/SM_CafeSet", loc=(-2450, -520, 4)),
        dict(mesh=f"{A}/Shared/SM_CafeSet", loc=(-1850, -520, 4)),
        dict(mesh=f"{A}/Rome/SM_Rome_Parasol", loc=(-2450, -520, 4)),
        dict(mesh=f"{A}/Rome/SM_Rome_Parasol", loc=(-1850, -520, 4)),
        dict(mesh=f"{A}/Rome/SM_Rome_Vespa", loc=(-600, -470, 4), yaw=90, tint="8FD6C4"),
        dict(mesh=f"{A}/Rome/SM_Rome_Vespa", loc=(2300, 470, 4), yaw=-90, tint="F28B82"),
        dict(mesh=f"{A}/Rome/SM_Rome_Colosseum", loc=(6800, -2400, 0), collide=False),
    ],
    lighting=dict(sun_pitch=-52, sun_yaw=30, sun_color="FFE0B0", sun_intensity=7.0, sky_color="FFF0D8", fog_color="F0E0C8"),
    player_start=(-3200, 0, 100),
    nav_center_x=500,
    npcs=[
        dict(id="Luca", name="Luca (barista)", mesh=f"{CH}/SK_Barista", loc=(-2150, -640, 98), yaw=90, conversation=f"{IT}/DA_Conv_Rome_Bar"),
        dict(id="Giulia", name="Giulia (romana)", mesh=f"{CH}/SK_Tourist", loc=(-500, 560, 98), yaw=-90, conversation=f"{IT}/DA_Conv_Rome_Giulia"),
        dict(id="Marco", name="Marco (contact)", mesh=f"{CH}/SK_Vendor", loc=(500, 2000, 98), yaw=180, conversation=f"{IT}/DA_Conv_Rome_Marco"),
    ],
    markers=[dict(id="Fountain", loc=(0, 1700, 100))],
    pedestrian_meshes=PEDS,
    pedestrians=[(-2800, 550, 98), (-800, 1600, 98), (800, 2800, 98), (1200, -550, 98), (2800, 550, 98), (-1000, -550, 98)],
)

# ---------------------------------------------------------------------------------------------------------------
# Per-city looks (visual polish): one strong time of day per city. Applied to the existing maps with
# greybox_kit.apply_look(CITY) (no rebuild). Toon keys are PP_Toon parameters (MI_PP_Toon_<City>); grading goes on the
# ToonPostProcess volume (white_temp > 6500 = warmer image); roof = per-city roof colour (M_ToonMaster v2);
# glow = mesh-name fragment -> Emissive strength (PP_Toon v2 lets emissive pixels through).
# ---------------------------------------------------------------------------------------------------------------
PARIS["look"] = dict(   # soft gold morning, lilac haze
    sun_pitch=-30, sun_yaw=120, sun_color="FFE6C4", sun_intensity=6.0,
    sky_color="BFD3F2", sky_intensity=1.2, fog_color="E9E1F0", fog_density=0.015, fog_start=3000,
    grading=dict(white_temp=6700, saturation=1.04, contrast=1.04, gain_shadows=(0.98, 0.96, 1.06, 1.0), bloom=0.3, vignette=0.3, ao=0.6),
    dof=dict(focal=2500, fstop=1.0, sensor=180),
    toon=dict(LitLevel=1.0, LightNorm=1.0, LightHueKeep=0.55, ShadowTint="C9C0EA", HazeColor="E9E1F0", HazeMax=0.35),
    roof="7C8AA0",
)
MADRID["look"] = dict(  # white-hot afternoon, violet shadows
    sun_pitch=-66, sun_yaw=30, sun_color="FFF4E0", sun_intensity=8.0,
    sky_color="FFE6C2", sky_intensity=1.0, fog_color="F5DDB5", fog_density=0.015, fog_start=3000,
    grading=dict(white_temp=6600, saturation=1.02, contrast=1.12, gain_shadows=(0.96, 0.94, 1.08, 1.0), bloom=0.3, vignette=0.3, ao=0.6),
    dof=dict(focal=2500, fstop=1.0, sensor=180),
    toon=dict(LitLevel=0.97, LightNorm=1.33, LightHueKeep=0.4, ShadowLevel=0.4, ShadowTint="A998E0", HazeColor="F5DDB5", HazeMax=0.3),
    roof="C46A4A",
)
TOKYO["look"] = dict(   # blue-hour dusk: last orange light, deep blue shade, glowing lanterns and vending machines
    sun_pitch=-10, sun_yaw=70, sun_color="FFC2A0", sun_intensity=2.2,
    sky_color="4A5AC0", sky_intensity=2.5, fog_color="4E5A8A", fog_density=0.02, fog_start=2500,
    grading=dict(white_temp=4800, saturation=1.0, contrast=1.08, gain_shadows=(0.9, 0.96, 1.15, 1.0), bloom=0.8, bloom_threshold=1.0, vignette=0.45, ao=0.6),
    dof=dict(focal=2500, fstop=1.0, sensor=180),
    toon=dict(LitLevel=0.8, LightNorm=0.5, LightHueKeep=0.45, EmissiveThreshold=2.2, ShadowTint="6F88E8", HazeColor="4E5A8A", HazeMax=0.4),
    roof="4A4F63",
    glow={"Lantern": 1.2, "Vending": 1.0},
)
ROME["look"] = dict(    # golden hour: long amber shadows across the piazza
    sun_pitch=-20, sun_yaw=60, sun_color="FFD199", sun_intensity=6.0,
    sky_color="FFE0B8", sky_intensity=1.0, fog_color="F2CFA0", fog_density=0.015, fog_start=3000,
    grading=dict(white_temp=6900, saturation=1.04, contrast=1.05, gain_shadows=(0.98, 0.94, 1.04, 1.0), bloom=0.35, vignette=0.35, ao=0.6),
    dof=dict(focal=2500, fstop=1.0, sensor=180),
    toon=dict(LitLevel=1.0, LightNorm=1.0, LightHueKeep=0.45, ShadowTint="C7A0C0", HazeColor="F2CFA0", HazeMax=0.3),
    roof="C46A4A",
)

# Street-level detail (Q6): road colour, paving (scale cm, joint width 0..1, strength), plazas, markings, crosswalks
PARIS["street"] = dict(road="9C9590", road_paving=(45, 0.08, 0.10), sidewalk="E6DFD2", sidewalk_paving=(120, 0.03, 0.06),
                       plazas={"PlaceDuTertre": ("D8CFC0", (90, 0.05, 0.08))}, marking="F2EEE6", kerb="CFC7B8",
                       crosswalks=[-1350, 900], dashes=False)
MADRID["street"] = dict(road="C9B08F", road_paving=(50, 0.08, 0.08), sidewalk="EFE3CF", sidewalk_paving=(100, 0.03, 0.05),
                        plazas={"PlazaMayor": ("E9D2AE", (200, 0.03, 0.07))}, marking="FFF6E6", kerb="D9C7A8",
                        crosswalks=[-2050, 2600])
TOKYO["street"] = dict(road="4A4E62", sidewalk="E3E0EA", sidewalk_paving=(60, 0.04, 0.06),
                       plazas={"ShrineGrounds": ("D9CFC0", (80, 0.05, 0.06))}, marking="F4F4F8", kerb="B8B4C4",
                       crosswalks=[-1050, 500, 2250])
ROME["street"] = dict(road="6F6A66", road_paving=(25, 0.1, 0.12), sidewalk="E6D8C0", sidewalk_paving=(90, 0.03, 0.05),
                      plazas={"Piazza": ("E8D6B5", (160, 0.03, 0.07))}, marking="EDE6DA", kerb="CDBFA6",
                      crosswalks=[-2150, 0], dashes=False)
