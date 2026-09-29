# Editor TODO (things you need to do by hand)

Claude keeps this list up to date. Tick items off (or delete them) when you've done them.

## Open
- [ ] **Review the Paris story draft:** `docs/Paris_Mission_Draft.md` (edit lines or plot, and answer the 3 questions at the bottom). This blocks M7.
- [ ] **Editor preference:** Edit → Editor Preferences → General → Performance → uncheck **Use Less CPU when in Background**. Automated tests/PIE runs slow to ~3 FPS when the editor isn't focused. (Claude can't change this one from Python.)
- [ ] **Playtest the Paris mission by hand:** Play from the main menu → Paris. Order from Camille (try both small-talk choices on different runs) → Marcel (try failing on purpose to see the "lose the tail" branch) → package at the café. Check click-to-talk, tile clicking, the P pause menu, and the walk animation.
- [ ] **Try the Conversation Editor** (Tools → LinguaSpy → Conversation Editor) once rebuilt: select `DA_Conv_Cafe`, edit a line, use Quick tiles, and try "Play from node".

## Decisions pending
- [ ] **Git LFS for `Content/`:** optional, but recommended before the repo grows.
- [ ] **Japanese font:** if the engine's fallback CJK font looks rough, download Noto Sans JP (Google Fonts, free) and tell Claude where it is so it can import it.

## Done
- [x] Recompile after Phase 1 C++ drop (2026-09-29)
- [x] Grader accent fix compiled (2026-09-29)
- [x] Phase 2 M2–M5 rebuild (2026-09-29)
- [x] M6 editor module rebuild (2026-09-29)
