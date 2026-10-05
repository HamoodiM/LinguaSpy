# Phase 5 Log

Decisions and results while executing `docs/Phase5_Plan.md`. Newest last.

## 2026-10-04: Stage 0 (design)
- Wrote `docs/Learning_Design.md` (SLA research → mechanics) and `docs/Pilot_Study_Protocol.md`.
- User decisions: TTS approved, pilot language French, typed input on, ship the pilot build before the 3 new missions.
- **No learning-outcome claims** anywhere until a real pilot produces data. The protocol's results table stays TBD.

## 2026-10-05: Stage 1 + 2 (C++, one build)
Built with the editor closed. It compiled first try, and all **19** `LinguaSpy.*` tests pass headless (10 new).

**Data**
- `ConversationTypes.h`:
  - `FChallengeHint`, `GapTokenIndices`, `RecognitionDistractors`, `ModelAudio` on challenges;
  - `LineAudio` / `LineAudioSlow` on nodes;
  - new node type `IntelCheck` (`FIntelCheck`);
  - `EScaffoldLevel`;
  - `FGradeResult.bAccentOnly` / `FirstWrongIndex`.
- `UMissionAsset`: `BriefingChunks` (`FBriefingChunk`), `CanDoStatements`, `DebriefItems`.
- `ULanguagePackAsset`: `GrammarNotes` (`FGrammarNote`, inline rather than a separate asset: simpler to author from Python), `bAllowTypedInput` / `CanType()`.
- Save **v3**: `HandlerModes`, `FieldNoteCounts`, `SeenFieldNotes`, `BriefingsCompleted`, `ParticipantCode`, `AssessmentResults`, plus `VoiceVolume` and `bTelemetryEnabled` in settings.
  - Migration: cities already played → OnCall (Shadowing if "always show translations" was on). Completed missions count as briefed.
  - `bAlwaysShowTranslation` is now unused (kept for migration only).

**Learning core**
- `UScaffoldPolicy` (pure):
  - level from SRS box, minus session errors;
  - Assemble tiles (no distractors);
  - auto gap = longest word (two gaps for 6+ tokens);
  - recognition options (authored, else a distractor tile swapped in, else a word-order swap; never an accepted answer);
  - hint choice (accent-only first, then the first-wrong-token hint, then a general hint, then length or generic).
- `UConversationSubsystem` rewrite:
  - `FChallengePresentation` / `FAnswerFeedback` delegates;
  - feedback ladder: prompt costs 40% of the penalty, recast the rest, plus model audio;
  - `SubmitGapWords` / `SubmitRecognition` / `SubmitIntel`;
  - `RepeatLine(bSlow)`, where the first replay is free and later ones cost 2 cover;
  - handler modes: Shadowing auto-reveals for free; Radio silence allows 2 peeks;
  - field note counting, announced after the conversation;
  - debrief = review session with `bDebrief`;
  - telemetry on every event.
- Fluency tally: a pass after a prompt counts as a peek.
- `UMissionSubsystem`:
  - `RecordMissionItem` and `SelectDebriefItems` (failed, then assisted, then clean);
  - the debrief starts right after `OnMissionCompleted`;
  - `AdaptHandlerMode`: S/A means one step less help, C one step more;
  - `NeedsBriefing` / `MarkBriefingCompleted`, `GetEarnedCanDos`.
- Audio: a `Voice` channel and slider. `PlayVoice` is a UI sound, so it plays while paused, and the music ducks to 50% under it.

**Pilot instrumentation**
- `ULearningTelemetrySubsystem`:
  - JSONL in `Saved/Telemetry/<code|anon>_<session>.jsonl`;
  - off by default; `-PilotMode` or `Saved/pilot.ini` turns it on;
  - no network code;
  - the root fields are `t`, `t_ms`, `session`, `participant`, `event`. `t_ms` is named that way so the event's own `ms` doesn't overwrite it.
- `UAssessmentAsset` / `UAssessmentSubsystem`:
  - forms per language;
  - counterbalancing by a stable hash of the participant code: pre and delayed use the same form, post uses the other;
  - results go to the save and telemetry;
  - placement sets the handler mode (≥ 70% means OnCall).

**UI**
- `UConversationWidget` rewrite: recognition options, typed box (gap / produce), hint line, Repeat/Slower, intel options, peeks-left text, debrief labels, native font on tiles.
- New widgets:
  - `UBriefingWidget`: study cards, then listen/read/situation quiz; a missed item returns once;
  - `UAssessmentWidget`: participant code, Pre/Post/1-week buttons in pilot mode, "Check my level" otherwise;
  - `UFieldNoteWidget`.
- Settings: Voice slider; the handler mode cycles for the current city, or all cities when opened from the menu; learning-log toggle and an "Open log folder" button.
- Main menu: Field tests / Check my level button. In pilot mode it auto-opens when no code is set.
- Pause menu: "Mission phrases (briefing)", a review-only run that records no completion.
- Phrasebook: Field notes tab.
- HUD: the mission-complete banner lists can-dos and the handler change.
- Controller: opens the briefing (game paused) when a mission needs one, deferring it while a conversation or debrief runs; shows field note cards.

**Cleanup**
- Deleted `Source/Personal/Variant_Strategy|Variant_TwinStick` and their include paths. Nothing else referenced them.
- `/Game/Variant_*` Blueprints now have no parent class until Stage 4 deletes them.

**Deferred (honest list)**
- Skill prerequisite graph: no pure code yet. The curriculum doc describes it; the missions are linear anyway. Moved to Phase 6.
- Mission replay: after the pilot build, per the user's scope cut.
- `LinguaSpy.OpenConversationEditor` console command and the hand-edit guard: with the content stage.
- Test name gotcha: a test named `X.Y.Z` hides an existing test `X.Y`. The new tests are named `GraderTolerant` and `SaveMigrationV3` because of this.
