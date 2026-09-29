# HANDOFF: Planning and Executing Phase 4 of LinguaSpy

This document is for a **fresh Claude session**. Your job is to **plan Phase 4 yourself, then execute it on your own**.
Read all of it before doing anything. It gives you the context needed to *plan well*, not just a task list:
who the user is, how they like to work, what exists, what's verified vs. assumed, where the rough edges are,
and the design questions Phase 4 has to answer.

Companion docs (read in this order):
1. `CLAUDE.md`: the living project guide. Architecture, conventions, art pipeline, automation gotchas. **Authoritative.**
2. `EDITOR_TODO.md`: things only the user can do or decide. Check what's still open.
3. `docs/Phase3_Log.md`, `docs/Phase3_Plan.md`, `docs/Phase2_Plan.md`: how earlier phases were planned and what was decided.
4. `docs/City_Scripts_Review.md`, `docs/Paris_Mission_Draft.md`: all authored language content, in readable form.
5. Your auto-memory (`MEMORY.md`), especially the build rule in `recompile-handoff`.

---

## 1. The product, in one paragraph

**LinguaSpy** (working title) is a stylized, low-detail, **top-down spy game for learning a language** (UE 5.8.2, C++).
The player picks a city, which fixes the language (Paris/French, Madrid/Spanish, Tokyo/Japanese, Rome/Italian), and plays spy missions there.
Locals speak the target language. A **helper** (the handler in your ear) translates. The player **answers by building
sentences from word tiles** (Duolingo-style; there's no typing or voice). Wrong answers cost **cover**. Blowing your cover triggers a
**practice drill** of the phrases you missed, then the conversation restarts, or the mission branches or fails.
All content is **authored and offline**, with no LLM or network at runtime (a deliberate user decision). The art is cel-shaded
with ink outlines, pastel palettes per city, and chunky low-poly "figurine" characters.

## 2. The user and how they want to work (important for planning)

- **Solo developer, hands-on in the editor**, working on Windows 11 with Visual Studio 2026 (MSVC 14.51). Friendly and pragmatic. They set
  direction and review, and they like Claude to do the heavy lifting autonomously.
- **They want to be asked questions before a phase is planned.** Every phase so far started with one or two rounds of
  `AskUserQuestion` (multiple choice with a recommended option first), then a written plan, then `ExitPlanMode` for approval.
  Keep the question rounds tight: 2–4 questions per round, and only real decisions.
- **They often aren't watching while you execute.** For Phase 3 they said they might be asleep and couldn't intervene.
  So once the plan is approved, **execute autonomously**:
  - don't block on questions;
  - make sensible calls and log them in `docs/Phase4_Log.md`;
  - put anything that genuinely needs them in `EDITOR_TODO.md`.
- **Builds:** you may run `Build.bat` yourself **only while the editor is closed**, and you must fix compile errors yourself.
  If the editor is open, Live Coding blocks Build.bat, so ask the user to rebuild (they asked to always be told).
  You may also **launch the editor yourself** after a build (`UnrealEditor.exe Personal.uproject`) and wait for unreal-mcp.
- **They playtest and report concrete bugs** mid-session (e.g. "characters are huge while moving", "water flickers"). Treat those as top
  priority, fix the root cause, and add the lesson to the gotchas in CLAUDE.md.
- **They appreciate visual progress**: send PIE screenshots with `SendUserFile` at milestones.
- **Style decisions already made by the user** (don't reopen them without reason):
  - word tiles, not typing;
  - offline authored content;
  - cel-shading with outlines;
  - Blender-modeled characters;
  - procedural MetaSound audio (no downloads);
  - "build first, review later" for new-language scripts;
  - First Contact is the Paris tutorial.
- **Commits**: commit after every stage. The message ends with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. The repo uses **Git LFS**
  for `*.uasset` and `*.umap` (the user just enabled it). There's no remote or PR workflow; it's local git only.

## 3. Where the project stands (end of Phase 3)

**Playable loop.** Main menu (`Lvl_MainMenu`) → city card → city map → missions with a HUD objective tracker and beacons →
conversations (NPC line, helper translation, optional romaji, tile or choice replies) → cover meter → practice drill on
cover blown → mission branching, failure or completion → a save profile that persists progress and settings.

**Content**:

| City | Map | Missions | Conversations | Notes |
|---|---|---|---|---|
| Paris (fr) | `Lvl_Paris` | First Contact (tutorial) → Opération Croissant (6 convs, branches: Marcel suspicious → Luc; Élise cover-blown → mission failed) | 9 | Seine + bridge, café, kiosk, metro, boulangerie, Place du Tertre |
| Madrid (es) | `Lvl_Madrid` | La Tapa Secreta | 3 | plaza with arcades and fountain, tapas bar, metro |
| Tokyo (ja) | `Lvl_Tokyo` | Konbini Contact | 3 | kana/kanji tiles, romaji readings, `\|` tokenization, shrine + torii |
| Rome (it) | `Lvl_Rome` | Protocollo Espresso | 3 | piazza with fountain, trattoria, Vespas |

The skills practiced so far (the `SkillTag`s) are Ordering, SmallTalk, Numbers, Politeness, Directions, Code, Time, Shopping and Preferences. The content is almost all
**A1 level**.

**Verified** (scripted PIE runs through unreal-mcp, plus screenshots):
- every mission in every city completes;
- both Croissant branches work, and the Élise failure restarts the mission;
- Japanese renders on the engine fallback font;
- characters animate at the correct scale;
- the menu shows 4 playable cities;
- all `LinguaSpy.*` automation tests pass (Grader, GraderLanguages, MissionTransitions, SaveRoundTrip, NpcConversationResolve).

**NOT verified yet (assume there may be bugs)**:
- **Real mouse input**: click an NPC to walk over and talk, click tiles, click choice buttons. Scripted tests bypass input.
- **Audio**: nobody has *listened* to the procedural MetaSounds. Volume and harshness are unknown.
- **The Conversation Editor tab** (`Source/PersonalEditor`): it compiles and loads, but nobody has looked at it.
- **The occlusion fade** in real play, and the pause menu via the P key.
- The user's review of the ES/JA/IT scripts is pending (`docs/City_Scripts_Review.md`).

Check `EDITOR_TODO.md` and ask the user whether they've done these playtests. Their answers may add bug fixes to Phase 4.

## 4. Architecture you'll extend (read CLAUDE.md for the full map)

- **Code**: `Source/Personal/LinguaSpy/{Data,Language,Dialogue,Cover,Interaction,Camera,Audio,Mission,Save,UI,Tests}`, about 7.8k lines,
  plus the editor module `Source/PersonalEditor` (Slate Conversation Editor). Types used by the editor module are `PERSONAL_API`.
- **Data model**:
  - `ULanguagePackAsset` (city, language, flags such as `bUsesWordSpacing`/`bHasReadings`, map, ordered missions, ambience);
  - `UConversationAsset` (node graph: `NpcLine`, `PlayerChallenge`, `PlayerChoice`, `End` with an `Outcome`; `CoverBlownOutcome`);
  - `FWordTileChallenge` (tiles with readings, accepted answers, `SkillTag`, `CoverPenalty`);
  - `UMissionAsset` (objective graph; transitions matched on outcome plus required/blocked flags; objective types TalkTo, ReachLocation, Collect).
- **Runtime**:
  - `UConversationSubsystem` (world) runs conversations, grading, cover, practice drills, and exposes `StartConversationFromAsset` for tools and debugging;
  - `UMissionSubsystem` (game instance) handles city entry, mission and objective state, and flags;
  - `ULanguageProgressSubsystem` records **per-city, per-skill pass/fail counts** (`FSkillStats`, keyed `"<City>.<Skill>"`), but **nothing uses them yet**;
  - `ULinguaSpySaveSubsystem` holds one profile: `ULinguaSpySaveGame` v1 with skill stats, per-city progress and settings (helper on/off, readings on/off).
- **UI**: every widget is **built in C++** (no WBPs). Shared style (colours, rounded brushes, easing) lives in `UI/LinguaSpyStyle.h`.
  `BindWidgetOptional` everywhere, so WBP restyles stay possible. The main-menu cards are generated from all packs via AssetManager (`LanguagePack` primary asset type).
- **Content is generated by scripts**:
  - `Tools/Content/*_content.py` (conversations, missions, packs) on top of `lingua_content.py` helpers;
  - `Tools/Greybox/greybox_kit.py` + `cities.py` (city maps from layout dicts);
  - `Tools/Blender/build_characters.py` and `build_kits.py` (characters and props);
  - `Tools/Content/art_setup.py` (imports, `M_ToonMaster`, `PP_Toon`);
  - `Tools/Content/audio_setup.py` (MetaSounds).

  Re-running a script **overwrites** its assets. **Run order matters for Paris**: `paris_content.py`, then `paris_croissant.py`
  (the second re-writes the pack with both missions).
- **How to add a city** (the proven pipeline):
  1. Write the kit functions in `build_kits.py` (`build_city("X")`) and run them in Blender.
  2. `art_setup.import_kits(("X",))`, then assign `M_ToonMaster`.
  3. Write `x_content.py` (pack + conversations + mission).
  4. Add a layout dict to `cities.py`, then `build_city_map(X)`.
  5. Re-run `x_content.py` so the pack gets its `CityMap`.
  6. Add ambience in `audio_setup.py`.

  The menu picks the city up automatically.
- **How to add missions or conversations**: extend the city's content script. NPC dialogue can be gated per mission (`RequiredMission`),
  per flag, or by "only if current target". Use the `Validate()` UFUNCTIONs to check assets.

## 5. Environment, tools and hard-won gotchas

- **Paths and commands**:
  - UE: `C:/Program Files/Epic Games/UE_5.8/Engine`.
  - Build: `Engine/Build/BatchFiles/Build.bat PersonalEditor Win64 Development -Project=".../Personal.uproject" -WaitMutex`.
  - Tests: run `Automation RunTests LinguaSpy` from Python (`SystemLibrary.execute_console_command`), then grep `Saved/Logs/Personal.log`
    for `Test Completed`.
- **unreal-mcp** (VibeUE plugin, `http://127.0.0.1:8000/mcp`) only works while the editor runs.
  - Python calls have a **30 s timeout**: split big jobs up. A timeout doesn't necessarily mean failure; the work may have finished, so check.
  - Sessions expire after an editor restart; `ToolSearch` reconnects.
  - Useful VibeUE services: `BlueprintService`, `SkeletonService`, `MetaSoundService`, `MaterialService`, `WidgetService.start_pie/stop_pie`.
- **Blender MCP** (`mcp__Blender__*`) needs Blender running with its add-on; the user opens it. `.blend` files live in `Tools/Blender/`.
- **Editor speed**: the editor drops to about 3 FPS when unfocused ("Use Less CPU when in Background" can't be changed from Python; it's in EDITOR_TODO).
  Automation tests wait for 10 FPS and time out after about 10 minutes. Game time doesn't advance *during* a Python call, so check movement in a later call.
- **PIE tricks**:
  - find subsystems with `unreal.ObjectIterator(unreal.<Subsystem>)` (the path contains `UEDPIE`);
  - screenshots with `shot showui` (the first shot after starting PIE often captures the editor viewport, so take two);
  - use `unreal.load_asset` during PIE (`EditorAssetLibrary` fails there);
  - `ConversationSubsystem.continue_` is the Python name for `Continue`.
- **Import pipeline traps** (all in CLAUDE.md):
  - the FBX recipe for mannequin-skeleton meshes: armature object named `Armature`, a real `root` bone, cm units. Otherwise the root gets scale ×100 and characters are huge while walking;
  - UE 5.8 imports FBX through **Interchange**;
  - skeletal materials must be set by rebuilding the `SkeletalMaterial` list;
  - after setting a Blueprint CDO from Python, **recompile the BP**;
  - `unreal.Rotator(roll, pitch, yaw)` takes its arguments in that order;
  - stacked flat slabs at the same height z-fight.
- **Kits face +Y in UE.** In the maps, the street runs along X and north is +Y.

## 6. Phase 4: the stated goal and the design space

The roadmap in CLAUDE.md says: **"Phase 4: more cities/languages, spaced-repetition practice, phrasebook, helper-reliance scoring."**
That list came from early brainstorming. The user hasn't specified details, so **planning Phase 4 means turning these into a
concrete, coherent design** (and possibly re-prioritizing with the user). Here's the thinking so far, to seed your plan.

### 6a. Learning systems (the biggest value gap)
The game records skill stats but never uses them. Today a player practices only when their cover gets blown, and nothing is
remembered between sessions except counts. Candidates:
- **Spaced repetition (SRS).** You need a stable id per challenge: conversation asset + node id + choice index is a natural key.
  Store a schedule per item in the save (e.g. a Leitner box or SM-2 lite: box, due time or session count, last result). This means a **save version
  bump**; `ULinguaSpySaveGame` is v1 with no migration code yet, so add migration.
  - **Where to hook it:** `UConversationSubsystem::SubmitAnswer` already calls `ULanguageProgressSubsystem::RecordResult`.
    Extend it with an item id, or add a new `USRSSubsystem`.
  - **Design question:** real time vs. session-based scheduling. The game is offline and single-player, so session- or mission-count-based
    scheduling is simpler to test.
- **Practice mode / "Safe house".** A place to drill due items outside missions:
  - **Option A:** a menu-level practice screen that reuses `UConversationWidget` and the challenge grading.
  - **Option B:** a small "safe house" map.

  The subsystem currently assumes an active conversation with an NPC. `StartConversationFromAsset` plus the practice-drill
  code path are the obvious base, possibly with a new "practice session" mode that feeds arbitrary challenges.
- **Phrasebook.** A per-city list of learned phrases, meaning the accepted answers of challenges the player has passed, with the helper translation,
  readings, and stats by skill.
  - **Data:** needs "phrases seen/learned" tracking in the save, or it can be derived from the SRS items.
  - **UI:** a new code-built widget reachable from the menu and the pause menu. The intent prompts and helper translations already exist in the assets.
- **Helper reliance scoring.** The helper is currently a free, global toggle (`FLinguaSpySettings::bHelperEnabled`).
  - **Design space:** charges per mission; cover cost for peeking; a bonus for answering with the helper muted; a per-mission "fluency grade".
  - **Hook:** `UConversationWidget::HandleHelperToggle` and `RefreshHelperAndReadings`. A likely design is per-line reveal ("tap to translate") instead of a global toggle.
  - **Decide how it feeds the SRS**, e.g. an answer given with help counts as weaker.
- **Weak-skill surfacing.** `ULanguageProgressSubsystem::GetWeakSkills(CityId)` exists and nothing calls it. Show it on the
  city card, the phrasebook, or in debriefs.

### 6b. More cities / languages
The pipeline in §4 works end to end, so a new city costs roughly: a Blender kit, a layout dict, a content script, and ambience.
Questions for the user:
- **Which languages?** Candidates: German/Berlin, Portuguese/Lisbon, Korean/Seoul, Mandarin/Beijing.
  - Korean and Mandarin are both `bUsesWordSpacing=false`-style (or Korean with spaces) and have readings.
  - Mandarin should show **pinyin with tone marks** in `Reading`.
  - The CJK fallback font rendered Japanese fine; verify Hangul and Hanzi early.
- **How many cities, and how much content each?** The existing non-Paris cities have only one short mission each. Deepening those
  (a second mission each, more skills) may be worth more than breadth.
- **Script review.** Drafts need a native-quality check. The user reviews later ("build first, review later"), so keep
  writing scripts to `docs/` for review.

### 6c. Hardening and cleanup (probably worth a slot in the plan)
- **Fix whatever the user's playtest finds**, since mouse input, audio, the editor tool and the fade were never checked by a human.
- **Template leftovers still in the repo:**
  - `Source/Personal/Variant_Strategy` and `Variant_TwinStick` (they're in `Personal.Build.cs` include paths);
  - `/Game/Variant_*`, `/Game/TopDown`, `/Game/LevelPrototyping` content (`BP_SpyCharacter` still reuses TopDown input assets and the cursor FX).

  Removing them shrinks the build, but it's a user decision; ask.
- **Settings screen**: volume sliders (the settings are in `ULinguaSpyAudioSettings`, but there's no in-game UI), plus the helper and readings toggles.
- **Menu**: there's no "Continue" affordance; progress simply resumes per city. There's also no per-city reset.
- **Save versioning and migration** (needed as soon as the save format changes).
- **Content-script idempotency**: re-running scripts overwrites hand edits. Consider making the Conversation Editor the source of truth for
  finished content, or adding a guard.
- **Audio** is minimal: no per-city music, just an ambience pad. The user hasn't given feedback yet.

### 6d. Questions you will probably need to ask in your first `AskUserQuestion` round
Pick the ones that actually change your plan:
1. **Scope and priorities.** Which of these should Phase 4 cover, and in what order: learning systems (SRS, practice, phrasebook, helper scoring), new cities, deeper content for the existing cities, hardening/cleanup?
2. **Helper-reliance model.** Charges, cover cost, bonus for muting, or grades only?
3. **Practice UX.** A menu screen or an in-world safe house? SRS scheduling by real time or by sessions?
4. **New languages.** Which ones and how many? A second mission for Madrid/Tokyo/Rome?
5. **Playtest results.** Did they playtest (mouse input, audio, Conversation Editor)? Any bugs or feel issues to fold in?
6. **Cleanup.** OK to delete the template variants and TopDown content?

## 7. How to run Phase 4 (process that has worked)

1. **Orient** (read-only):
   - read this file, CLAUDE.md and EDITOR_TODO;
   - `git log --oneline | head -20`;
   - skim the headers in `Source/Personal/LinguaSpy`;
   - check whether the editor and Blender are running (`tasklist`).
2. **Ask** one round of 2–4 multiple-choice questions (recommended option first). A second small round is fine if answers open new forks.
3. **Plan.** Write the plan to your plan file *and* to `docs/Phase4_Plan.md`, following the Phase 3 format:
   - a Context section;
   - stages ordered autonomy-first, with **all C++ batched early** so there are few rebuilds;
   - critical files;
   - risks and fallbacks;
   - verification.

   Then call `ExitPlanMode`.
4. **Execute autonomously**, stage by stage:
   - build yourself only when the editor is closed;
   - after each stage: commit, update `docs/Phase4_Log.md` with the decisions, keep `EDITOR_TODO.md` current, and send a screenshot at visual milestones.
5. **Verify like Phase 3 did**:
   - automation tests for every pure-logic piece (SRS scheduling, save migration, helper scoring math: write them as static, testable functions like `USentenceGrader` and `UMissionAsset::ResolveTransition`);
   - scripted PIE runs through the new flows via unreal-mcp;
   - screenshots with `shot showui`.

   Report honestly what was and wasn't verified.
6. **Finish**:
   - update CLAUDE.md (roadmap, layout, new conventions or gotchas) and the memory files if the user's preferences changed;
   - write the next `HANDOFF.md` if asked.

## 8. Housekeeping done at handoff time
- `Lvl_Paris_Greybox` deleted (user-approved). The content scripts now point only at `Lvl_Paris`.
- Git LFS enabled by the user (`*.uasset`, `*.umap`). Python caches and `.blend1` backups are git-ignored.
- `Saved/Export/` (FBX exports) is not in git. It's regenerable from the Blender scripts and `.blend` files.
