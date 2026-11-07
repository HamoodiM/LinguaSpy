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
- **Phase 5 (planned, see `docs/Phase5_Plan.md`)**: research-grounded learning loop (`docs/Learning_Design.md`):
  briefing → scaffolded field challenges → debrief, feedback ladder, offline TTS voice, field notes, intel checks,
  handler modes, CEFR can-dos. Also A2 second missions for Madrid/Tokyo/Rome, assessment mode + local telemetry +
  packaged pilot build (`docs/Pilot_Study_Protocol.md`), replay, per-city music, template cleanup.
- **Phase 6–7 (ideas)**: FSRS, Berlin/German, teacher mode, localization, accessibility, offline speech input, more languages.
- **No learning-outcome claims** (README, docs, store pages) until a real pilot has produced data.

## Code layout & conventions
- New gameplay code lives in `Source/Personal/LinguaSpy/`:
  - `Data/` — content schemas (`UConversationAsset`, `ULanguagePackAsset`, structs in `ConversationTypes.h`). Phase 5 added:
    challenge `Hints` / `GapTokenIndices` / `RecognitionDistractors` / `ModelAudio`; node `LineAudio(Slow)`; node type
    `IntelCheck` (`FIntelCheck`); pack `GrammarNotes` (field notes), `bAllowTypedInput`, `SpecialCharacters` (accent keys).
  - `Language/` — `USentenceGrader` (pure, testable), `ULanguageProgressSubsystem` (per-skill stats), `USrsScheduler` (pure Leitner
    scheduling, `FSrsItemKey` = `<conversation path>#<NodeId>#<ChoiceIndex>`), `USpacedRepetitionSubsystem` (records every graded
    answer, review queues, phrasebook items, `SetDebugTimeOffsetHours` for tests, `FindStrongestForPhrase` = same phrase in another
    conversation counts), `UFluencyScoring` (pure grade from mistakes/peeks), `UScaffoldPolicy` (pure: scaffold level from SRS box
    and session errors — Recognize / Assemble / AssemblePlus / GapFill / Produce — tiles, gaps, recognition options, hints).
  - `Dialogue/` — `UConversationSubsystem` (world subsystem driving the active conversation, cover-blown practice drills and
    `Review` sessions via `StartReviewSession`/`StartReviewForCity`; `RevealTranslation` = tap-to-translate peek; tallies feed
    `UMissionSubsystem::AddFluencyTally` when an NPC conversation ends). Phase 5: delegates carry `FChallengePresentation` /
    `FAnswerFeedback`; feedback ladder (1st miss = hint + 40% penalty, 2nd = recast with model audio); `SubmitGapWords` /
    `SubmitRecognition` / `SubmitIntel`; `RepeatLine(bSlow)`; handler modes (Shadowing / OnCall / RadioSilence, per city in the
    save); debrief = review with `bDebrief` (doesn't promote SRS boxes); field notes announced after the conversation.
  - `Cover/` — `UCoverComponent` (cover meter on the player).
  - `Interaction/` — `ANpcCharacter` (mannequin-skeleton body, faces the player while talking, `ConversationOptions`
    gated by flags / `RequiredMission` / current target), `APedestrianCharacter` (wandering locals, AIController + navmesh).
  - `Camera/` — `UOcclusionFadeComponent` (fades occluders via Custom Primitive Data[0] → `M_ToonMaster` dither).
  - `Audio/` — `ULinguaSpyAudioSettings` (Project Settings → Game → LinguaSpy Audio: sounds + base mix levels),
    `ULinguaSpyAudioSubsystem` (volume = mix level × player slider × master, live slider changes, loop fade-in) behind
    `LinguaSpyAudio::PlayUI/PlayLoop(…, ESpyAudioChannel)`; `PlayVoice` (Voice channel, UI sound so it plays while paused,
    ducks music).
  - `Mission/` — `UMissionAsset` (objective graph, `ResolveTransition`), `UMissionSubsystem` (city/mission/flags),
    `UMissionTargetComponent` (id + beacon), `AMissionMarker` (ReachLocation), `AMissionPickup` (Collect). Missions carry
    `BriefingChunks`, `CanDoStatements`, `DebriefItems`; the subsystem records mission items, starts the debrief, adapts the
    handler mode (`AdaptHandlerMode`: S/A = less help, C = more) and tracks briefings.
  - `Telemetry/` — `ULearningTelemetrySubsystem`: local JSONL learning log (`Saved/Telemetry`), off unless enabled or Pilot mode
    (`-PilotMode`, or `pilot.ini` in Saved/ or next to the packaged game). **No network code — keep it that way.**
  - `Assessment/` — `UAssessmentAsset` (test forms; pure `ScoreItem`), `UAssessmentSubsystem` (runs forms without feedback,
    counterbalances A/B by participant code, stores `FAssessmentResult`s, placement sets the handler mode).
  - `Save/` — `ULinguaSpySaveGame` (v3: skills, cities, settings incl. volumes + voice + telemetry toggle, `SrsItems`, `BestGrades`,
    `HandlerModes`, field notes, `BriefingsCompleted`, `ParticipantCode`, `AssessmentResults`; `Migrate()` runs after
    load) + `ULinguaSpySaveSubsystem`. **Bump `CurrentVersion` + add a step in `Migrate` whenever the format changes.**
  - `UI/` — code-built widgets (conversation, tiles, HUD, main menu, city cards, pause); `BindWidgetOptional` lets a WBP restyle later.
    `UOverlayPanelWidget` is the shared frame for `USettingsWidget`, `UPhrasebookWidget`, `UPracticeScreenWidget` (Safe house),
    `UBriefingWidget` (opened by the controller when a mission needs it; pauses the game), `UAssessmentWidget`, `UFieldNoteWidget`.
  - `Tests/` — automation tests (`LinguaSpy.*`: Grader, GraderLanguages, MissionTransitions, SaveRoundTrip, SaveRoundTripSrs,
    SaveMigration, NpcConversationResolve, Srs.Schedule, Srs.ItemKey, FluencyGrade, Scaffold.Choose, Scaffold.Material,
    GraderTolerant, Feedback.Hint, Debrief.Select, Handler.Adapt, SaveMigrationV3, Assessment.Scoring, Telemetry.Format).
    **Naming gotcha:** a test named `X.Y.Z` hides an existing test named `X.Y` — pick a sibling name instead.
- `Source/PersonalEditor/` — editor-only module: **Tools → LinguaSpy → Conversation Editor** tab (`SConversationEditor`).
- `Tools/Content/*.py` — Python content scripts (run in-editor via unreal-mcp): `paris_content.py` (+ `paris_croissant.py`),
  `madrid|tokyo|rome_content.py`, `art_setup.py` (imports + toon materials), `audio_setup.py` (MetaSounds). They overwrite
  the assets they build; assets saved from the Conversation Editor get the `LinguaSpy.HandEdited` metadata tag and
  `lingua_content.get_or_create` skips them (set `lingua_content.FORCE = True` to override).
  Phase 5: `paris_assessment.py` (FR_A/FR_B test forms), `voice_build.py` (Piper TTS → SoundWaves; run synth / import /
  assign as separate calls — synth exceeds the 30 s MCP limit in-editor; re-run after any content script, which rebuilds nodes).
- `Tools/Analysis/pilot_report.py` — stdlib-only pilot analysis (`--selftest` uses synthetic data, clearly marked).
- `Tools/Test/pie_driver.py` (scripted PIE; answers at whatever scaffold level is presented) + `capture_window.ps1` (PrintWindow).
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
- `set_collision_enabled(NO_COLLISION)` on a placed actor's component was **not saved** with the map (the Seine and
  Tokyo lanterns kept full collision → navmesh island on the river). Use `set_collision_profile_name("NoCollision")`
  (+ `can_ever_affect_navigation=False`) after `modify()`; `greybox_kit.place(collide=False)` does this now.
- Complex-as-simple collision only takes effect for navigation after the mesh's simple collision is removed
  (`StaticMeshEditorSubsystem.remove_collisions`); `set_complex_collision` does both (torii fix).
- After editing a city map, run `BuildPaths` and wait before saving: a map saved mid-rebuild shipped an **empty
  navmesh** to `-game` (nobody could walk in Paris).
- Look/visual-polish tools: `Tools/Content/pp_toon_v2.py` (PP_Toon edited in place), `toon_master_v2.py`,
  `greybox_kit.apply_look / apply_street_detail / apply_nav_fixes / apply_menu_look` (values in `cities.py`), notes in
  `docs/Visual_Polish_Log.md`. Never re-run `build_city_map()` just to change lighting.
- **Git LFS:** if the editor logs `Package is unloadable ... Invalid value for PACKAGE_FILE_TAG`, the working copy holds LFS
  pointer files (e.g. after `git lfs migrate`). Close the editor and run `git lfs checkout` (objects are in `.git/lfs/objects`).

## Known issues / next up
- Toon post-process thresholds (`PP_Toon` T1/T2/levels) are tuned by eye; revisit per city if lighting changes.
- Audio: CC0 samples in `/Game/LinguaSpy/Audio/CC0` via `Tools/Content/audio_import_cc0.py` (sources in `Saved/AudioSources`,
  credits in `docs/Credits.md`); pick sounds with `Tools/Audio/analyze_audio.py` — avoid sustained single-pitch pads
  (flatness ≈ 0) and bright clicks (centroid > ~3 kHz). All cities share one music track + street bed for now.
- Git LFS is enabled for `*.uasset` / `*.umap`.
- Phase 5 progress: `docs/Phase5_Log.md`. Pilot kit: `docs/pilot_kit/`. `HANDOFF.md` is the Phase 4 brief (historical).
- **Claims:** no learning-outcome claims anywhere (README, resume text, store pages) until a real pilot has produced data.

## Build / test
- Editor must be closed (or use Live Coding for non-header changes) when adding new UCLASSes:
  `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" PersonalEditor Win64 Development -Project="C:/UnrealEngineProjects/Personal/Personal.uproject" -WaitMutex`
- Tests: Session Frontend → Automation → `LinguaSpy`, or
  `UnrealEditor-Cmd.exe Personal.uproject -ExecCmds="Automation RunTests LinguaSpy; Quit" -unattended -nullrhi`
- Editor automation is available through the `unreal-mcp` server (VibeUE plugin) while the editor is open.
