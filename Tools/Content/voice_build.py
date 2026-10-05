"""Spoken lines for a language pack, generated OFFLINE at authoring time with Piper (docs/Phase5_Plan.md Stage 3).
Nothing here runs in the shipped game: the game only plays the imported SoundWaves.

Run inside the editor after the content scripts (they rebuild nodes, which clears the audio references):
    exec(open(r"C:/UnrealEngineProjects/Personal/Tools/Content/voice_build.py", encoding="utf-8").read())
    build_french(step="synth")    # 1) Piper -> Saved/VoiceGen/fr/... (skips unchanged lines; fast)
    build_french(step="import")   # 2) WAV -> /Game/LinguaSpy/Audio/Voice/French/... (split if it gets slow)
    build_french(step="assign")   # 3) set LineAudio / LineAudioSlow / ModelAudio / briefing + assessment audio, save

Voices (licenses checked 2026-10-04, credited in docs/Credits.md):
  fr_FR-siwis-medium  (CC-BY 4.0, SIWIS French Speech Synthesis Database) - the handler: briefings, model answers, tests
  fr_FR-mls-medium    (CC-BY 4.0, Multilingual LibriSpeech, 125 speakers)  - NPCs, one speaker per character
Tooling: rhasspy/piper 2023.11.14-2 Windows build (MIT) in Saved/VoiceGen/tools (not in git).
"""
import hashlib
import os
import subprocess
import unreal

EAL = unreal.EditorAssetLibrary
ROOT = r"C:/UnrealEngineProjects/Personal"
TOOLS = os.path.join(ROOT, "Saved/VoiceGen/tools")
PIPER = os.path.join(TOOLS, "piper/piper.exe")
SLOW_SCALE = 1.4  # length_scale for the "Slower" button

HANDLER = ("fr_FR-siwis-medium.onnx", None)
NPC_VOICES = {  # conversation asset -> (model, speaker). Speakers picked by median pitch (3 higher, 3 lower voices)
    "DA_Conv_Cafe": ("fr_FR-mls-medium.onnx", 3),            # Camille
    "DA_Conv_Cafe_Croissant": ("fr_FR-mls-medium.onnx", 3),
    "DA_Conv_Kiosk": ("fr_FR-mls-medium.onnx", 27),          # Marcel
    "DA_Conv_Kiosk_Retry": ("fr_FR-mls-medium.onnx", 27),
    "DA_Conv_Kiosk_Croissant": ("fr_FR-mls-medium.onnx", 27),
    "DA_Conv_Metro_Luc": ("fr_FR-mls-medium.onnx", 36),      # Luc
    "DA_Conv_Bakery_Sophie": ("fr_FR-mls-medium.onnx", 48),  # Sophie
    "DA_Conv_Painter_Hugo": ("fr_FR-mls-medium.onnx", 63),   # Hugo
    "DA_Conv_Bridge_Elise": ("fr_FR-mls-medium.onnx", 75),   # Elise
}
DEFAULT_NPC = ("fr_FR-mls-medium.onnx", 0)

PACK = "/Game/LinguaSpy/Data/French/DA_Lang_French_Paris"
ASSESS = "/Game/LinguaSpy/Data/French/Assessment"
DEST = "/Game/LinguaSpy/Audio/Voice/French"
WAVS = os.path.join(ROOT, "Saved/VoiceGen/fr")


def _clean(text):
    # Guillemets and the narrator's parentheses are not spoken
    text = str(text).replace("«", "").replace("»", "").replace("\u00a0", " ")
    return " ".join(text.split())


def _join(tokens):
    return " ".join(str(t) for t in tokens)


def collect():
    """Every line to speak: (folder under DEST, asset name, text, voice, length_scale)."""
    jobs = []
    pack = EAL.load_asset(PACK)
    for conv_ptr in pack.get_editor_property("conversations"):
        conv = conv_ptr if isinstance(conv_ptr, unreal.Object) else conv_ptr.load_synchronous()
        name = conv.get_name()
        voice = NPC_VOICES.get(name, DEFAULT_NPC)
        for n in conv.get_editor_property("nodes"):
            nid = str(n.get_editor_property("node_id"))
            line = _clean(n.get_editor_property("native_line"))
            if line:
                jobs.append((name, f"VO_{nid}", line, voice, 1.0))
                jobs.append((name, f"VO_{nid}_Slow", line, voice, SLOW_SCALE))
            ntype = n.get_editor_property("type")
            if ntype == unreal.ConversationNodeType.PLAYER_CHALLENGE:
                ans = n.get_editor_property("challenge").get_editor_property("accepted_answers")
                if ans:
                    jobs.append((name, f"VO_{nid}_Model", _join(ans[0].get_editor_property("tokens")), HANDLER, 1.0))
            elif ntype == unreal.ConversationNodeType.PLAYER_CHOICE:
                for i, ch in enumerate(n.get_editor_property("choices")):
                    ans = ch.get_editor_property("challenge").get_editor_property("accepted_answers")
                    if ans:
                        jobs.append((name, f"VO_{nid}_c{i}_Model", _join(ans[0].get_editor_property("tokens")), HANDLER, 1.0))
    for m_ptr in pack.get_editor_property("missions"):
        m = m_ptr if isinstance(m_ptr, unreal.Object) else m_ptr.load_synchronous()
        mid = str(m.get_editor_property("mission_id"))
        for i, c in enumerate(m.get_editor_property("briefing_chunks")):
            jobs.append((f"Briefing_{mid}", f"VO_chunk{i}", _clean(c.get_editor_property("text")), HANDLER, 1.0))
    for form in _forms():
        for item in form.get_editor_property("items"):
            if item.get_editor_property("type") == unreal.AssessmentItemType.LISTEN_CHOOSE:
                iid = str(item.get_editor_property("item_id"))
                jobs.append(("Assessment", f"VO_{iid}", _clean(item.get_editor_property("target_text")), HANDLER, 1.0))
    return jobs


def _forms():
    out = []
    for path in EAL.list_assets(ASSESS, recursive=False):
        a = EAL.load_asset(path)
        if isinstance(a, unreal.AssessmentAsset):
            out.append(a)
    return out


def _wav_path(folder, name):
    return os.path.join(WAVS, folder, name + ".wav")


def synth(jobs):
    made = skipped = 0
    for folder, name, text, (model, speaker), scale in jobs:
        out = _wav_path(folder, name)
        os.makedirs(os.path.dirname(out), exist_ok=True)
        key = hashlib.sha1(f"{text}|{model}|{speaker}|{scale}".encode("utf-8")).hexdigest()
        stamp = out + ".sha1"
        if os.path.exists(out) and os.path.exists(stamp) and open(stamp).read() == key:
            skipped += 1
            continue
        cmd = [PIPER, "--model", os.path.join(TOOLS, model), "--output_file", out, "--length_scale", str(scale),
               "--sentence_silence", "0.15"]
        if speaker is not None:
            cmd += ["--speaker", str(speaker)]
        r = subprocess.run(cmd, input=text.encode("utf-8"), capture_output=True, cwd=TOOLS,
                           creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        if r.returncode != 0 or not os.path.exists(out):
            unreal.log_error(f"piper failed for {folder}/{name}: {r.stderr[-300:]}")
            continue
        open(stamp, "w").write(key)
        made += 1
    return {"synthesized": made, "unchanged": skipped}


def import_all(jobs, max_items=400):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []
    for folder, name, *_ in jobs:
        src = _wav_path(folder, name)
        dest_dir = f"{DEST}/{folder}"
        stamp = src + ".imported"
        sha = open(src + ".sha1").read() if os.path.exists(src + ".sha1") else ""
        if EAL.does_asset_exist(f"{dest_dir}/{name}") and os.path.exists(stamp) and open(stamp).read() == sha:
            continue
        t = unreal.AssetImportTask()
        t.filename = src
        t.destination_path = dest_dir
        t.destination_name = name
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append((t, stamp, sha))
        if len(tasks) >= max_items:
            break
    if tasks:
        tools.import_asset_tasks([t for t, _, _ in tasks])
        for t, stamp, sha in tasks:
            if EAL.does_asset_exist(f"{t.destination_path}/{t.destination_name}"):
                open(stamp, "w").write(sha)
    return {"imported": len(tasks)}


def _sound(folder, name):
    path = f"{DEST}/{folder}/{name}"
    return EAL.load_asset(path) if EAL.does_asset_exist(path) else None


def assign():
    counts = {"lines": 0, "models": 0, "chunks": 0, "items": 0}
    pack = EAL.load_asset(PACK)
    for conv_ptr in pack.get_editor_property("conversations"):
        conv = conv_ptr if isinstance(conv_ptr, unreal.Object) else conv_ptr.load_synchronous()
        name = conv.get_name()
        nodes = list(conv.get_editor_property("nodes"))
        for n in nodes:
            nid = str(n.get_editor_property("node_id"))
            s = _sound(name, f"VO_{nid}")
            if s:
                n.set_editor_property("line_audio", s)
                counts["lines"] += 1
            s = _sound(name, f"VO_{nid}_Slow")
            if s:
                n.set_editor_property("line_audio_slow", s)
            ntype = n.get_editor_property("type")
            if ntype == unreal.ConversationNodeType.PLAYER_CHALLENGE:
                s = _sound(name, f"VO_{nid}_Model")
                if s:
                    c = n.get_editor_property("challenge")
                    c.set_editor_property("model_audio", s)
                    n.set_editor_property("challenge", c)
                    counts["models"] += 1
            elif ntype == unreal.ConversationNodeType.PLAYER_CHOICE:
                choices = list(n.get_editor_property("choices"))
                for i, ch in enumerate(choices):
                    s = _sound(name, f"VO_{nid}_c{i}_Model")
                    if s:
                        c = ch.get_editor_property("challenge")
                        c.set_editor_property("model_audio", s)
                        ch.set_editor_property("challenge", c)
                        counts["models"] += 1
                n.set_editor_property("choices", choices)
        conv.set_editor_property("nodes", nodes)
        EAL.save_loaded_asset(conv, False)
    for m_ptr in pack.get_editor_property("missions"):
        m = m_ptr if isinstance(m_ptr, unreal.Object) else m_ptr.load_synchronous()
        mid = str(m.get_editor_property("mission_id"))
        chunks = list(m.get_editor_property("briefing_chunks"))
        for i, c in enumerate(chunks):
            s = _sound(f"Briefing_{mid}", f"VO_chunk{i}")
            if s:
                c.set_editor_property("audio", s)
                counts["chunks"] += 1
        m.set_editor_property("briefing_chunks", chunks)
        EAL.save_loaded_asset(m, False)
    for form in _forms():
        items = list(form.get_editor_property("items"))
        for item in items:
            s = _sound("Assessment", f"VO_{item.get_editor_property('item_id')}")
            if s:
                item.set_editor_property("audio", s)
                counts["items"] += 1
        form.set_editor_property("items", items)
        EAL.save_loaded_asset(form, False)
    return counts


def build_french(step="all"):
    jobs = collect()
    out = {"jobs": len(jobs)}
    if step in ("synth", "all"):
        out.update(synth(jobs))
    if step in ("import", "all"):
        out.update(import_all(jobs))
    if step in ("assign", "all"):
        out.update(assign())
    return out
