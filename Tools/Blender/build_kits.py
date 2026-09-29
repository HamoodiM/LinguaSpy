"""LinguaSpy environment kits, built procedurally in Blender (low-poly, chunky, flat-shaded).

Vertex colors carry the palette. Alpha = 1 marks "tintable" surfaces (walls, main body): the UE toon material
multiplies those by a per-instance Tint, so one building mesh can be placed in many pastel colors.
Alpha = 0 surfaces (window frames, roofs, details) keep their authored color.

Units: meters, pivot at bottom-center, facing -Y (the street side). Exported with apply_unit_scale -> cm in UE.

Usage (Blender):
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Blender/build_kits.py", encoding="utf-8").read())
    build_city("Paris")            # builds + exports every Paris piece to Saved/Export/Kits/Paris/SM_*.fbx
"""
import bpy, bmesh, math, os, random
from mathutils import Vector, Matrix

EXPORT_ROOT = r"C:/UnrealEngineProjects/Personal/Saved/Export/Kits"


def hexc(h, a=0.0):
    def lin(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    return (lin(int(h[0:2], 16)), lin(int(h[2:4], 16)), lin(int(h[4:6], 16)), a)


TINT = (1.0, 1.0, 1.0, 1.0)  # white + tintable
DARK = hexc("22263A")
GLASS = hexc("5E6B85")
FRAME = hexc("F7F4EE")


class Kit:
    def __init__(self):
        self.bm = bmesh.new()
        self.col = self.bm.loops.layers.color.new("Col")

    def _paint(self, verts, color):
        faces = set()
        for v in verts:
            faces.update(v.link_faces)
        for f in faces:
            f.smooth = False
            for loop in f.loops:
                loop[self.col] = color

    def box(self, center, size, color, rot_z=0.0):
        verts = bmesh.ops.create_cube(self.bm, size=1.0)["verts"]
        m = Matrix.Translation(Vector(center)) @ Matrix.Rotation(rot_z, 4, 'Z') @ Matrix.Diagonal((*size, 1.0))
        bmesh.ops.transform(self.bm, matrix=m, verts=verts)
        self._paint(verts, color)
        return verts

    def cyl(self, center, radius, height, color, segs=10, r_top=None, rot=None):
        verts = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segs,
                                      radius1=radius, radius2=radius if r_top is None else r_top, depth=height)["verts"]
        m = Matrix.Translation(Vector(center)) @ (rot if rot is not None else Matrix())
        bmesh.ops.transform(self.bm, matrix=m, verts=verts)
        self._paint(verts, color)
        return verts

    def sphere(self, center, radii, color, segs=10, rings=6):
        verts = bmesh.ops.create_uvsphere(self.bm, u_segments=segs, v_segments=rings, radius=1.0)["verts"]
        bmesh.ops.transform(self.bm, matrix=Matrix.Translation(Vector(center)) @ Matrix.Diagonal((*radii, 1.0)), verts=verts)
        self._paint(verts, color)
        return verts

    def wedge_roof(self, center, width, depth, height, color):
        """Triangular prism roof along X."""
        cx, cy, cz = center
        hw, hd = width / 2, depth / 2
        v = [self.bm.verts.new(p) for p in [
            (cx - hw, cy - hd, cz), (cx + hw, cy - hd, cz), (cx + hw, cy + hd, cz), (cx - hw, cy + hd, cz),
            (cx - hw, cy, cz + height), (cx + hw, cy, cz + height)]]
        for face in [(0, 1, 2, 3), (0, 1, 5, 4), (2, 3, 4, 5), (0, 4, 3), (1, 2, 5)]:
            self.bm.faces.new([v[i] for i in face])
        self._paint(v, color)
        return v

    def finish(self, name, collection):
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces[:])
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        mesh.color_attributes.active_color_name = "Col"
        mesh.color_attributes.render_color_index = mesh.color_attributes.find("Col")
        old = bpy.data.objects.get(name)
        if old:
            bpy.data.objects.remove(old, do_unlink=True)
        obj = bpy.data.objects.new(name, mesh)
        collection.objects.link(obj)
        mat = bpy.data.materials.get("M_ToonMaster") or bpy.data.materials.new("M_ToonMaster")
        obj.data.materials.append(mat)
        return obj


# ---------------------------------------------------------------------------------------------------------------
# Buildings
# ---------------------------------------------------------------------------------------------------------------

def building(name, col, cols=3, floors=4, module=3.0, depth=10.0, roof="flat", roof_color="5E6B85",
             window="tall", shutters=None, shop_awning=None, balcony=False, sign=None, arcade=False, seed=0):
    """Street-facing building. Wall faces -Y at y = -depth/2."""
    random.seed(seed)
    k = Kit()
    w = cols * module
    h = floors * module
    front = -depth / 2
    k.box((0, 0, h / 2), (w, depth, h), TINT)                       # wall body (tintable)
    k.box((0, front - 0.05, 0.25), (w + 0.1, 0.2, 0.5), hexc("B8B4AA"))  # plinth

    for f in range(floors):
        z0 = f * module
        for c in range(cols):
            x = -w / 2 + module * (c + 0.5)
            if f == 0:
                if arcade:
                    # Arch opening: dark recess with a rounded top
                    k.box((x, front - 0.01, 1.3), (module * 0.7, 0.1, 2.6), DARK)
                    k.cyl((x, front - 0.01, 2.6), module * 0.35, 0.1, DARK, segs=10,
                          rot=Matrix.Rotation(math.pi / 2, 4, 'X'))
                elif c == cols // 2:
                    k.box((x, front - 0.02, 1.1), (1.2, 0.1, 2.2), hexc("7A4E35"))   # door
                    k.box((x, front - 0.06, 2.35), (1.5, 0.12, 0.2), FRAME)
                else:
                    k.box((x, front - 0.02, 1.4), (module * 0.7, 0.1, 1.8), GLASS)  # shop window
                    k.box((x, front - 0.06, 0.45), (module * 0.75, 0.12, 0.15), FRAME)
                continue
            # Upper-floor windows
            ww, wh = (1.0, 1.7) if window == "tall" else (1.4, 1.2)
            wz = z0 + module * 0.5
            k.box((x, front - 0.02, wz), (ww, 0.1, wh), GLASS)
            k.box((x, front - 0.07, wz - wh / 2 - 0.06), (ww + 0.3, 0.16, 0.12), FRAME)   # sill
            k.box((x, front - 0.05, wz + wh / 2 + 0.08), (ww + 0.2, 0.12, 0.14), FRAME)   # lintel
            if shutters:
                for s in (-1, 1):
                    k.box((x + s * (ww / 2 + 0.25), front - 0.05, wz), (0.4, 0.08, wh), hexc(shutters))
            if balcony and f == 1:
                k.box((x, front - 0.45, z0 + 0.15), (ww + 0.6, 0.8, 0.12), FRAME)
                k.box((x, front - 0.82, z0 + 0.6), (ww + 0.6, 0.05, 0.8), DARK)

    if shop_awning:
        stripes = 6
        for i in range(stripes):
            x = -w / 2 + (i + 0.5) * w / stripes
            color = hexc(shop_awning) if i % 2 == 0 else FRAME
            k.box((x, front - 0.7, 2.9), (w / stripes, 1.4, 0.1), color, )
    if sign:
        k.box((0, front - 0.12, module - 0.45), (w * 0.6, 0.12, 0.5), hexc(sign))

    rc = hexc(roof_color)
    if roof == "mansard":
        k.box((0, 0, h + 0.1), (w + 0.3, depth + 0.3, 0.2), FRAME)
        # Sloped mansard: a tapered box + dormers
        verts = k.box((0, 0, h + 1.2), (w, depth, 2.0), rc)
        for v in verts:
            if v.co.z > h + 1.5:
                v.co.x *= 0.85
                v.co.y *= 0.7
        for c in range(cols):
            x = -w / 2 + module * (c + 0.5)
            k.box((x, front + 0.9, h + 1.0), (0.8, 0.6, 1.0), FRAME)
            k.box((x, front + 0.58, h + 1.0), (0.5, 0.05, 0.6), GLASS)
    elif roof == "terracotta":
        k.box((0, 0, h + 0.1), (w + 0.4, depth + 0.4, 0.2), FRAME)
        k.wedge_roof((0, 0, h + 0.2), w + 0.6, depth + 0.8, 1.6, rc)
    elif roof == "kawara":
        k.box((0, 0, h + 0.1), (w + 0.3, depth + 0.3, 0.2), hexc("3A3F4F"))
        k.wedge_roof((0, 0, h + 0.2), w + 1.0, depth + 1.2, 1.3, rc)
    else:  # flat with parapet
        k.box((0, 0, h + 0.25), (w + 0.2, depth + 0.2, 0.5), rc)
    return k.finish(name, col)


# ---------------------------------------------------------------------------------------------------------------
# Props
# ---------------------------------------------------------------------------------------------------------------

def lamp(col, name="SM_Lamp", color="2F3446", head="F2DE9B"):
    k = Kit()
    k.cyl((0, 0, 0.1), 0.18, 0.2, hexc(color))
    k.cyl((0, 0, 1.9), 0.06, 3.6, hexc(color), r_top=0.045)
    k.sphere((0, 0, 3.8), (0.25, 0.25, 0.3), hexc(head))
    k.cyl((0, 0, 4.1), 0.18, 0.15, hexc(color), r_top=0.05)
    return k.finish(name, col)


def tree(col, name="SM_Tree", leaf="7FB37A", trunk="7A5A44", round_=True, fruit=None):
    k = Kit()
    k.cyl((0, 0, 1.2), 0.16, 2.4, hexc(trunk), segs=8, r_top=0.12)
    if round_:
        k.sphere((0, 0, 3.1), (1.3, 1.3, 1.1), hexc(leaf, 1.0))
        k.sphere((0.6, 0.3, 2.7), (0.8, 0.8, 0.7), hexc(leaf, 1.0))
    else:
        for i, z in enumerate((2.2, 3.0, 3.7)):
            k.cyl((0, 0, z), 1.3 - i * 0.35, 0.9, hexc(leaf, 1.0), segs=8, r_top=0.2)
    if fruit:
        random.seed(3)
        for i in range(10):
            a = random.random() * math.tau
            k.sphere((math.cos(a) * 1.1, math.sin(a) * 1.1, 2.6 + random.random() * 1.0), (0.12, 0.12, 0.12), hexc(fruit), segs=6, rings=4)
    return k.finish(name, col)


def bench(col, name="SM_Bench"):
    k = Kit()
    for x in (-0.7, 0.7):
        k.box((x, 0, 0.22), (0.08, 0.5, 0.44), hexc("2F3446"))
    k.box((0, 0, 0.46), (1.7, 0.5, 0.06), hexc("A67B5B"))
    k.box((0, 0.24, 0.8), (1.7, 0.06, 0.4), hexc("A67B5B"))
    return k.finish(name, col)


def cafe_set(col, name="SM_CafeSet", table="F7F4EE", chair="A67B5B"):
    k = Kit()
    k.cyl((0, 0, 0.73), 0.38, 0.05, hexc(table), segs=12)
    k.cyl((0, 0, 0.37), 0.04, 0.72, hexc("2F3446"), segs=6)
    k.cyl((0, 0, 0.02), 0.22, 0.04, hexc("2F3446"), segs=8)
    for s in (-1, 1):
        k.box((s * 0.65, 0, 0.45), (0.42, 0.42, 0.06), hexc(chair))
        k.box((s * 0.84, 0, 0.75), (0.05, 0.42, 0.6), hexc(chair))
        for dx in (-0.18, 0.18):
            for dy in (-0.18, 0.18):
                k.box((s * 0.65 + dx, dy, 0.22), (0.04, 0.04, 0.44), hexc("2F3446"))
    return k.finish(name, col)


def counter(col, name="SM_Counter", color="A67B5B", top="F7F4EE"):
    k = Kit()
    k.box((0, 0, 0.5), (2.2, 0.7, 1.0), hexc(color, 1.0))
    k.box((0, 0, 1.03), (2.3, 0.8, 0.06), hexc(top))
    return k.finish(name, col)


def awning(col, name="SM_Awning", color="D9534F", width=6.0):
    k = Kit()
    stripes = 8
    for i in range(stripes):
        x = -width / 2 + (i + 0.5) * width / stripes
        c = hexc(color) if i % 2 == 0 else FRAME
        verts = k.box((x, -0.8, 3.0), (width / stripes, 1.6, 0.08), c)
        for v in verts:
            if v.co.y < -1.2:
                v.co.z -= 0.35
    for x in (-width / 2 + 0.1, width / 2 - 0.1):
        k.cyl((x, -1.5, 1.4), 0.04, 2.8, hexc("2F3446"), segs=6)
    return k.finish(name, col)


def kiosk(col, name="SM_Kiosk"):
    k = Kit()
    k.cyl((0, 0, 1.3), 1.0, 2.6, hexc("3E7D5A", 1.0), segs=12)
    k.cyl((0, 0, 2.75), 1.2, 0.3, hexc("2F3446"), segs=12)
    k.cyl((0, 0, 3.4), 1.2, 1.0, hexc("3E7D5A", 1.0), segs=12, r_top=0.1)
    k.sphere((0, 0, 4.0), (0.2, 0.2, 0.2), hexc("F2C14E"))
    k.box((0, -1.02, 1.6), (1.2, 0.05, 1.0), hexc("F7F4EE"))
    return k.finish(name, col)


def metro_arch(col, name="SM_MetroArch"):
    k = Kit()
    green = hexc("3E7D5A")
    for x in (-1.2, 1.2):
        k.cyl((x, 0, 1.5), 0.08, 3.0, green, segs=8)
        k.sphere((x, 0, 3.05), (0.18, 0.18, 0.26), hexc("F2C14E"))
    k.box((0, 0, 2.7), (2.4, 0.1, 0.45), hexc("F2DE9B"))
    k.box((0, 1.0, 0.02), (2.2, 2.0, 0.04), DARK)
    for x in (-1.15, 1.15):
        k.box((x, 1.0, 0.5), (0.05, 2.0, 1.0), green)
    return k.finish(name, col)


def lattice_tower(col, name, color, heights, bases, top_spire=2.0, stripes=None):
    """Tapered lattice-looking tower (Eiffel / Tokyo Tower) made of stacked frustums with dark bands."""
    k = Kit()
    z = 0.0
    for i, (hh, (rb, rt)) in enumerate(zip(heights, bases)):
        c = hexc(stripes[i % len(stripes)]) if stripes else hexc(color)
        k.cyl((0, 0, z + hh / 2), rb, hh, c, segs=4, r_top=rt)
        k.cyl((0, 0, z + hh + 0.2), rt * 1.3, 0.4, DARK if not stripes else hexc("F7F4EE"), segs=4)
        z += hh + 0.4
    k.cyl((0, 0, z + top_spire / 2), 0.25, top_spire, hexc(color), segs=6, r_top=0.05)
    obj = k.finish(name, col)
    obj.rotation_euler.z = math.pi / 4
    return obj


def dome_palace(col, name="SM_Palace", wall="F3E3C3", dome="8FB8DE"):
    k = Kit()
    k.box((0, 0, 4), (24, 12, 8), hexc(wall, 1.0))
    for i in range(7):
        x = -9 + i * 3
        k.box((x, -6.05, 4.5), (1.2, 0.1, 2.5), GLASS)
    k.cyl((0, 0, 9), 4.0, 2.0, hexc(wall, 1.0), segs=12)
    k.sphere((0, 0, 10.5), (3.8, 3.8, 3.5), hexc(dome), segs=12, rings=8)
    k.cyl((0, 0, 14.5), 0.2, 1.5, hexc("F2C14E"), segs=6)
    return k.finish(name, col)


def colosseum(col, name="SM_Colosseum"):
    k = Kit()
    segs = 24
    R = 14.0
    for i in range(segs):
        if i in (0, 1, 2):  # broken section
            continue
        a = i / segs * math.tau
        x, y = math.cos(a) * R, math.sin(a) * R
        tiers = 3 if i < 14 else 4
        for t in range(tiers):
            k.box((x, y, 2 + t * 3.8), (3.2, 1.6, 3.6), hexc("E3C89A", 1.0), rot_z=a + math.pi / 2)
            k.box((x * 1.06, y * 1.06, 1.8 + t * 3.8), (1.4, 0.2, 2.2), DARK, rot_z=a + math.pi / 2)
    return k.finish(name, col)


def fountain(col, name="SM_Fountain", stone="E3C89A", water="8FB8DE"):
    k = Kit()
    k.cyl((0, 0, 0.35), 3.0, 0.7, hexc(stone, 1.0), segs=16)
    k.cyl((0, 0, 0.62), 2.7, 0.1, hexc(water), segs=16)
    k.cyl((0, 0, 1.2), 0.35, 1.8, hexc(stone, 1.0), segs=8)
    k.cyl((0, 0, 2.1), 1.1, 0.25, hexc(stone, 1.0), segs=12)
    k.sphere((0, 0, 2.6), (0.35, 0.35, 0.5), hexc(water))
    return k.finish(name, col)


def vespa(col, name="SM_Vespa", color="8FD6C4"):
    k = Kit()
    for y in (-0.55, 0.55):
        k.cyl((0, y, 0.25), 0.25, 0.12, DARK, segs=10, rot=Matrix.Rotation(math.pi / 2, 4, 'Y') @ Matrix.Rotation(math.pi / 2, 4, 'X'))
    k.box((0, 0.2, 0.55), (0.45, 0.9, 0.4), hexc(color))
    k.box((0, -0.55, 0.8), (0.25, 0.25, 0.9), hexc(color))
    k.box((0, -0.62, 1.25), (0.7, 0.08, 0.06), DARK)
    k.box((0, 0.25, 0.85), (0.35, 0.6, 0.12), hexc("7A4E35"))
    return k.finish(name, col)


def vending(col, name="SM_Vending", color="D9534F"):
    k = Kit()
    k.box((0, 0, 0.9), (1.0, 0.8, 1.8), hexc(color, 1.0))
    k.box((0, -0.41, 1.2), (0.8, 0.05, 0.8), hexc("F7F4EE"))
    for i in range(3):
        for j in range(4):
            k.box((-0.28 + j * 0.19, -0.44, 0.95 + i * 0.25), (0.1, 0.03, 0.16), hexc(["8FB8DE", "F2C14E", "7FB37A"][i]))
    k.box((0, -0.41, 0.3), (0.6, 0.05, 0.2), DARK)
    return k.finish(name, col)


def lantern(col, name="SM_Lantern", color="D9534F"):
    k = Kit()
    k.cyl((0, 0, 0.4), 0.28, 0.1, DARK, segs=10)
    k.sphere((0, 0, 0.0), (0.3, 0.3, 0.4), hexc(color), segs=10, rings=6)
    k.cyl((0, 0, -0.42), 0.2, 0.08, DARK, segs=10)
    return k.finish(name, col)


def torii(col, name="SM_Torii", color="E05A4F"):
    k = Kit()
    for x in (-2.2, 2.2):
        k.cyl((x, 0, 2.5), 0.28, 5.0, hexc(color), segs=10)
        k.cyl((x, 0, 0.2), 0.35, 0.4, DARK, segs=10)
    k.box((0, 0, 4.2), (5.4, 0.4, 0.35), hexc(color))
    verts = k.box((0, 0, 5.1), (6.8, 0.5, 0.35), DARK)
    for v in verts:
        if abs(v.co.x) > 3.0:
            v.co.z += 0.3
    k.box((0, 0, 4.65), (0.4, 0.3, 0.6), hexc(color))
    return k.finish(name, col)


def parasol(col, name="SM_Parasol", color="F2C14E"):
    k = Kit()
    k.cyl((0, 0, 1.2), 0.04, 2.4, hexc("F7F4EE"), segs=6)
    k.cyl((0, 0, 2.4), 1.6, 0.5, hexc(color), segs=8, r_top=0.05)
    return k.finish(name, col)


def bakery_front(col, name="SM_BakeryFront"):
    k = Kit()
    k.box((0, 0, 1.4), (4.0, 0.3, 2.8), hexc("C98E4A", 1.0))
    k.box((-0.9, -0.16, 1.3), (1.6, 0.05, 1.4), GLASS)
    k.box((1.1, -0.16, 1.1), (0.9, 0.05, 2.1), hexc("7A4E35"))
    k.box((0, -0.18, 2.55), (3.6, 0.06, 0.4), hexc("F2DE9B"))
    for i in range(4):
        k.cyl((-1.4 + i * 0.3, -0.25, 0.85), 0.05, 0.7, hexc("E3B06A"), segs=6,
              rot=Matrix.Rotation(0.4, 4, 'Y'))  # baguettes in the window
    return k.finish(name, col)


def easel(col, name="SM_Easel"):
    k = Kit()
    for x in (-0.3, 0.3):
        k.box((x, 0, 0.8), (0.05, 0.05, 1.6), hexc("A67B5B"))
    k.box((0, 0.35, 0.75), (0.05, 0.05, 1.5), hexc("A67B5B"))
    k.box((0, -0.05, 1.2), (0.8, 0.04, 0.6), hexc("F7F4EE"))
    k.box((-0.1, -0.08, 1.25), (0.3, 0.02, 0.25), hexc("8FB8DE"))
    k.box((0.15, -0.08, 1.1), (0.25, 0.02, 0.2), hexc("F28B82"))
    return k.finish(name, col)


def ticket_booth(col, name="SM_TicketBooth", color="3D5A80"):
    k = Kit()
    k.box((0, 0, 1.2), (2.0, 1.6, 2.4), hexc(color, 1.0))
    k.box((0, -0.81, 1.5), (1.4, 0.05, 0.8), GLASS)
    k.box((0, -0.9, 0.95), (1.5, 0.3, 0.06), FRAME)
    k.box((0, 0, 2.55), (2.2, 1.8, 0.3), hexc("F2DE9B"))
    return k.finish(name, col)


def bridge(col, name="SM_Bridge", stone="D8CFC0"):
    k = Kit()
    # Deck along Y (crossing the river which runs along X), 24 m long, 8 m wide
    k.box((0, 0, 0.6), (8.0, 24.0, 0.6), hexc(stone, 1.0))
    for s in (-1, 1):
        k.box((s * 3.9, 0, 1.25), (0.3, 24.0, 0.7), hexc(stone, 1.0))
        for y in range(-10, 11, 4):
            k.cyl((s * 3.9, y, 1.9), 0.12, 0.6, hexc("2F3446"), segs=6)
            k.sphere((s * 3.9, y, 2.3), (0.18, 0.18, 0.2), hexc("F2DE9B"))
    for y in (-6, 6):
        k.cyl((0, y, -1.5), 1.2, 3.6, hexc(stone, 1.0), segs=8)
    return k.finish(name, col)


# ---------------------------------------------------------------------------------------------------------------
# Cities
# ---------------------------------------------------------------------------------------------------------------

def _collection(name):
    col = bpy.data.collections.get(name)
    if not col:
        col = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(col)
    return col


def export_collection(col, city):
    folder = f"{EXPORT_ROOT}/{city}"
    os.makedirs(folder, exist_ok=True)
    out = []
    for obj in col.objects:
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        loc = obj.location.copy()
        obj.location = (0, 0, 0)
        path = f"{folder}/{obj.name}.fbx"
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                                 global_scale=1.0, apply_scale_options='FBX_SCALE_NONE', mesh_smooth_type='FACE',
                                 colors_type='SRGB', bake_space_transform=False, axis_forward='-Y', axis_up='Z')
        obj.location = loc
        out.append(path)
    return out


def build_city(city, export=True):
    col = _collection("Kit_" + city)
    for obj in list(col.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    if city == "Shared":
        lamp(col); tree(col); bench(col); cafe_set(col); counter(col); parasol(col)
    elif city == "Paris":
        for i, (cols, floors, win) in enumerate([(2, 4, "tall"), (3, 4, "tall"), (3, 5, "tall"), (4, 4, "tall")]):
            building(f"SM_Paris_Building_{i}", col, cols=cols, floors=floors, roof="mansard", roof_color="5E6B85",
                     window=win, balcony=True, seed=i)
        building("SM_Paris_Cafe", col, cols=3, floors=4, roof="mansard", balcony=True, shop_awning="D9534F", sign="3E7D5A")
        awning(col, "SM_Paris_Awning", "D9534F")
        kiosk(col, "SM_Paris_Kiosk")
        metro_arch(col, "SM_Paris_MetroArch")
        bakery_front(col, "SM_Paris_BakeryFront")
        ticket_booth(col, "SM_Paris_TicketBooth")
        easel(col, "SM_Paris_Easel")
        bridge(col, "SM_Paris_Bridge")
        lattice_tower(col, "SM_Paris_Eiffel", "8C7B6B", heights=[9, 10, 14], bases=[(9, 5), (4.5, 2.2), (2.0, 0.6)], top_spire=4)
        lamp(col, "SM_Paris_Lamp", "2F3446", "F2DE9B")
        tree(col, "SM_Paris_Tree", "7FB37A")
    elif city == "Madrid":
        for i, (cols, floors) in enumerate([(3, 3), (4, 3), (3, 4), (2, 3)]):
            building(f"SM_Madrid_Building_{i}", col, cols=cols, floors=floors, roof="terracotta", roof_color="C8643B",
                     window="tall", shutters="3E7D5A", balcony=True, seed=i)
        building("SM_Madrid_Arcade", col, cols=5, floors=3, roof="terracotta", roof_color="C8643B", arcade=True, balcony=True)
        building("SM_Madrid_TapasBar", col, cols=3, floors=3, roof="terracotta", roof_color="C8643B", shop_awning="F2A65A", sign="C8643B")
        tree(col, "SM_Madrid_OrangeTree", "5E9E5A", fruit="F2A65A")
        dome_palace(col, "SM_Madrid_Palace", "F3E3C3", "8FB8DE")
        fountain(col, "SM_Madrid_Fountain", "E6DFD2", "8FB8DE")
        parasol(col, "SM_Madrid_Parasol", "F2C14E")
        lamp(col, "SM_Madrid_Lamp", "2F3446", "F7F4EE")
    elif city == "Tokyo":
        for i, (cols, floors) in enumerate([(2, 3), (2, 4), (3, 3), (2, 5)]):
            building(f"SM_Tokyo_Building_{i}", col, cols=cols, floors=floors, roof="flat", roof_color="B8B4C8",
                     window="wide", sign=["F28B82", "8FB8DE", "F2C14E", "B7A6C9"][i], seed=i)
        building("SM_Tokyo_Konbini", col, cols=3, floors=1, roof="flat", roof_color="F7F4EE", window="wide", sign="7FB37A")
        building("SM_Tokyo_Shrine", col, cols=2, floors=1, roof="kawara", roof_color="3A3F4F")
        vending(col, "SM_Tokyo_Vending", "D9534F")
        lantern(col, "SM_Tokyo_Lantern", "E05A4F")
        torii(col, "SM_Tokyo_Torii", "E05A4F")
        lattice_tower(col, "SM_Tokyo_Tower", "E05A4F", heights=[10, 12, 12], bases=[(8, 4.5), (4.2, 2.0), (1.8, 0.6)],
                      top_spire=5, stripes=["E05A4F", "F7F4EE"])
        tree(col, "SM_Tokyo_Tree", "F4A7B9", round_=True)  # cherry blossom
        lamp(col, "SM_Tokyo_Lamp", "B8B4C8", "F7F4EE")
    elif city == "Rome":
        for i, (cols, floors) in enumerate([(3, 3), (3, 4), (4, 3), (2, 4)]):
            building(f"SM_Rome_Building_{i}", col, cols=cols, floors=floors, roof="terracotta", roof_color="B85C38",
                     window="tall", shutters="4E7A5A", seed=i)
        building("SM_Rome_Trattoria", col, cols=3, floors=3, roof="terracotta", roof_color="B85C38",
                 shutters="4E7A5A", shop_awning="3E7D5A", sign="B85C38")
        fountain(col, "SM_Rome_Fountain", "E3C89A", "8FD6C4")
        colosseum(col, "SM_Rome_Colosseum")
        vespa(col, "SM_Rome_Vespa", "8FD6C4")
        parasol(col, "SM_Rome_Parasol", "F7F4EE")
        tree(col, "SM_Rome_Pine", "5E8C5A", round_=False)
        lamp(col, "SM_Rome_Lamp", "2F3446", "F2DE9B")

    return export_collection(col, city) if export else [o.name for o in col.objects]
