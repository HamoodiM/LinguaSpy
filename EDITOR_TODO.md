# Editor TODO (things you need to do by hand)

Claude keeps this list up to date. Tick items off (or delete them) when you've done them.

## Open
- [ ] **(visual-polish) Approve the city moods and outlines.** Paris gold morning, Madrid white noon, Tokyo blue dusk,
  Rome golden hour (`cities.py` `look`, re-apply with `greybox_kit.apply_look`). Outline strength: PP_Toon
  `DepthSensitivity` / `Thickness`. Before/after: `C:\GameDevCareer\trailers\linguaspy\polish\before_after\`.
- [ ] **(visual-polish) 10-minute playtest of the new camera** (yaw 90, 25 m, FOV 40): click feel, occlusion fade of the
  south row, NPCs on the south sidewalk during conversations.
- [ ] **Play the Phase 5 learning loop in Paris with real mouse and keyboard.** Use a fresh profile: Reset progress, or start with `-PilotMode` and code `TEST-1`. Check:
  1. the briefing (study cards → quick check);
  2. the hint after a first mistake, and Repeat / Slower;
  3. the intel checks (café price, directions, painter's time);
  4. the debrief after the mission, and the field-note cards;
  5. handler mode changes (Settings);
  6. typing a gap word with the accent keys.
  Say what feels slow, confusing or too easy.
- [ ] **Listen to the voices** (`/Game/LinguaSpy/Audio/Voice/French`). Claude chose the NPC speakers by measured pitch and can't hear them. If one sounds bad, change its speaker in `NPC_VOICES` (`Tools/Content/voice_build.py`) and re-run the build. Ideally a French speaker spot-checks pronunciation.
- [ ] **French content review:** the hints, briefing phrases, field notes (`Tools/Content/paris_content.py`, `paris_croissant.py`) and test items (`paris_assessment.py`). A native or fluent speaker would be ideal.
- [ ] **Test the packaged build on another Windows PC** (`Saved/Packaged/Windows`, start with `Start LinguaSpy (Pilot).bat`). See `docs/pilot_kit/Laptop_Setup.md`.
- [ ] **Pilot logistics (only you can do these):**
  - written permission from the partner organization;
  - parent/guardian consent if students are under 18 (templates in `docs/pilot_kit/Consent_Templates.md`);
  - if this is for Carleton coursework or publication, ask the Research Ethics Board *before* running it;
  - pick the date.
- [ ] **Listen to the new audio.** The procedural MetaSounds are gone, replaced with CC0 samples (see `docs/Credits.md`). Claude checked loudness and brightness numerically but can't hear them. Say if anything still sounds harsh, too loud or too quiet.
  - You can adjust volumes in-game: Settings, from the menu or the P pause menu.
  - The base mix levels are in Project Settings → Game → LinguaSpy Audio.
  - To swap a sound, edit `SOUNDS` in `Tools/Content/audio_import_cc0.py` and re-run it.
- [ ] **(Phase 5, later stages) Decide on per-city music.** All four cities currently share *Busy but Calm Street* and the same street bed. Do you want a different CC0 track per city? If you have favourites (e.g. from OpenGameArt), point Claude at them.
- [ ] **Try the Phase 4 learning loop with real mouse input:**
  1. play a mission, tapping **Translate** on a line or two;
  2. check the **grade** in the Mission Complete banner;
  3. from the menu, open **Safe house** on the city card. Items become due 10 minutes after you learn them;
  4. try **Phrasebook** (card or P menu) and **Settings**.
- [ ] **Review the city scripts:** `docs/City_Scripts_Review.md` (Spanish, Japanese, Italian). Mark any lines you'd change.
- [ ] **Look-and-feel notes:** toon band strength, outline thickness, colours, character proportions. The knobs are the `PP_Toon` material parameters and `Tools/Blender/*.py`.
- [ ] **Editor preference:** Edit → Editor Preferences → General → Performance → uncheck **Use Less CPU when in Background** (it speeds up Claude's automated runs).
- [ ] **Clean up the save backups if you like.** `Saved/SaveGames/` has backups from Phase 4 testing: `*.user_backup_v1.sav` is your original, `*.user_backup_migrated_v2.sav` is the same profile migrated, and `*.phase4_test.sav` is the test profile. Your real profile has been restored as `LinguaSpyProfile.sav`.

## Decisions pending
- [ ] **Japanese font:** the engine fallback renders fine. Optionally swap in Noto Sans JP for a nicer look.

## Done
- Conversation Editor checked visually (2026-11-08): opens, lists all 25 conversations, shows flow + node details, validation reports no problems. IntelCheck nodes were labelled "?" in the flow list; now "INTEL".
- [x] C++ toolchain reinstalled (VS 2022 Build Tools, MSVC 14.44, SDK 22621, .NET 4.8 SDK); visual-polish branch built, 23/23 tests pass; plural fix compiled with it (2026-11-07)
- [x] Phase 5 plan decisions: TTS yes, French pilot, typed input yes, pilot build first (2026-10-04)
- [x] Recompile after Phase 1 C++ drop (2026-09-29)
- [x] Grader accent fix compiled (2026-09-29)
- [x] Phase 2 M2–M5 rebuild (2026-09-29)
- [x] M6 editor module rebuild (2026-09-29)
- [x] Paris story draft approved as-is (2026-09-30)
- [x] Phase 3 Stage 1 C++ rebuild (2026-09-30)
- [x] Git LFS enabled; `Lvl_Paris_Greybox` deleted (2026-09-30)
- [x] Reported: characters huge while moving (fixed, FBX root scale) and water flicker (fixed, z-fighting) (2026-09-30)
- [x] Playtest of all four cities with real mouse input: no issues besides the audio (2026-09-29)
- [x] Reported: audio sounds harsh, like a ringing. Replaced with analyzed CC0 samples in Phase 4 (2026-09-29)
- [x] Reported: editor launched "very broken". The cause was LFS pointer files in the working copy, fixed with `git lfs checkout` (2026-09-29)
