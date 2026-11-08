# Phase 5 Plan: Research-Grounded Learning, Deeper Cities, Pilot-Ready Prototype

## Context
Phases 1–4 built a complete, playtested game loop in 4 cities: conversations, word tiles, cover, missions, Leitner SRS,
Safe house, phrasebook, fluency grades, settings and CC0 audio.

**The core pedagogical problem:** the game drops players straight into *producing full sentences* under penalty, with
no exposure first, one fixed difficulty, no audio, and feedback that just gives the answer.
`docs/Learning_Design.md` reviews the SLA and learning-science research and turns it into mechanics. **Phase 5
implements that design**, deepens the cities, and turns LinguaSpy into a **packaged, measurable prototype** that can be
piloted with real learners (`docs/Pilot_Study_Protocol.md`) and shown in a portfolio.

Phase 5 is much larger than earlier phases. It's split into **four tracks** and twelve stages with a commit per stage.
Any track can stop at a stage boundary and leave a working game.

| Track | Goal | Stages |
|---|---|---|
| **L: Learning** | Input-first loop, scaffold ladder, feedback ladder, audio, field notes, intel checks, handler modes, CEFR can-dos | 0, 1, 2, 3, 6 |
| **C: Content** | A2 second missions for Madrid/Tokyo/Rome; retrofit all 5 existing missions with briefings, audio and hints | 0, 5, 6 |
| **P: Pilot/prototype** | Assessment mode, local telemetry, pilot mode, packaged build, analysis script, portfolio README | 2, 8, 9 |
| **H: Hardening** | Mission replay, native font, template cleanup, per-city music, props, content guard | 1, 4, 5, 7 |

### Carried-over user decisions (Phase 5 v1)
These still hold:
- No new city (Berlin/German later).
- One **medium mission (4–6 conversations)** each for Madrid, Tokyo and Rome, at **A1+ to A2**. New `SkillTag`s:
  `Time`, `Past`, `Future`, `Transport`, `Help`.
- Mission replay without touching story progress.
- 3–5 Blender props, with retinting as the fallback.
- One CC0 music track per city plus Noto Sans JP. Approving the plan approves those downloads.
- Delete the template C++ variants and content.
- Scripts are "build first, review later" in `docs/Phase5_Scripts.md`.

### New decisions to confirm (see "Decisions needed" at the end)
- Offline TTS voice generation at authoring time (Stage 3).
- Pilot language (French recommended).
- Typed input for the L3/L4 scaffold levels (Latin-script languages only this phase).

### Working mode (unchanged)
- Execute autonomously, log decisions in `docs/Phase5_Log.md`, and put anything for the user in `EDITOR_TODO.md`.
- Commit after every stage. Build only while the editor is closed.
- Screenshots use `PrintWindow`. Use the HTTP JSON-RPC client if unreal-mcp didn't connect.

---

## Stage 0: Design and script drafts (no editor)
**0a. Learning design.** `docs/Learning_Design.md` (done) and `docs/Pilot_Study_Protocol.md` (done).

**0b. Curriculum map** (`docs/Curriculum.md`). For each city:
- the ordered missions, with **CEFR can-do statements**, `SkillTag`s and their **prerequisite graph**;
- each mission's **target chunks (4–8)**, chosen by frequency and recycled. The rule: every chunk is met ≥ 3 times
  across briefing, field and debrief, and ≥ 1 time in the next mission;
- the grammar patterns that trigger **field notes**.

**0c. Script drafts** (`docs/Phase5_Scripts.md`, in the `City_Scripts_Review.md` table format). Each challenge now also
lists:
- its **hint categories**, used by feedback-ladder step 1;
- its **gap-fill variant** (which tokens get blanked at L3);
- whether it's an **intel check** and the answer the player must act on.

The three new missions keep the v1 storylines:
- **Madrid, "El Palacio".** Javier (Past) → Rosa at the taquilla (Numbers/Time/Shopping; *intel check*: opening time) →
  Reach palace gate → Álvaro, guard (`Suspicious` → Carmen detour, Help) → Collect *la carpeta* → Diego (Future;
  blowing cover fails the mission).
- **Tokyo, "Tower Line".** Kenji, station (Transport/Numbers; *intel check*: which line) → Tanaka (Time; *intel check*:
  三時半) → Hana's yatai (Ordering with counters, Preferences) → Reach tower viewpoint → Officer Mori at the kōban (Help;
  `Suspicious` → Sato detour) → Yuki (Past; blowing cover fails the mission).
- **Rome, "Operazione Colosseo".** Luca (Past) → Sofia at the tabaccheria (Transport, Numbers; *intel check*: price) →
  Giulia (Time; *intel check*: bus departure) → Reach fermata → Paolo, guide (Help; `Suspicious` → Luca detour) →
  Marco (Future; blowing cover fails the mission).

**0d. Retrofit drafts** for the 5 existing missions: First Contact, Opération Croissant and the Madrid/Tokyo/Rome
mission 1. Add a briefing chunk list, debrief items, hints and 1 intel check each. **Don't touch existing `NodeId`s** (SRS
keys). Add nodes only.

**Content rules:**
- Accents and readings follow CLAUDE.md, and elided words stay one tile.
- Japanese tiles are `|` chunks with romaji in `Reading`.
- 2–4 distractors per challenge, and ≤ 8 tiles per answer.
- Cover penalties are tiered: tutorial 10, first missions 15–20, A2 missions 20–30.
- Uncertain lines are flagged for native review.

---

## Stage 1: C++ batch A (learning core + hardening)
Editor closed, `Build.bat PersonalEditor Win64 Development`. This build also compiles the pending Phase 4 plural fix.

### 1a. Schema additions (`Data/ConversationTypes.h`)
All fields are optional, with defaults that reproduce today's behaviour, so old assets keep working.
- `FWordTileChallenge`:
  - `TArray<FChallengeHint> Hints`: `{ int32 TokenIndex (or -1), FText HintText, FName HintCategory }`;
  - `TArray<int32> GapTokenIndices` (L3);
  - `TArray<FText> RecognitionDistractors` (L0: wrong full-sentence options in the native language);
  - `TSoftObjectPtr<USoundBase> ModelAudio`.
- `FConversationNode`:
  - `TSoftObjectPtr<USoundBase> LineAudio`;
  - new node type **`IntelCheck`**: `FText Question`, `TArray<FText> Options`, `int32 CorrectIndex`, `FName IntelFlag`
    (set on success, so missions can require it), `float SuspicionCost`.
- New **`UMissionBriefingAsset`** (or an inline struct on `UMissionAsset`):
  - `TArray<FBriefingChunk>`: `{ FString Text, FString Reading, FText Meaning, TSoftObjectPtr<USoundBase> Audio,
    TSoftObjectPtr<UTexture2D> Icon, FName SkillTag }`;
  - `TArray<FText> CanDoStatements`;
  - `TArray<FSrsItemKey-ish> DebriefItems`.
- New **`UGrammarNoteAsset`**: `PatternId`, `Title`, `Body`, `TArray<FName> ExampleNodeRefs`, `int32 TriggerAfterEncounters`.
- **`ULanguagePackAsset`**: `TArray<FSkillPrereq>` (Skill → required skills); `TArray<UGrammarNoteAsset*>`.

### 1b. Scaffold ladder (`Language/ScaffoldPolicy.{h,cpp}`, pure static)
- `EScaffoldLevel { Recognize, Assemble, AssemblePlus, GapFill, Produce }`.
- `Choose(const FSrsItemState*, int32 RecentErrors, bool bHasGapData, bool bTypedInputAllowed)`:
  - an unseen item or box 0 → Assemble (Recognize if it errored in the briefing);
  - box 1–2 → AssemblePlus;
  - box 3–4 → GapFill if data exists;
  - box 5 → Produce;
  - ≥ 2 recent errors step down one level;
  - fall back to AssemblePlus when a level's data is missing.
- `BuildTileSet(Challenge, Level)` → the tiles to show: none, or no distractors, or all.
- Grading for GapFill/Produce reuses `USentenceGrader` per token, with tolerant normalization: case, surrounding spaces,
  a punctuation-insensitive option. **Accents are still strict**, but a missing accent is reported as a *hint* ("check
  the accent on *café*"), not a plain fail.

### 1c. Feedback ladder (`UConversationSubsystem`)
- **Attempt 1 wrong:** broadcast `OnFeedbackPrompt(FirstWrongIndex, HintText)` and charge `CoverPenalty × 0.4`.
- **Attempt 2 right:** counts as assisted in SRS.
- **Attempt 2 wrong:** recast. Show the model, play `ModelAudio`, require a rebuild (L1, no penalty), charge the
  remaining penalty, and record a fail.
- The tally feeds `UFluencyScoring` (prompts count half a mistake).

### 1d. Mission flow: Briefing → Field → Debrief (`UMissionSubsystem` + UI)
- `StartMission` opens a **`UBriefingWidget`** (overlay) when the mission has briefing data and it isn't a replay with
  "skip briefing" chosen. Its steps are listen&match, read&match, then situation→chunk.
- Results are recorded to SRS as box-0 "seen" plus a briefing accuracy number (telemetry).
- On mission complete, a **`UDebriefWidget`** runs 3–5 retrieval items through the scaffold ladder, then shows the
  grade and the **can-do statements earned**.
- Neither the briefing nor the debrief costs cover.

### 1e. Handler modes
`EHandlerMode { Shadowing, OnCall, RadioSilence }`, stored per city in the save.
- **Shadowing** auto-reveals translations (not counted as peeks).
- **On call** is today's behaviour.
- **Radio silence** allows N peeks per conversation.
- After a mission, the game suggests a mode from the fluency grade. The player changes it in Settings or the P menu.

### 1f. Audio playback, repeat and slow
- `UConversationSubsystem` plays `LineAudio` on node enter, through the audio subsystem's new `Voice` channel (plus a
  slider).
- Conversation UI buttons: **Repeat** (free once, then +2 suspicion) and **Slow** (0.75× pitch-preserving if the
  platform supports it; otherwise use a pre-generated slow file `LineAudioSlow`).

### 1g. Field notes
`ULanguageProgressSubsystem` counts pattern encounters (from `ExampleNodeRefs`). On reaching the threshold, the HUD pops
a field note card. Seen notes are stored in the save and listed in a new phrasebook **Field notes** tab.

### 1h. Hardening (from v1)
- **Mission replay:** `RequestReplay`, a transient `ReplayProgress`, a pure `FMissionReplay::ApplyCompletion`,
  `UMissionListWidget` with Play/Replay, a **Missions** button on the city card, and a HUD "REPLAY" tag. This is
  unchanged from v1.
- **Native font:** applied to tiles, readings, phrasebook, briefing and debrief.
- **Editor module:** `LinguaSpy.OpenConversationEditor` console command, plus the hand-edit guard metadata
  (`LinguaSpy.HandEdited=1`). The Conversation Editor gets the new fields: hints, gap indices, intel checks, audio refs.
- **Template code removal:** delete `Variant_Strategy` and `Variant_TwinStick`, and their include paths in
  `Personal.Build.cs`.

### 1i. Save v3
Bump `CurrentVersion` to 3 and add a `Migrate` step with these new fields:
- `HandlerMode` per city;
- `SeenFieldNotes`;
- `BriefingsCompleted`;
- `EarnedCanDos`;
- `ParticipantCode` (pilot);
- `AssessmentResults`: `{ FormId, Phase(pre/post/delayed), Score, Max, UtcTicks, per-item results }`.

Migration defaults are: `OnCall`, empty sets, briefings marked done for already-completed missions.

### 1j. Tests (new)
- `LinguaSpy.Scaffold.Choose` and `.BuildTileSet`;
- `LinguaSpy.Feedback.Ladder` (penalty split, assisted flag);
- `LinguaSpy.Grader.Tolerant` (case/space/punctuation; accent → hint);
- `LinguaSpy.MissionReplay`;
- `LinguaSpy.SaveMigration.V3`;
- `LinguaSpy.Prereq.Graph` (cycle detection, unlock);
- `LinguaSpy.IntelCheck` (flag set and suspicion);
- all existing tests stay green.

---

## Stage 2: C++ batch B (pilot instrumentation)
This can share the Stage 1 build if it's ready in time.

### 2a. Telemetry (`Telemetry/LearningTelemetrySubsystem`, GameInstance)
- **Format:** append-only **JSONL** at `Saved/Telemetry/<ParticipantCode or "anon">_<session>.jsonl`.
- **Events:** `session_start/end`, `mission_start/end`, `briefing_item`, `challenge` (item key, skill, scaffold level,
  attempt #, hints, peeks, ms to answer, pass), `intel_check`, `repeat/slow`, `cover_blown`, `drill_item`,
  `review_item`, `field_note_shown`, `assessment_item`.
- **Rules:** **off by default**, on in Pilot mode or by setting. **No network code.** The Settings screen shows
  "Telemetry: on/off, file location", with an **Export** button that copies the logs to a user-chosen folder (or opens
  the folder).
- **Test:** `LinguaSpy.Telemetry.Format` writes and parses events.

### 2b. Assessment mode (`Assessment/`)
- `UAssessmentAsset`: `FormId`, `Language`, items. Item types: ListenChoose, MeaningChoose, GapFill, Assemble.
- `UAssessmentSubsystem` runs a form with **no feedback** and no SRS writes, then stores the result in the save and
  telemetry.
- Menu: **Assessment** entry (shown in Pilot mode or from Settings → "Check my level").
- Test: `LinguaSpy.Assessment.Scoring`.

### 2c. Pilot mode
- Turned on with a command-line flag `-PilotMode` or a `Saved/pilot.ini`.
- First launch asks for the participant code, turns on telemetry, and unlocks the Assessment menu.
- A **study flow** button ("Pre-test → Play → Post-test") routes the session in order.
- **Don't record names.** The code field rejects spaces and anything longer than 8 characters.

---

## Stage 3: Voice audio pipeline (authoring-time TTS, offline)
- **Tool:** **Piper** (MIT-licensed, local, CPU) with one voice per language (fr_FR, es_ES, it_IT, ja_JP if one exists;
  otherwise an alternative such as VOICEVOX for Japanese, which has its own per-voice terms).
  - **Each voice model's license gets checked and recorded before use** in `docs/Credits.md`. Voices with
    non-commercial or unclear terms are skipped.
  - Downloads need **user approval**: binary + voices, roughly 60–120 MB each.
- **Script:** `Tools/Audio/generate_voice.py` walks the conversation/briefing/assessment data (exported JSON from the
  content scripts) and synthesizes per line:
  - a normal and a 0.8× file into `Saved/VoiceGen/<lang>/<asset>/<node>.wav`;
  - **a different voice per NPC** where possible (speaker id / pitch shift), so characters sound distinct.
- **Import:** `Tools/Content/voice_import.py` imports the files to `/Game/LinguaSpy/Audio/Voice/<Lang>/...` and fills
  in `LineAudio`/`ModelAudio`/`Audio` refs. It's idempotent and respects the hand-edit guard.
- **QA:** `analyze_audio.py` checks loudness. Japanese pitch accent and liaison in French aren't perfect in TTS: log it
  and put a native-speaker spot check into EDITOR_TODO. **Fallback:** if no acceptable voice exists for a language, that
  language ships text-only and the UI hides Repeat/Slow.

---

## Stage 4: Template content cleanup (editor)
Unchanged from v1:
1. Find what BP_Spy* depends on.
2. Move `/Game/TopDown/Input` and `/Game/Cursor` FX into `/Game/LinguaSpy`.
3. Delete `Variant_*`, `TopDown`, `LevelPrototyping` and `Cursor` only where nothing references them, `git rm` any
   leftovers, and keep the Mannequins.
4. Point `SimpleMapName` at `Lvl_MainMenu`.
5. Verify the maps, BPs and click-to-move.
6. Log what was deleted and its size.

## Stage 5: New props (Blender MCP)
Unchanged from v1: `SM_Madrid_Taquilla`, `SM_Tokyo_Yatai`, `SM_Tokyo_Koban`, `SM_Rome_Fermata`, and optionally
`SM_Rome_Tabaccheria`. The fallback is retinting existing props. **Plus** 24–40 flat **briefing icons** (coffee,
ticket, clock, bus, …) as simple 256 px PNGs in the toon palette. They're generated procedurally (Blender orthographic
renders of kit props, or an SVG→PNG script), so there are no external assets.

## Stage 6: Content build (editor)
1. **New mission scripts:** `madrid_palacio.py`, `tokyo_tower.py`, `rome_colosseo.py` (as in v1), now including briefing
   chunks, hints, gap indices, intel checks, can-dos and debrief items.
2. **Retrofit scripts** for the 5 existing missions add briefing/debrief/hints/intel nodes **without changing existing
   NodeIds**. A guard asserts that every pre-existing SRS key still resolves.
3. **Grammar notes** per language: 3–5 each (e.g. FR *je voudrais*, passé composé with *avoir*, *vous* politeness;
   JA を/が particles, ました past, counters; ES/IT preterite/passato prossimo, *ir a* / future).
4. **Assessment forms A/B** for French (pilot), plus A/B for es/it/ja if time allows. Item difficulty is spread evenly,
   and **no item appears verbatim in gameplay** (they test transfer, not memorization of the exact line).
5. **Layouts** (`cities.py`): new NPCs, markers, pickups and props. Rebuild Madrid/Tokyo/Rome after the git hand-edit
   check (from v1).
6. **Validation:** `Validate()` on all assets. The new **content validator** (`Tools/Content/validate_curriculum.py`)
   checks:
   - chunk encounter counts (≥ 3);
   - every challenge has ≥ 1 hint;
   - prerequisite graph is acyclic;
   - audio refs exist;
   - tile count ≤ 8;
   - no assessment item duplicates a gameplay answer.
7. Run the voice pipeline (Stage 3) on the new and retrofitted content. Re-run the content guard check.

## Stage 7: Per-city music, ambience and Japanese font
Unchanged from v1:
- CC0 only, ≤ 20 MB per file / ≤ 80 MB total, and the analyzer gates.
- Paris musette/jazz, Madrid guitar, Tokyo koto/lo-fi, Rome mandolin; the menu keeps *Busy but Calm Street*.
- Fountain loops for the Madrid/Rome plazas.
- Noto Sans JP (OFL) as a composite font, with a manual fallback via EDITOR_TODO.
- `docs/Audio_Credits.md` → `docs/Credits.md`, now also covering TTS voices.
- **Music ducks** under voice lines (−6 dB on the Voice channel).

## Stage 8: Analysis tooling
- `Tools/Analysis/pilot_report.py` (Python, pandas + matplotlib, offline) takes telemetry JSONL + `survey.csv` and
  produces `pilot_report.md` and charts:
  - pre/post/delayed scores, paired test, Cohen's d (paired) with CI;
  - accuracy by scaffold level and skill;
  - hints/peeks over time (helper reliance curve);
  - cover-blown hotspots per node;
  - briefing → field transfer;
  - SUS scoring.
- **Synthetic-data self-test:** `pilot_report.py --selftest` generates a *clearly labelled fake* dataset to check the
  pipeline. Its outputs carry a "SYNTHETIC — NOT RESULTS" watermark, and the script refuses to write `pilot_report.md`
  in selftest mode.

## Stage 9: Packaged prototype and portfolio
- **Package** a Windows Development (or Shipping) build:
  - `RunUAT BuildCookRun -project=... -platform=Win64 -clientconfig=Shipping -cook -stage -pak -archive`;
  - fix cook errors (template deletion helps);
  - check build size and first-launch time.
- **First-run experience:**
  - title → short story intro → language pick → **optional placement** (Assessment form, which picks the starting
    handler mode and skips the tutorial briefing if the player scores high) → Paris tutorial;
  - controls overlay;
  - all UI text in the language-neutral helper language (English).
- **Pilot kit** (`docs/pilot_kit/`):
  - facilitator script;
  - consent/assent templates (plain language, *not* legal advice; the organizer adapts them);
  - SUS + engagement survey (printable and as a Google Form spec);
  - observer sheet;
  - laptop setup checklist (`-PilotMode` shortcut, telemetry folder).
- **Portfolio:**
  - repo `README.md` with a pitch, a GIF (captured with `PrintWindow` frames → ffmpeg if installed), architecture
    diagram, learning design summary with citations, tech highlights and how to run;
  - "Results" says *pilot planned* until real data exists.
  - An itch.io page is optional. Publishing is the **user's** action.

## Stage 10: Verification and docs
- **Tests:** all `LinguaSpy.*` pass headless (≈ 20 suites).
- **Scripted PIE** (`pie_driver.py`, extended):
  - every new mission: direct path, `Suspicious` detour, contact fail → restart;
  - briefing → field → debrief on a retrofitted mission;
  - feedback ladder (wrong → prompt → right = assisted);
  - scaffold level rising across repeated reviews (using `SetDebugTimeOffsetHours`);
  - intel check gating a marker;
  - handler modes;
  - field note trigger;
  - replay isolation (v1 checks);
  - assessment run → save + telemetry rows;
  - telemetry off by default and no file written;
  - packaged build boots to the menu.
- **Screenshots:** briefing, each scaffold level, feedback prompt, intel check, field note, debrief with can-dos,
  Missions overlay with replay, assessment screen, the new props/NPCs per city, Japanese in Noto Sans JP, and the
  Conversation Editor.
- **Docs:**
  - CLAUDE.md: the new systems in the code layout, plus the content rules (hints, encounter counts, NodeId stability);
  - `docs/Phase5_Log.md`;
  - EDITOR_TODO: listen to voices/music, native-speaker review of scripts and TTS, **run the pilot**.
- **Honesty:** list what wasn't verified: audio by ear, native quality, real learners. **No learning-outcome claims
  until the pilot runs.**

---

## Future phases (planned now, not built)
### Phase 6: Classroom and adaptivity
- **FSRS scheduler** behind `USrsScheduler` (parameters fit from pilot telemetry); per-item difficulty.
- **Berlin/German** city (Phase 5 v1 decision) with 2 missions, built on the full Phase 5 loop.
- **Teacher mode:** class code, assign missions, export a class CSV, a local "class report" from multiple telemetry
  files. Still no server.
- **Typed input for Japanese** (romaji → kana converter in the grader) and full L4 production for every language.
- **Helper language localization** (UI and translations in French/Arabic/Spanish for non-English speakers), using UE's
  localization dashboard.
- **Accessibility:** dyslexia-friendly font option, colorblind-safe tile states, subtitles size, remappable input,
  controller support.
- **Second pilot** with a control comparison (e.g. half the group uses flashcards for the same chunks for the same time)
  to test *relative* effectiveness.

### Phase 7: Speaking and breadth
- **Offline speech input** (whisper.cpp, small model, local) for "say it aloud" challenges, graded on recognized text
  only (no pronunciation scoring claims).
- **Shadowing exercises** (listen → repeat → compare waveform/pitch contour visually).
- **New languages:** Korean (Hangul + romanization), Mandarin (hanzi + pinyin, tone marks), Portuguese, Arabic (RTL
  text; needs UI work).
- **Narrative:** a cross-city arc that connects the missions; NPC relationship memory (NPCs remember what you told
  them).
- **Content tooling:** spreadsheet → DataAsset importer so non-programmers (teachers, native speakers) can author
  missions; a validation report in the Conversation Editor.
- **Release:** Steam/itch.io build pipeline, crash reporting (opt-in), and a store page.

---

## Critical files (Phase 5)
- **New C++:**
  - `Language/ScaffoldPolicy.*`;
  - `Telemetry/LearningTelemetrySubsystem.*`;
  - `Assessment/{AssessmentAsset,AssessmentSubsystem}.*`;
  - `Data/{MissionBriefingAsset,GrammarNoteAsset}.*`;
  - `Mission/MissionReplay.h`;
  - `UI/{BriefingWidget,DebriefWidget,IntelCheckWidget,FieldNoteWidget,MissionListWidget,AssessmentWidget}.*`;
  - tests.
- **Changed C++:**
  - `ConversationTypes.h`, `ConversationSubsystem`, `MissionSubsystem`, `LanguageProgressSubsystem`;
  - `LinguaSpySaveGame` (v3), `LinguaSpyAudio` (Voice channel);
  - `ConversationWidget`, `WordTileWidget`, `SpyHUDWidget`, `SettingsWidget`, `PhrasebookWidget`, `CityCardWidget`,
    `MainMenuWidget`;
  - `Personal.Build.cs`, `PersonalEditor/*`.
- **Scripts:**
  - `Tools/Content/{madrid_palacio,tokyo_tower,rome_colosseo,retrofit_*,voice_import,validate_curriculum}.py`;
  - `Tools/Audio/generate_voice.py`;
  - `Tools/Analysis/pilot_report.py`;
  - `lingua_content.py`, `cities.py`, `build_kits.py`, `pie_driver.py`, `audio_import_cc0.py`.
- **Docs:**
  - `Learning_Design.md`, `Pilot_Study_Protocol.md`, `Curriculum.md`, `Phase5_Scripts.md`, `Phase5_Log.md`;
  - `Credits.md`, `pilot_kit/*`, `README.md`, `CLAUDE.md`, `EDITOR_TODO.md`.

## Risks and fallbacks
- **Scope.** This is ~3× Phase 4. The priority order if time runs short: **L (1b–1d, 1f) → P (2a–2c, 8, 9) → C
  retrofit of Paris → new missions → H.** Paris alone, fully retrofitted, is enough for the pilot.
- **TTS quality or licensing.** Text-only fallback per language. Never ship a voice with unclear terms.
- **Changing shipped content breaks SRS keys.** Add-only retrofit plus a key-resolution guard.
- **Typed input and IME.** Typing is limited to Latin-script languages this phase. Japanese GapFill uses a 4-option pick.
- **Telemetry and privacy.** Off by default, local only, codes not names, and no network code at all. Easy to audit.
- **Pilot logistics** (permissions, minors' consent) are outside Claude's control: they're in the user's EDITOR_TODO
  with the pilot kit ready.
- **Plus the v1 risks:** map rebuilds vs. hand edits, replay leakage, template deletion, CC0 music gates,
  composite-font creation, editor crashes.

## Decisions (answered 2026-10-04)
1. **TTS:** approved. Download Piper + one voice per language; licenses are checked and logged before use.
2. **Pilot language:** French (Paris).
3. **Typed input** at L3/L4 for French/Spanish/Italian: yes.
4. **Scope cut line:** yes. Order: Track L + P + Paris retrofit → pilot build → new missions → rest of H.
