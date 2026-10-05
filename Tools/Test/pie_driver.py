"""Scripted PIE helpers for LinguaSpy (run in the editor through unreal-mcp; each call must stay under ~30 s).

    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Test/pie_driver.py", encoding="utf-8").read())
    start_pie("/Game/LinguaSpy/Maps/Lvl_Paris")      # next call: game time only advances between calls
    talk_to("Camille", peek_nodes=1, wrong_answers=0)
    reach("Metro") / collect("Package")
    status()
    shot()                                           # Saved/Screenshots/WindowsEditor (take two after PIE start)
    stop_pie()

Conversations are driven through UConversationSubsystem directly (delegates fire synchronously), so one call can
play a whole conversation: choices pick the first available option, challenges are answered with the first
accepted answer at whatever scaffold level they are presented (pick / tiles / gap words / typed), intel checks with
the right option. The first `wrong_answers` challenges get a wrong answer on both attempts (prompt, then recast);
`prompt_then_right` challenges get one wrong attempt and then the right one (feedback ladder).
"""
import builtins
import unreal

if not hasattr(builtins, "LS_DRIVER"):
    builtins.LS_DRIVER = {"challenge": None, "node": None, "mode": None, "bound": None, "log": []}
S = builtins.LS_DRIVER


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def _pie_object(cls):
    for obj in unreal.ObjectIterator(cls):
        name = obj.get_path_name()
        if "Default__" not in name and ("UEDPIE" in name or "GameInstance" in name or "PIE" in name):
            return obj
    return None


def conv():
    w = world()
    return w.get_subsystem(unreal.ConversationSubsystem) if w and hasattr(w, "get_subsystem") else _pie_object(unreal.ConversationSubsystem)


def gi_sub(cls):
    return _pie_object(cls)


def start_pie(map_path=None):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if map_path:
        les.load_level(map_path)
    les.editor_request_begin_play()
    S["bound"] = None
    return "PIE requested"


def stop_pie():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
    S["bound"] = None
    return "PIE stop requested"


def _on_node(node, mode):
    S["node"] = node
    S["mode"] = mode
    S["challenge"] = None


def _on_challenge(presentation):
    S["challenge"] = presentation


def bind():
    c = conv()
    if c is None:
        raise RuntimeError("No PIE ConversationSubsystem")
    # The delegates only hold weak references to Python callables, and every exec of this file redefines the
    # functions, so keep the bound ones alive in the persistent state (builtins) and never rebind per call.
    if S["bound"] != c.get_path_name():
        S["cb_node"], S["cb_challenge"] = _on_node, _on_challenge
        c.on_node_entered.add_callable(S["cb_node"])
        c.on_challenge_ready.add_callable(S["cb_challenge"])
        S["bound"] = c.get_path_name()
    return c


def player():
    return unreal.GameplayStatics.get_player_pawn(world(), 0)


def find_npc(target_or_name):
    for npc in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.NpcCharacter):
        if str(npc.get_mission_target_id()) == target_or_name or str(npc.get_display_name()) == target_or_name \
                or target_or_name in npc.get_name():
            return npc
    return None


def _answer(challenge, correct):
    tokens = list(challenge.accepted_answers[0].tokens)
    if not correct:
        tokens = list(reversed(tokens)) if len(tokens) > 1 else tokens + [tokens[0]]
    return tokens


def _submit(c, pres, correct):
    """Answers a presented challenge at its scaffold level. Returns the FAnswerFeedback."""
    ch = pres.challenge
    level = pres.level
    model = list(ch.accepted_answers[0].tokens)
    if level == unreal.ScaffoldLevel.RECOGNIZE:
        sep = " "
        joined = sep.join(model)
        opts = list(pres.recognition_options)
        right = opts.index(joined) if joined in opts else 0
        return c.submit_recognition(right if correct else (right + 1) % len(opts))
    if level == unreal.ScaffoldLevel.GAP_FILL:
        gaps = list(ch.gap_token_indices) or None
        if not gaps:
            # mirror UScaffoldPolicy::GetGapIndices (longest word; two for 6+ tokens)
            order = sorted(range(len(model)), key=lambda i: (-len(model[i]), -i))
            gaps = sorted(order[:2 if len(model) >= 6 else 1])
        words = [model[i] for i in gaps] if correct else ["zzz"] * len(gaps)
        return c.submit_gap_words(words)
    return c.submit_answer(_answer(ch, correct))


def drive(peek_nodes=0, wrong_answers=0, max_steps=120, prompt_then_right=0):
    """Plays the running conversation/review to the end. Peeks at the first `peek_nodes` challenge nodes."""
    c = bind()
    peeks = wrongs = graded = prompted = intel = 0
    levels = []
    for _ in range(max_steps):
        if not c.is_active():
            break
        mode = c.get_mode()
        node = S["node"]
        if mode == unreal.ConversationMode.COVER_BLOWN:
            c.continue_()
            continue
        if S["challenge"] is None and node is not None and node.type == unreal.ConversationNodeType.INTEL_CHECK \
                and mode == unreal.ConversationMode.CONVERSATION:
            c.submit_intel(node.intel.correct_index)
            intel += 1
            c.continue_()
            continue
        if S["challenge"] is None and node is not None and node.type == unreal.ConversationNodeType.PLAYER_CHOICE:
            for i in range(len(node.choices)):
                if c.is_choice_available(i):
                    c.select_choice(i)
                    break
            continue
        if S["challenge"] is not None:
            if peeks < peek_nodes:
                c.reveal_translation()
                peeks += 1
            correct = wrongs >= wrong_answers
            if not correct:
                wrongs += 1
            pres = S["challenge"]
            S["challenge"] = None
            levels.append(str(pres.level).split(".")[-1])
            first_right = correct and prompted >= prompt_then_right
            fb = _submit(c, pres, first_right)
            if not fb.final and c.get_mode() != unreal.ConversationMode.COVER_BLOWN:
                if not first_right and correct:
                    prompted += 1
                fb = _submit(c, pres, correct)
            graded += 1
            c.continue_()
            continue
        c.continue_()
    return {"graded": graded, "peeks": peeks, "wrong": wrongs, "prompted": prompted, "intel": intel,
            "levels": levels, "active": c.is_active()}


def talk_to(target, peek_nodes=0, wrong_answers=0, prompt_then_right=0):
    npc = find_npc(target)
    if not npc:
        return f"no NPC {target}"
    c = bind()
    ok = c.start_conversation(npc, player().get_component_by_class(unreal.CoverComponent))
    if not ok:
        return f"could not start conversation with {target}"
    return drive(peek_nodes, wrong_answers, prompt_then_right=prompt_then_right)


def _teleport_to_actor_named(fragment):
    for a in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.Actor):
        if fragment.lower() in a.get_actor_label().lower() if hasattr(a, "get_actor_label") else fragment.lower() in a.get_name().lower():
            loc = a.get_actor_location()
            player().set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 100), False, True)
            return a.get_name()
    return None


def reach(target):
    for a in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.MissionMarker):
        comp = a.get_component_by_class(unreal.MissionTargetComponent)
        if comp and str(comp.get_editor_property("target_id")) == target:
            loc = a.get_actor_location()
            player().set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 60), False, True)
            return a.get_name()
    return f"no marker {target}"


def collect(target):
    for a in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.MissionPickup):
        comp = a.get_component_by_class(unreal.MissionTargetComponent)
        if comp and str(comp.get_editor_property("target_id")) == target:
            loc = a.get_actor_location()
            player().set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 60), False, True)
            return a.get_name()
    return f"no pickup {target}"


def status():
    m = gi_sub(unreal.MissionSubsystem)
    srs = gi_sub(unreal.SpacedRepetitionSubsystem)
    out = {}
    if m:
        mission = m.get_active_mission()
        out["mission"] = str(mission.get_editor_property("mission_id")) if mission else None
        out["target"] = str(m.get_current_target_id())
        t = m.get_current_mission_tally()
        out["tally"] = (t.challenges, t.mistakes, t.peeks)
        g = m.get_last_mission_grade()
        out["last_grade"] = (g.grade, g.challenges, g.mistakes, g.peeks)
        city = m.get_current_city_id()
        out["city"] = str(city)
        if srs:
            out["due"] = srs.get_due_count(city)
            out["learned"] = srs.get_learned_count(city)
    return out


def shot(name="LinguaSpy"):
    w = world()
    pc = unreal.GameplayStatics.get_player_controller(w, 0)
    unreal.SystemLibrary.execute_console_command(w, "shot showui", pc)
    return "shot requested"


def close_overlays():
    """Closes briefing / field-note / assessment overlays (they pause the game) the way the Close button does."""
    closed = []
    for cls in (unreal.BriefingWidget, unreal.FieldNoteWidget, unreal.AssessmentWidget):
        for w in unreal.ObjectIterator(cls):
            if "Default__" not in w.get_path_name() and w.is_in_viewport():
                w.close()
                closed.append(cls.__name__)
    return closed


def telemetry_file():
    t = gi_sub(unreal.LearningTelemetrySubsystem)
    return t.get_session_file() if t else None
