"""LinguaSpy stylized characters, built procedurally in Blender on the UE mannequin skeleton.

Style: chunky low-poly "figurines". Every body part is rigidly bound (weight 1.0) to a single bone, so the
existing mannequin animations work without skinning artifacts. Colors are baked into vertex colors
(attribute "Col"), so UE needs one shared toon material (M_ToonCharacter reads VertexColor).

Usage (Blender, with Saved/Export/SKM_Manny_Simple.fbx exported from UE):
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Blender/build_characters.py", encoding="utf-8").read())
    setup_scene()                 # import + normalize the skeleton (once)
    build("Spy", SPY)             # builds one character mesh bound to the armature
    export_fbx("Spy")             # -> Saved/Export/Characters/SK_Spy.fbx
"""
import bpy, bmesh, math
from mathutils import Vector, Matrix

SKELETON_FBX = r"C:/UnrealEngineProjects/Personal/Saved/Export/SKM_Manny_Simple.fbx"
EXPORT_DIR = r"C:/UnrealEngineProjects/Personal/Saved/Export/Characters"
ARM = "root"


# ---------------------------------------------------------------------------------------------------------------
# Scene setup
# ---------------------------------------------------------------------------------------------------------------

def setup_scene():
    """Imports the mannequin FBX (if needed) and bakes the armature to meter scale with no parent."""
    if ARM not in bpy.data.objects:
        bpy.ops.import_scene.fbx(filepath=SKELETON_FBX, automatic_bone_orientation=False, ignore_leaf_bones=False)
    arm = bpy.data.objects[ARM]
    for name in list(bpy.data.objects.keys()):
        if name.startswith("SKM_Manny_Simple_LOD") and name != "SKM_Manny_Simple_LOD0":
            bpy.data.objects.remove(bpy.data.objects[name], do_unlink=True)
    if arm.parent:
        mw = arm.matrix_world.copy()
        empty = arm.parent
        arm.parent = None
        arm.matrix_world = mw
        if empty.type == 'EMPTY':
            for child in list(empty.children):
                child.parent = None
            bpy.data.objects.remove(empty, do_unlink=True)
        for n in list(bpy.data.objects.keys()):
            if n.endswith("LodGroup"):
                bpy.data.objects.remove(bpy.data.objects[n], do_unlink=True)
    # Apply scale so bones are in meters (FBX export converts back to cm)
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    ref = bpy.data.objects.get("SKM_Manny_Simple_LOD0")
    if ref:
        ref.hide_set(True)
        ref.hide_render = True
    return arm


def joint(name):
    """World-space head position of a bone."""
    arm = bpy.data.objects[ARM]
    return arm.matrix_world @ arm.data.bones[name].head_local


# ---------------------------------------------------------------------------------------------------------------
# Primitive builders (each returns list of (bmesh verts) added; parts are tagged with bone + color)
# ---------------------------------------------------------------------------------------------------------------

class Builder:
    def __init__(self):
        self.bm = bmesh.new()
        self.col_layer = self.bm.loops.layers.color.new("Col")
        self.deform = self.bm.verts.layers.deform.verify()
        self.groups = []  # bone names -> vertex group index

    def _group(self, bone):
        if bone not in self.groups:
            self.groups.append(bone)
        return self.groups.index(bone)

    def _tag(self, geom_verts, bone, color):
        gi = self._group(bone)
        faces = set()
        for v in geom_verts:
            v[self.deform][gi] = 1.0
            faces.update(v.link_faces)
        for f in faces:
            for loop in f.loops:
                loop[self.col_layer] = (*color, 1.0)
            f.smooth = False

    def _place(self, verts, matrix):
        bmesh.ops.transform(self.bm, matrix=matrix, verts=verts)

    def box(self, center, size, bone, color, rot=None):
        res = bmesh.ops.create_cube(self.bm, size=1.0)
        verts = res["verts"]
        m = Matrix.Translation(center) @ (rot.to_4x4() if rot else Matrix()) @ Matrix.Diagonal((*size, 1.0))
        self._place(verts, m)
        self._tag(verts, bone, color)
        return verts

    def blob(self, center, radii, bone, color, segs=8, rings=6):
        res = bmesh.ops.create_uvsphere(self.bm, u_segments=segs, v_segments=rings, radius=1.0)
        verts = res["verts"]
        self._place(verts, Matrix.Translation(center) @ Matrix.Diagonal((*radii, 1.0)))
        self._tag(verts, bone, color)
        return verts

    def limb(self, a, b, r1, r2, bone, color, segs=8, extend=0.0):
        """Tapered cylinder from point a to b (radius r1 at a, r2 at b)."""
        a, b = Vector(a), Vector(b)
        d = b - a
        length = d.length
        a2 = a - d.normalized() * extend
        length += extend * 2
        res = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segs, radius1=r1, radius2=r2, depth=length)
        verts = res["verts"]
        rot = Vector((0, 0, 1)).rotation_difference(d.normalized()).to_matrix().to_4x4()
        mid = a2 + d.normalized() * (length / 2)
        self._place(verts, Matrix.Translation(mid) @ rot)
        self._tag(verts, bone, color)
        return verts

    def disc(self, center, radius, height, bone, color, segs=12):
        res = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segs, radius1=radius, radius2=radius, depth=height)
        verts = res["verts"]
        self._place(verts, Matrix.Translation(center))
        self._tag(verts, bone, color)
        return verts

    def cone(self, center, r_bottom, r_top, height, bone, color, segs=12):
        res = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segs, radius1=r_bottom, radius2=r_top, depth=height)
        verts = res["verts"]
        self._place(verts, Matrix.Translation(center))
        self._tag(verts, bone, color)
        return verts

    def finish(self, name):
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        if "Col" in mesh.color_attributes:
            mesh.color_attributes.active_color_name = "Col"
            mesh.color_attributes.render_color_index = mesh.color_attributes.find("Col")
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        for bone in self.groups:
            obj.vertex_groups.new(name=bone)
        arm = bpy.data.objects[ARM]
        obj.parent = arm
        obj.matrix_parent_inverse = arm.matrix_world.inverted()
        mod = obj.modifiers.new("Armature", 'ARMATURE')
        mod.object = arm
        mat = bpy.data.materials.get("M_ToonCharacter") or bpy.data.materials.new("M_ToonCharacter")
        obj.data.materials.append(mat)
        return obj


def hexc(h):
    """sRGB hex -> linear rgb tuple (Blender color attributes of type BYTE_COLOR store sRGB, FLOAT_COLOR linear)."""
    def lin(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    return (lin(int(h[0:2], 16)), lin(int(h[2:4], 16)), lin(int(h[4:6], 16)))


# ---------------------------------------------------------------------------------------------------------------
# Character recipe
# ---------------------------------------------------------------------------------------------------------------

def build(name, cfg):
    """cfg: dict of colors + outfit flags (see archetypes below)."""
    old = bpy.data.objects.get("SK_" + name)
    if old:
        bpy.data.objects.remove(old, do_unlink=True)

    B = Builder()
    skin, top, bottom, shoes, hair = (hexc(cfg[k]) for k in ("skin", "top", "bottom", "shoes", "hair"))
    accent = hexc(cfg.get("accent", "222222"))
    J = joint

    for side, s in (("l", 1), ("r", -1)):
        # Legs (chunky, slightly tapered)
        B.limb(J(f"thigh_{side}"), J(f"calf_{side}"), 0.085, 0.07, f"thigh_{side}", bottom, extend=0.02)
        B.blob(J(f"calf_{side}"), (0.072, 0.072, 0.072), f"calf_{side}", bottom)
        B.limb(J(f"calf_{side}"), J(f"foot_{side}"), 0.07, 0.058, f"calf_{side}", bottom)
        # Feet: rounded boxes pointing forward (-Y)
        foot = J(f"foot_{side}")
        ball = J(f"ball_{side}")
        B.box(Vector((foot.x, (foot.y + ball.y) / 2 - 0.02, 0.045)), (0.11, 0.24, 0.09), f"foot_{side}", shoes)
        # Arms
        sleeve = top if cfg.get("long_sleeves", True) else skin
        B.blob(J(f"upperarm_{side}"), (0.075, 0.075, 0.075), f"upperarm_{side}", top)
        B.limb(J(f"upperarm_{side}"), J(f"lowerarm_{side}"), 0.062, 0.055, f"upperarm_{side}", top)
        B.blob(J(f"lowerarm_{side}"), (0.055, 0.055, 0.055), f"lowerarm_{side}", sleeve)
        B.limb(J(f"lowerarm_{side}"), J(f"hand_{side}"), 0.054, 0.046, f"lowerarm_{side}", sleeve)
        hand = J(f"hand_{side}")
        fingers = J(f"middle_01_{side}")
        B.blob(hand + (fingers - hand) * 0.6, (0.055, 0.055, 0.06), f"hand_{side}", skin)

    # Pelvis + torso (a chunky pear shape)
    pelvis = J("pelvis")
    B.blob(pelvis + Vector((0, 0, 0.0)), (0.19, 0.14, 0.13), "pelvis", bottom)
    chest = (J("spine_03") + J("spine_05")) / 2
    B.blob(Vector((0, chest.y, (J("spine_01").z + J("neck_01").z) / 2)), (0.2, 0.15, 0.28), "spine_03", top, segs=10, rings=8)
    B.limb(J("neck_01"), J("head"), 0.055, 0.05, "neck_01", skin)

    # Big head
    head = J("head") + Vector((0, -0.02, 0.13))
    HS = 1.3  # head scale: chunky, big-headed figurines (applied to everything bound to "head" below)
    B.blob(head, (0.15, 0.15, 0.16), "head", skin, segs=12, rings=8)
    # Eyes (dark), placed on the front (-Y)
    for s in (1, -1):
        B.blob(head + Vector((0.055 * s, -0.135, 0.02)) , (0.022, 0.012, 0.03), "head", hexc("22263A"), segs=6, rings=4)
    B.blob(head + Vector((0, -0.15, -0.02)) , (0.022, 0.02, 0.03), "head", skin, segs=6, rings=4)  # nose

    # Hair / hats
    hair_style = cfg.get("hair", None) and cfg.get("hair_style", "short")
    if hair_style == "short":
        B.blob(head + Vector((0, 0.01, 0.06)) , (0.155, 0.155, 0.12), "head", hair)
    elif hair_style == "bun":
        B.blob(head + Vector((0, 0.01, 0.06)) , (0.155, 0.155, 0.12), "head", hair)
        B.blob(head + Vector((0, 0.1, 0.16)) , (0.07, 0.07, 0.07), "head", hair)
    elif hair_style == "long":
        B.blob(head + Vector((0, 0.03, 0.03)) , (0.16, 0.16, 0.15), "head", hair)
        B.box(head + Vector((0, 0.08, -0.12)) , (0.28, 0.1, 0.22), "head", hair)
    elif hair_style == "bald":
        B.blob(head + Vector((0, 0.06, -0.02)) , (0.15, 0.1, 0.07), "head", hair)

    hat = cfg.get("hat")
    hat_color = hexc(cfg.get("hat_color", cfg["hair"]))
    if hat == "fedora":
        B.disc(head + Vector((0, 0, 0.13)) , 0.25, 0.02, "head", hat_color, segs=16)
        B.cone(head + Vector((0, 0, 0.2)) , 0.15, 0.12, 0.13, "head", hat_color)
        B.disc(head + Vector((0, 0, 0.16)) , 0.152, 0.03, "head", accent)  # band
    elif hat == "cap":
        B.blob(head + Vector((0, 0.01, 0.1)) , (0.155, 0.155, 0.08), "head", hat_color)
        B.box(head + Vector((0, -0.17, 0.1)) , (0.2, 0.12, 0.02), "head", hat_color)
    elif hat == "beret":
        B.blob(head + Vector((0.03, 0.0, 0.13)) , (0.17, 0.16, 0.06), "head", hat_color)
    elif hat == "bucket":
        B.cone(head + Vector((0, 0, 0.12)) , 0.23, 0.15, 0.1, "head", hat_color)
        B.disc(head + Vector((0, 0, 0.19)) , 0.15, 0.06, "head", hat_color)
    elif hat == "helmet":
        B.blob(head + Vector((0, 0, 0.08)) , (0.17, 0.17, 0.13), "head", hat_color)
        B.disc(head + Vector((0, 0, 0.04)) , 0.2, 0.02, "head", hat_color)

    # Outfit extras
    if cfg.get("sunglasses"):
        B.box(head + Vector((0, -0.145, 0.025)) , (0.2, 0.03, 0.05), "head", hexc("15171F"))
    if cfg.get("mustache"):
        B.box(head + Vector((0, -0.152, -0.055)) , (0.1, 0.02, 0.025), "head", hair)
    if cfg.get("trench"):
        # Coat skirt flaring from the waist to the knees, plus a collar
        waist = pelvis + Vector((0, 0, 0.05))
        B.cone(Vector((0, pelvis.y, (waist.z + J("calf_l").z) / 2)), 0.25, 0.21, waist.z - J("calf_l").z + 0.05, "pelvis", top, segs=10)
        B.cone(J("neck_01") + Vector((0, 0.0, -0.01)), 0.13, 0.1, 0.08, "spine_05", top, segs=10)
        B.box(Vector((0, -0.16, pelvis.z + 0.12)), (0.36, 0.04, 0.05), "pelvis", accent)  # belt
    if cfg.get("apron"):
        B.box(Vector((0, -0.17, pelvis.z + 0.05)), (0.3, 0.03, 0.55), "spine_03", accent)
    if cfg.get("vest"):
        B.blob(Vector((0, chest.y, (J("spine_01").z + J("neck_01").z) / 2 + 0.05)), (0.21, 0.16, 0.22), "spine_03", accent, segs=10, rings=8)
    if cfg.get("scarf"):
        B.cone(J("neck_01"), 0.12, 0.1, 0.07, "neck_01", accent, segs=10)
        B.box(J("neck_01") + Vector((0.06, -0.12, -0.12)), (0.07, 0.03, 0.2), "spine_05", accent)
    if cfg.get("hood"):
        B.blob(J("neck_01") + Vector((0, 0.1, 0.02)), (0.16, 0.08, 0.1), "spine_05", top)
    if cfg.get("backpack"):
        B.box(Vector((0, chest.y + 0.2, chest.z)), (0.26, 0.12, 0.32), "spine_03", accent)
    if cfg.get("camera"):
        B.box(Vector((0, -0.19, chest.z)), (0.1, 0.05, 0.07), "spine_03", hexc("15171F"))
    if cfg.get("overalls"):
        B.box(Vector((0, -0.16, chest.z - 0.05)), (0.24, 0.03, 0.22), "spine_03", bottom)
    if cfg.get("cane"):
        hand = J("hand_r")
        B.limb(hand, Vector((hand.x - 0.02, hand.y - 0.05, 0.02)), 0.018, 0.018, "hand_r", hexc("5A3E2B"), segs=6)

    # Scale all head-bound geometry (face, hair, hats) around the head center
    head_group = B._group("head")
    pivot = head - Vector((0, 0, 0.1))
    for v in B.bm.verts:
        if v[B.deform].get(head_group, 0.0) > 0.5:
            v.co = pivot + (v.co - pivot) * HS

    obj = B.finish("SK_" + name)
    return obj


def export_fbx(name):
    import os
    os.makedirs(EXPORT_DIR, exist_ok=True)
    arm = bpy.data.objects[ARM]
    obj = bpy.data.objects["SK_" + name]
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = f"{EXPORT_DIR}/SK_{name}.fbx"
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'ARMATURE', 'MESH'},
                             apply_unit_scale=True, global_scale=1.0, apply_scale_options='FBX_SCALE_NONE',
                             add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
                             armature_nodetype='NULL', bake_anim=False, mesh_smooth_type='FACE',
                             colors_type='SRGB', use_armature_deform_only=False)
    return path


# ---------------------------------------------------------------------------------------------------------------
# Archetypes (base palettes; city variants override colors)
# ---------------------------------------------------------------------------------------------------------------

SPY = dict(skin="F1C7A5", top="C9A66B", bottom="3A3F4F", shoes="1E2029", hair="3B2A20", hair_style="short",
           hat="fedora", hat_color="4A3B30", accent="2E2A24", trench=True, sunglasses=True)

ARCHETYPES = {
    "Barista": dict(skin="E8B894", top="F7F4EE", bottom="3A3F4F", shoes="4A3B30", hair="5A3825", hair_style="bun",
                    accent="7A4E35", apron=True, long_sleeves=False),
    "Vendor": dict(skin="D9A27E", top="8FB8DE", bottom="4A4E5E", shoes="2E2A24", hair="6B6B6B", hair_style="short",
                   hat="cap", hat_color="3D5A80", accent="E0A458", vest=True, mustache=True),
    "Elder": dict(skin="F0CFB5", top="B7A6C9", bottom="5E5A6E", shoes="3B2A20", hair="D8D8D8", hair_style="bald",
                  hat="beret", hat_color="3D3A4B", accent="D9534F", scarf=True, cane=True),
    "Young": dict(skin="C98E6A", top="7FB37A", bottom="3B4A6B", shoes="F7F4EE", hair="2A1D16", hair_style="short",
                  accent="F2C14E", hood=True),
    "Tourist": dict(skin="F4D2B8", top="F2A65A", bottom="C9B79C", shoes="8C7B6B", hair="C98E4A", hair_style="long",
                    hat="bucket", hat_color="F2DE9B", accent="3E7D5A", backpack=True, camera=True, long_sleeves=False),
    "Worker": dict(skin="B97A57", top="E07A5F", bottom="3D5A80", shoes="2E2A24", hair="2A1D16", hair_style="short",
                   hat="helmet", hat_color="F2C14E", accent="3D5A80", overalls=True),
}


def build_all(export=True):
    setup_scene()
    names = []
    x = 0.0
    for name, cfg in [("Spy", SPY)] + list(ARCHETYPES.items()):
        obj = build(name, cfg)
        names.append(obj.name)
        if export:
            export_fbx(name)
    return names
