# DRAFT: Opération Croissant (Paris, French beginner)

> **Status: for your review.** Edit anything: plot, names, lines, difficulty. Mark changes inline or reply with notes.
> Once you approve it, Claude builds it into assets and the Paris map. Accepted answers ignore case and punctuation, but
> accents count. Elided words are single tiles (`s'il`, `l'église`).

**Briefing (handler):** *"Agent, a courier is moving a microfilm through Paris today. Our contact Marcel knows where it
is. Get it to Élise on the bridge before the other side does, and don't let anyone hear an accent."*

## Flow
```
Camille (café) ──> Marcel (kiosk) ──code ok──> Sophie (boulangerie) ──> Hugo (painter) ──> reach the bridge ──> Élise
                         └─cover blown─> Luc (metro booth) ──┘                                                   └─cover blown─> mission failed (restart)
```

## 1. Café: Camille (barista). Outcome: `GotNote`
| # | Camille says | Helper | Player must say (accepted answers) | Distractors | Skill |
|---|---|---|---|---|---|
| 1 | Bonjour ! Qu'est-ce que je vous sers ? | Hello! What can I get you? | *Bonjour, je voudrais un café, s'il vous plaît* / *Bonjour, un café, s'il vous plaît* | veux, thé, merci | Ordering |
| 2 | Vous êtes touriste ? | Are you a tourist? | **CHOICE.** A: *Oui, je suis en vacances* (flag `CoverTourist`) · B: *Non, je suis ici pour le travail* (flag `CoverWork`) | travail/vacances, oui/non | SmallTalk |
| 3 | Un grand ou un petit ? | A large or a small? | *Un petit, s'il vous plaît* / *Petit, s'il vous plaît* | grand, deux | Ordering |
| 4 | Ça fait deux euros cinquante. | That's two euros fifty. | *Voilà deux euros cinquante* | trois, quinze | Numbers |
| 5 | Merci ! Oh, et... un monsieur a laissé ce message pour vous. (END) | Thanks! Oh, and... a gentleman left this message for you. | | | |

## 2. Kiosk: Marcel (contact). Outcome: `GotBakery`, or `Suspicious` if your cover is blown
| # | Marcel says | Helper | Player must say | Distractors | Skill |
|---|---|---|---|---|---|
| 1 | Bonjour ! Vous cherchez quelque chose ? | Hello! Looking for something? | *Bonjour, où est la gare ?* (+ variants with *s'il vous plaît*) | le, musée, qui | Directions |
| 2 | Vous aimez les journaux d'aujourd'hui ? | Do you like today's newspapers? *(code phrase! Reply: "I prefer yesterday's newspaper")* | *Je préfère le journal d'hier* | aime, la, demain | Code |
| 3 | Très bien. Le colis est chez le boulanger, rue Lepic. Demandez « la baguette du matin ». (END) | Very good. The package is at the baker's on rue Lepic. Ask for "the morning baguette". | | | |

## 2b. Metro booth: Luc (backup contact, only if Marcel got suspicious). Outcome: `GotBakery`
| # | Luc says | Helper | Player must say | Distractors | Skill |
|---|---|---|---|---|---|
| 1 | Bonjour, je peux vous aider ? | Hello, can I help you? | *Un ticket, s'il vous plaît* | deux, billet, merci | Ordering |
| 2 | Voilà. Deux euros dix. | Here. Two euros ten. | *Voilà deux euros dix* | douze, vingt | Numbers |
| 3 | Votre ami Marcel a téléphoné. Allez à la boulangerie, rue Lepic. | Your friend Marcel called. Go to the bakery on rue Lepic. | *Merci beaucoup, au revoir* | bonjour, bien | Politeness |
| 4 | Et demandez « la baguette du matin ». (END) | And ask for "the morning baguette". | | | |

## 3. Boulangerie: Sophie (courier). Outcome: `GotPackage`
| # | Sophie says | Helper | Player must say | Distractors | Skill |
|---|---|---|---|---|---|
| 1 | Bonjour ! Vous désirez ? | Hello! What would you like? | *Je voudrais la baguette du matin* | soir, le, croissant | Ordering |
| 2 | Ah... Une seule ? | Ah... Just one? | *Oui, une seule, s'il vous plaît* / *Une seule, s'il vous plaît* | deux, non | Numbers |
| 3 | Ça fait un euro vingt. | That's one euro twenty. | **CHOICE.** A: *Voilà un euro vingt* → 5 · B: *Je peux payer par carte ?* → 4 | deux, trente / espèces | Numbers / Shopping |
| 4 | Non, désolée, seulement en espèces. | No, sorry, cash only. | *D'accord, voilà un euro vingt* | carte, deux | Numbers |
| 5 | Merci. Attention... elle est fragile, cette baguette. (END) | Thanks. Careful... this baguette is fragile. | | | |

## 4. Place du Tertre: Hugo (street painter). Outcome: `GotDirections`
| # | Hugo says | Helper | Player must say | Distractors | Skill |
|---|---|---|---|---|---|
| 1 | Vous aimez ma peinture ? | Do you like my painting? | **CHOICE.** A: *Oui, c'est très beau* (+cover) · B: *Désolé, je suis pressé* | laid, lent | SmallTalk |
| 2 | Je peux vous aider ? | Can I help you? | *Quelle heure est-il ?* | quel, temps, où | Time |
| 3 | Il est midi moins dix. | It's ten to twelve. | | | |
| 4 | Autre chose ? | Anything else? | *Où est le pont, s'il vous plaît ?* | la, gare, quand | Directions |
| 5 | Le pont ? Tout droit, puis à droite, après l'église. (END) | The bridge? Straight on, then right, after the church. | | | |

## 5. The bridge: Élise (handler's agent). Outcome: `MissionDone`, or `Burned` → mission failed
| # | Élise says | Helper | Player must say | Distractors | Skill |
|---|---|---|---|---|---|
| 1 | Il fait beau aujourd'hui, non ? | Nice weather today, isn't it? *(code: "Yes, but it's going to rain tonight")* | *Oui, mais il va pleuvoir ce soir* | demain, neiger, matin | Code |
| 2 | Vous avez quelque chose pour moi ? | Do you have something for me? | **CHOICE.** A: *Oui, voici la baguette* → 4 · B: *Quel est le mot de passe ?* → 3 | pain, quoi | Code |
| 3 | « Croissant. » | "Croissant." | *Oui, voici la baguette* | pain, quoi | Code |
| 4 | Parfait. Le train part à quelle heure ? | Perfect. What time does the train leave? *(Tell her: 2 p.m.)* | *À quatorze heures* | quatre, deux, midi | Time |
| 5 | Merci, agent. La France vous remercie. (END) | Thank you, agent. France thanks you. | | | |

## Map additions (Paris)
A boulangerie (with Sophie), a metro ticket booth (Luc) by the existing metro entrance, a small square with an easel (Hugo),
and the Seine at the east end with a stylized bridge (a marker and Élise).

## Questions for you
1. Should **Opération Croissant replace** the current "First Contact" test mission, or come after it as mission 2 (with First Contact as a short tutorial)?
2. Is the difficulty right? Right now it's mostly A1 (greetings, ordering, numbers, time, directions).
3. Should cover penalties be harsher on code phrases? Currently a normal line costs 34 cover and a code phrase 40–50.
