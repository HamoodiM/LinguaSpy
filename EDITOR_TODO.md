# Editor TODO (things you need to do by hand)

Claude keeps this list up to date. Tick items off (or delete them) when you've done them.

## Open
- [ ] **Playtest all four cities from the main menu**, using real mouse input:
  - **Paris:** First Contact, then Opération Croissant. Try failing Marcel on purpose to reach Luc.
  - **Madrid, Tokyo and Rome:** one mission each.
  - **Check:** click-to-talk, tile clicking, choice buttons, the romaji toggle in Tokyo, the P pause menu, and buildings fading when they hide the spy.
- [ ] **Listen to the audio:** UI sounds, city ambience and menu music are procedural MetaSounds, and Claude can't hear them. Say if anything is too loud, harsh or annoying. Volumes are in Project Settings → Game → LinguaSpy Audio.
- [ ] **Review the city scripts:** `docs/City_Scripts_Review.md` (Spanish, Japanese, Italian). Mark any lines you'd change.
- [ ] **Look-and-feel notes:** toon band strength, outline thickness, colours, character proportions. The knobs are the `PP_Toon` material parameters and `Tools/Blender/*.py`.
- [ ] **Editor preference:** Edit → Editor Preferences → General → Performance → uncheck **Use Less CPU when in Background** (it speeds up Claude's automated runs).
- [ ] **Try the Conversation Editor** (Tools → LinguaSpy → Conversation Editor). It has never been visually checked.

## Decisions pending
- [ ] **Git LFS for `Content/`:** recommended now. The repo holds many binary assets (maps, meshes, MetaSounds).
- [ ] **Delete `Lvl_Paris_Greybox`?** It's the obsolete Phase 1 map, now replaced by `Lvl_Paris`.
- [ ] **Japanese font:** the engine fallback renders fine. Optionally swap in Noto Sans JP for a nicer look.

## Done
- [x] Recompile after Phase 1 C++ drop (2026-09-29)
- [x] Grader accent fix compiled (2026-09-29)
- [x] Phase 2 M2–M5 rebuild (2026-09-29)
- [x] M6 editor module rebuild (2026-09-29)
- [x] Paris story draft approved as-is (2026-09-30)
- [x] Phase 3 Stage 1 C++ rebuild (2026-09-30)
- [x] Reported: characters huge while moving (fixed, FBX root scale) and water flicker (fixed, z-fighting) (2026-09-30)
