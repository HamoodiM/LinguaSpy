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
        dict(label="Seine", center=(4100, 0, 2), size=(800, 7000, 4), color="7FA7C9", collide=False),
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
        dict(mesh=f"{A}/Paris/SM_Paris_Bridge", loc=(4100, 0, -88), yaw=90, tint="D8CFC0", label="Bridge"),
        dict(mesh=f"{A}/Paris/SM_Paris_Tree", loc=(5200, 900, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_Tree", loc=(5200, -900, 4)),
        dict(mesh=f"{A}/Paris/SM_Paris_Eiffel", loc=(7800, 2600, 0), scale=(1.4, 1.4, 1.4), collide=False),
    ],
    complex_collision=[f"{A}/Paris/SM_Paris_Bridge", f"{A}/Paris/SM_Paris_MetroArch"],
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
