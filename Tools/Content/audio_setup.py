"""DEPRECATED (Phase 4): replaced by audio_import_cc0.py (CC0 samples). These sustained pure-tone MetaSounds sounded harsh.
Procedural LinguaSpy audio (MetaSounds, via VibeUE MetaSoundService). No downloaded assets.
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/audio_setup.py", encoding="utf-8").read())
    build_all_audio()
"""
import unreal

M = unreal.MetaSoundService
EAL = unreal.EditorAssetLibrary
P = "/Game/LinguaSpy/Audio"
ON_PLAY = "UE.Source.OnPlay"
ON_FINISHED = "UE.Source.OneShot.OnFinished"
AUDIO_OUT = "UE.OutputFormat.Mono.Audio:0"


def _fresh(name):
    path = f"{P}/{name}"
    if EAL.does_asset_exist(path):
        M.delete_meta_sound(path)
    return M.create_meta_sound(P, name, "Mono").asset_path


def _interface(a):
    ids = {}
    for n in M.list_nodes(a):
        if n.class_name == "Input.Trigger":
            ids["play"] = n.node_id
        elif n.class_name == "Output.Trigger":
            ids["finished"] = n.node_id
        elif n.class_name == "Output.Audio":
            ids["out"] = n.node_id
    return ids


def _mixer(a, count):
    count = max(2, min(8, count))
    return M.add_node(a, "AudioMixer", f"Audio Mixer (Mono, {count})", "").node_id, count


def note_sound(name, notes, wave="Sine", attack=0.004, decay=0.25, gain=0.45):
    """One-shot: notes = [(frequency_hz, start_delay_s), ...]. Finishes when the last note's envelope is done."""
    a = _fresh(name)
    io = _interface(a)
    mix, slots = _mixer(a, len(notes))
    last_env = None
    for i, (freq, delay) in enumerate(notes[:slots]):
        env = M.add_node(a, "AD Envelope", "AD Envelope", "Audio", 1, 300, i * 200).node_id
        M.set_node_input_default(a, env, "Attack Time", str(attack), "Float")
        M.set_node_input_default(a, env, "Decay Time", str(decay), "Float")
        if delay > 0:
            dl = M.add_node(a, "UE", "Trigger Delay", "", 1, 100, i * 200).node_id
            M.set_node_input_default(a, dl, "Delay Time", str(delay), "Float")
            M.connect_nodes(a, io["play"], ON_PLAY, dl, "In")
            M.connect_nodes(a, dl, "Out", env, "Trigger")
        else:
            M.connect_nodes(a, io["play"], ON_PLAY, env, "Trigger")
        osc = M.add_node(a, "UE", wave, "Audio", 1, 300, i * 200 + 100).node_id
        M.set_node_input_default(a, osc, "Frequency", str(freq), "Float")
        mul = M.add_node(a, "UE", "Multiply", "Audio", 1, 500, i * 200).node_id
        M.connect_nodes(a, osc, "Audio", mul, "PrimaryOperand")
        M.connect_nodes(a, env, "Out Envelope", mul, "AdditionalOperands")
        M.connect_nodes(a, mul, "Out", mix, f"In {i}")
        M.set_node_input_default(a, mix, f"Gain {i}", str(gain), "Float")
        last_env = env
    for j in range(len(notes), slots):
        M.set_node_input_default(a, mix, f"Gain {j}", "0.0", "Float")
    M.connect_nodes(a, mix, "Out", io["out"], AUDIO_OUT)
    M.connect_nodes(a, last_env, "On Done", io["finished"], ON_FINISHED)
    M.save_meta_sound(a)
    return a


def ambience(name, pad_freqs, noise_cutoff=500.0, noise_gain=0.08, pad_gain=0.03, wave="Sine"):
    """Looping bed: low-passed noise (distant street) + a quiet sustained chord (city flavour)."""
    a = _fresh(name)
    io = _interface(a)
    mix, slots = _mixer(a, 1 + len(pad_freqs))
    noise = M.add_node(a, "UE", "Noise", "Audio", 1, 100, 0).node_id
    lpf = M.add_node(a, "UE", "One-Pole Low Pass Filter", "Audio", 1, 300, 0).node_id
    M.set_node_input_default(a, lpf, "Cutoff Frequency", str(noise_cutoff), "Float")
    M.connect_nodes(a, noise, "Audio", lpf, "In")
    M.connect_nodes(a, lpf, "Out", mix, "In 0")
    M.set_node_input_default(a, mix, "Gain 0", str(noise_gain), "Float")
    for i, f in enumerate(pad_freqs[:slots - 1], start=1):
        osc = M.add_node(a, "UE", wave, "Audio", 1, 300, i * 150).node_id
        M.set_node_input_default(a, osc, "Frequency", str(f), "Float")
        M.connect_nodes(a, osc, "Audio", mix, f"In {i}")
        M.set_node_input_default(a, mix, f"Gain {i}", str(pad_gain), "Float")
    M.connect_nodes(a, mix, "Out", io["out"], AUDIO_OUT)
    M.save_meta_sound(a)
    return a


def build_all_audio():
    ui = {
        "Click":           note_sound("MS_UI_Click", [(1400, 0)], "Sine", decay=0.05, gain=0.35),
        "TilePlace":       note_sound("MS_UI_TilePlace", [(660, 0)], "Triangle", decay=0.09, gain=0.5),
        "Pass":            note_sound("MS_UI_Pass", [(880, 0), (1320, 0.09)], "Sine", decay=0.3),
        "Fail":            note_sound("MS_UI_Fail", [(196, 0), (147, 0.12)], "Saw", decay=0.25, gain=0.25),
        "CoverBlown":      note_sound("MS_UI_CoverBlown", [(440, 0), (370, 0.14), (294, 0.28)], "Triangle", decay=0.35),
        "MissionComplete": note_sound("MS_UI_MissionComplete", [(523, 0), (659, 0.11), (784, 0.22), (1046, 0.33)], "Triangle", decay=0.45),
        "MenuSelect":      note_sound("MS_UI_MenuSelect", [(740, 0), (988, 0.07)], "Sine", decay=0.18),
    }
    city = {
        "Paris":  ambience("MS_Amb_Paris", [220.0, 277.18, 329.63], wave="Triangle"),       # A major, accordion-ish
        "Madrid": ambience("MS_Amb_Madrid", [164.81, 196.0, 246.94], wave="Triangle"),      # E minor, guitar-ish
        "Tokyo":  ambience("MS_Amb_Tokyo", [293.66, 329.63, 440.0], wave="Sine"),           # pentatonic, koto-ish
        "Rome":   ambience("MS_Amb_Rome", [261.63, 329.63, 392.0], wave="Triangle"),        # C major, mandolin-ish
    }
    menu = ambience("MS_Music_Menu", [196.0, 246.94, 293.66, 392.0], noise_gain=0.0, pad_gain=0.04, wave="Sine")

    # Wire into project settings (LinguaSpy Audio) and the language packs
    settings = unreal.get_default_object(unreal.load_class(None, "/Script/Personal.LinguaSpyAudioSettings"))
    for key, path in ui.items():
        settings.set_editor_property(key, EAL.load_asset(path))  # C++ property names
    settings.set_editor_property("MenuMusic", EAL.load_asset(menu))
    _write_audio_config({**{k: v for k, v in ui.items()}, "MenuMusic": menu})

    packs = {"Paris": "/Game/LinguaSpy/Data/French/DA_Lang_French_Paris", "Madrid": "/Game/LinguaSpy/Data/Spanish/DA_Lang_Spanish_Madrid",
             "Tokyo": "/Game/LinguaSpy/Data/Japanese/DA_Lang_Japanese_Tokyo", "Rome": "/Game/LinguaSpy/Data/Italian/DA_Lang_Italian_Rome"}
    for c, pack_path in packs.items():
        pack = EAL.load_asset(pack_path)
        pack.set_editor_property("ambient_sound", EAL.load_asset(city[c]))
        EAL.save_loaded_asset(pack, False)
    return ui, city, menu


def _write_audio_config(entries):
    """Persists ULinguaSpyAudioSettings to Config/DefaultGame.ini (replaces the section if present)."""
    import os, re
    ini = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_config_dir()) + "DefaultGame.ini"
    text = open(ini, encoding="utf-8").read() if os.path.exists(ini) else ""
    section = "[/Script/Personal.LinguaSpyAudioSettings]"
    body = "\n".join(f"{k}={v}.{v.split('/')[-1]}" for k, v in entries.items())
    text = re.sub(r"\n?\[/Script/Personal\.LinguaSpyAudioSettings\][^\[]*", "", text).rstrip() + f"\n\n{section}\n{body}\n"
    open(ini, "w", encoding="utf-8").write(text)


def _prop(key):
    return {"Click": "click", "TilePlace": "tile_place", "Pass": "pass", "Fail": "fail", "CoverBlown": "cover_blown",
            "MissionComplete": "mission_complete", "MenuSelect": "menu_select"}[key]
