# Pilot kit

Everything a facilitator needs to run the LinguaSpy pilot described in `docs/Pilot_Study_Protocol.md`.

| File | Use |
|---|---|
| `Facilitator_Script.md` | What to say and do, minute by minute |
| `Laptop_Setup.md` | Installing the build, pilot mode, collecting the logs |
| `Consent_Templates.md` | Organizer permission, parent/guardian consent, student assent. **Templates only, not legal advice**: the organizer adapts them. |
| `Survey.md` | SUS + engagement questions (paper or Google Form) and the CSV format the analysis script expects |
| `Observer_Sheet.md` | Printable sheet for noting where students get stuck |

**After the session**
1. Copy every laptop's telemetry folder into one place (see `Laptop_Setup.md`).
2. Type the survey answers into `survey.csv`.
3. Run:
   `python Tools/Analysis/pilot_report.py --logs <folder> --survey survey.csv --out pilot_report`

Report only what that output says, and fill in section 8 of the protocol.
