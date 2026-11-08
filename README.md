# LinguaSpy

**A stylized top-down spy game for learning a language.** You're undercover in Paris, Madrid, Tokyo or Rome, and
the locals speak the language. Your handler whispers translations in your ear. You answer by building sentences.
Say the wrong thing and your cover slips. Blow it, and you're pulled back to the safe house to drill the phrases
that gave you away.

*Unreal Engine 5.8 · C++ · Python editor tooling · Blender procedural art · prototype v0.6*

![LinguaSpy gameplay](docs/linguaspy.gif)

- **Trailer and write-up:** [hamoodim.github.io/projects/linguaspy](https://hamoodim.github.io/projects/linguaspy/)
- **Windows build:** [latest release](https://github.com/HamoodiM/LinguaSpy/releases/latest) (64-bit, unzip and run)

## Gameplay systems
- **Cover meter and conversations**: NPC conversations are branching graphs. Wrong answers drain cover. A blown cover branches the mission into a practice drill on the skill you missed.
- **Missions**: each mission is an objective graph that advances on conversation outcomes, flags, pickups and reach-location markers. 6 missions across 4 cities.
- **Sentence building**: a word-tile reply UI. Typed answers have accent keys. A per-phrase scaffold picks how much help you get, from picking the right sentence up to typing it from scratch.
- **Camera occlusion fade**: a component sweeps a sphere from the camera to the player every tick, then dithers out any building in the way through custom primitive data in the toon material.
- **NPC life**: talking NPCs turn to face the player. Wandering pedestrians use an AI controller on the navmesh.
- **Handler modes**: translate everything, on call, or radio silence. The mode adapts to your mission grade.

## Engineering highlights
- **Data-driven content**: conversations and missions are DataAssets, and the C++ never contains language content. A custom **Slate conversation editor** tab edits and validates the graphs, with undo.
- **Pure, unit-tested core**: the accent-aware grader handles 4 languages, including unspaced Japanese chunks with romaji. The scaffold policy, Leitner scheduler, fluency scoring and debrief selection are pure functions too. **23 automation tests** run headless.
- **Versioned save format** (v4) with step-by-step migrations and round-trip tests.
- **Content pipeline in Python**: scripts build every conversation, mission and city layout through the editor's Python API. Assets edited by hand are tagged, so the scripts leave them alone.
- **Toon rendering**: one master material (vertex colours, per-instance tint, dithered fade) plus a post-process with cel bands and depth/normal ink outlines.
- **Procedural characters and city kits**: built with Blender Python and bound to the UE5 Mannequin skeleton.
- **Offline voices**: Piper TTS, one speaker per character, with normal and slow takes. Lines are hashed, so only changed lines are regenerated.

## Running it
- **Play:** download the [Windows build](https://github.com/HamoodiM/LinguaSpy/releases/latest), or open `Personal.uproject` in UE 5.8 and press Play on `Lvl_MainMenu`. The C++ builds on first open. The project and module still carry the `Personal` name from the template I started from.
- **Tests:** Session Frontend → Automation → `LinguaSpy`, or
  `UnrealEditor-Cmd.exe Personal.uproject -ExecCmds="Automation RunTests LinguaSpy; Quit" -unattended -nullrhi`

## Layout
```
Source/Personal/LinguaSpy/   gameplay: Data, Language, Dialogue, Mission, Cover, Camera, Interaction, Save, UI, Audio, Tests
Source/PersonalEditor/       Conversation Editor (Slate)
Tools/Content/               content build scripts (Python, run in the editor)
Tools/Blender/               procedural characters and city kits
docs/                        learning design, scripts, credits; docs/dev-log has the build-phase notes
```

## Learning design
The loop follows second-language acquisition research:
- a briefing before any cover is at stake;
- scaffolding that fades as you improve;
- hints before answers;
- spaced repetition.

The full write-up with citations is [`docs/Learning_Design.md`](docs/Learning_Design.md). The game also has an assessment mode and an offline pilot-study kit ([`docs/Pilot_Study_Protocol.md`](docs/Pilot_Study_Protocol.md)). No learning results are claimed until a pilot has run.

## Credits
Audio, voices and fonts: [`docs/Credits.md`](docs/Credits.md). Designed and led by Mohammad Mohammad, using Claude Code as an AI coding assistant.
