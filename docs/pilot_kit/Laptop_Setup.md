# Laptop setup

**Requirements:** Windows 10/11, a GPU from roughly 2016 or newer, headphones, about 2 GB free.

## Install
1. Copy the `Windows` folder of the packaged build to the laptop (for example `C:\LinguaSpy`).
2. Start the game with **`Start LinguaSpy (Pilot).bat`** in that folder. It passes `-PilotMode`.
   - Alternative: an empty file named `pilot.ini` next to the `.bat` / `Personal.exe` turns pilot mode on too.
3. On first launch the game asks for the participant code. Enter a test code (`TEST-1`) to check it works, then **reset**:
   - Main menu → Reset progress (twice).
   - Delete the profile and logs listed below, so the real participant starts clean.

## What pilot mode does
- It turns on the local learning log and asks for a participant code (letters, digits and `-`, max 8 characters).
- It shows the **Field tests** button with Before / After / One week later.
- Nothing is sent over the network. The game has no network code.

## Where the files are (packaged build)
| What | Where |
|---|---|
| Learning logs | `%LOCALAPPDATA%\Personal\Saved\Telemetry\<code>_<date-time>.jsonl` (the Field tests panel has an "open the log folder" button) |
| Save profile (progress, test results) | `%LOCALAPPDATA%\Personal\Saved\SaveGames\LinguaSpyProfile.sav` |

## After the session
1. Copy each laptop's `Telemetry` folder into one folder per session, for example `pilot_logs/laptop3/`.
2. Keep the save profiles until the one-week test is done; that test reads the same profile.
3. When the study is over, delete logs and profiles from the laptops.

## One-week test on a different laptop
Copy that student's `LinguaSpyProfile.sav` into the same place on the new laptop before launching.
