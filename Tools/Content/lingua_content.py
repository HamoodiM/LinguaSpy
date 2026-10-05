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


HAND_EDITED_TAG = "LinguaSpy.HandEdited"
FORCE = False  # set lingua_content.FORCE = True to overwrite hand-edited assets anyway


class HandEditedSkip(Exception):
    pass


def is_hand_edited(asset):
    try:
        return EAL.get_metadata_tag(asset, HAND_EDITED_TAG) == "1"
    except Exception:
        return False


def get_or_create(folder, name, cls):
    """Loads /Game/<folder>/<name> or creates a DataAsset of `cls` there.
    Assets saved from the Conversation Editor carry the LinguaSpy.HandEdited tag and are not overwritten (unless FORCE)."""
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        asset = EAL.load_asset(path)
        if is_hand_edited(asset) and not FORCE:
            unreal.log_warning(f"[lingua_content] SKIP hand-edited asset {path} (set FORCE=True to overwrite)")
            raise HandEditedSkip(path)
        return asset
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


def hint(text, token=-1):
    """Feedback-ladder hint. token = index in the first answer it is about (-1 = general)."""
    h = unreal.ChallengeHint()
    h.set_editor_property("token_index", token)
    h.set_editor_property("text", text)
    return h


def challenge(intent, tile_spec, answers, skill, penalty=34.0, sep=" ", hints=None, gaps=None, recognition=None):
    """answers: list of strings split by `sep` (use '|' for languages without spaces).
    hints: [hint(...)]; gaps: token indices typed in the gap-fill scaffold; recognition: wrong full sentences."""
    c = unreal.WordTileChallenge()
    c.set_editor_property("hints", hints or [])
    c.set_editor_property("gap_token_indices", gaps or [])
    c.set_editor_property("recognition_distractors", recognition or [])
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


def intel(node_id, line, translation, next_node, question, options, correct, flag="", skill="", cost=10.0, reading=""):
    """IntelCheck node: the NPC says something the player must act on (listening comprehension)."""
    n = node(node_id, unreal.ConversationNodeType.INTEL_CHECK, line, translation, next_node, reading=reading)
    i = unreal.IntelCheck()
    i.set_editor_property("question", question)
    i.set_editor_property("options", options)
    i.set_editor_property("correct_index", correct)
    i.set_editor_property("intel_flag", flag)
    i.set_editor_property("skill_tag", skill)
    i.set_editor_property("suspicion_cost", cost)
    n.set_editor_property("intel", i)
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


def chunk(text, meaning, situation="", skill="", reading=""):
    """A briefing phrase (taught before the mission)."""
    c = unreal.BriefingChunk()
    c.set_editor_property("text", text)
    c.set_editor_property("meaning", meaning)
    c.set_editor_property("situation", situation)
    c.set_editor_property("skill_tag", skill)
    c.set_editor_property("reading", reading)
    return c


def mission(folder, name, mission_id, title, briefing, start, objectives, chunks=None, can_dos=None, debrief=4):
    m = get_or_create(folder, name, unreal.MissionAsset)
    if chunks is not None:
        # Keep audio already generated for unchanged phrases (voice_build.py fills it in)
        old = {c.get_editor_property("text"): c.get_editor_property("audio") for c in m.get_editor_property("briefing_chunks")}
        for c in chunks:
            if c.get_editor_property("text") in old:
                c.set_editor_property("audio", old[c.get_editor_property("text")])
        m.set_editor_property("briefing_chunks", chunks)
    if can_dos is not None:
        m.set_editor_property("can_do_statements", can_dos)
    m.set_editor_property("debrief_items", debrief)
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


def grammar_note(note_id, title, body, triggers, after=2):
    g = unreal.GrammarNote()
    g.set_editor_property("note_id", note_id)
    g.set_editor_property("title", title)
    g.set_editor_property("body", body)
    g.set_editor_property("trigger_tokens", triggers)
    g.set_editor_property("trigger_after", after)
    return g


def language_pack(folder, name, city_id, language, city, code, tagline, color_hex, sort_order,
                  city_map="", word_spacing=True, readings=False, missions=None, conversations=None,
                  grammar_notes=None, allow_typing=None, special_chars=None):
    pack = get_or_create(folder, name, unreal.LanguagePackAsset)
    if special_chars is not None:
        pack.set_editor_property("special_characters", special_chars)
    if grammar_notes is not None:
        pack.set_editor_property("grammar_notes", grammar_notes)
    if allow_typing is not None:
        pack.set_editor_property("allow_typed_input", allow_typing)
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
