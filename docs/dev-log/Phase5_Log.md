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

## 2026-10-05: Stage 3 + Paris retrofit + scripted PIE
**Voices (Stage 3)**
- Piper 2023.11.14-2 (MIT, Windows build) plus fr_FR-siwis-medium and fr_FR-mls-medium, both CC-BY 4.0. They live in `Saved/VoiceGen/tools`, which is not in git.
  - Rejected: fr_FR-tom (AGPL) and fr_FR-upmc (CC-BY-SA).
  - Credits: `docs/Credits.md`, renamed from `Audio_Credits.md`.
- NPC voices are MLS speakers picked by median pitch: Camille 3, Marcel 27, Luc 36, Sophie 48, Hugo 63, Élise 75.
- siwis is the handler: it voices the briefing phrases, model answers and listening test items.
- `Tools/Content/voice_build.py` runs in three steps: synth → import → assign. It hashes each line, so unchanged lines are skipped.
  - Paris: 139 clips (40 lines × normal/slow, 33 model answers, 16 briefing phrases, 10 test items).
  - The first synth took 72 s inside the editor, over the 30 s MCP limit, but it completed. Each Piper call reloads the model. Next time, run the synth step outside the editor, or batch the lines per voice.
- **Not verified:** how the voices sound. Claude can't listen. This is in EDITOR_TODO.

**Paris retrofit** (`paris_content.py`, `paris_croissant.py`; all NodeIds unchanged)
- Every challenge got hints, some with authored gaps.
- Cover penalties: tutorial 12, codes 20; Croissant 18, codes 25. They were 34–50.
- 8 briefing phrases plus 3 can-dos per mission.
- 3 NPC lines became intel checks: the café price, the kiosk directions, the painter's time.
- 6 French field notes on the pack.
- `paris_assessment.py`: FR_A / FR_B, 15 items each (5 listen, 4 read, 3 gap, 3 assemble). Every French asset passes `Validate()`.
- `lingua_content.get_or_create` skips assets tagged `LinguaSpy.HandEdited`. The Conversation Editor now stamps that tag on every edit, and the `LinguaSpy.OpenConversationEditor` console command opens it.

**Scripted PIE** (fresh profile in pilot mode; the user's save was backed up and restored)
- Briefing: opens on mission start and pauses the game. Study cards, then a 16-item quiz.
- First Contact:
  - new items come as Assemble;
  - a wrong answer gets a prompt, and the second try counts as assisted;
  - a full miss leads to a recast, and the retry drops to Recognize;
  - intel checks pass;
  - grade A, then the handler went Shadowing → OnCall;
  - the debrief (4 items) ran;
  - the "Où est" field note appeared;
  - the Croissant briefing opened only after the debrief.
- Croissant: full run, intel check, grade S, 5-item debrief, city complete.
- Telemetry: 70 events in one JSONL session. The sample is in `Saved/Telemetry/test_runs/`.

**Fixes from the run** (second build; 19/19 tests pass)
- Recycled phrases now take the strongest SRS state of any item with the same phrase (`FindStrongestForPhrase`). Before this fix, Croissant's coffee order restarted at Assemble.
- Debrief passes don't promote SRS boxes. A debrief is minutes after the mission, so it's consolidation, not spaced retrieval.
- Field notes queue one card at a time (four had stacked at once).
- Review/debrief helper text no longer threatens the grade.

## 2026-10-05: Stages 4, 8, 9 (cleanup, analysis, pilot build) + Madrid "El Palacio"
**Stage 8: `Tools/Analysis/pilot_report.py`**
- Uses only the standard library: pandas and matplotlib aren't installed, and pilot laptops shouldn't need them.
- Reports a paired t-test (own incomplete-beta implementation), exact Wilcoxon, Cohen's d_z with an approximate CI, accuracy by scaffold level and skill, reliance by session thirds, intel and briefing accuracy, cover-blown hotspots and SUS. Charts are SVG.
- `--selftest` checks the statistics against known values and writes clearly marked synthetic output under `Saved/PilotReport_SELFTEST`. Real runs refuse logs containing `SYN-*` participants.

**Stage 4: template cleanup**
- The asset-registry walk from `/Game/LinguaSpy` found 7 template dependencies, all used by BP_SpyController: the click-to-move input and the cursor FX chain.
  - Moved them to `/Game/LinguaSpy/Input` and `/Game/LinguaSpy/FX/Cursor`. No redirectors were left.
- Deleted `/Game/TopDown`, `Variant_Strategy`, `Variant_TwinStick`, `LevelPrototyping` and `Cursor`: 353 files, about 9 MB.
  - The template maps' 10 external-actor files wouldn't delete in-editor (their C++ classes were gone), so they were removed on disk.
- Editor start map and content-browser path now point at LinguaSpy.
- **Not verified with real input:** click-to-move after the move. The PIE driver teleports. It's in EDITOR_TODO.

**Stage 9: packaged build**
- `DefaultGame.ini`: project name LinguaSpy, version 0.5.0, Shipping/pak/IoStore. `DirectoriesToAlwaysCook` covers `/Game/LinguaSpy/Data` (test forms are only found through the asset registry) and `/Game/LinguaSpy/Audio/Voice`. `MapsToCook` lists all 5 maps.
- First Shipping build: OK, 704 MB, `Saved/Packaged/Windows`. The first run compiles the engine monolithically and the SM6 global shaders, so it took about an hour.
- Pilot mode also turns on with a `pilot.ini` next to the packaged game, because Shipping keeps `Saved/` under `%LOCALAPPDATA%`.
- `Tools/Packaging/package.sh` packages and adds `Start LinguaSpy (Pilot).bat` (runs `-PilotMode`) and `CREDITS.md`.
- Pilot kit: `docs/pilot_kit/` (facilitator script, laptop setup, consent templates, survey + CSV format, observer sheet). Portfolio `README.md`.

**Accent keys and leniency** (found while writing the observer sheet: English keyboards can't easily type é/à/ç)
- `ULanguagePackAsset::SpecialCharacters` drives on-screen keys under the typing box, in conversations and gap-fill test items.
- A typed answer that's right apart from accents gets the accent hint first. On the last attempt it passes as assisted ("Accepted - but watch the accents").
- Typed test gap words ignore accents, since that would measure the keyboard as much as the French. Test updated; 19/19 pass.

**Madrid "El Palacio"** (`madrid_palacio.py`, A2; `cities.MADRID` gets Rosa, Álvaro, PalaceGate, Folder and gated options for Javier/Carmen/Diego/Álvaro)
- Map rebuilt; it had only ever been script-built.
- Scripted PIE on a temporary profile:
  - TapaSecreta: S, plus debrief;
  - El Palacio briefing → Javier (past) → Rosa (intel sets `KnowsOpening`) → gate;
  - Álvaro, blown deliberately → drill → `Suspicious` → Carmen (Help) → Álvaro retry (`AskedCarmen`) → folder → Diego (future);
  - mission complete with grade B and 3 A2 can-dos earned. All Spanish assets validate.
- The taquilla is a retinted Paris ticket booth for now (Blender prop later).
- **No Spanish voices yet.** Licenses are checked:
  - es_ES-davefx (CC0) and es_ES-sharvard (CC-BY 3.0) are OK;
  - it_IT-paola (CC0) and it_IT-serena (CC-BY 4.0) are OK;
  - Japanese: the only Piper voice, hi_fi_captain, is CC BY-NC-SA, so it's excluded by the plan's rule and Tokyo stays text-only.

**Remaining Phase 5**
- Tokyo "Tower Line" and Rome "Operazione Colosseo" (drafts in `docs/Phase5_Scripts.md`).
- Spanish/Italian voices.
- Mission-1 retrofit for Madrid/Tokyo/Rome.
- Mission replay, Blender props, per-city music, Noto Sans JP.
