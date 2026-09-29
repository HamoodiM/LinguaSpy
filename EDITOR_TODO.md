# Editor TODO (things you need to do by hand)

Claude keeps this list up to date. Tick items off (or delete them) when you've done them.

## Open
- [ ] **Recompile (full rebuild recommended):** the click-to-move walk animation fix (`PersonalCharacter.cpp` constructor) and the grader accent fix (`SentenceGrader.cpp`). Constructor changes don't hot-reload reliably, so close the editor, build, and reopen.
- [ ] **After recompiling:** run the tests (Tools → Session Frontend → Automation → `LinguaSpy`), or ask Claude to run them.
- [ ] **Playtest click-to-talk:** click Camille (café) and Marcel (kiosk). Check that you walk over and the panel opens. Then click tiles into and out of the answer row and use Say it / Clear / Continue / Walk away. This hasn't been tested with real mouse input.
- [ ] **Check the walk animation** plays on click-to-move after the rebuild.

## Decisions pending
- [ ] **Git LFS for `Content/`:** optional, but recommended before the repo grows.
- [ ] **Japanese font:** if the engine's fallback CJK font looks rough, download Noto Sans JP (Google Fonts, free) and tell Claude where it is so it can import it. (Phase 2, M2)

## Done
- [x] Recompile after Phase 1 C++ drop (2026-09-29)
