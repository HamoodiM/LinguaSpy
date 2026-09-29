"""Shared helpers for authoring LinguaSpy content from Python (run inside the Unreal editor).

Usage from an editor Python session / unreal-mcp:
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/paris_content.py", encoding="utf-8").read())

These scripts (re)generate DataAssets. Re-running overwrites the listed assets, so once content is being
edited by hand in the editor (or the Conversation Editor tool), stop re-running the script for that asset.
"""
import unreal

EAL = unreal.EditorAssetLibrary
T = unreal.ConversationNodeType
O = unreal.ObjectiveType


def get_or_create(folder, name, cls):
    """Loads /Game/<folder>/<name> or creates a DataAsset of `cls` there."""
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, cls, factory)


def tiles(spec):
    """'word word2' or [('コーヒー', 'koohii'), ...] -> [WordTile]."""
    items = spec.split(" ") if isinstance(spec, str) else spec
    out = []
    for item in items:
        tile = unreal.WordTile()
        if isinstance(item, tuple):
            tile.set_editor_property("text", item[0])
            tile.set_editor_property("reading", item[1])
        else:
            tile.set_editor_property("text", item)
        out.append(tile)
    return out


def challenge(intent, tile_spec, answers, skill, penalty=34.0, sep=" "):
    """answers: list of strings split by `sep` (use '|' for languages without spaces)."""
    c = unreal.WordTileChallenge()
    c.set_editor_property("intent_prompt", intent)
    c.set_editor_property("tiles", tiles(tile_spec))
    accepted = []
    for a in answers:
        ans = unreal.AcceptedAnswer()
        ans.set_editor_property("tokens", a.split(sep))
        accepted.append(ans)
    c.set_editor_property("accepted_answers", accepted)
    c.set_editor_property("skill_tag", skill)
    c.set_editor_property("cover_penalty", penalty)
    return c


def node(node_id, node_type, line, translation, next_node="", reading="", challenge_=None, choices=None,
         fail="", flags=None, outcome=""):
    n = unreal.ConversationNode()
    n.set_editor_property("node_id", node_id)
    n.set_editor_property("type", node_type)
    n.set_editor_property("native_line", line)
    n.set_editor_property("native_line_reading", reading)
    n.set_editor_property("helper_translation", translation)
    n.set_editor_property("next_node", next_node)
    n.set_editor_property("fail_node", fail)
    n.set_editor_property("set_flags", flags or [])
    n.set_editor_property("outcome", outcome)
    if challenge_ is not None:
        n.set_editor_property("challenge", challenge_)
    if choices is not None:
        n.set_editor_property("choices", choices)
    return n


def choice(label, challenge_, next_node, fail="", flags=None, required=None):
    c = unreal.ConversationChoice()
    c.set_editor_property("label", label)
    c.set_editor_property("challenge", challenge_)
    c.set_editor_property("next_node", next_node)
    c.set_editor_property("fail_node", fail)
    c.set_editor_property("set_flags", flags or [])
    c.set_editor_property("required_flags", required or [])
    return c


def conversation(folder, name, title, language, start, nodes, cover_blown_outcome=""):
    conv = get_or_create(folder, name, unreal.ConversationAsset)
    conv.set_editor_property("title", title)
    conv.set_editor_property("language", language)
    conv.set_editor_property("start_node", start)
    conv.set_editor_property("cover_blown_outcome", cover_blown_outcome)
    conv.set_editor_property("nodes", nodes)
    EAL.save_loaded_asset(conv, False)
    return conv


def transition(next_objective, outcome="", required=None, blocked=None):
    t = unreal.ObjectiveTransition()
    t.set_editor_property("next_objective", next_objective)
    t.set_editor_property("required_outcome", outcome)
    t.set_editor_property("required_flags", required or [])
    t.set_editor_property("blocked_flags", blocked or [])
    return t


def objective(objective_id, obj_type, target, description, transitions, flags=None):
    o = unreal.MissionObjective()
    o.set_editor_property("objective_id", objective_id)
    o.set_editor_property("type", obj_type)
    o.set_editor_property("target_id", target)
    o.set_editor_property("description", description)
    o.set_editor_property("transitions", transitions)
    o.set_editor_property("set_flags", flags or [])
    return o


def mission(folder, name, mission_id, title, briefing, start, objectives):
    m = get_or_create(folder, name, unreal.MissionAsset)
    m.set_editor_property("mission_id", mission_id)
    m.set_editor_property("title", title)
    m.set_editor_property("briefing", briefing)
    m.set_editor_property("start_objective", start)
    m.set_editor_property("objectives", objectives)
    EAL.save_loaded_asset(m, False)
    return m


def srgb(hex_color):
    def lin(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    return unreal.LinearColor(lin(int(hex_color[0:2], 16)), lin(int(hex_color[2:4], 16)), lin(int(hex_color[4:6], 16)), 1.0)


def language_pack(folder, name, city_id, language, city, code, tagline, color_hex, sort_order,
                  city_map="", word_spacing=True, readings=False, missions=None, conversations=None):
    pack = get_or_create(folder, name, unreal.LanguagePackAsset)
    pack.set_editor_property("city_id", city_id)
    pack.set_editor_property("language_name", language)
    pack.set_editor_property("city_name", city)
    pack.set_editor_property("language_code", code)
    pack.set_editor_property("tagline", tagline)
    pack.set_editor_property("accent_color", srgb(color_hex))
    pack.set_editor_property("sort_order", sort_order)
    pack.set_editor_property("uses_word_spacing", word_spacing)
    pack.set_editor_property("has_readings", readings)
    if city_map:
        pack.set_editor_property("city_map", EAL.load_asset(city_map))
    if missions is not None:
        pack.set_editor_property("missions", missions)
    if conversations is not None:
        pack.set_editor_property("conversations", conversations)
    EAL.save_loaded_asset(pack, False)
    return pack
