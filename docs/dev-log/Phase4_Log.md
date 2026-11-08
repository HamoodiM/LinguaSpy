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

## Stage 3: verification (2026-09-29)
**Bug found and fixed: `Version` was never serialized.** SaveGame serialization skips properties that equal the class default, so the saved `Version` always loaded as "current" and migration never ran.
- `Version` now defaults to 0 and is stamped when a profile is created or saved.
- There's a new test for it.
- Your real v1 profile now logs `migrated v0 -> v2`.

**Editor crash.** The first `shot showui` crashed the editor inside D3D12RHI/SlateRHIRenderer. None of our modules were in the callstack. After that, screenshots were taken with a Win32 `PrintWindow` capture of the editor window only.

**Scripted PIE** (`Tools/Test/pie_driver.py`, test profile; your profile was backed up first and restored afterwards):
- **Menu:** the v1→v2 migration works and the music loop plays at 0.24 (mix 0.6 × music slider 0.5 × master 0.8).
- **Paris, First Contact** (Camille with 1 peek and 1 mistake, Marcel clean, then the package):
  - 8 answers, 1 mistake and 1 peek gave **grade A** (reliance 0.17);
  - the next mission started with a fresh tally;
  - 7 SRS items were learned. The coffee phrase went fail, then an assisted pass, and stayed in box 1 with `AssistedPasses` 1.
- **SRS time:** 0 items due right away and 7 due at +1 h (debug offset).
- **Review session** (7 items, 1 wrong):
  - `OnReviewFinished` reported (6, 7);
  - the wrong item went back to box 1, due in 10 minutes;
  - the retry wasn't counted;
  - the others moved from box 1 to box 2;
  - the mission tally was untouched.
- **Persistence:** after a PIE restart the profile loaded with 7 SRS items, and the best grade was saved (A).
- **UI through real button delegates:**
  - the Safe house opened with 7 due and skill bars;
  - the review drill showed "Safe house drill / REVIEW - 7 left";
  - the summary read "7 of 7 right on the first try";
  - closing it refreshed the cards;
  - the phrasebook showed 7 phrases with filters and strength bars;
  - Settings showed its sliders, and a live music slider change took the channel from 0.24 to 0.12 on the playing component.
- **Tokyo, full mission:**
  - romaji and the Translate hint appear before a peek;
  - Translate reveals the helper line and counts 1 peek;
  - the mission debrief banner read "GRADE A", with "City cleared" queued behind it.

**Fixed after verification:** "1 mistakes"-style plurals in the debrief and pause tally. It's .cpp only and still needs compiling (listed in EDITOR_TODO).

**Not verified**
- **Audio by ear.**
- **Real mouse clicks on the new buttons.** I broadcast `OnClicked` instead.
- **The pause-menu Phrasebook/Settings path.** P is bound through input and wasn't scripted, but it uses the same widgets.
- **Extra practice when nothing is due.** It's covered by the unit test of the filler selection, but I didn't play it.
