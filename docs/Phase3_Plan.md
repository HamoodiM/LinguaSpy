# Phase 3 Plan: Finish Content (M7 + M8) + Stylized Art, Life, Audio, UI

## Context
Phase 2 delivered the systems:
- branching conversations and missions;
- save;
- menu and city select;
- the Conversation Editor tool.

It also delivered a Paris test mission. Two Phase 2 milestones are still open and are **merged into this phase** (user decision):
- **M7:** build "Opération Croissant" from `docs/Paris_Mission_Draft.md`. It is approved as-is. First Contact stays as the tutorial mission 1, and Croissant becomes mission 2.
- **M8:** Madrid, Tokyo and Rome maps plus short missions.

Phase 3 proper is the art and feel pass. The user chose:
- **Look:** cel-shaded with ink outlines, and pastel palettes per city.
- **Characters:** modeled in **Blender via MCP**, rigged to the UE mannequin skeleton. One spy plus 4–6 modular NPC archetypes.
- **Extras:** camera occlusion fade, UI skin, audio pass (**procedural MetaSounds**, no downloads), and NPC life.

**Working conditions:**
- The user may be **asleep and cannot intervene**.
- I may build C++ myself while the editor is closed, and must fix build errors myself.
- No mid-run questions. I make sensible calls and log them in `docs/Phase3_Log.md`, with anything needing the user in `EDITOR_TODO.md`.
- City scripts for Spanish, Japanese and Italian: build first, the user reviews later.
- **Update memory** [[recompile-handoff]]: builds are allowed while the editor is closed; if the editor is open, hand off.

## Execution order (autonomy-first)
Stage 1 needs no editor. Every later stage needs the UE editor (unreal-mcp), and the Blender stages need Blender MCP. At the start of stage 2, if the editor isn't running, I launch it myself (`UnrealEditor.exe Personal.uproject`) and wait for unreal-mcp. If **Blender MCP is unreachable**, I skip stages 4–5, use toon-shaded mannequins and cube kits as the fallback, and log it.

### Stage 1: C++ batch (editor closed, I build via `Build.bat PersonalEditor Win64 Development`)
All the phase's C++ lands in one drop, so the rest is content work.

- **Mission-scoped NPC dialogue:**
  - `FNpcConversationOption` gains `FName RequiredMission` (only offered while that mission is active) and `bool bOnlyIfCurrentTarget`.
  - `ANpcCharacter::ResolveConversation` checks them via `UMissionSubsystem::GetActiveMission()`.
  - Files: `Source/Personal/LinguaSpy/Interaction/NpcCharacter.*`.
- **NPC life:**
  - `ANpcCharacter` switches from the static cylinder to the skeletal `GetMesh()` (mannequin skeleton, `ABP_Unarmed` idle). `BodyMesh` stays as an optional fallback.
  - When a conversation starts, the NPC smoothly faces the player (interp in Tick while talking), then returns to its original yaw.
  - Hook: `UConversationSubsystem::OnConversationStarted`/`Ended`.
  - New **`APedestrianCharacter`** (`Interaction/`): no conversation. An `AAIController` loop uses `UNavigationSystemV1::GetRandomReachablePointInRadius` plus `MoveToLocation`, with random waits and randomized archetype mesh and colors.
- **Camera occlusion fade:** new `UOcclusionFadeComponent` (`Camera/`) on `APersonalCharacter`.
  - Each tick it sphere-sweeps from camera to pawn (Visibility, multi).
  - Occluding primitives get `SetCustomPrimitiveDataFloat(0, Fade)`, lerped over time, and are restored when clear.
  - The master material (stage 3) reads CPD[0] into a dithered opacity mask.
  - Reference: the template's `/Game/Variant_Strategy/M_DitherStrategy`.
- **Audio hooks:**
  - `ULinguaSpyAudioSettings` (`UDeveloperSettings`, config): soft refs for UI click, tile place, pass, fail, cover blown, mission complete, menu select.
  - `ULanguagePackAsset` gains `AmbientSound` and `MusicSound`.
  - `APersonalGameMode::StartPlay` spawns 2D ambient and music components.
  - The widgets (`ConversationWidget`, `SpyHUDWidget`, `MainMenuWidget`, `PauseMenuWidget`) call a small `LinguaSpyAudio::PlayUI(this, ESpyUISound)` helper (`UGameplayStatics::PlaySound2D`).
- **UI skin (code-built widgets):**
  - `LinguaSpyStyle.h` gains rounded-box brushes (`FSlateBrush` with `ESlateBrushDrawType::RoundedBox`, corner radii, outline), a per-city accent colour and consistent spacing.
  - Tiles pop in (render-transform scale) and shake on a wrong answer; the conversation panel slides in; mode and cover-blown banners pulse. All of this is `NativeTick`-driven, with no UMG animation assets.
  - Pending fixes: HUD "Talking to ." shows "Talking. Stay in character." when there's no NPC. The menu wrap, backdrop and banner fixes are already in source.
- **Tests:** extend with `LinguaSpy.NpcConversationResolve`, a pure static helper for option selection (flags + mission).
- **Build:** loop on compile errors until green. Then commit.

### Stage 2: M7, Paris content (editor)
- `Tools/Content/paris_content.py`:
  - keep First Contact (mission 1);
  - add **MA_Paris_Croissant** (mission 2) with all 6 conversations exactly as in `docs/Paris_Mission_Draft.md`;
  - wire outcomes: GotNote, GotBakery/Suspicious, GotPackage, GotDirections, bridge reach, MissionDone/Burned → MissionFailed;
  - add the Camille/Marcel mission-2 dialogue via `RequiredMission`.
- **Paris map** (`Lvl_Paris_Greybox`, renamed to `Lvl_Paris` in stage 5):
  - add the boulangerie with Sophie, the metro booth with Luc (at the existing metro), the Place du Tertre easel with Hugo, and the Seine at the east end with a bridge (`Marker_Bridge`, Élise);
  - extend the ground and navmesh;
  - place 4–6 pedestrians.
- **Verify:** a scripted PIE run through both branches (the Marcel-suspicious path via Luc, and the direct path) to MissionComplete, plus the Élise cover-blown → MissionFailed restart.

### Stage 3: Toon look (editor, Python + `MaterialEditingLibrary` / VibeUE `MaterialNodeService`)
- **`M_ToonMaster`** (opaque/masked): BaseColor param, Roughness 1, Specular 0, and a CPD[0]-driven dither fade (`DitherTemporalAA`, opacity mask). All `MI_*` get reparented from `M_Flat` to it, keeping their colours.
- **`PP_Toon` post-process material:**
  - cel quantization (2–3 bands): SceneColor luminance ÷ DiffuseColor → stepped light ratio → recombined, with the palette's shadow tint;
  - ink outlines: a Sobel on SceneDepth and WorldNormal in a Custom HLSL node, with thickness and colour params;
  - sky masked out by depth.
- **Placement:** an unbound `PostProcessVolume` per city map and the menu map. It carries `PP_Toon`, and also sets auto-exposure off, bloom low and a slight vignette.
- **Per-city lighting presets** (sun colour/angle, skylight tint, fog), written as a function in the greybox kit.
- **Stylized sky:** a gradient sky sphere material instead of the realistic atmosphere look, with its own colours per city.

### Stage 4: Characters in Blender (Blender MCP)
- **Get the skeleton:** export `SKM_Manny_Simple` (skeleton + reference pose) from UE via `AssetExportTask` FBX, then import it into Blender.
- **Model** procedurally (bmesh/modifiers, `blender-3d-modeling` skill), around the armature so the proportions match:
  - chunky low-poly bodies: a big head, simple hands, flat-shaded, vertex colors or material slots (Skin, Top, Bottom, Hair, Accent);
  - the **spy:** trench coat, hat and sunglasses;
  - **archetypes:** barista (apron), vendor (cap + vest), elder (cane + scarf), young (hoodie), worker (overalls), tourist (camera + backpack);
  - swappable hat/hair meshes as separate skeletal parts.
- **Rig:** parent to the armature with automatic weights, then check the deformation on the mannequin anims.
- **Export** FBX → import into `/Game/LinguaSpy/Art/Characters/` with Skeleton = `SK_Mannequin`.
- **Materials:** `M_ToonCharacter` (same toon master) with colour params. Per-city NPC colour palettes go into a small `DA_CharacterPalette_<City>`, or simply MI variants.
- **Hook up:** `BP_SpyCharacter`, the NPCs and the pedestrians use the new meshes. Check that walk/idle animation plays in PIE.

### Stage 5: M8, three cities + environment kits (Blender + editor)
- **Greybox kit:** `Tools/Greybox/greybox_kit.py`, a parameterized builder that takes a layout dict (streets/plazas, facade rows, props, landmarks, NPC/marker/pickup placements, palette, lighting preset) and builds a map. It reuses the Paris layout code, and Paris is re-generated through it as `Lvl_Paris`.
- **Environment kits** (Blender → FBX → `/Game/LinguaSpy/Art/<City>/`), low-poly and chunky, with the toon master:
  - **shared:** modular facade blocks with inset windows and doors, lamps, trees, benches, awnings;
  - **Paris:** mansard roofs, kiosk, café set, metro arch, Eiffel;
  - **Madrid:** arcaded plaza, terracotta roofs, orange trees, palace dome;
  - **Tokyo:** narrow shopfronts, lanterns, vending machine, konbini, torii, Tokyo Tower;
  - **Rome:** ochre facades, trattoria, fountain, Vespa, Colosseum.
- **Content**, one script per city. Each gets its mission script written to `docs/` for later review:
  - **Madrid, "La Tapa Secreta":** 3 conversations (tapas bar order, ask for the metro, code phrase).
  - **Tokyo, "Konbini Contact":** 3 conversations, with kana + kanji tiles, romaji readings and `|` tokenization. Check that the CJK font renders; if `DroidSansFallback` looks rough, note Noto Sans JP in `EDITOR_TODO`.
  - **Rome, "Protocollo Espresso":** 3 conversations (espresso at the bar, directions to the fountain, code phrase).
  - Each pack gets its `CityMap`, missions and a palette. The menu cards become playable.

### Stage 6: Audio (MetaSounds via VibeUE `MetaSoundService`)
- **UI sounds:** a soft click, a tile "tock", a pass chime (major arpeggio), a fail buzz, a cover-blown sting and a mission-complete fanfare. All are synthesized from oscillators and envelopes.
- **Per-city ambience:** filtered noise beds (street hum and distant chatter texture), with a city-flavoured tone layer (accordion-ish drone for Paris, a guitar-pluck arpeggio for Madrid, a koto-ish pentatonic pluck for Tokyo, a mandolin tremolo for Rome). Kept low-key.
- Assign them in `ULinguaSpyAudioSettings` and in the packs.

### Stage 7: Polish + verification + docs
- Screenshot every city in PIE (`shot showui`): menu, conversation, a Japanese screen with romaji, cover blown, and the occlusion fade in action.
- Run the full test suite (`Automation RunTests LinguaSpy`).
- Update `CLAUDE.md`: art pipeline, kits, audio and conventions. Mark Phases 2 and 3 done and note the Phase 4 ideas.
- Update `EDITOR_TODO.md`:
  - playtest list per city;
  - review of the ES/JA/IT scripts;
  - anything that fell back.
- Commit after each stage.

## Critical files / assets
- C++:
  - `Source/Personal/LinguaSpy/{Interaction,Camera,Audio,UI,Mission}/…`
  - `APersonalCharacter`, `APersonalGameMode`
  - `LanguagePackAsset.h`
  - `UI/LinguaSpyStyle.h`
- Scripts:
  - `Tools/Content/{paris,madrid,tokyo,rome}_content.py`
  - `Tools/Content/lingua_content.py` (reuse `conversation`, `mission`, `objective`, `language_pack` helpers)
  - `Tools/Greybox/greybox_kit.py`
  - `Tools/Blender/*.py` (character/prop builders, kept for reproducibility)
- Content:
  - `/Game/LinguaSpy/{Materials,Art,Audio,Maps,Data}`
  - maps `Lvl_Paris`, `Lvl_Madrid`, `Lvl_Tokyo`, `Lvl_Rome`, `Lvl_MainMenu`

## Risks and fallbacks
- **Rigging auto-weights on chunky meshes:** keep the limbs cylindrical, align them to the bones, and fix bad deforms by splitting the mesh per bone. Fallback: toon-shaded mannequins with hats.
- **The toon PP on deferred lighting** can band on dark scenes. Tune thresholds per city; the fallback is unlit colours plus outlines only.
- **The editor at 3 FPS** (background throttle, which can't be changed from Python). Automation will be slow; use polling rather than long waits.
- **Build errors while the user sleeps:** iterate on the UBT log. Never leave the tree unbuildable; revert the offending change if it's stuck.

## Verification
- Green build; the `LinguaSpy.*` tests pass.
- Scripted PIE runs complete First Contact and both Croissant branches, plus one mission each in Madrid, Tokyo and Rome.
- The menu shows 4 playable cities with progress, and the save persists across a PIE restart.
- Screenshots confirm cel shading and outlines, the new characters animating, the occlusion fade, the UI skin, and Japanese text with romaji.
- Audio assets exist and the hooks fire (log check, since audio can't be heard via screenshots).
