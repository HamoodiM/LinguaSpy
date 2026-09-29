# Editor TODO (things you need to do by hand)

Claude keeps this list up to date. Tick items off (or delete them) when you've done them.

## Open
- [ ] **Full rebuild now (Phase 2 C++ drop, milestones 2–5).** Close the editor, build `PersonalEditor`, reopen. This adds many new classes/headers, changes a constructor, and adds config (`DefaultGame.ini` asset manager), so Live Coding is not enough. If the build fails, tell Claude; it reads the log.
- [ ] **Playtest click-to-talk:** click Camille (café) and Marcel (kiosk). Check that you walk over and the panel opens. Then click tiles into and out of the answer row and use Say it / Clear / Continue / Walk away. This hasn't been tested with real mouse input.
- [ ] **Check the walk animation** plays on click-to-move.

## Decisions pending
- [ ] **Git LFS for `Content/`:** optional, but recommended before the repo grows.
- [ ] **Japanese font:** if the engine's fallback CJK font looks rough, download Noto Sans JP (Google Fonts, free) and tell Claude where it is so it can import it. (Phase 2, M2)

## Done
- [x] Recompile after Phase 1 C++ drop (2026-09-29)
- [x] Grader accent fix compiled (2026-09-29)
