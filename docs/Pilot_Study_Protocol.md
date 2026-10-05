# Pilot Study Protocol: LinguaSpy with MathemaTech students

**Status: planned, not yet run.** Nothing in this document is a result. Results get filled in *only* from real sessions
(section 8). Until then, the resume, README and any portfolio page describe the study as planned or in progress, never as
done.

## 1. Purpose
A small formative pilot that answers:
1. **Learning:** do beginners improve on the target phrases after playing (immediate and 1-week delayed post-test)?
2. **Usability:** can students play without help? (SUS score, observed blockers)
3. **Engagement:** do they want to keep playing? (short survey, voluntary extra play time)
4. **Design:** which mechanics help or hurt (briefing, scaffolds, feedback prompts, handler modes)? This comes from
   telemetry and interviews.

This is a **single-group pre/post pilot**. With no control group it can show that students improved and liked it, but
**not** that LinguaSpy beats other methods. Say this in every write-up.

## 2. Permissions and ethics (do these first)
- [ ] **Written permission from MathemaTech** (organizer/owner) to run the session with their students.
- [ ] **If participants are under 18:** a parent/guardian consent form plus student assent. Use plain language: what
      they'll do, that it's voluntary, that they can stop anytime, and what data is collected.
- [ ] **If this is ever used for Carleton coursework or publication,** check with Carleton's Research Ethics Board
      *before* collecting data. Ethics clearance usually can't be granted retroactively. A pure product usability test
      for your own portfolio usually doesn't need REB, but ask if unsure.
- [ ] **Data:** collect anonymous participant codes only (e.g. `MT-07`). No names in game logs. Keep the code ↔ name
      sheet on paper with the organizer, or don't make one at all. Telemetry stays on the test laptops, then goes to a
      local encrypted folder. Delete it after analysis or on request.

## 3. Participants
- **Target:** 8–20 MathemaTech students with **no or minimal prior knowledge** of the chosen language. Screen with one
  question plus the pre-test; exclude anyone who scores near the ceiling.
- **Language:** one language for everyone, to keep it simple. Recommended: **French (Paris)**, the deepest content and
  most relevant in Ottawa. Alternative: Spanish (Madrid) if many already know French.
- **Record:** age range and prior-language background (aggregate only).

## 4. Materials
- A packaged Windows build of LinguaSpy (Plan Stage 9) on each laptop, with **Pilot mode** on: participant-code entry,
  telemetry on, assessment unlocked.
- **Pre-test form A / post-test form B** (counterbalanced: half get B first), built into the game's Assessment mode.
  About 15 items: 5 listening recognition, 4 meaning recognition, 3 gap-fill, 3 sentence assembly.
- **Delayed post-test** one week later: form A again (or a third form C if authored).
- **SUS questionnaire** (10 items, standard wording) plus 4 engagement questions (1–5 Likert): "I wanted to keep
  playing", "I felt nervous answering" (anxiety check), "The handler's help was useful", "I could say these phrases in
  real life". Add 2 open questions: best part, most confusing part.
- An observer sheet: timestamps of moments a student asks for help or gets stuck, and where.

## 5. Session plan (≈ 60 min)
| Time | Activity |
|---|---|
| 0–5 | Welcome, assent, enter participant code |
| 5–13 | **Pre-test** (in game, no feedback) |
| 13–48 | **Play**: Paris tutorial (First Contact) → Opération Croissant. Observer notes only, no coaching unless stuck > 2 min |
| 48–55 | **Immediate post-test** (other form) |
| 55–60 | SUS + engagement survey, 1–2 question chat |
| +7 days | **Delayed post-test** (5–8 min) plus optional Safe house review before it (log whether they did) |

## 6. Measures and analysis
- **Learning gain:** post − pre score per student. Report mean, SD, **paired t-test or Wilcoxon** (small n), and
  **Cohen's d for paired data**. Interpret against L2 within-group benchmarks (Plonsky & Oswald 2014: ≈ 0.6 small, 1.0
  medium, 1.4 large).
- **Retention:** delayed − pre, and delayed vs. immediate.
- **Usability:** mean SUS (68 is the conventional average; > 80 is good).
- **Process (telemetry):** accuracy by scaffold level, hints per item, peeks per conversation, time-to-answer, where
  cover was blown, briefing accuracy vs. field accuracy.
- **Script:** `Tools/Analysis/pilot_report.py` (Plan Stage 9) reads the exported JSONL logs plus a CSV of survey
  answers, and outputs `pilot_report.md` with tables and charts.
- **Pre-register** the hypotheses (H1: post > pre; H2: delayed > pre) in this file *before* the session, so the write-up
  can't cherry-pick.

## 7. Iteration
Run the pilot, fix the top 3 issues, and optionally run a second, smaller session. Two rounds of "tested → fixed →
retested" make a stronger story than one perfect-sounding number.

## 8. Results (fill in only from real data)
| Measure | Value |
|---|---|
| Date(s) / n | _TBD_ |
| Pre-test mean (SD) | _TBD_ |
| Immediate post mean (SD), d | _TBD_ |
| Delayed post mean (SD), d | _TBD_ |
| SUS mean | _TBD_ |
| "Wanted to keep playing" (% agree) | _TBD_ |
| Top 3 issues found → fixes | _TBD_ |

## 9. How to describe it (by stage)
- **Before the pilot:** "Designed a pre/post pilot study (parallel test forms, delayed retention test, SUS) with
  in-game anonymous telemetry; pilot with MathemaTech students scheduled for [month]."
- **After the pilot:** "Piloted with [n] MathemaTech students: mean test score rose from [x]% to [y]% (d = [z]), [w]%
  retained after one week; SUS [s]." Use only the real numbers. If results are mixed, say what you changed in response.
  Iterating on mixed results is a good interview story.
