"""Imports the chosen CC0 audio (see docs/Audio_Credits.md) and wires it into LinguaSpy.
Replaces the Phase 3 procedural MetaSounds (audio_setup.py), which were sustained pure tones ("ringing").

Sources are downloaded into Saved/AudioSources (git-ignored); see docs/Audio_Credits.md for the URLs.
Run in the editor (unreal-mcp / Python), in steps to stay under the 30 s call limit:
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/audio_import_cc0.py", encoding="utf-8").read())
    import_all()          # SoundWaves -> /Game/LinguaSpy/Audio/CC0
    wire_all()            # project settings (UI sounds, menu music, mix levels) + language packs
    remove_old_audio()    # deletes the MS_* MetaSounds and the analysis staging folder

Choices were made with Tools/Audio/analyze_audio.py (low spectral centroid / little >4 kHz energy for UI sounds,
no sustained single-pitch pads for loops). Mix levels are set so that with default player sliders UI peaks land
around -12 dBFS, music around -30 dB RMS and the street bed around -37 dB RMS.
"""
import os
import re
import unreal

SRC = r"C:/UnrealEngineProjects/Personal/Saved/AudioSources"
DEST = "/Game/LinguaSpy/Audio/CC0"
EAL = unreal.EditorAssetLibrary

# asset name -> (source file relative to SRC, loops)
SOUNDS = {
    "S_UI_Click":           ("kenney_ui-audio/Audio/click2.ogg", False),
    "S_UI_TilePlace":       ("kenney_interface-sounds/Audio/drop_003.ogg", False),
    "S_UI_Pass":            ("kenney_interface-sounds/Audio/confirmation_001.ogg", False),
    "S_UI_Fail":            ("kenney_interface-sounds/Audio/error_008.ogg", False),
    "S_UI_MenuSelect":      ("kenney_interface-sounds/Audio/maximize_008.ogg", False),
    "S_UI_CoverBlown":      ("kenney_music-jingles/Audio/Pizzicato jingles/jingles_PIZZI14.ogg", False),
    "S_UI_MissionComplete": ("kenney_music-jingles/Audio/Pizzicato jingles/jingles_PIZZI02.ogg", False),
    "S_Amb_Street":         ("oga/sfx_100_v2/sfx100v2_loop_highway.ogg", True),
    "S_Music_CalmStreet":   ("oga/busy_but_calm_street_0.ogg", True),
}

# ULinguaSpyAudioSettings property -> asset
UI_SETTINGS = {
    "Click": "S_UI_Click",
    "TilePlace": "S_UI_TilePlace",
    "Pass": "S_UI_Pass",
    "Fail": "S_UI_Fail",
    "CoverBlown": "S_UI_CoverBlown",
    "MissionComplete": "S_UI_MissionComplete",
    "MenuSelect": "S_UI_MenuSelect",
    "MenuMusic": "S_Music_CalmStreet",
}
MIX = {"UIVolume": 0.5, "AmbientVolume": 1.0, "MusicVolume": 0.6, "LoopFadeInSeconds": 1.5}

PACKS = {
    "/Game/LinguaSpy/Data/French/DA_Lang_French_Paris": ("S_Amb_Street", "S_Music_CalmStreet"),
    "/Game/LinguaSpy/Data/Spanish/DA_Lang_Spanish_Madrid": ("S_Amb_Street", "S_Music_CalmStreet"),
    "/Game/LinguaSpy/Data/Japanese/DA_Lang_Japanese_Tokyo": ("S_Amb_Street", "S_Music_CalmStreet"),
    "/Game/LinguaSpy/Data/Italian/DA_Lang_Italian_Rome": ("S_Amb_Street", "S_Music_CalmStreet"),
}


def import_all():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, (rel, loops) in SOUNDS.items():
        path = os.path.join(SRC, rel)
        if not os.path.exists(path):
            unreal.log_warning(f"Missing source {path} (see docs/Audio_Credits.md)")
            continue
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = DEST
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = False
        tools.import_asset_tasks([task])
        wave = EAL.load_asset(f"{DEST}/{name}")
        if wave:
            wave.set_editor_property("looping", loops)
            EAL.save_loaded_asset(wave, False)
    return [n for n in SOUNDS if EAL.does_asset_exist(f"{DEST}/{n}")]


def _path(name):
    return f"{DEST}/{name}.{name}"


def wire_all():
    settings = unreal.get_default_object(unreal.load_class(None, "/Script/Personal.LinguaSpyAudioSettings"))
    for prop, name in UI_SETTINGS.items():
        settings.set_editor_property(prop, EAL.load_asset(f"{DEST}/{name}"))
    for prop, value in MIX.items():
        settings.set_editor_property(prop, value)
    _write_audio_config()

    for pack_path, (amb, music) in PACKS.items():
        pack = EAL.load_asset(pack_path)
        if not pack:
            unreal.log_warning(f"Missing pack {pack_path}")
            continue
        pack.set_editor_property("ambient_sound", EAL.load_asset(f"{DEST}/{amb}"))
        pack.set_editor_property("music_sound", EAL.load_asset(f"{DEST}/{music}"))
        EAL.save_loaded_asset(pack, False)


def _write_audio_config():
    """Persists ULinguaSpyAudioSettings to Config/DefaultGame.ini (replaces the section)."""
    ini = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_config_dir()) + "DefaultGame.ini"
    text = open(ini, encoding="utf-8").read() if os.path.exists(ini) else ""
    lines = [f"{prop}={_path(name)}" for prop, name in UI_SETTINGS.items()]
    lines += [f"{prop}={value:.6f}" for prop, value in MIX.items()]
    section = "[/Script/Personal.LinguaSpyAudioSettings]\n" + "\n".join(lines) + "\n"
    text = re.sub(r"\n?\[/Script/Personal\.LinguaSpyAudioSettings\][^\[]*", "", text).rstrip() + "\n\n" + section
    open(ini, "w", encoding="utf-8").write(text)


def remove_old_audio():
    removed = []
    for asset in EAL.list_assets("/Game/LinguaSpy/Audio", recursive=False, include_folder=False):
        if asset.split("/")[-1].startswith("MS_"):
            if EAL.delete_asset(asset.split(".")[0]):
                removed.append(asset)
    if EAL.does_directory_exist(f"{DEST}/_Staging"):
        EAL.delete_directory(f"{DEST}/_Staging")
    return removed
