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
