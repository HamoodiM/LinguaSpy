# LinguaSpy Curriculum Map

How the missions build on each other. Each mission teaches 4–8 target phrases in its briefing and recycles them in
its conversations, its debrief and the next mission. It names the CEFR "can-do" statements it trains. Research
background: `docs/Learning_Design.md`.

**Content rules (checked when content is built)**
- Every target phrase is met at least 3 times: briefing, field and debrief. At least once more in the next mission.
- Every challenge has at least one feedback hint, ≤ 8 tiles in its answer and 2–4 distractors.
- Cover penalties are tiered: tutorial 10–12, first missions 15–20, A2 missions 20–30. Code phrases cost more.
- Intel checks (listening) appear at least once per mission, on information the player then uses.
- Never rename or move existing NodeIds or conversations: SRS keys depend on them. Retrofit by adding.

## Skill tags and how they depend on each other
```
Greetings ──► Politeness ──► Ordering ──► Shopping
                   │             │            ▲
                   ▼             ▼            │
              SmallTalk       Numbers ────────┘
                   │             │
                   ▼             ▼
              Preferences      Time ──► Transport
                                 │
Directions ──────────────────────┴──► Help
Past / Future (A2) build on SmallTalk + Time
Code = mission-specific phrases (any level)
```
Missions are linear for now. In Phase 6 the dependency graph will gate optional side missions and Safe house
recommendations; it isn't in code yet (see Phase5_Log "Deferred").

## Paris (French): pilot content
| Mission | Level | Target phrases (briefing) | Can-do statements | Intel checks | Field notes it triggers |
|---|---|---|---|---|---|
| **Operation First Contact** (tutorial) | A1 | Bonjour · je voudrais · s'il vous plaît · Merci · Je suis en vacances · Où est la gare ? · à gauche · le journal d'hier | Greet someone and order a drink politely; Say why you are in a city; Ask where a place is and follow simple directions | Price at the café (€2.50); Way to the station (straight, then left) | je voudrais, vous, où est, je suis |
| **Opération Croissant** | A1+ | Voilà · deux euros cinquante · un ticket · une seule · Je peux payer par carte ? · Quelle heure est-il ? · à droite · Il va pleuvoir | Handle prices and pay in a shop or café; Ask for and understand the time; Follow simple directions to a place | The painter's time (11:50) | prices, il va + infinitive |

Recycling: *je voudrais*, *s'il vous plaît*, *Bonjour*, *où est* and *je suis* come back in Croissant. The scaffold
recognizes them across conversations, so a learner who mastered them in First Contact doesn't start over.

**Pilot tests** (`paris_assessment.py`): FR_A / FR_B, parallel forms.
- Each has 15 items: 5 listen, 4 read, 3 gap-fill, 3 assemble.
- They cover Politeness, Directions, Ordering, Numbers, SmallTalk, Time, Shopping and Preferences.
- Listening, reading and gap items recombine the taught words in new sentences.
- Five assembly items (A13–A15, B13, B15) are core phrases taught almost word for word. Report them separately.

## Madrid, Tokyo, Rome (A1 now, A2 second missions planned)
| City | Mission 1 (built, Phase 3) | Mission 2 (planned, Phase5_Plan Stage 0c/6) | New skills |
|---|---|---|---|
| Madrid | (see City_Scripts_Review.md) | **El Palacio**: Javier (past) → Rosa, taquilla (numbers/time; intel: opening time) → palace gate → Álvaro (help detour) → folder → Diego (future) | Past, Future, Time, Help |
| Tokyo | (see City_Scripts_Review.md) | **Tower Line**: Kenji (transport; intel: which line) → Tanaka (time; intel: 三時半) → Hana (counters) → viewpoint → Mori (help) → Yuki (past) | Transport, Time, Past, Help |
| Rome | (see City_Scripts_Review.md) | **Operazione Colosseo**: Luca (past) → Sofia (ticket, price; intel) → Giulia (bus time; intel) → fermata → Paolo (help) → Marco (future) | Past, Future, Transport, Time, Help |

Their mission 1 also gets the Paris-style retrofit: briefing, hints, intel checks, field notes and voices. That work
comes after the pilot build (user's scope decision, 2026-10-04). Japanese typing scaffolds stay off: gap-fill and
produce fall back to tiles until IME input exists (Phase 6).
