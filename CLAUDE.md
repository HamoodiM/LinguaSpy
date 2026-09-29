# LinguaSpy (working title) — project guide

Unreal Engine 5.8 C++ project (module `Personal`), started from the Top Down template.

## Vision (end goal — keep in mind for every change)
A **stylized, low-detail, top-down spy game for learning a language**.
- The player picks a city/language (e.g. **Paris → French**) and plays a spy storyline set there.
- Locals speak the target language. A **helper** (handler in the player's ear) translates what they say.
- The player **replies by building sentences** (currently from **word tiles**, Duolingo-style).
- Failing to produce a correct sentence costs **cover**. When cover is blown, the mission is interrupted
  and the player must **practice the weak skill** (e.g. Greetings, Ordering, Directions) before retrying.
- All language content is **authored and offline** — no LLM/network dependency at runtime.
- Art direction: **stylized, simple shapes, flat/pastel colours**, readable from the top-down camera. Not realistic.

Future ideas (not yet built): multiple cities/languages, mission/story framework, persistent skill
progress (save game), spaced-repetition practice, helper reliance scoring, optional typed/voice input.

## Roadmap
- **Phase 1 (done)** — core loop vertical slice: greybox Paris street, NPC conversations, helper
  translation, word-tile replies, grader, cover meter, cover-blown → practice drill.
- **Phase 2 (current, see `docs/Phase2_Plan.md`)** — branching missions, 4 cities (Paris/French, Madrid/Spanish,
  Tokyo/Japanese, Rome/Italian), main menu + city select, minimal save, Slate conversation editor tool.
- **Phase 3** — stylized art pass (toon/flat shading, stylized characters, landmarks, UI skin, camera occlusion fade).
- **Phase 4** — more cities/languages, spaced-repetition practice, phrasebook, helper-reliance scoring.

## Code layout & conventions
- New gameplay code lives in `Source/Personal/LinguaSpy/`:
  - `Data/` — content schemas (`UConversationAsset`, `ULanguagePackAsset`, structs in `ConversationTypes.h`).
  - `Language/` — `USentenceGrader` (pure, testable), `ULanguageProgressSubsystem` (per-skill stats).
  - `Dialogue/` — `UConversationSubsystem` (world subsystem driving the active conversation + practice drills).
  - `Cover/` — `UCoverComponent` (cover meter on the player).
  - `Interaction/` — `ANpcCharacter` (MissionTarget id + flag-based `ConversationOptions`).
  - `Mission/` — `UMissionAsset` (objective graph, `ResolveTransition`), `UMissionSubsystem` (city/mission/flags),
    `UMissionTargetComponent` (id + beacon), `AMissionMarker` (ReachLocation), `AMissionPickup` (Collect).
  - `Save/` — `ULinguaSpySaveGame` + `ULinguaSpySaveSubsystem` (single profile: skills per city, mission state, settings).
  - `UI/` — code-built widgets (conversation, tiles, HUD, main menu, city cards, pause); `BindWidgetOptional` lets a WBP restyle later.
  - `Tests/` — automation tests (`LinguaSpy.*`: Grader, GraderLanguages, MissionTransitions, SaveRoundTrip).
- `Source/PersonalEditor/` — editor-only module: **Tools → LinguaSpy → Conversation Editor** tab (`SConversationEditor`).
- `Tools/Content/*.py` — Python content scripts (run in-editor via unreal-mcp). They overwrite the assets they build;
  once an asset is hand-edited (editor/tool), stop regenerating it from the script.
- Game content lives under `/Game/LinguaSpy/` (`Data/`, `UI/`, `Blueprints/`, `Maps/`).
- **Never hardcode language content in C++.** Lines, translations, tiles, and accepted answers go in DataAssets.
  Source files are UTF-8; accents matter (grader treats `café` ≠ `cafe`).
- Tile conventions: elided words are ONE tile (`s'il`, `d'acqua`, `l'eau`); answers join with spaces, or with nothing
  for `bUsesWordSpacing=false` languages (Japanese chunks: `コーヒー|を|ください`). Readings (romaji) live on `FWordTile.Reading`.
- Mission flow: conversation `End` nodes report `Outcome` (default `Completed`); `CoverBlownOutcome` lets a mission branch
  after a blown cover. Objectives advance via the first matching transition (outcome + required/blocked flags).
- Keep logic in C++; Blueprints are thin subclasses for assets/layout.
- `Source/Personal/Variant_Strategy` and `Variant_TwinStick` are unused template leftovers (kept for now).

## Human-in-the-loop
- `EDITOR_TODO.md` (repo root) is the running list of things the user must do by hand in the editor
  (recompiles, playtests, manual editor edits Claude couldn't do, pending decisions). Keep it concise and current:
  add items when you hand something off, move them to Done when confirmed.

## Gameplay wiring (important)
- The template's `/Game/TopDown/Blueprints/BP_TopDown*` are **pure Blueprint** (parents: Character /
  PlayerController / GameModeBase) and do NOT use the C++ classes. The game uses
  `/Game/LinguaSpy/Blueprints/BP_SpyCharacter|BP_SpyController|BP_SpyGameMode` (parents: `APersonalCharacter`,
  `APersonalPlayerController`, `APersonalGameMode`). `BP_SpyGameMode` is the project default and the Paris map's override.
- Startup/default map: `/Game/LinguaSpy/Maps/Lvl_MainMenu` (`AMenuGameMode`, city cards from all `ULanguagePackAsset`s).
  Cities: `Lvl_Paris_Greybox` (street along X; café/Camille north x≈-900, kiosk/Marcel south x≈900, metro marker east).
  `APersonalGameMode::StartPlay` enters the city (finds the pack whose `CityMap` is this map).
  Materials: `M_Flat` + `MI_*` pastel instances in `/Game/LinguaSpy/Materials`.
- Content: `/Game/LinguaSpy/Data/<Language>/` (conversations, `Missions/`, `DA_Lang_*` packs). Madrid/Tokyo/Rome packs exist
  without maps ("Coming soon") until M8. Paris content is generated by `Tools/Content/paris_content.py`.
- UI is code-built by default (`UConversationWidget`, `USpyHUDWidget`, `UWordTileWidget`); the controller creates it.

## Editor-automation gotchas (unreal-mcp / Python)
- After setting Blueprint CDO values from Python, **recompile the Blueprint** (`BlueprintEditorLibrary.compile_blueprint`)
  or instances won't pick them up.
- `unreal.Rotator(roll, pitch, yaw)` positional order; don't build `unreal.Color` positionally (channel order differs) —
  convert sRGB hex to `LinearColor` manually.
- World subsystems in PIE: find via `unreal.ObjectIterator(unreal.ConversationSubsystem)` (path contains `UEDPIE`).
- Screenshot of PIE incl. UI: `SystemLibrary.execute_console_command(pie_world, "shot showui", pc)` → `Saved/Screenshots/WindowsEditor`.
- When C++ needs a rebuild, ask the user to recompile/relaunch (don't run Build.bat while the editor is open).

## Known issues / next up
- Tall facades and awnings can occlude the player from the top-down camera → camera occlusion fade (Phase 3).
- Consider Git LFS for `Content/` binaries before the repo grows.

## Build / test
- Editor must be closed (or use Live Coding for non-header changes) when adding new UCLASSes:
  `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" PersonalEditor Win64 Development -Project="C:/UnrealEngineProjects/Personal/Personal.uproject" -WaitMutex`
- Tests: Session Frontend → Automation → `LinguaSpy`, or
  `UnrealEditor-Cmd.exe Personal.uproject -ExecCmds="Automation RunTests LinguaSpy; Quit" -unattended -nullrhi`
- Editor automation is available through the `unreal-mcp` server (VibeUE plugin) while the editor is open.
