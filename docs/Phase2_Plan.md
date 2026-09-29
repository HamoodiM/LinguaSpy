# Phase 2 Plan: Missions, Four Cities, Menus, Authoring Tool

## Goal
Turn the Phase 1 slice into a small game:
- **Branching spy missions:** a player's choices and failures change what happens next.
- **Four cities:** Paris/French, Madrid/Spanish, Tokyo/Japanese, Rome/Italian. Each gets its own stylized greybox map.
- **Main menu with city select,** plus a minimal save.
- **A custom editor tool** so writing conversations stops being painful.

**Scope:**
- **Paris** gets a full branching mission (5–8 conversations). I'll draft it and you'll review it.
- **Madrid, Tokyo and Rome** each get one short mission (2–3 conversations) to prove the pipeline.

**Decisions (from the Q&A):**
- Missions use branching objectives.
- I draft the story and you edit it.
- Authoring happens in a custom editor tool.
- Japanese is shown as kana + kanji, with a romaji toggle.
- A minimal save is included now.

---

## Milestone 1: Fixes (already started)
- **Click-to-move anim fix:** set `bUseAccelerationForPaths = true` in the `APersonalCharacter` constructor. The anim BP gates `ShouldMove` on acceleration, and path following produced none.
- **Grader fix:** `FText::ToLower` for accented capitals. Needs a recompile, then re-run `LinguaSpy.Grader`.
- **`EDITOR_TODO.md`:** tracks everything that needs you in the editor.

## Milestone 2: Multi-language core
- **`ULanguagePackAsset`** gains:
  - `LanguageCode`.
  - `bUsesWordSpacing` (false for Japanese).
  - `bHasReadings` (romaji).
  - An optional `UFont` override.
  - Accent colour/palette (for the menu card).
- **Tiles become a struct:** `FWordTile { FString Text; FString Reading; }`, replacing `TArray<FString>`. Nodes also gain `NativeLineReading` (romaji of the NPC line). The existing French assets get migrated.
- **Joining and grading:**
  - `USentenceGrader::JoinTokens` becomes language-aware (no spaces for Japanese).
  - Normalization also strips full-width punctuation: `。、！？「」` and the full-width space.
  - Japanese tokens are authored as chunks: `コーヒー` `を` `ください`.
- **UI:** a romaji toggle next to the Helper button; tiles show the reading as a small second line when it's on.
- **Font:** the engine's `DroidSansFallback` covers CJK. I'll verify the render in PIE, and optionally import Noto Sans JP later (listed in `EDITOR_TODO.md`).
- **Tests:** extend `LinguaSpy.Grader` with Japanese and Spanish (`¿…?`) cases.

## Milestone 3: Branching conversations + mission framework
**Conversations** (`ConversationTypes.h`)
- **New `PlayerChoice` node:** 2–3 English intents such as "Play it cool" or "Ask about the package". Each has its own tile challenge and next node, so what you choose to say steers the story.
- **Node fields:**
  - `SetFlags`: flags set on entering the node.
  - `Outcome`: an `FName` reported when an `End` node finishes, e.g. `GotPackage` or `Suspicious`.
- **Cover blown** reports the `CoverBlown` outcome after the practice drill.

**Missions** (new `Source/Personal/LinguaSpy/Mission/`)
- **`UMissionAsset`** (PrimaryDataAsset):
  - Id, title, briefing, owning city.
  - `StartObjective`.
  - `TArray<FMissionObjective>`.
- **`FMissionObjective`:**
  - Id and type: `TalkTo` (NpcId), `ReachLocation` (MarkerId) or `Collect` (PickupId).
  - Display text.
  - `SetFlags`.
  - Transitions: `TArray<{ RequiredOutcome / RequiredFlags / BlockedFlags → NextObjective }>`, where NextObjective can be `MissionComplete` or `MissionFailed`.
- **`UMissionSubsystem`** (GameInstance subsystem):
  - Holds the active mission, current objective, flags and completed missions.
  - Listens to `UConversationSubsystem` outcomes and to marker/pickup events.
  - Broadcasts objective changes.
- **World hooks:**
  - `ANpcCharacter` gets an `NpcId` plus `TArray<{RequiredFlags, BlockedFlags, Conversation}>`, so NPCs say different things as the mission progresses.
  - A new `AMissionMarker` (trigger volume + stylized beacon mesh) for places.
  - A new `AMissionPickup` for items.
  - Only the current target shows a floating beacon.
- **HUD:** an objective tracker under the cover meter, plus a mission-complete/failed banner.
- **Tests:** a transition-resolution test on a small in-code mission.

## Milestone 4: Minimal save
- **`ULinguaSpySaveGame`**, one profile slot, containing:
  - Per-skill stats (from `ULanguageProgressSubsystem`).
  - Per-city mission state: active mission, current objective, flags, completed missions.
  - Settings: helper and romaji toggles.
- **When it saves:** auto-save on objective change, conversation end and returning to the menu. It loads at GameInstance startup.
- **Tests:** a save → load round-trip automation test.

## Milestone 5: Main menu + city select
- **`Lvl_MainMenu`:** a simple stylized backdrop with a menu game mode (UI only).
- **Code-built `UMainMenuWidget`:**
  - Title.
  - City cards generated from the registered language packs, via the AssetManager primary asset type `LanguagePack`.
  - Each card shows the flag colours/palette and mission progress.
  - Continue / New Game.
- **Starting a city:** picking a card stores the selected pack in the GameInstance and calls `OpenLevel(CityMap)`.
- **Pause menu (Esc):** Resume, Return to Menu, and a romaji/helper settings toggle.
- **`ULinguaSpyGameInstance`** (C++) holds the selected pack. It needs a project-setting change, which I'll try via config.

## Milestone 6: Conversation editor tool
- **New editor-only module `PersonalEditor`** (`Type: Editor`), with a code-built Slate tab: **Window → LinguaSpy → Conversation Editor**.
  - **Left:** conversation assets, filterable by city.
  - **Middle:** node list shown as an indented flow, with type icons and next/fail/choice links as clickable chips.
  - **Right:** node details.
    - Line, translation, reading and type.
    - Challenge editor: tile chips; accepted answers typed as sentences and auto-split into tokens; "add missing tiles"; distractor suggestions.
  - **Validation panel:**
    - Unreachable nodes, dangling links and duplicate ids.
    - Answer tokens missing from the tile bank.
    - Missing translations/readings.
  - **"Play from node" button:** starts PIE and jumps straight into that node.
- **`IsDataValid`** on `UConversationAsset` and `UMissionAsset`, so problems also show on save and in Data Validation.
- **Missions** are edited in the standard details panel, which is manageable size-wise. A mission tab or graph view can come in Phase 3.
- **Needs a full rebuild and editor restart** (new module in `.uproject`).

## Milestone 7: Paris full mission (draft → your review)
**"Opération Croissant"** (draft outline for you to rewrite):
1. **Briefing** from the handler (helper voice).
2. **Café (Camille).** Order, and she slips you a note. *Choice:* chat about the weather to look local (+cover), or ask directly about "the delivery" (−cover, but faster).
3. **Kiosk (Marcel).** Code phrase exchange.
   - **Success →** sets `KnowsBakery`.
   - **Wrong code once →** `MarcelSuspicious`, and you're sent to the metro backup contact instead.
4. **Branch A, the boulangerie:** order "the usual" (quantities, prices) and pick up the bread with the package inside.
   **Branch B, the metro ticket booth:** buy a ticket and ask for the platform (numbers, directions) to meet the backup contact.
5. **Street painter:** ask for the time and directions to the bridge (time, directions).
6. **Bridge meet:** final exchange using numbers and times. Outcome: mission complete, or burned (retry from the last objective).

**Skills covered:** greetings, ordering, politeness, directions, numbers/prices, time, small talk. The Paris map gains a boulangerie, a metro entrance and a bridge/Seine edge.

## Milestone 8: Madrid, Tokyo, Rome
- **Reusable greybox kit:** `Tools/Greybox/greybox_kit.py`, a parameterized Python builder (street, plaza, facades, props, palette, NPC/marker placement). Maps become reproducible from code. Paris gets ported to it too.

| City | Map concept (stylized, flat pastel) | Short mission |
|---|---|---|
| **Madrid** (Spanish) | Arcaded plaza (Plaza Mayor feel), terracotta/ochre, tapas bar, orange trees, a flat stylized royal-palace silhouette | "La Tapa Secreta": order tapas, ask for the metro, give a code phrase |
| **Tokyo** (Japanese) | Narrow shopping street, lanterns, vending machines, a konbini, a torii gate, blocky neon signs, pink/teal palette, a Tokyo Tower silhouette | "Konbini Contact": buy an item politely, ask the station direction, give a code phrase |
| **Rome** (Italian) | Piazza with a fountain, ochre/terracotta, a trattoria with an espresso bar, Vespas, a Colosseum silhouette | "Protocollo Espresso": order espresso at the bar, directions to the fountain, give a code phrase |

- Each city gets its own `ULanguagePackAsset`, mission, conversations and palette.

## Recompile checkpoints (I'll tell you each time)
1. **Now:** anim fix + grader fix (constructor change, so a full rebuild is safest).
2. **After M2–M5** (one batched C++ drop).
3. **After M6:** new editor module, so a full rebuild and restart are required.
4. Small fixes as needed during content.

## Verification
- **Automation tests:** the `LinguaSpy.*` grader tests (FR/ES/JA), mission transitions, and the save round-trip.
- **Scripted PIE runs** per city: menu → city → mission start → a branch → complete. Screenshots via `shot showui`, including the Japanese tiles with romaji.
- **The editor tool:** validation catches seeded errors, and "Play from node" works.
- **You:** playtest click-to-talk/tiles and feel, review the Paris story, and add anything else from `EDITOR_TODO.md`.

## Risks
- **Japanese typography:** the fallback font may look rough, which would mean importing Noto Sans JP.
- **Slate tool size:** it's the largest single item. If it runs long, a simpler v1 ships first (node list + details + validation) and "Play from node" follows.
- **Editor performance:** about 3 FPS in the background makes automated PIE runs slow.
