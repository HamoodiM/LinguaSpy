# Phase 4 Log: Learning Systems + Audio Replacement

The plan is in `docs/Phase4_Plan.md`. This log records the calls I made while executing it, and what was and wasn't verified.

## Stage 1: C++ batch (2026-09-29)
**Result:** the build succeeded on the first try. All 10 `LinguaSpy.*` tests pass headless (`UnrealEditor-Cmd … Automation RunTests LinguaSpy`). 5 of them are new: `Srs.Schedule`, `Srs.ItemKey`, `SaveMigration`, `FluencyGrade` and `SaveRoundTripSrs`.

### What landed
**SRS core (`Language/SrsScheduler`)**
- An item's key is `"<conversation path>#<NodeId>#<ChoiceIndex>"`.
- It uses Leitner boxes 0–5. The intervals are 10 min (boxes 0 and 1), 1 day, 3 days, 7 days and 21 days.
- An unaided pass moves the item up one box. An assisted pass keeps the box but reschedules it. A fail puts it back in box 1.

**`USpacedRepetitionSubsystem`**
- It records every graded answer. It also builds review queues: due items first, then the weakest items as "extra practice".
- It provides learned items for the phrasebook and per-skill strength.
- `SetDebugTimeOffsetHours` fast-forwards time for automation.

**Fluency grade (`Language/FluencyScoring`)**
- Reliance = (mistakes + 0.35 × peeks) / challenges.
- Grades: S = clean; A ≤ 0.25; B ≤ 0.6; otherwise C.

**Save v2**
- Adds `SrsItems`, `BestGrades`, four volume settings and `bAlwaysShowTranslation`.
- `bHelperEnabled` is removed: tap-to-translate replaces it.
- `ULinguaSpySaveGame::Migrate` runs after load and re-saves.

**Conversation subsystem**
- `RevealTranslation` and `IsTranslationRevealed` handle per-line peeks.
- Tallies (challenges, mistakes, peeks) go to `UMissionSubsystem::AddFluencyTally` when an NPC conversation ends, including walk-aways and side chats.
- The new `Review` mode is started by `StartReviewSession` / `StartReviewForCity`. Review and cover-blown practice share one code path.
- `FConversationPracticeItem` now carries its conversation.

**Mission subsystem**
- It keeps the tally per mission run and grades the mission on completion.
- It stores the best grade per `MissionId`.

**UI**
- `UOverlayPanelWidget` is a shared frame for the new `USettingsWidget`, `UPhrasebookWidget` and `UPracticeScreenWidget` (Safe house).
- The city card gains:
  - a due badge;
  - a "Work on: <weakest skill>" line (the first real caller of `GetWeakSkills`);
  - best grades;
  - Safe house and Phrasebook buttons.
- The main menu gets a Settings button.
- The pause menu's Helper/Romaji toggles become a Phrasebook button, a Settings button and a live tally line ("on track for A").
- The conversation panel's Helper toggle becomes a per-line **Translate** button.
- The HUD's banners now queue. Before, the next mission's banner overwrote "Mission complete" instantly. Mission complete now shows the grade and the tallies.

**Audio**
- `ULinguaSpyAudioSubsystem`: loops fade in (1.5 s).
- Each volume = project mix level × player slider × master.
- Slider changes re-apply to the live loops. `PlayLoop` now takes a channel (Music/Ambience).

### Decisions
- **A peek is counted once per node per conversation run.** Answers on a revealed node, or while "always show translations" is on, count as *assisted*.
- **Answers during a cover-blown drill also count as assisted**, because the player has just seen the model answer.
- **In a review session, only the first attempt at each item counts** toward SRS and skill stats. Retries later in the same session would otherwise inflate the box.
- **The fluency tally isn't saved.** Quitting mid-mission resets it, which is acceptable for short missions.
- **Mission debrief:** I used the existing HUD banner (now queued) rather than a separate panel. It's less UI, and the banner already pulses.
- **Items appear in the phrasebook** only after at least one pass (assisted passes included). Failed-only items still show up in Safe house reviews.
- **The Safe house is only reachable from the city card** (menu). In a city, the pause menu offers the phrasebook and settings.

## Incident: assets were Git LFS pointers (2026-09-29)
- **Symptom:** after the rebuild, the editor launched "very broken". Every `.uasset`/`.umap` failed with `Package is unloadable … Invalid value for PACKAGE_FILE_TAG`.
- **Cause:** the working copy held 131-byte LFS pointer files. This was left over from enabling Git LFS (the history rewrite at reflog `HEAD@{2}`). All 660 LFS objects were present in `.git/lfs/objects`.
- **Fix:** I closed the editor and ran `git lfs checkout`, which restored 660 of 660 files, then relaunched. No packages were unloadable afterwards.
- I added the gotcha to CLAUDE.md.

## Stage 2: CC0 audio (2026-09-29)
- **Downloaded:** only the files listed in the plan, all under 5 MB, into `Saved/AudioSources/` (git-ignored). The sources are listed in `docs/Audio_Credits.md`.
- **Skipped:** the Gregor Quendel city recordings, because they're CC-BY-SA, not CC0.
- **Unreal-mcp:** it failed to connect at session start, so I talked to its HTTP endpoint directly with a small JSON-RPC client in my scratchpad.
- **Analysis:** UE imports OGG directly through its bundled libsndfile. I exported every candidate to WAV with `SoundExporterWAV`, then measured them with `Tools/Audio/analyze_audio.py` (peak, RMS, spectral centroid, energy above 4 kHz, flatness, pitch contour).

**Picks**
| Role | File | Why |
|---|---|---|
| Click | `click2` | centroid 368 Hz |
| Tile | `drop_003` | soft |
| Pass | `confirmation_001` | rising |
| Fail | `error_008` | low, 0.14 s |
| Menu select | `maximize_008` | soft, rising |
| Cover blown | Pizzicato jingle 14 | falling |
| Mission complete | Pizzicato jingle 02 | rising |
| Ambience | `highway` loop | broadband noise bed, not a tone |
| Music (menu and cities) | *Busy but Calm Street* | |

**Rejected**
- *Ambient Relaxing Loop*: it sustains a single 787 Hz tone (flatness 0.00), which is the same "ringing" problem as the old MetaSounds.
- `ambient_03`: a drone.
- Anything with a centroid above about 3 kHz (`close_*`, `open_*`, `scratch_*`, `glass_004`).

**Mix levels (project settings)**
- UI 0.5, Ambience 1.0, Music 0.6.
- The player defaults are master 0.8, music 0.5, ambience 0.5 and UI 0.7.
- The resulting in-game levels are roughly: UI peaks at −12 dBFS, music at −30 dB RMS, street bed at −37 dB RMS.

**Cleanup and open items**
- The old `MS_*` MetaSounds are removed. `delete_asset` removed them in memory but left the files on disk, so I finished with `git rm`.
- `audio_setup.py` is marked deprecated.
- Every city uses the same music and street bed for now. Per-city music is a follow-up, noted in EDITOR_TODO.
- **Not verified by ear.** I can't listen. The user needs to confirm.
