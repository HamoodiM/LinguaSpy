# LinguaSpy Learning Design: what the research says and what we build from it

This document grounds LinguaSpy's mechanics in second-language acquisition (SLA) and learning-science research. Every
Phase 5+ learning feature traces back to a principle here. If a feature can't be justified by one of these, it's
decoration, not pedagogy.

## The problem with the Phase 1–4 loop
The current loop is: walk up to an NPC → read a target-language line (helper translates) → **build a full sentence from
tiles** → lose cover if wrong. That is a production-first design. A brand-new learner meets *bonjour, je voudrais un
café* for the first time **at the moment they must produce it under penalty**. Research says this is backwards:

| Issue in current loop | Why it's a problem |
|---|---|
| Production before any exposure | Learners need meaningful input before they can produce (Krashen 1982; VanPatten 2004). |
| One difficulty level for every learner | Support that helps novices hurts experts (expertise-reversal: Kalyuga et al. 2003). Support must fade. |
| Wrong answer → model answer + penalty | Prompts that make the learner self-correct produce more uptake than just giving the answer (Lyster & Ranta 1997; Lyster & Saito 2010). |
| No listening | NPC lines are text only. Listening is a core skill, and sound-text pairing supports encoding (Mayer 2009). |
| Cover penalty on every slip | Anxiety measurably impairs L2 performance (Horwitz, Horwitz & Cope 1986). Early stakes should be low. |
| Grammar is never made noticeable | Learners acquire what they notice (Schmidt 1990); brief explicit instruction is effective (Norris & Ortega 2000). |
| No measure of learning | We can't say whether it works, which matters for a prototype and a pilot. |

What already works and stays: spaced repetition (Phase 4 Leitner), retrieval practice (tile challenges *are* retrieval),
chunk-based tiles (formulaic language), task framing (missions), helper peeks counted as "assisted".

## Principles → mechanics

### 1. Comprehensible input before output
**Research.** Acquisition is driven by input slightly above the learner's level, "i+1" (Krashen 1982). Processing
instruction, where learners must attend to form to get meaning, beats output drills for initial learning (VanPatten
2004). Output still matters: it forces learners to notice gaps (Swain 1985). The order is input first, output after.

**Mechanic: Mission Briefing (the "dossier").** Before each mission, the handler briefs the player on **4–8 target
chunks** (the "cover phrases"). Each chunk shows text + audio + an icon/picture + meaning, then:
1. *Listen & match*: hear a chunk, pick its meaning (recognition, no penalty);
2. *Read & match*: see a chunk, pick its meaning;
3. *Which would you say?*: pick the right chunk for a situation (still recognition).

No production yet, and no cover at stake. Takes 2–4 minutes.

### 2. Scaffolding that fades (Zone of Proximal Development)
**Research.** Scaffolding means support matched to what the learner can do with help, then withdrawn (Wood, Bruner &
Ross 1976). Fixed heavy support becomes redundant and harmful as expertise grows (Kalyuga et al. 2003). Retrieval
should be effortful but successful ("desirable difficulties": Bjork 1994).

**Mechanic: Scaffold ladder per item**, chosen from that item's SRS strength:

| Level | Task | Cognitive demand |
|---|---|---|
| L0 Recognize | Pick which of 3 sentences matches the intent | recognition |
| L1 Assemble | Tiles, **no distractors**, first tile pre-placed | ordering only |
| L2 Assemble+ | Tiles with 2–4 distractors (today's challenge) | selection + ordering |
| L3 Gap-fill | Sentence shown with 1–2 key words missing; type them (or pick, for Japanese) | recall of key forms |
| L4 Produce | Larger tile bank / full typed answer with tolerant grading | free recall |

A new item starts at L0/L1 in the field (it was just met in the briefing). Correct unassisted answers step it up, and
errors step it down. A pure `UScaffoldPolicy` maps `(SrsBox, recent errors)` to a level, so it's unit-testable.

### 3. Corrective feedback that prompts before it tells
**Research.** In classroom studies, *prompts* (elicitation, metalinguistic clues, clarification requests) led to more
learner self-repair than *recasts* (the teacher just says the right form) (Lyster & Ranta 1997). A meta-analysis found
prompts at least as effective as recasts (Lyster & Saito 2010).

**Mechanic: Feedback ladder** on a wrong answer:
1. **Prompt.** The grader already knows which tiles are wrong (`FGradeResult::TileCorrect`). Highlight the first wrong
   slot and give an authored **hint category** ("Check the verb ending", "Word order: adjective after the noun",
   "Politeness: you need *vous*"). Hints are content (`FWordTileChallenge::Hints`), never hardcoded.
2. **Second try.** A correct answer here counts as *assisted* for SRS (box kept, not advanced) with a reduced cover cost.
3. **Recast.** Show the model answer, play its audio, and make the player rebuild it once (rehearsal). Full penalty.

### 4. Noticing and brief explicit grammar ("Field notes")
**Research.** Learners acquire forms they consciously notice (Schmidt 1990). A meta-analysis of 49 studies found
explicit instruction produced large, durable gains over implicit exposure (Norris & Ortega 2000).

**Mechanic: Field notes.** After a learner has met a pattern 2–3 times in context (e.g. *je voudrais / tu voudrais*, the
passé composé with *avoir*), the handler pops a 2–3 line note with the pattern highlighted in the examples they just
saw. This is inductive: examples first, rule second. Notes are collected in the phrasebook under a "Field notes" tab.
Data: `UGrammarNoteAsset` (pattern id, title, explanation, example refs, trigger count).

### 5. Formulaic chunks first, analysis later
**Research.** A large part of fluent speech is prefabricated multi-word sequences (Wray 2002), and teaching them as units
helps learners (Boers & Lindstromberg 2012). Vocabulary needs repeated encounters, around 10 for solid knowledge (Webb
2007), and high-frequency words give the most coverage (Nation 2001).

**Mechanic.** Keep chunk tiles. Each mission's chunks are chosen from a frequency list and **recycled across at least 3
conversations and the next mission** (content rule, checked by a validator that counts encounters per chunk). Later
missions split earlier chunks into their parts (*je voudrais* → *je* + *voudrais*) so learners can recombine them.

### 6. Tasks with real communicative purpose
**Research.** Task-based language teaching: learners use language to reach a non-linguistic goal, with a focus on
meaning plus attention to form (Ellis 2003). Negotiating meaning during interaction drives acquisition (Long 1996).

**Mechanic: Intel checks (listening as gameplay).** Some NPC replies contain information the player must *use*. "The
palace opens at ten" → the player sets a watch before entering, and "Take line 3" → the player chooses the platform.
This is a listening comprehension check that gates progress, not a quiz. It turns the spy fantasy into a real
information-gap task. Node type `IntelCheck` with an answer key and options.
**"Can you repeat that?"** Players can ask the NPC to repeat (audio again) or slow down (0.75× audio). Each request
costs a little suspicion, like in a real conversation.

### 7. Dual coding: audio + text + image
**Research.** People learn better from words plus pictures than from words alone, and spoken narration with visuals
beats on-screen text alone for novices (Mayer 2009; Paivio 1986).

**Mechanic.** Every NPC line and model answer has **audio**. Briefing chunks get an icon. Audio is generated offline at
authoring time by a local neural TTS (see Plan Stage 3) and imported as SoundWaves. At runtime nothing touches the
network, which keeps the "authored and offline" rule.

### 8. Retrieval practice, spacing and interleaving
**Research.** Testing yourself beats re-studying for long-term retention (Roediger & Karpicke 2006). Spaced practice
beats massed practice, with the best gap growing with the retention interval (Cepeda et al. 2006). Interleaving
categories helps learners discriminate them (Kornell & Bjork 2008), though the evidence for L2 grammar is mixed. Modern
schedulers model memory per item: half-life regression (Settles & Meeder 2016) and FSRS (Ye et al. 2022).

**Mechanic.** Keep the Phase 4 Leitner SRS and add:
- **interleaved Safe house sessions** that mix skills rather than block by skill;
- **mission debriefs**: 3–5 retrieval items from the mission's chunks, right after it ends (the first spaced exposure);
- **Phase 6:** swap Leitner for FSRS behind the same `USrsScheduler` interface. Our telemetry will provide the data to
  fit its parameters.

### 9. Low-anxiety stakes and graduated helper reliance
**Research.** Foreign-language anxiety correlates negatively with performance (Horwitz et al. 1986). Self-determination
work on games argues for competence and autonomy feedback over punishment.

**Mechanics.**
- **Cover penalties scale with mission tier.** The tutorial and first missions use small penalties, and the stakes grow
  as the player grows. Briefings and debriefs never cost cover.
- **Handler modes fade:** *Shadowing* (translation auto-shown) → *On call* (tap to translate, today's mode) → *Radio
  silence* (limited peeks per conversation). The mode is suggested from fluency grades, and the player can always
  override it (autonomy).
- **Blown cover reframed** as "go back to the safe house and drill", as today. The drill now uses the scaffold ladder
  starting at the item's current level, not a cold repeat.

### 10. A curriculum aligned to the CEFR
**Research.** The CEFR (Council of Europe 2001; Companion Volume 2020) describes levels with "can-do" statements, a
shared vocabulary for teachers.

**Mechanic.** Every mission declares the CEFR can-do statements it targets (e.g. A1 "Can ask for and give things in
shops"). Skills form a **prerequisite graph**: *Ordering* needs *Greetings* + *Politeness*. The city screen shows
can-do progress. This is what a teacher would read, and what the pilot report cites.

### 11. Measuring learning (built into the prototype)
**Research.** Pre/post designs need parallel test forms, a delayed post-test for retention, and effect sizes judged
against L2 benchmarks (Plonsky & Oswald 2014). Usability is measured with the SUS (Brooke 1996).

**Mechanics.**
- **Assessment mode:** a short (≈ 15 items, ≈ 8 min) pre-test/post-test per language, with parallel forms A/B. Items
  include listening recognition, meaning recognition, gap-fill and sentence assembly. No feedback is given, and results
  are stored locally.
- **Local telemetry:** a JSONL event log covering session start/end, briefing results, every challenge (item, scaffold
  level, attempts, hints, peeks, time-to-answer, pass/fail), cover-blown events and drills. It's anonymous (participant
  code only), local file only, with an export button. **No network.**
- **Learning curve report:** a Python script turns the logs into per-skill accuracy over attempts, time on task and
  pre/post gains with effect sizes. See `docs/Pilot_Study_Protocol.md`.

## Mission loop after Phase 5
```
City → Mission select ─► BRIEFING (input, recognition, no stakes)
                          │
                          ▼
                      FIELD (conversations: scaffolded challenges, intel checks,
                          │   feedback ladder, field notes, handler mode)
                          │        └─ cover blown → SAFE HOUSE DRILL (scaffold ladder) → retry
                          ▼
                      DEBRIEF (retrieval of the mission's chunks, grade S/A/B/C,
                          │   can-do statements earned)
                          ▼
                      SAFE HOUSE (spaced, interleaved review between sessions)
```

## What we deliberately don't do
- **No LLM or network at runtime.** Content is authored and verifiable, which also makes it suitable for classrooms.
- **No speech recognition yet.** Offline ASR (e.g. whisper.cpp) is a Phase 7 candidate. Pronunciation scoring is
  research-grade hard, and doing it badly demotivates learners.
- **No streaks or loss-aversion dark patterns.** Motivation comes from story, competence feedback and can-do progress.

## References
- Bjork, R. A. (1994). Memory and metamemory considerations in the training of human beings. In J. Metcalfe & A. Shimamura (Eds.), *Metacognition: Knowing about knowing*. MIT Press.
- Boers, F., & Lindstromberg, S. (2012). Experimental and intervention studies on formulaic sequences in a second language. *Annual Review of Applied Linguistics, 32*, 83–110.
- Brooke, J. (1996). SUS: A "quick and dirty" usability scale. In *Usability Evaluation in Industry*. Taylor & Francis.
- Cepeda, N. J., Pashler, H., Vul, E., Wixted, J. T., & Rohrer, D. (2006). Distributed practice in verbal recall tasks: A review and quantitative synthesis. *Psychological Bulletin, 132*(3), 354–380.
- Council of Europe (2020). *Common European Framework of Reference for Languages: Companion Volume*.
- Ellis, R. (2003). *Task-based Language Learning and Teaching*. Oxford University Press.
- Horwitz, E. K., Horwitz, M. B., & Cope, J. (1986). Foreign language classroom anxiety. *The Modern Language Journal, 70*(2), 125–132.
- Kalyuga, S., Ayres, P., Chandler, P., & Sweller, J. (2003). The expertise reversal effect. *Educational Psychologist, 38*(1), 23–31.
- Kornell, N., & Bjork, R. A. (2008). Learning concepts and categories: Is spacing the "enemy of induction"? *Psychological Science, 19*(6), 585–592.
- Krashen, S. (1982). *Principles and Practice in Second Language Acquisition*. Pergamon.
- Long, M. H. (1996). The role of the linguistic environment in second language acquisition. In W. Ritchie & T. Bhatia (Eds.), *Handbook of Second Language Acquisition*. Academic Press.
- Lyster, R., & Ranta, L. (1997). Corrective feedback and learner uptake. *Studies in Second Language Acquisition, 19*(1), 37–66.
- Lyster, R., & Saito, K. (2010). Oral feedback in classroom SLA: A meta-analysis. *Studies in Second Language Acquisition, 32*(2), 265–302.
- Mayer, R. E. (2009). *Multimedia Learning* (2nd ed.). Cambridge University Press.
- Nation, I. S. P. (2001). *Learning Vocabulary in Another Language*. Cambridge University Press.
- Norris, J. M., & Ortega, L. (2000). Effectiveness of L2 instruction: A research synthesis and quantitative meta-analysis. *Language Learning, 50*(3), 417–528.
- Paivio, A. (1986). *Mental Representations: A Dual Coding Approach*. Oxford University Press.
- Plonsky, L., & Oswald, F. L. (2014). How big is "big"? Interpreting effect sizes in L2 research. *Language Learning, 64*(4), 878–912.
- Roediger, H. L., & Karpicke, J. D. (2006). Test-enhanced learning: Taking memory tests improves long-term retention. *Psychological Science, 17*(3), 249–255.
- Schmidt, R. (1990). The role of consciousness in second language learning. *Applied Linguistics, 11*(2), 129–158.
- Settles, B., & Meeder, B. (2016). A trainable spaced repetition model for language learning. *Proceedings of ACL 2016*.
- Swain, M. (1985). Communicative competence: Some roles of comprehensible input and comprehensible output in its development. In S. Gass & C. Madden (Eds.), *Input in Second Language Acquisition*. Newbury House.
- VanPatten, B. (Ed.) (2004). *Processing Instruction: Theory, Research, and Commentary*. Lawrence Erlbaum.
- Webb, S. (2007). The effects of repetition on vocabulary knowledge. *Applied Linguistics, 28*(1), 46–65.
- Wood, D., Bruner, J. S., & Ross, G. (1976). The role of tutoring in problem solving. *Journal of Child Psychology and Psychiatry, 17*(2), 89–100.
- Wray, A. (2002). *Formulaic Language and the Lexicon*. Cambridge University Press.
- Ye, J., Su, J., & Cao, Y. (2022). A stochastic shortest path algorithm for optimizing spaced repetition scheduling. *Proceedings of KDD 2022*.

*Citations are from the standard SLA/learning-science literature. Check page numbers against the originals before
quoting them in anything published.*
