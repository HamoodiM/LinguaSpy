# Editor TODO (things you need to do by hand)

Claude keeps this list up to date. Tick items off (or delete them) when you've done them.

## Open
- [ ] **Compile the plural fix.** It's in `SpyHUDWidget.cpp` and `PauseMenuWidget.cpp`, so it reads "1 mistake", not "1 mistakes". It's .cpp only: press Ctrl+Alt+F11 (Live Coding) or rebuild.
- [ ] **Listen to the new audio.** The procedural MetaSounds are gone, replaced with CC0 samples (see `docs/Audio_Credits.md`). Claude checked loudness and brightness numerically but can't hear them. Say if anything still sounds harsh, too loud or too quiet.
  - You can adjust volumes in-game: Settings, from the menu or the P pause menu.
  - The base mix levels are in Project Settings → Game → LinguaSpy Audio.
  - To swap a sound, edit `SOUNDS` in `Tools/Content/audio_import_cc0.py` and re-run it.
- [ ] **Decide on per-city music.** All four cities currently share *Busy but Calm Street* and the same street bed. Do you want a different CC0 track per city? If you have favourites (e.g. from OpenGameArt), point Claude at them.
- [ ] **Try the Phase 4 learning loop with real mouse input:**
  1. play a mission, tapping **Translate** on a line or two;
  2. check the **grade** in the Mission Complete banner;
  3. from the menu, open **Safe house** on the city card. Items become due 10 minutes after you learn them;
  4. try **Phrasebook** (card or P menu) and **Settings**.
- [ ] **Review the city scripts:** `docs/City_Scripts_Review.md` (Spanish, Japanese, Italian). Mark any lines you'd change.
- [ ] **Look-and-feel notes:** toon band strength, outline thickness, colours, character proportions. The knobs are the `PP_Toon` material parameters and `Tools/Blender/*.py`.
- [ ] **Editor preference:** Edit → Editor Preferences → General → Performance → uncheck **Use Less CPU when in Background** (it speeds up Claude's automated runs).
- [ ] **Try the Conversation Editor** (Tools → LinguaSpy → Conversation Editor). It has never been visually checked.
- [ ] **Clean up the save backups if you like.** `Saved/SaveGames/` has backups from Phase 4 testing: `*.user_backup_v1.sav` is your original, `*.user_backup_migrated_v2.sav` is the same profile migrated, and `*.phase4_test.sav` is the test profile. Your real profile has been restored as `LinguaSpyProfile.sav`.

## Decisions pending
- [ ] **Japanese font:** the engine fallback renders fine. Optionally swap in Noto Sans JP for a nicer look.

## Done
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
