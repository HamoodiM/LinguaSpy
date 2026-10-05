# Phase 5 Scripts: A2 second missions (draft, "build first, review later")

These are the drafts for Madrid, Tokyo and Rome mission 2 (`docs/Phase5_Plan.md` Stage 0c, `docs/Curriculum.md`).
- Format: NPC line / reading → helper translation → what the player says (accepted answers | tiles incl. distractors), skill, hints.
- ⚠ marks lines that need a native or fluent speaker's check.
- Cover penalties: 20 normal, 25 code phrases. Tiles: ≤ 8 per answer, 2–4 distractors.

---

## Madrid: "El Palacio" (`Madrid_Palacio`)
**Briefing phrases:** ayer visité (yesterday I visited) · dos entradas (two tickets) · ¿A qué hora abre? (what time does it open?) ·
con tarjeta (by card) · ¿Me puede ayudar? (can you help me?) · vengo de Londres (I come from London) · mañana voy a (tomorrow I'm going to) · la carpeta (the folder)

**Can-dos:** Talk about what you did yesterday (A2) · Buy tickets and ask about opening times (A2) · Ask a stranger for help (A2)

### 1. Javier, tapas bar (gated `RequiredMission=Madrid_TapaSecreta`)
| Node | NPC line | Translation | Player | Skill | Hints |
|---|---|---|---|---|---|
| j1 | ¡Hombre, otra vez por aquí! ¿Qué hizo ayer? | Hey, you again! What did you do yesterday? | "Ayer visité el museo" \| tiles: Ayer visité el museo visito mañana parque | Past | `visité` = I visited (past; `visito` is present); `ayer` = yesterday |
| j2 | ¿Y le gustó? | And did you like it? | choice: "Sí, me gustó mucho" (Sí me gustó mucho gusta poco) / "No mucho, era aburrido" (No mucho era aburrido es) | Past | `me gustó` = I liked it (past) |
| j3 | Mire, el palacio abre pronto. La taquilla está al lado. | Look, the palace opens soon. The ticket booth is next door. | → End `GotTip` | | |

### 2. Rosa, taquilla (new NPC; female voice)
| Node | NPC line | Translation | Player | Skill | Hints |
|---|---|---|---|---|---|
| r1 | Buenos días. ¿Cuántas entradas? | Good morning. How many tickets? | "Dos entradas por favor" \| Dos entradas por favor tres entrada gracias | Numbers | `dos` = two; plural `entradas` |
| r2 | ¿Algo más? | Anything else? | "¿A qué hora abre el palacio?" \| A qué hora abre el palacio cierra cuándo | Time | `abre` = opens (`cierra` = closes) |
| r3 | **intel** Abre a las diez y cierra a las seis. | It opens at ten and closes at six. | Q: "When does the palace open?" [9:00 · **10:00** · 6:00]. Flag `KnowsOpening` | Time | |
| r4 | Son veinticuatro euros. ¿Efectivo o tarjeta? | That's twenty-four euros. Cash or card? | choice: "Con tarjeta por favor" (Con tarjeta por favor efectivo en) / "En efectivo aquí tiene" (En efectivo aquí tiene tarjeta con) | Shopping | `con tarjeta` = by card; `en efectivo` = in cash ⚠ |
| r5 | (End) Gracias. ¡Que disfrute la visita! | Thanks. Enjoy the visit! | outcome `GotTickets` | | |

### 3. Reach: palace gate (marker `PalaceGate`, far end of the plaza by the arcade)

### 4. Álvaro, palace guard (new NPC; `CoverBlownOutcome=Suspicious`)
| Node | NPC line | Translation | Player | Skill | Hints |
|---|---|---|---|---|---|
| a1 | Buenos días. ¿De dónde viene? | Good morning. Where are you from? | "Soy turista vengo de Londres" \| Soy turista vengo de Londres voy a | SmallTalk | `vengo de` = I come from |
| a2 | ¿Y qué quiere ver? | And what do you want to see? | "Quiero ver la sala del trono" \| Quiero ver la sala del trono quiere cocina | Preferences | `quiero` = I want; `la sala del trono` = the throne room ⚠ |
| a3 | (End) Adelante. | Go ahead. | outcome `Inside` | | |

**Suspicious detour:** Carmen (returning) in the plaza.
- Line: "¿Está perdido?" (Are you lost?)
- Player: "Sí ¿me puede ayudar?" (Sí me puede ayudar puedo ayuda), skill **Help**.
- Carmen: "Diga al guardia que tiene entradas." (Tell the guard you have tickets.)
- The mission then returns to the gate, where Álvaro has a retry conversation.

### 5. Collect: la carpeta (pickup `Folder`, inside the arcade)

### 6. Diego, contact (`CoverBlownOutcome=Burned` → mission fails)
| Node | NPC line | Translation | Player | Skill | Hints |
|---|---|---|---|---|---|
| d1 | ¿Tiene la carpeta? | Do you have the folder? | "Sí aquí está la carpeta" \| Sí aquí está la carpeta el libro | Code (25) | |
| d2 | ¿Y cuándo sale de Madrid? | And when do you leave Madrid? | "Mañana voy a salir de Madrid" \| Mañana voy a salir de Madrid ayer fui | Future (25) | near future: `voy a` + infinitive; `mañana` = tomorrow |
| d3 | (End) Perfecto. Buen viaje, agente. | Perfect. Safe travels, agent. | outcome `MissionDone` | | |

**Field notes (Spanish):**
- es_preterite: *visité / me gustó*, past for finished actions; triggers `visité` `gustó`.
- es_ir_a: *voy a + infinitive*; triggers `voy`.
- es_usted: *¿Me puede ayudar?*, polite you; triggers `puede`.

---

## Tokyo: "Tower Line" (`Tokyo_TowerLine`)
Tiles are chunks separated by `|`, with romaji in `Reading`. Typing scaffolds stay off for Japanese (no IME yet).

**Briefing phrases:**
- 東京タワー (Tōkyō Tawā)
- まで (made, "as far as")
- いくらですか (ikura desu ka, "how much?")
- 今何時ですか (ima nanji desu ka, "what time is it?")
- 三時半 (sanji han, "3:30")
- ふたつ (futatsu, "two things")
- 道に迷いました (michi ni mayoimashita, "I got lost")
- とりました (torimashita, "I took")

**Can-dos:** Buy a ticket and ask the fare (A2) · Ask and understand the time (A2) · Order using counters (A2) · Ask the police for help (A2)

| # | NPC | Line (reading) | Translation | Player (chunks) | Skill | Notes |
|---|---|---|---|---|---|---|
| 1 | Kenji, station (new) | いらっしゃいませ。どちらまで？ (irasshaimase. dochira made?) | Welcome. Where to? | 東京タワー\|まで\|いくら\|ですか + distractors から\|なん | Transport/Numbers | `まで` = to/as far as; `から` = from |
| 1b | Kenji, **intel** | 大江戸線で二百二十円です。 (Ōedo-sen de nihyaku nijū en desu) | 220 yen on the Ōedo line. | Q: fare? [**¥220** · ¥120 · ¥2,200] | Numbers | ⚠ fare realism |
| 1c | Kenji | 一日券もありますよ。 | There's also a day pass. | choice: きっぷ\|を\|一枚\|ください / 一日券\|を\|ください | Shopping | |
| 2 | Tanaka (returning, gated) | また会いましたね。 | We meet again. | 今\|何時\|ですか + distractors 何\|今日 | Time | |
| 2b | Tanaka, **intel** | 三時半です。 (sanji han desu) | It's half past three. | Q: time? [3:00 · **3:30** · 4:30] | Time | |
| 3 | Hana, yatai (new) | 何にしますか？ | What will you have? | ラーメン\|を\|ふたつ\|ください + ひとつ\|が | Ordering/Numbers | counters: ひとつ, ふたつ |
| 3b | Hana | 辛さはどうしますか？ | How spicy? | choice: 普通\|で\|お願いします / 辛く\|して\|ください | Preferences | ⚠ |
| 4 | Reach: tower viewpoint (`TowerView`) | | | | | |
| 5 | Officer Mori, kōban (new; Suspicious → Sato detour) | どうしましたか？ | What's the matter? | 道に\|迷いました + 道を\|行きます | Help | past -ました |
| 6 | Yuki, contact (Burned → fail) | きのう何をしましたか？ | What did you do yesterday? | きのう\|しゃしん\|を\|とりました + あした\|とります | Past (25) | |

**Field notes (Japanese):** ja_mashita (past -ました) · ja_counters (ひとつ / ふたつ) · ja_made_kara (まで / から)

---

## Rome: "Operazione Colosseo" (`Rome_Colosseo`)
**Briefing phrases:** ieri ho visitato (yesterday I visited) · un biglietto (a ticket) · quanto costa? (how much?) · a che ora parte? (what time does it leave?) ·
mi sono perso (I got lost) · può aiutarmi? (can you help me?) · domani parto (tomorrow I leave) · la fermata (the stop)

**Can-dos:** Talk about a past visit (A2) · Buy a bus ticket and ask the price (A2) · Ask when transport leaves (A2) · Ask for help (A2)

| # | NPC | Line | Translation | Player (tiles) | Skill | Notes |
|---|---|---|---|---|---|---|
| 1 | Luca (returning, gated) | Allora, cosa hai fatto ieri? | So, what did you do yesterday? | Ieri ho visitato il Vaticano \| + visito domani | Past | passato prossimo: ho + participle |
| 2 | Sofia, tabaccheria (new) | Buongiorno, desidera? | Good morning, what would you like? | Un biglietto per il Colosseo per favore \| + due biglietti | Transport | ⚠ "biglietto per il Colosseo" (bus ticket bought at the tabaccheria) |
| 2b | Sofia | Quanto costa? → **intel** "Un euro e cinquanta." | One euro fifty. | player asks: Quanto costa \| + quando dove; intel Q: price? [€1.15 · **€1.50** · €5.10] | Numbers | |
| 3 | Giulia (returning) | **intel** L'autobus parte alle cinque e un quarto. | The bus leaves at a quarter past five. | player first: A che ora parte l'autobus \| + arriva quando; intel Q [**5:15** · 5:45 · 4:15] | Time | `l'autobus` is one tile |
| 4 | Reach: la fermata (`BusStop`) | | | | | |
| 5 | Paolo, tour guide (new; Suspicious → Luca detour) | Tutto bene? | Everything OK? | Mi sono perso può aiutarmi \| + perduto aiuto | Help | ⚠ `perso`/`persa` gender: accept both |
| 6 | Marco, contact (Burned → fail) | Quando parti? | When do you leave? | Domani parto per Milano \| + ieri partito | Future (25) | present tense for planned future |

**Field notes (Italian):** it_passato_prossimo (ho visitato) · it_present_future (domani parto) · it_lei (può aiutarmi, polite)

---

## Retrofit of mission 1 in Madrid / Tokyo / Rome (after the new missions)
Same pattern as Paris:
- add briefing phrases, hints and can-dos;
- turn one NPC line into an intel check (Madrid: Carmen's "Siga todo recto y luego gire a la derecha" → which way?);
- add field notes;
- set tutorial penalties to 12;
- regenerate the voices: one model per language, licenses to be checked first.

Spanish and Italian get accent keys (`áéíóúñ¿¡` / `àèéìòù`). Japanese doesn't type.
