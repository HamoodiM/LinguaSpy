# Credits (audio, voices, fonts)

Every sound in `/Game/LinguaSpy/Audio/CC0/` comes from a public **CC0 (public domain)** release. No attribution is required, but we credit the authors anyway.

The source archives were downloaded into `Saved/AudioSources/`, which git ignores. `Tools/Content/audio_import_cc0.py` re-imports the files from there.

| Used for | File | Pack / page | Author | License |
|---|---|---|---|---|
| UI sounds (click, tile, pass, fail, menu select) | `kenney_interface-sounds/Audio/*.ogg` | [Interface Sounds](https://kenney.nl/assets/interface-sounds) | Kenney (kenney.nl) | CC0 |
| UI sounds (alternates) | `kenney_ui-audio/Audio/*.ogg` | [UI Audio](https://kenney.nl/assets/ui-audio) | Kenney (kenney.nl) | CC0 |
| Mission complete / cover blown stingers | `kenney_music-jingles/Audio/*` | [Music Jingles](https://kenney.nl/assets/music-jingles) | Kenney (kenney.nl) | CC0 |
| City music | `busy_but_calm_street_0.ogg` | [Busy but Calm Street](https://opengameart.org/content/busy-but-calm-street) | SkyleTheFrench | CC0 |
| Menu music | `Ambient-Loop-isaiah658.ogg` | [Ambient Relaxing Loop](https://opengameart.org/content/ambient-relaxing-loop) | isaiah658 | CC0 |
| Street ambience bed | `sfx_100_v2/sfx100v2_loop_*.ogg` | [100 CC0 SFX #2](https://opengameart.org/content/100-cc0-sfx-2) | rubberduck | CC0 |

The procedural MetaSounds from Phase 3 (`Tools/Content/audio_setup.py`) have been retired. They were sustained sine and triangle chords with no envelope, which the user heard as a harsh ringing.

## Voices (Phase 5)
Spoken lines are generated offline at authoring time by `Tools/Content/voice_build.py`. The shipped game only plays
the imported SoundWaves (`/Game/LinguaSpy/Audio/Voice/`); no speech engine runs at runtime.

| Use | Voice model | Source data | License |
|---|---|---|---|
| Handler: briefings, model answers, test items | [fr_FR-siwis-medium](https://huggingface.co/rhasspy/piper-voices/tree/main/fr/fr_FR/siwis/medium) | SIWIS French Speech Synthesis Database (Honnet, Lazaridis, Garner, Yamagishi; University of Edinburgh) | CC-BY 4.0 |
| NPC voices (one speaker per character) | [fr_FR-mls-medium](https://huggingface.co/rhasspy/piper-voices/tree/main/fr/fr_FR/mls/medium) | Multilingual LibriSpeech, French (Pratap et al., 2020; openslr.org/94) | CC-BY 4.0 |
| Synthesis tool (not shipped) | [Piper](https://github.com/rhasspy/piper) 2023.11.14-2 | Michael Hansen / Rhasspy | MIT |

Voices that were checked and **not** used: fr_FR-tom (AGPLv3), fr_FR-upmc (CC-BY-SA 4.0, share-alike).
