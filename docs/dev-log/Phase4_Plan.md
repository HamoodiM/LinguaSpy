# Phase 4 Plan: Learning Systems + Audio Replacement

## Context
Phase 3 finished the playable loop in 4 cities. The user playtested it. Mouse input, UI, the pause menu and the fade all work. **The one problem is the audio: it sounds harsh, like a deafening ring.** The cause is `Tools/Content/audio_setup.py`. The ambience and menu "music" are sustained pure sine/triangle chords with no envelope, played on a loop forever. The UI blips are raw oscillators too.

The biggest value gap is in learning. `ULanguageProgressSubsystem` records per-skill stats, but nothing reads them. Practice only happens when cover is blown, and the helper is a free global toggle.

**User decisions (2 question rounds):**
- **Scope:** learning systems plus an audio overhaul. No new cities and no deeper content in this phase.
- **Audio:** replace the MetaSounds with **public CC0 assets that I download**. I'll log the sources and licenses. Besides UI sounds, there will be **street ambience per city plus a soft music loop**, and a settings screen with volume sliders.
- **Helper:** **tap-to-translate** per NPC line, which is free. Each mission gets a **fluency grade** (S/A/B/C) from peeks and mistakes. An answer given after peeking counts as weaker in spaced repetition (SRS).
- **Practice:** a **menu screen** ("Safe house"), not an in-world map.
- **SRS:** **Leitner boxes on real time**, with injected timestamps in tests.
- **Playtest:** nothing else is broken, so there's no bug-fix slot beyond audio.

**Working mode (from HANDOFF):**
- Execute autonomously and log decisions in `docs/Phase4_Log.md`.
- Put anything that needs the user in `EDITOR_TODO.md`.
- Commit after every stage.
- Build only while the editor is closed. The editor isn't running right now; Blender is.
- The first execution step copies this plan to `docs/Phase4_Plan.md`.

## Stage 1: C++ batch (editor closed, `Build.bat PersonalEditor Win64 Development`)
All of the phase's C++ lands in one drop.

### 1a. SRS core (`Language/`)
- **`FSrsItemKey`:**
  - fields: conversation `FSoftObjectPath`, `NodeId`, `ChoiceIndex` (INDEX_NONE for PlayerChallenge);
  - `ToString()`/`Parse()` give a stable save key, e.g. `/Game/.../CV_X.CV_X#node#1`.
- **`USrsScheduler`**, pure static and testable, like `USentenceGrader`:
  - **Boxes:** 5 boxes with intervals of 10 min, 1 day, 3 days, 7 days and 21 days. Plus box 0, meaning "seen, not learned".
  - **`Update(State, bPassed, bAssisted, Now)`:**
    - an unassisted pass moves the item up one box;
    - an assisted pass keeps the box but reschedules it;
    - a fail resets it to box 1, due in 10 minutes.
  - **Other helpers:** `IsDue(State, Now)`, `SelectReviewItems(Items, Now, Max)` (due first, most overdue first, then lowest box), `Strength(State)` (0–1 for UI dots).
- **`FSrsItemState`** is stored in the save: `Box`, `DueUtcTicks` (int64), `LastReviewUtcTicks`, `Passes`, `Failures`, `AssistedPasses`, `CityId`, `SkillTag`.
- **`USpacedRepetitionSubsystem`** (a GameInstance subsystem):
  - `RecordResult(Key, City, Skill, bPassed, bAssisted)`;
  - `GetDueCount(City)`;
  - `BuildReviewQueue(City, Max)` (falls back to the weakest items when nothing is due);
  - `GetLearnedItems(City)` for the phrasebook.
  - It uses `FDateTime::UtcNow()`, with a test hook to override "now".

### 1b. Save v2 + migration (`Save/`)
- `ULinguaSpySaveGame::CurrentVersion = 2`. New fields:
  - `TMap<FString, FSrsItemState> SrsItems`;
  - `TMap<FName, FMissionGrade> BestGrades` (grade, peeks, mistakes);
  - settings gain `MasterVolume`, `MusicVolume`, `AmbienceVolume`, `UIVolume` and `bAlwaysShowTranslation` (default false). This replaces `bHelperEnabled`, whose old value is ignored because tap-to-translate becomes the default.
- Add a static, testable `ULinguaSpySaveGame::Migrate(Save)`. It runs `v1→v2` stepwise in `ULinguaSpySaveSubsystem` after loading, then saves.

### 1c. Helper reliance + fluency grade
- **`UConversationSubsystem`:**
  - `RevealTranslation()` marks the current node revealed and increments `PeekCount`;
  - it also tracks `MistakeCount` and `ChallengeCount` per conversation;
  - a challenge answered on a revealed node, or while `bAlwaysShowTranslation` is on, counts as `bAssisted`;
  - `SubmitAnswer` then calls both `ULanguageProgressSubsystem::RecordResult` (existing) and `USpacedRepetitionSubsystem::RecordResult`.
- **Fluency scoring:** `UFluencyScoring::ComputeGrade(Challenges, Mistakes, Peeks)`, a pure static.
  - It weights a mistake more heavily than a peek.
  - Thresholds: S means no peeks and no mistakes; C means reliance above about 60%.
- **Mission grade:** `UMissionSubsystem` aggregates the conversation tallies for the active mission. It resets on mission start and fail, computes the grade on completion, stores the best grade in the save, and exposes `OnMissionGraded`.

### 1d. Review session (drills outside missions)
- `UConversationSubsystem::StartReviewSession(TArray<FSrsItemKey>)` adds a new `EConversationMode::Review`.
  - Each item loads its conversation, shows that node's NPC line (for context) plus the challenge.
  - There's no cover or mission, and results feed the SRS.
  - It fires `OnReviewFinished(Passed, Total)`.
- Refactor `EnterPracticeItem` to take `(Conversation, NodeId, ChoiceIndex)` so that cover-blown practice and review share one path.

### 1e. UI (code-built, `LinguaSpyStyle.h`, `BindWidgetOptional`)
- **`UConversationWidget`:**
  - the global helper toggle becomes a per-line **"Translate"** button (it hides the translation again on the next node);
  - add a "Review" mode label and a review progress counter.
- **`UCityCardWidget`:**
  - a due badge ("5 to review") and a weakest-skill line (from `GetWeakSkills`, which has had no callers until now);
  - the buttons become **Play / Safe house / Phrasebook**, plus the best grade per mission.
- **New `UPracticeScreenWidget`** (menu "Safe house"):
  - skill bars per `SkillTag`;
  - "Review N due" or "Extra practice";
  - it hosts a `UConversationWidget` in the menu level for the drill, then shows a summary.
- **New `UPhrasebookWidget`:**
  - per city, lists learned items: the first accepted answer (joined using `bUsesWordSpacing`), the reading, the `IntentPrompt` as the meaning, the skill, and strength dots;
  - a skill filter;
  - it opens from the city card and from the pause menu (current city).
- **New `USettingsWidget`:** Master/Music/Ambience/UI sliders, readings on/off and "always show translation". It opens from the main menu and the pause menu.
- **Mission debrief:** when a mission completes, a debrief panel in `USpyHUDWidget` shows the grade, peeks, mistakes and best grade.

### 1f. Audio plumbing (`Audio/`)
- **`ULinguaSpyAudioSubsystem`** (a GameInstance subsystem):
  - `PlayUI`, `PlayLoop(Channel)` tracks the ambience/music `UAudioComponent`s;
  - `ApplyVolumes()` sets each volume multiplier to the save setting × master, so slider changes apply live.
  - The existing `LinguaSpyAudio::PlayUI/PlayLoop` functions forward to it, so callers don't change.
  - Loops fade in and out (`FadeIn`/`FadeOut`, 1.5 s).
- `ULanguagePackAsset::MusicSound` already exists. `APersonalGameMode::StartPlay` already plays both loops, so only the assets change.

### 1g. Tests (`Tests/`)
- `LinguaSpy.Srs.Schedule`: box moves, assisted pass, fail reset, due and overdue ordering.
- `LinguaSpy.Srs.ItemKey`: string round-trip.
- `LinguaSpy.SaveMigration`: a v1 save becomes v2 with defaults, and stats and missions are preserved.
- `LinguaSpy.FluencyGrade`: grade thresholds.
- Extend `SaveRoundTrip` with the SRS items.

Build, loop on errors until green, then commit.

## Stage 2: Audio replacement (downloads + editor)
- **Downloads:** approving this plan lets me download only these CC0 files into `Saved/AudioSources/`, which is git-ignored. I'll check each size first and skip anything over 50 MB:
  - Kenney **Interface Sounds** and **UI Audio**: `kenney_interface-sounds.zip`, `kenney_ui-audio.zip` from kenney.nl;
  - Kenney **Music Jingles** (for the mission-complete and cover-blown stingers);
  - one or two **CC0 ambience loops** and one or two **CC0 soft music loops** from OpenGameArt, CC0-tagged pages only.

  If none of the OpenGameArt pages fits (it needs a gentle street bed), I'll ship UI sounds plus music only, and add a note to `EDITOR_TODO` asking the user for a street ambience.
- **Pick and measure:** a new script, `Tools/Audio/analyze_audio.py`, uses Python `wave`/numpy (or ffmpeg if present) to measure peak and RMS. I choose soft, low-frequency-friendly UI clicks, avoiding high-pitched beeps. I convert OGG to WAV if the UE importer rejects OGG.
- **Loudness targets:** UI peak at about −12 dBFS, music about −20 dB RMS, ambience about −26 dB RMS. I also set lower default volumes.
- **Import:** a new `Tools/Content/audio_import_cc0.py` imports into `/Game/LinguaSpy/Audio/CC0/`.
  - It sets `bLooping` on the loops.
  - It wires `ULinguaSpyAudioSettings` (it reuses `_write_audio_config` from `audio_setup.py`) and each pack's `AmbientSound`/`MusicSound`.
- **Remove the MetaSounds:** delete the old `MS_*` MetaSounds and mark `audio_setup.py` deprecated.
- **Credits:** `docs/Audio_Credits.md` lists each file, its source URL and its license.

## Stage 3: Wiring, content checks, PIE verification (editor)
- **Asset validation:** extend the `Validate()` checks so every challenge has a unique `NodeId` per conversation (SRS keys depend on it). Run them on every conversation.
- **Scripted PIE (unreal-mcp):**
  - play First Contact while peeking on some lines, and check the grade and that assisted items land in box 0/1;
  - play the rest of the mission and check the debrief;
  - fast-forward "now" through the test hook so items become due;
  - run the Safe house review from the menu, check that the phrasebook lists the passed phrases, and check that settings sliders change the live volume multipliers (log check);
  - restart PIE and check that the save persists v2.
- **Screenshots** (`shot showui`, sent with `SendUserFile`): city card with the due badge, Translate button, debrief grade, Safe house, phrasebook (including Tokyo with readings), settings.

## Stage 4: Tests + docs
- Run `Automation RunTests LinguaSpy`; all tests must pass.
- Update `CLAUDE.md`:
  - roadmap: Phase 4 done, Phase 5 ideas (new cities, deeper missions);
  - layout: new classes;
  - conventions: SRS keys depend on stable `NodeId`s, so don't rename nodes of shipped content; CC0 audio pipeline and credits.
- Finish `docs/Phase4_Log.md`.
- `EDITOR_TODO.md`:
  - **listen to the new audio**;
  - try the Safe house and the phrasebook;
  - the still-open script review.

## Critical files
- **Changed:**
  - `Source/Personal/LinguaSpy/Dialogue/ConversationSubsystem.{h,cpp}`
  - `Save/LinguaSpySaveGame.h`, `Save/LinguaSpySaveSubsystem.cpp`
  - `Language/LanguageProgressSubsystem.*`
  - `Mission/MissionSubsystem.*`
  - `UI/{ConversationWidget,CityCardWidget,MainMenuWidget,PauseMenuWidget,SpyHUDWidget}.*`
  - `Audio/LinguaSpyAudio.*`
  - `PersonalGameMode.cpp`
- **New:**
  - `Language/{SrsScheduler,SpacedRepetitionSubsystem,FluencyScoring}.*`
  - `UI/{PracticeScreenWidget,PhrasebookWidget,SettingsWidget}.*`
  - `Tests/{Srs,SaveMigration,FluencyGrade}Tests.cpp`
  - `Tools/Content/audio_import_cc0.py`, `Tools/Audio/analyze_audio.py`
  - `docs/{Phase4_Plan,Phase4_Log,Audio_Credits}.md`
- **Reuse:**
  - `USentenceGrader` (grading in review);
  - `StartConversationFromAsset` and the practice-drill path;
  - `GetWeakSkills`;
  - `LinguaSpyStyle.h` brushes and easing;
  - `_write_audio_config` in `audio_setup.py`.

## Risks and fallbacks
- **Hosting `UConversationWidget` in the menu level:** it binds to the world's `UConversationSubsystem`, which exists in any world. If the menu game mode or input breaks it, the fallback is a stripped-down drill widget.
- **Downloads fail or the licenses are unclear:** use only files explicitly marked CC0. Otherwise ship Kenney UI sounds plus jingles and leave the ambience and music empty (silence beats a drone), and log it.
- **I can't hear the audio:** I rely on the numeric loudness analysis and conservative volumes, and the user confirms by ear (`EDITOR_TODO`).
- **Stale SRS keys from renamed nodes:** unknown keys are ignored when building queues and pruned on load.

## Verification
- A green build, and all `LinguaSpy.*` tests pass, both new and existing.
- The scripted PIE flows above pass. The save migrates and persists.
- Screenshots of every new screen are sent.
- The audio analysis report shows no files above the peak targets.
- I'll say plainly that the audio feel is unverified until the user listens.
