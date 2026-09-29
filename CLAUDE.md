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
- **Phase 2 (done, see `docs/Phase2_Plan.md`)** — branching missions, 4 cities (Paris/French, Madrid/Spanish,
  Tokyo/Japanese, Rome/Italian), main menu + city select, minimal save, Slate conversation editor tool.
- **Phase 3 (done, see `docs/Phase3_Plan.md`)** — merged Phase 2's M7 (Opération Croissant) + M8 (Madrid/Tokyo/Rome) with the art pass:
  cel-shading + outlines, Blender-modeled characters on the mannequin skeleton, env kits, occlusion fade, UI skin, MetaSound audio, NPC life.
- **Phase 4 (done, see `docs/Phase4_Plan.md` / `docs/Phase4_Log.md`)** — learning systems (Leitner SRS, Safe house review screen,
  phrasebook, tap-to-translate + per-mission fluency grade S/A/B/C, settings screen) and CC0 audio replacing the MetaSounds.
- **Phase 5 (ideas)** — more cities/languages (German, Portuguese, Korean, Mandarin w/ pinyin), second missions for Madrid/Tokyo/Rome,
  per-city CC0 music, template cleanup (`Variant_*`, `/Game/TopDown`).

## Code layout & conventions
- New gameplay code lives in `Source/Personal/LinguaSpy/`:
  - `Data/` — content schemas (`UConversationAsset`, `ULanguagePackAsset`, structs in `ConversationTypes.h`).
  - `Language/` — `USentenceGrader` (pure, testable), `ULanguageProgressSubsystem` (per-skill stats), `USrsScheduler` (pure Leitner
    scheduling, `FSrsItemKey` = `<conversation path>#<NodeId>#<ChoiceIndex>`), `USpacedRepetitionSubsystem` (records every graded
    answer, review queues, phrasebook items, `SetDebugTimeOffsetHours` for tests), `UFluencyScoring` (pure grade from mistakes/peeks).
  - `Dialogue/` — `UConversationSubsystem` (world subsystem driving the active conversation, cover-blown practice drills and
    `Review` sessions via `StartReviewSession`/`StartReviewForCity`; `RevealTranslation` = tap-to-translate peek; tallies feed
    `UMissionSubsystem::AddFluencyTally` when an NPC conversation ends).
  - `Cover/` — `UCoverComponent` (cover meter on the player).
  - `Interaction/` — `ANpcCharacter` (mannequin-skeleton body, faces the player while talking, `ConversationOptions`
    gated by flags / `RequiredMission` / current target), `APedestrianCharacter` (wandering locals, AIController + navmesh).
  - `Camera/` — `UOcclusionFadeComponent` (fades occluders via Custom Primitive Data[0] → `M_ToonMaster` dither).
  - `Audio/` — `ULinguaSpyAudioSettings` (Project Settings → Game → LinguaSpy Audio: sounds + base mix levels),
    `ULinguaSpyAudioSubsystem` (volume = mix level × player slider × master, live slider changes, loop fade-in) behind
    `LinguaSpyAudio::PlayUI/PlayLoop(…, ESpyAudioChannel)`.
  - `Mission/` — `UMissionAsset` (objective graph, `ResolveTransition`), `UMissionSubsystem` (city/mission/flags),
    `UMissionTargetComponent` (id + beacon), `AMissionMarker` (ReachLocation), `AMissionPickup` (Collect).
  - `Save/` — `ULinguaSpySaveGame` (v2: skills, cities, settings incl. volumes, `SrsItems`, `BestGrades`; `Migrate()` runs after
    load) + `ULinguaSpySaveSubsystem`. **Bump `CurrentVersion` + add a step in `Migrate` whenever the format changes.**
  - `UI/` — code-built widgets (conversation, tiles, HUD, main menu, city cards, pause); `BindWidgetOptional` lets a WBP restyle later.
    `UOverlayPanelWidget` is the shared frame for `USettingsWidget`, `UPhrasebookWidget`, `UPracticeScreenWidget` (Safe house).
  - `Tests/` — automation tests (`LinguaSpy.*`: Grader, GraderLanguages, MissionTransitions, SaveRoundTrip, SaveRoundTripSrs,
    SaveMigration, NpcConversationResolve, Srs.Schedule, Srs.ItemKey, FluencyGrade).
- `Source/PersonalEditor/` — editor-only module: **Tools → LinguaSpy → Conversation Editor** tab (`SConversationEditor`).
- `Tools/Content/*.py` — Python content scripts (run in-editor via unreal-mcp): `paris_content.py` (+ `paris_croissant.py`),
  `madrid|tokyo|rome_content.py`, `art_setup.py` (imports + toon materials), `audio_setup.py` (MetaSounds). They overwrite
  the assets they build; once an asset is hand-edited (editor/tool), stop regenerating it from the script.
- `Tools/Greybox/greybox_kit.py` + `cities.py` — layout-driven city map builder (`build_city_map(PARIS|MADRID|TOKYO|ROME)`).
- `Tools/Blender/build_characters.py` / `build_kits.py` — procedural Blender builders (run via Blender MCP) for the 7 characters
  and the per-city environment kits; `.blend` files saved next to them. Exports go to `Saved/Export/`.
- Game content lives under `/Game/LinguaSpy/` (`Data/`, `UI/`, `Blueprints/`, `Maps/`).
- **Never hardcode language content in C++.** Lines, translations, tiles, and accepted answers go in DataAssets.
  Source files are UTF-8; accents matter (grader treats `café` ≠ `cafe`).
- Tile conventions: elided words are ONE tile (`s'il`, `d'acqua`, `l'eau`); answers join with spaces, or with nothing
  for `bUsesWordSpacing=false` languages (Japanese chunks: `コーヒー|を|ください`). Readings (romaji) live on `FWordTile.Reading`.
- Mission flow: conversation `End` nodes report `Outcome` (default `Completed`); `CoverBlownOutcome` lets a mission branch
  after a blown cover. Objectives advance via the first matching transition (outcome + required/blocked flags).
- Keep logic in C++; Blueprints are thin subclasses for assets/layout.
- **SRS keys depend on conversation asset paths and `NodeId`s** — don't rename/move nodes or conversations of shipped content
  (stale keys are skipped, but the player's progress on them is lost).
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
  Cities: `Lvl_Paris`, `Lvl_Madrid`, `Lvl_Tokyo`, `Lvl_Rome` (built by the greybox kit; street along X, north = +Y).
  `APersonalGameMode::StartPlay` enters the city (pack whose
  `CityMap` is this map) and starts ambience.
- Content: `/Game/LinguaSpy/Data/<Language>/` (conversations, `Missions/`, `DA_Lang_*` packs). Paris: First Contact (tutorial)
  → Opération Croissant. Madrid/Tokyo/Rome: one mission each (`docs/City_Scripts_Review.md`).

## Art pipeline (Phase 3)
- Look: `M_ToonMaster` (VertexColor × per-instance `Tint` where VertexColor.a = 1, roughness 1, CPD[0] dithered fade) on
  all kit/character meshes; `PP_Toon` (unbound PostProcessVolume "ToonPostProcess" in every map) = cel bands + depth/normal
  ink outlines. Tints: `MI_Tint_<HEX>` under `/Game/LinguaSpy/Materials/Tints` (created on demand by the greybox kit).
- Kits: `/Game/LinguaSpy/Art/{Shared,Paris,Madrid,Tokyo,Rome}` (face +Y in UE; north-row buildings are rotated 180°).
- Characters: `/Game/LinguaSpy/Art/Characters/SK_{Spy,Barista,Vendor,Elder,Young,Tourist,Worker}` on `SK_Mannequin`,
  rigid "figurine" parts (each part 100% bound to one bone), colors in vertex colors.
- **FBX recipe for mannequin-skeleton meshes (critical):** armature object named `Armature` with a real `root` bone
  (all orphan bones parented to it); bake rig+mesh ×100 and set scene `unit_settings.scale_length = 0.01`, export with
  `apply_unit_scale=True, FBX_SCALE_NONE`. Otherwise Interchange puts ×100 on the root bone → characters are huge while
  moving (blendspaces use the mesh ref pose). Verify with `SkeletonService.get_bone_transform(mesh, "root")` scale = 1.
- UE 5.8 imports FBX through Interchange; skeletal-mesh materials must be set by rebuilding the `SkeletalMaterial` list.
- UI is code-built by default (`UConversationWidget`, `USpyHUDWidget`, `UWordTileWidget`); the controller creates it.

## Editor-automation gotchas (unreal-mcp / Python)
- After setting Blueprint CDO values from Python, **recompile the Blueprint** (`BlueprintEditorLibrary.compile_blueprint`)
  or instances won't pick them up.
- `unreal.Rotator(roll, pitch, yaw)` positional order; don't build `unreal.Color` positionally (channel order differs) —
  convert sRGB hex to `LinearColor` manually.
- World subsystems in PIE: find via `unreal.ObjectIterator(unreal.ConversationSubsystem)` (path contains `UEDPIE`).
- Screenshot of PIE incl. UI: `SystemLibrary.execute_console_command(pie_world, "shot showui", pc)` → `Saved/Screenshots/WindowsEditor`.
- When C++ needs a rebuild: if the editor is closed, build with Build.bat yourself (then launch the editor); if it is open,
  ask the user to recompile/relaunch (Live Coding blocks Build.bat).
- `EditorAssetLibrary.load_asset` fails during PIE — use `unreal.load_asset("/Game/...Asset.Asset")` instead.
- Scripted PIE: `Tools/Test/pie_driver.py` (drives conversations/reviews through the subsystem delegates). Python callables bound
  to dynamic delegates are weakly held — keep them alive (the driver stores them in `builtins`).
- `shot showui` crashed the editor once in 5.8 (D3D12/Slate RHI, Phase 4). Safer: capture the editor window with Win32
  `PrintWindow` (PowerShell). SaveGame serialization skips properties equal to the class default (why `Version` defaults to 0).
- If unreal-mcp failed to connect at session start, it can still be reached over plain HTTP JSON-RPC at `http://127.0.0.1:8000/mcp`.
- `EditorAssetLibrary.delete_asset` on the MetaSounds removed them in memory but left the files — check disk, `git rm` if needed.
- The first `shot showui` after starting PIE often captures the editor viewport — take a second shot.
- Blender MCP: never leave the armature hidden when running `transform_apply` (it silently skips hidden objects).
- Stacked flat slabs at the same top height z-fight (flicker) — keep water/plazas a few cm above road/sidewalk.
- **Git LFS:** if the editor logs `Package is unloadable ... Invalid value for PACKAGE_FILE_TAG`, the working copy holds LFS
  pointer files (e.g. after `git lfs migrate`). Close the editor and run `git lfs checkout` (objects are in `.git/lfs/objects`).

## Known issues / next up
- Toon post-process thresholds (`PP_Toon` T1/T2/levels) are tuned by eye; revisit per city if lighting changes.
- Audio: CC0 samples in `/Game/LinguaSpy/Audio/CC0` via `Tools/Content/audio_import_cc0.py` (sources in `Saved/AudioSources`,
  credits in `docs/Audio_Credits.md`); pick sounds with `Tools/Audio/analyze_audio.py` — avoid sustained single-pitch pads
  (flatness ≈ 0) and bright clicks (centroid > ~3 kHz). All cities share one music track + street bed for now.
- Git LFS is enabled for `*.uasset` / `*.umap`.
- Next phase: Phase 5 ideas above; `HANDOFF.md` is the Phase 4 brief (historical).

## Build / test
- Editor must be closed (or use Live Coding for non-header changes) when adding new UCLASSes:
  `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" PersonalEditor Win64 Development -Project="C:/UnrealEngineProjects/Personal/Personal.uproject" -WaitMutex`
- Tests: Session Frontend → Automation → `LinguaSpy`, or
  `UnrealEditor-Cmd.exe Personal.uproject -ExecCmds="Automation RunTests LinguaSpy; Quit" -unattended -nullrhi`
- Editor automation is available through the `unreal-mcp` server (VibeUE plugin) while the editor is open.
