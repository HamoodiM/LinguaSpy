# Phase 5 Plan: Deeper Cities, Mission Replay, Per-City Audio, Cleanup

## Context
Phase 4 shipped the learning systems:
- Leitner SRS;
- the Safe house review screen;
- the phrasebook;
- tap-to-translate with per-mission fluency grades;
- a settings screen;
- CC0 audio.

The user playtested Phase 4 and found no issues.

The biggest content gap now is depth. Madrid, Tokyo and Rome each have one short, 3-conversation A1 mission, while Paris has two. Two other gaps:
- Cleared cities offer nothing to replay, even though the best grades are already saved.
- Every city shares one music track and one street bed.

The repo also still carries the Top Down template leftovers.

**User decisions (3 question rounds):**
- **Scope:** deepen the existing cities, per-city music and audio polish, and cleanup/hardening. **No new city** this phase. If one is added later, it will be Berlin/German.
- **New missions:** one **medium mission (4–6 conversations)** each for **Madrid, Tokyo and Rome**, bringing every city to 2 missions.
- **Level:** **A1+ to A2**. That means past tense, near future, numbers and prices, telling the time, transport and asking for help. New `SkillTag`s: `Time`, `Past`, `Future`, `Transport`, `Help`. `Numbers` and `Shopping` get more use.
- **Mission replay:** a completed mission can be replayed for a better grade without touching story progress.
- **Props:** **3–5 new Blender props** in the kit style. If Blender MCP is unavailable, the fallback is to retint existing props.
- **Downloads:** I pick **one CC0 music track per city** and fetch **Noto Sans JP (SIL OFL)**. Approving this plan approves those downloads, within the limits in Stage 5.
- **Cleanup:** delete the **template C++ variants** and the **template content**. Assets that BP_Spy* still uses get moved to `/Game/LinguaSpy` first. Mannequin content stays.
- **Process:** the script review stays "build first, review later": drafts go into `docs/Phase5_Scripts.md`.

**Working mode (unchanged):**
- Execute autonomously and log decisions in `docs/Phase5_Log.md`.
- Put anything that needs the user in `EDITOR_TODO.md`.
- Commit after every stage.
- Build only while the editor is closed.
- Screenshots use the Win32 `PrintWindow` capture, not `shot showui` (it crashed the editor in Phase 4).
- If unreal-mcp fails to connect at session start, use the direct HTTP JSON-RPC client.

## Stage 0: Script drafts (no editor)
Write `docs/Phase5_Scripts.md` in the `City_Scripts_Review.md` table format. Each conversation lists: NPC line, reading (Japanese), helper translation, accepted answers and tiles with distractors, `SkillTag`, and branches and outcomes. The stories continue the hooks the first missions left behind:

**Madrid: "El Palacio" (`Madrid_Palacio`).** Hook: "the folder is at the palace".
1. **Javier** (tapas bar; mission-gated `RequiredMission` line). Small talk in the past tense: *¿Qué hizo ayer?* → *Ayer visité el museo* (**Past**).
2. **Rosa** (new; ticket booth, *taquilla*):
   - *Dos entradas, por favor* (**Numbers**);
   - *¿A qué hora abre el palacio?* → "at ten" (**Time**);
   - a choice to pay by card or in cash (**Shopping**).
3. **Reach** the palace gate marker, at the far end of the plaza by the arcade.
4. **Álvaro** (new; palace guard):
   - *Soy turista, vengo de Londres*, and a question about the visit;
   - blowing his cover gives the `Suspicious` outcome;
   - that branch detours to **Carmen**, who asks *¿Me puede ayudar?* (**Help**), then returns to the gate.
5. **Collect** the folder (*la carpeta*).
6. **Diego** (contact):
   - handoff in the near future tense: *Mañana voy a salir de Madrid* (**Future**);
   - blowing cover here fails the mission (restart).

**Tokyo: "Tower Line" (`Tokyo_TowerLine`).** Hook: the item Yuki handed over.
1. **Kenji** (new; station attendant): 東京タワー・まで・いくら・ですか (**Transport/Numbers**). A choice between a one-way ticket and a day pass.
2. **Tanaka** (returning; gated): 今・何時・ですか and understanding the reply 三時半です (**Time**).
3. **Hana** (new; ramen stall, *yatai*):
   - order with counters: ラーメン・を・ふたつ・ください (**Ordering/Numbers**);
   - a spice-level choice (**Preferences**).
4. **Reach** the tower viewpoint marker.
5. **Officer Mori** (new; police box, *kōban*):
   - 道に・迷いました (**Help**);
   - blowing cover gives `Suspicious`, which detours to Sato at the konbini.
6. **Yuki** (contact):
   - past tense: きのう・しゃしん・を・とりました (**Past**);
   - blowing cover fails the mission.

**Rome: "Operazione Colosseo" (`Rome_Colosseo`).** Hook: "the documents are at the Colosseum".
1. **Luca** (returning; gated): *Ieri ho visitato il Vaticano* (**Past**).
2. **Sofia** (new; tabaccheria, where bus tickets are sold):
   - *Un biglietto per il Colosseo, per favore* (**Transport**);
   - *Quanto costa?* → "one euro fifty" (**Numbers**).
3. **Giulia** (returning): *A che ora parte l'autobus?* → "at a quarter past five" (**Time**).
4. **Reach** the bus stop marker (*fermata*).
5. **Paolo** (new; tour guide):
   - *Mi sono perso, può aiutarmi?* (**Help**);
   - blowing cover gives `Suspicious`, which detours to Luca.
6. **Marco** (contact):
   - *Domani parto per Milano* (**Future**);
   - blowing cover fails the mission.

**Content rules**
- Accents and readings follow the conventions in CLAUDE.md.
- Elided words stay one tile.
- Japanese tiles are chunks separated by `|`, with romaji in `Reading`.
- Each challenge offers 2–4 distractors.
- Cover penalties are 20–30.

## Stage 1: C++ batch (editor closed; `Build.bat PersonalEditor Win64 Development`)
This build also compiles the Phase 4 plural fix, which is still pending.

**1a. Mission replay**
- `UMissionSubsystem` gets these calls:
  - `RequestReplay(ULanguagePackAsset*, FName MissionId)`, used by the menu before `OpenLevel`;
  - `IsReplaying()`;
  - `GetReplayMission()`.
- **Isolation:** while a replay runs, `GetProgress()` returns a **transient** `FCityProgress ReplayProgress` instead of the save's entry. Flags, objectives and `SaveProgress` therefore never touch story progress.
- **What still counts:** SRS, skill stats and best grades are all recorded during a replay.
- **On completion:**
  - grade the run and update `BestGrades`;
  - end the replay;
  - restore the story state;
  - show a "REPLAY COMPLETE" banner.
- **On failure:** restart the replay.
- **Leaving:** `LeaveCity` or quitting drops the replay.
- **No save-format change:** the replay state is never saved.
- **Pure helper:** `FMissionReplay::ApplyCompletion(Progress, MissionId, bReplay)` is static and testable. A replay never appends to `CompletedMissions` or changes `ActiveMission`.
- **UI:**
  - a new `UMissionListWidget` overlay (reusing `UOverlayPanelWidget`) lists each mission with its title, briefing, best grade and a Play/Replay button;
  - the city card gets a third small button, **Missions**;
  - the HUD shows a "REPLAY" tag next to the mission title.

**1b. Native font everywhere.** Apply `ULanguagePackAsset::NativeFont` to:
- word tiles (`UWordTileWidget::SetFont`, called from `UConversationWidget`);
- reading lines;
- the phrasebook (already partly done).

Stage 5 then gives Tokyo its Noto Sans JP font.

**1c. Editor module (`Source/PersonalEditor`)**
- A console command, `LinguaSpy.OpenConversationEditor`, invokes the tab, so automation can open it and take a screenshot.
- The **hand-edit guard**: saving from the Conversation Editor stamps package metadata `LinguaSpy.HandEdited=1`. `lingua_content.py` skips assets carrying that tag unless `force=True`, and logs every skip.

**1d. Template code removal**
- Delete `Source/Personal/Variant_Strategy` and `Source/Personal/Variant_TwinStick`.
- Remove their six include paths from `Personal.Build.cs`.
- Grep again to confirm nothing else references them. I checked beforehand: none do outside those folders.

**1e. Tests**
- `LinguaSpy.MissionReplay`: completion via replay vs. story, and progress isolation.
- `LinguaSpy.ContentGuard`: it only covers the metadata-key constant. The real check is the Python guard, exercised in Stage 4.
- All existing tests stay green.

Build, fix errors, run the headless tests, commit.

## Stage 2: Template content cleanup (editor)
1. **Find what BP_Spy* still uses from the template.** Use the asset registry (`get_referencers` / `get_dependencies`) on `BP_SpyCharacter`, `BP_SpyController`, `BP_SpyGameMode`, the maps and the widgets.
   - This is expected to include the `/Game/TopDown/Input` actions and mapping context, plus the cursor FX in `/Game/Cursor`.
   - Also check whether the menu diorama uses `LevelPrototyping` meshes.
2. **Move** the referenced assets to `/Game/LinguaSpy/Input` and `/Game/LinguaSpy/FX` with `rename_asset`, then fix up redirectors.
3. **Delete** `/Game/Variant_Strategy`, `/Game/Variant_TwinStick`, `/Game/TopDown`, `/Game/LevelPrototyping` and `/Game/Cursor`, **only where nothing references them**.
   - Check the files on disk and `git rm` anything left behind (the Phase 4 gotcha).
   - `Content/Characters/Mannequins` stays.
4. **Config:** point `DefaultEditor.ini`'s `SimpleMapName` at `Lvl_MainMenu`, and remove the stale TopDown content-browser path.
5. **Verify:**
   - every map loads with no missing-reference warnings in the log;
   - all BPs compile;
   - quick PIE runs in the menu and in Paris confirm that click-to-move and the cursor FX still work.
6. Record what was deleted and its size in the log. Commit.

## Stage 3: New props (Blender MCP; the user opens Blender)
- **Add props** to `Tools/Blender/build_kits.py` in the existing chunky low-poly, vertex-colour style:
  - `SM_Madrid_Taquilla`: ticket booth with an awning;
  - `SM_Tokyo_Yatai`: ramen stall with noren and a lantern;
  - `SM_Tokyo_Koban`: police box;
  - `SM_Rome_Fermata`: bus stop pole, sign and bench;
  - optionally `SM_Rome_Tabaccheria`: a shopfront with a "T" sign.
- **Export and import:** export to `Saved/Export/`, then run `art_setup.import_kits(...)` and assign `M_ToonMaster`.
- **Fallback:** if Blender MCP isn't reachable, retint existing props (Paris kiosk → taquilla/koban, metro arch → bus shelter, parasol + café set → yatai) and log it.

## Stage 4: Content build (editor)
1. **One script per new mission:** `Tools/Content/madrid_palacio.py`, `tokyo_tower.py` and `rome_colosseo.py`, built on `lingua_content.py` like `paris_croissant.py`.
   - Each builds the conversations and the mission.
   - Each gates the returning NPCs' mission-2 dialogue with `RequiredMission`.
   - Each rewrites its pack with **both** missions.
   - **Run order per city:** base `x_content.py`, then the mission-2 script.
2. **Layouts in `cities.py`:** add the new NPCs, markers, pickups and props.
   - Madrid: Rosa, Álvaro, the palace gate, the folder, the taquilla.
   - Tokyo: Kenji, Hana, Mori, the tower viewpoint, the yatai, the koban.
   - Rome: Sofia, Paolo, the bus stop, the fermata and tabaccheria.
   - Place the new characters with the existing 7 character meshes.
3. **Rebuild the maps** with `build_city_map(MADRID|TOKYO|ROME)`.
   - First confirm with git that the maps have no hand edits since the last scripted build.
   - Then re-run the content scripts so each pack gets its `CityMap`.
   - Run `Validate()` on every new conversation and mission. There must be no errors.
4. **Guard check:** re-running `rome_content.py` must skip an asset stamped as hand-edited.
5. Commit, with a screenshot of each city's new area.

## Stage 5: Per-city music, ambience and Japanese font (downloads + editor)
**Music: one CC0 track per city**
- **Where from:** only pages explicitly tagged CC0 (OpenGameArt or similar). Each file must be ≤ 20 MB, and the total stays ≤ 80 MB.
- **Target feel:**
  - Paris: accordion or musette, or light jazz;
  - Madrid: Spanish guitar;
  - Tokyo: koto, shamisen or gentle lo-fi;
  - Rome: mandolin, or light Italian café.
- *Busy but Calm Street* stays as the **menu** track.
- **Gates (`Tools/Audio/analyze_audio.py`):**
  - no sustained single pitch (flatness above about 0.01 over the whole track, and a looped section without a drone);
  - spectral centroid under about 3 kHz;
  - high-frequency share under about 20%;
  - the level gets normalized in the mix settings.
- **Chiptune and harsh synth tracks are out.**
- **If no acceptable CC0 track exists** for a city, it keeps *Busy but Calm Street*, and this goes into EDITOR_TODO as a request for the user to suggest a track.

**Ambience per city** uses the already-downloaded `sfx_100_v2` loops:
- Madrid and Rome plazas: the fountain water loops, which pass the flatness and brightness gates;
- Paris and Tokyo: the street/highway bed.

**Wiring and credits**
- Extend `audio_import_cc0.py` (`SOUNDS` + `PACKS`) and re-run it.
- Update `docs/Audio_Credits.md`.

**Font**
- Get Noto Sans JP (SIL Open Font License) from the official Google Fonts / notofonts GitHub release: Regular and Bold TTF, about 5–10 MB each.
- Import it as a `UFontFace` and a composite `UFont` in `/Game/LinguaSpy/UI/Fonts`, and set it as `DA_Lang_Japanese_Tokyo.NativeFont`.
- If creating the composite font from Python fails, import the face and add the composite-font step to EDITOR_TODO.
- Credit it in `docs/Audio_Credits.md`, renamed to `docs/Credits.md`.

## Stage 6: Verification and docs
- **Tests:** all `LinguaSpy.*` pass headless, including `MissionReplay`.
- **Scripted PIE** (`Tools/Test/pie_driver.py`, extended for replay; back up and restore the user's save as in Phase 4):
  - **every new mission**: the direct path, the `Suspicious` detour, and the contact cover-blown → mission failed → restart;
  - **replay**: finish Madrid 2, replay Madrid 1 badly, and confirm that story progress and `CompletedMissions` are unchanged and the best grade isn't lowered; then replay it cleanly and confirm the best grade improves;
  - **SRS**: the new `SkillTag`s appear in Safe house skill bars and in phrasebook filters;
  - **audio**: the log shows the right per-city loops, and the channel volumes are correct.
- **Screenshots (`PrintWindow`):**
  - new NPCs and props in each city;
  - a Japanese conversation with Noto Sans JP on the tiles;
  - the Missions overlay with Replay buttons;
  - the replay banner;
  - the Conversation Editor tab (opened with the new console command).
- **Docs:**
  - CLAUDE.md: roadmap (Phase 5 done, Phase 6 ideas), layout (replay, font, guard), and the content list;
  - `docs/Phase5_Log.md`;
  - `EDITOR_TODO.md`: review `Phase5_Scripts.md`, listen to the city music, and the replay feel.
- **Honesty:** state clearly what wasn't verified: audio by ear, real mouse input, and native-speaker script quality.

## Critical files
- **Changed C++:**
  - `Source/Personal/LinguaSpy/Mission/MissionSubsystem.{h,cpp}`
  - `UI/{CityCardWidget,MainMenuWidget,SpyHUDWidget,ConversationWidget,WordTileWidget}.*`
  - `Personal.Build.cs`
  - `Source/PersonalEditor/*` (the Conversation Editor save path and the console command)
- **New C++:**
  - `UI/MissionListWidget.*`
  - `Mission/MissionReplay.h`
  - `Tests/MissionReplayTests.cpp`
- **Deleted:**
  - `Source/Personal/Variant_*`
  - `/Game/{Variant_Strategy,Variant_TwinStick,TopDown,LevelPrototyping,Cursor}`, keeping whatever gets moved first
- **Scripts:**
  - `Tools/Content/{madrid_palacio,tokyo_tower,rome_colosseo}.py`
  - `lingua_content.py` (guard)
  - `audio_import_cc0.py`
  - `Tools/Greybox/cities.py`
  - `Tools/Blender/build_kits.py`
  - `Tools/Test/pie_driver.py`
- **Docs:**
  - `docs/{Phase5_Plan,Phase5_Scripts,Phase5_Log,Credits}.md`
  - `CLAUDE.md`
  - `EDITOR_TODO.md`
- **Reuse:**
  - `lingua_content.conversation/mission/objective`
  - `greybox_kit.build_city_map`
  - `art_setup.import_kits`
  - `UOverlayPanelWidget`
  - `UFluencyScoring`
  - `BestGrades`
  - `analyze_audio.py`
  - `pie_driver.drive`

## Risks and fallbacks
- **Map rebuilds overwrite hand edits.** Check git history for non-script changes to the three maps first. If there are any, place the new actors with an additive helper instead of rebuilding.
- **Replay leaking into story progress.** The transient-progress design avoids it, and a unit test plus a PIE check cover it. When in doubt, never save while replaying.
- **Template deletion breaks input or cursor.** Move assets before deleting, check references, and run PIE. If anything breaks, restore from git (`git checkout -- Content/...`) and delete less.
- **A2 scripts in three languages.** Native-quality review is still pending. Keep sentences short and tile answers at 8 tiles or fewer, and flag uncertain lines in `Phase5_Scripts.md`.
- **No CC0 track passes the gates.** Keep the shared track for that city and ask the user.
- **Composite-font creation from Python.** Fall back to a manual step in EDITOR_TODO.
- **Editor crashes during automation.** Use `PrintWindow` for screenshots. Split long Python jobs to stay under 30 s.

## Verification summary
- A green build.
- All tests pass, including the new `MissionReplay` test.
- Three new missions, each completable via the direct path, the detour and the fail-restart.
- Replay isolation is verified.
- The template content is gone and the maps and PIE are clean.
- Each city has its own music, or a logged reason why not.
- Japanese renders in Noto Sans JP.
- Screenshots have been sent. Audio feel and script quality remain for the user to confirm.
