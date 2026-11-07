# Visual polish log (branch `visual-polish`)

Study notes for the visual-polish pass on LinguaSpy: what changed, why, and how to explain it in an interview.
Plan: `C:\GameDevCareer\plans\LinguaSpy_visual_polish_plan.md` (quick wins Q1–Q7, look presets M1).
Before/after pairs (same camera, same build except for the change): `C:\GameDevCareer\trailers\linguaspy\polish\before_after\`.

> **Build status (read this first).** The C++ half of this branch (bug fixes 1, 4–8, camera defaults in C++, the
> look-preset system and 4 new automation tests) is **written but not compiled**: this PC's Visual Studio C++ workload
> and Windows SDK are gone (`C:\Program Files (x86)\Windows Kits\10` is missing, VS 18 has no `VC\` folder, `vswhere`
> finds no instance; UBT: "Platform Win64 is not a valid platform to build. Sdk: not found"). Everything that is
> content or Python (camera via the Blueprint, PP_Toon, materials, lighting, street detail, menu, nav fixes) is done,
> in the maps, and verified in the running game with the existing binaries. See "To finish" at the end.

---

## 1. Camera reframe (Q1)

**What.** `BP_SpyCharacter` boom: arm 800 → 2500 cm, pitch -60 → -37, **yaw 0 → 90**, target offset +100 cm Y,
camera lag 6, FOV 90 → 40. Occlusion fade: sweep radius 60 → 200, max fade 0.85 → 1.0. The same defaults are in
`PersonalCharacter.cpp` / `OcclusionFadeComponent.h` (unbuilt) so a rebuilt C++ class matches the Blueprint.

**Why.** Streets run along X and every shopfront faces ±Y. A yaw-0 camera looked *down* the street, so facades
were edge-on slivers and ~55% of the frame was road. Yaw 90 looks *across* the street at the north row; a long lens
(40°) removes the 90° wide-angle splay and gives the "diorama" read. The camera-to-pawn distance is now 25 m, so the
south row sits between camera and street; with MaxFade 0.85 its 15% dither residue laid a grey veil over the whole
street (visible in the first "after" captures), hence MaxFade 1.0 and a wider sweep.

**How I picked the numbers.** `polish/tools/polish_extra.py sc_camtest` rendered six candidate boom settings in Paris
and Rome (`polish/camtest/`); -50/2000/38 (the plan's starting point) still showed mostly pavement, -35/3000/42 showed
two floors of facade but tiny characters; -37/2500/40 with a small north offset is the compromise.

**Talking point.** Camera framing is a data problem first: compute where the top-edge ray hits the facade plane
(`tan(pitch - vFOV/2)`), don't guess. Changing the CDO in Python + recompiling the Blueprint was enough; the C++
default is kept in sync for when the class is rebuilt.

## 2. PP_Toon v2: the investigation (Q2)

`Tools/Content/pp_toon_v2.py` edits the existing material's Custom node **in place** (never `build_pp_toon()`, which
deletes and recreates the asset and would break the blendable reference in every map's ToonPostProcess volume).

### 2.1 Why every city looked the same (light hue)
v1 computed `light = avg(sceneColor / albedo)` and output `albedo * band * LitLevel`. That keeps only the *luminance*
of the incoming light. The sun colour, sky colour and fog colour authored per city in `cities.py` were divided out
and never reached the screen; then a single global blue `ShadowTint` was applied everywhere. So Paris, Madrid, Tokyo
and Rome had identical grey-lavender light no matter how they were lit.

v2 splits the light into **brightness** (drives the band) and **hue** (kept):
```
lightRGB = col / albedo;  lum = avg(lightRGB);  light = lum / LightNorm
band     = soft 3-step of light (smoothstep, BandSoftness)
hue      = clamp(lightRGB / lum, 0, 2)                 // chroma of sun/sky at this pixel
toon     = albedo * band * LitLevel * lerp(1, hue, LightHueKeep)
```
Sunlit pixels now carry the sun's colour, shaded pixels the sky's: warm/cool splits for free. `LightNorm` decouples
"how bright is a lit surface" from the sun intensity, so a 2.2 lux dusk and an 8 lux noon both band correctly.

### 2.2 Emissive pass-through
Anything emissive has `col/albedo` far above the lit level, and v1 clamped it back to `albedo * 1` (no glow possible).
v2: `if (light > EmissiveThreshold) toon = col;` so lanterns and vending machines keep their glow (and feed bloom).

### 2.3 The outline fix: Laplacian of 1/depth
v1 drew an edge where the discrete Laplacian of **linear depth** was large:
`edgeD = |dL + dR - 2d| + |dU + dD - 2d|` (relative to `d`). On a plane seen at an angle, view depth along a screen
row is *not* linear: for a plane, `1/z` is an affine function of screen position (that is why GPUs interpolate
`1/w`), so `z = 1/(a·x + b)` is a curve and its second difference is non-zero. The steeper the angle (walls,
parapets, the ground near the horizon), the larger that error. Result: vertical hatching and streaks on every
grazing wall.

v2 takes the Laplacian of `1/z` instead, normalised by `d` so it is dimensionless:
```
edgeD = (|d/dL + d/dR - 2| + |d/dU + d/dD - 2|) / DepthSensitivity
```
On any plane this is ~0 (up to float noise), at a silhouette it is large (neighbour is far away), at a crease it is
non-zero (the plane changes). Same cost, no false edges. `DepthSensitivity` changed meaning (relative curvature of
1/z) and was retuned to 0.02.

Outline **width** now scales with the render height: `Thickness * max(ViewSize.y / OutlineRefHeight, 1)`, so lines
don't thin out at 1440p/4K.

### 2.4 Depth haze
`toon = lerp(toon, HazeColor, saturate((depth - HazeStart) / HazeRange) * HazeMax)`: cheap aerial perspective for a
camera that never sees the horizon; per-city haze colour.

**Talking points.** (1) Debugged by reading the shader, not by tweaking numbers: "why do four differently lit cities
look identical?" → the math divides the light colour out. (2) The 1/z argument is the same reason perspective-correct
interpolation uses 1/w. (3) Kept v1 in `art_setup.py` (`PP_CODE`) for reference; v2 is reproducible by script.

**Needs a human eye:** outline strength (`DepthSensitivity` 0.02, `Thickness` 1.5, `NormalThreshold` 0.35) is tuned
on four stills only.

## 3. Per-city time of day and grading (Q3) + look presets (M1)

`greybox_kit.apply_look(CITY)` edits the existing Lighting actors (no `build_city_map()`, which destroys every actor):
sun rotation/colour/intensity, sky light, height fog, the ToonPostProcess volume's grading (white balance,
saturation/contrast, shadow gain, bloom, vignette, AO) and tilt-shift DOF, plus a per-city
`MI_PP_Toon_<City>` (instance of PP_Toon with the city's toon parameters; the shared material stays generic).
Values live in `cities.py` (`CITY["look"]`), the single source of truth.

| City | Mood | Sun pitch/yaw, colour, lux | Notes |
|---|---|---|---|
| Paris | soft gold morning | -30/120, FFE6C4, 6 | lilac shadows (`ShadowTint` C9C0EA), lilac haze, zinc roofs |
| Madrid | white-hot afternoon | -66/30, FFF4E0, 8 | high sun, short violet shadows, contrast 1.12, terracotta roofs |
| Tokyo | blue-hour dusk | -10/70, FFC2A0, 2.2 | strong blue sky, WB 4800 (cooler), lanterns + vending machines glow |
| Rome | golden hour | -20/60, FFD199, 6 | long amber shadows across the piazza, WB 6900 (warmer) |

(UE's `WhiteTemp` is the temperature the camera treats as white: above 6500 the image gets warmer, below it cooler.)

**M1, the C++ preset system (written, unbuilt).** `Source/Personal/LinguaSpy/Look/CityLook.h/.cpp`:
`FCityLook` (plain data: sun, sky, fog, grading, DOF, `TMap<FName,float>` toon scalars + `TMap<FName,FLinearColor>`
toon colours), `UCityLookAsset` (DataAsset, referenced from `ULanguagePackAsset::Look`), `UCityLookLibrary::BlendLooks`
(pure: linear blend, colours in linear space, sun rotation by quaternion slerp so 350°→10° goes through 0°, map keys
present on one side keep that side's value, alpha clamped) and `UCityLookSubsystem` (world subsystem:
`ApplyLook` finds the sun / sky light / fog / unbound PPV, swaps PP_Toon for a dynamic instance and sets parameters;
`BlendTo(Target, Seconds)` eases between looks every tick). `APersonalGameMode::StartPlay` applies the pack's look.
Test: `LinguaSpy.Look.Blend`. Once built, the next step is a script that writes `DA_Look_<City>` from the same
`cities.py` dicts (the maps already hold the same values, so nothing changes visually until a look is blended,
e.g. per-mission moods or a dusk transition).

**Talking point.** Data-driven presentation mirrors the existing data-driven content (packs, missions): the same
dict feeds the editor tool and the runtime asset; the blend is a pure function with an automation test.

## 4. Materials: grounding, variation, roofs, paving, glow (Q4, Q6)

`Tools/Content/toon_master_v2.py` adds one Custom node ("ToonSurface") to `M_ToonMaster` between the vertex-colour/tint
lerp and Base Color, plus an `Emissive` scalar:
- **Contact darkening on walls only:** `lerp(ContactDark, 1, height/ContactHeight)` weighted by `1 - |N.z|`, so the
  bottom ~1.8 m of facades and props darkens and they sit *on* the street, while floors (road/sidewalk slabs, also
  M_ToonMaster) are untouched.
- **Per-object variation:** a hash of the object position jitters brightness/warmth ±5%: repeated kit buildings
  stop looking cloned.
- **Roof tint:** up-facing, untinted (vertex alpha 0), > 5 m surfaces take `RoofTint * RoofAmount`; per-city values
  in `MI_Tint_<HEX>_<City>` children (Paris zinc, Madrid/Rome terracotta, Tokyo kawara), no Blender re-export.
- **Paving:** a world-aligned running-bond grid on up-facing surfaces (`PavingScale/Joint/Strength`, off by default),
  switched on through `MI_Tint_<HEX>_Pave<cm>`: cobbles on the Paris/Rome roads, plaza tiles, sidewalk slabs.
- **Glow:** `MI_Glow_<HEX>_<x10>` (Tokyo lanterns 1.2, vending machines 1.0), passed through by PP_Toon v2.

`greybox_kit.apply_street_detail(CITY)` (Q6): per-city road colour (Paris warm cobble, Madrid sand, Tokyo dark
asphalt, Rome basalt), centre dashes, crosswalks, kerb strips. Markings are 1 cm slabs at +3 cm (the z-fight rule)
with the NoCollision profile.

## 5. Tilt-shift DOF (Q5)

Volume DOF: focal 2500 cm (= arm length), f/1.0, **sensor 180 mm**. With a 40° FOV the focal length scales with the
sensor width and blur grows with focal length², so a "medium format" sensor is what gives a miniature look at 25 m
(at 36 mm and f/1.4 the blur was under a pixel). The C++ character (unbuilt) also updates the focal distance every
tick to the camera-to-spy distance.

## 6. Menu (Q7)

`greybox_kit.apply_menu_look()`: the diorama is a parchment tabletop with a wooden edge in warm haze (fog + PP haze
via `MI_PP_Toon_MainMenu`), per-city roof colours by quadrant, warm grading. Camera motion and a title treatment
(plan L5/M11) are not done.

## 7. Playtest bug fixes

| # | Bug | Root cause | Fix | State |
|---|---|---|---|---|
| 1 | Briefing opens over a ground-level view with a black shape (Paris) | The controller opened the briefing in `BeginPlay` and paused the game immediately. While paused, `UWorld::Tick` skips `UpdateCameraManager` (LevelTick.cpp: only `if (!bIsPaused ...)`), so the view stayed at `APlayerCameraManager::InitializeFor`'s first update: the controller's own spawn location, at ground level. | Briefing waits until the camera manager has rendered the pawn's view for 2 frames (`PawnViewFrames`, retried from `PlayerTick`), then forces `UpdateCamera(0)` before pausing. | C++, unbuilt |
| 2 | Paris Seine: pawns walk "into the river" and get stuck near (4317, 332) | `cities.py` asked for `collide=False` on the Seine slab, but `place()` used `set_collision_enabled(NO_COLLISION)`, which was not saved with the map: the slab kept full collision, so Recast built a walkable **navmesh island on the water** between the quais (verified: projection at (4100, ±1000) hit navmesh; path to the road was partial). The (4317, 332) spot is the deck edge pressed against the parapet. | `apply_nav_fixes`: two NavModifierVolumes (NavArea_Null) over the water north/south of the deck; `place(collide=False)` now uses the serialized NoCollision profile + `can_ever_affect_navigation=False`. Verified in game: no navmesh on the river, holding the mouse north stops at the parapet (y 333), click-to-move from there back to the road works. | Done, verified |
| 3 | Tokyo "Walk to the shrine gate": no nav path to (2250, -1000) | `SM_Tokyo_Torii` had one auto-generated convex hull: a solid 680 × 70 × 557 cm slab. The gate was a wall and the marker sat inside it, in a navmesh hole. Setting complex-as-simple alone did not help until the simple collision was removed (the cached nav collision came from the hull). | Torii: simple collision removed, complex-as-simple (`cities.py complex_collision`; `set_complex_collision` now also removes simple collision). Verified: only the pillars cut the navmesh; editor path query from the player start reaches the marker. | Done |
| 4 | Quick-check counter grows after a miss (1/16 → 2/17) | A missed item is appended once to the quiz array (by design, "comes back at the end"), and the counter used the live array length as the total. | Pure `UBriefingWidget::GetQuizStep`: first round counts against the first-round size; retries show as "Second look (k / n)". Test `LinguaSpy.Briefing.QuizStep`. | C++, unbuilt |
| 5 | MISSION COMPLETE banner and next-mission banner overlap the debrief panel | Completion broadcasts `OnMissionCompleted` (9 s banner), immediately starts the debrief conversation (panel), then `StartNextMission` (6 s intro banner queued). Banners had no notion of other UI. | HUD banners are *held* (hidden, timer frozen, new ones queued) while a conversation/debrief is active or the game is paused (briefing, field note, pause menu); the mission intro banner is skipped when a briefing will introduce the mission; the next briefing waits until banners are done. Order is now: debrief → grade banner → next briefing. | C++, unbuilt |
| 6 | City card "Missions 1/2 - grades A" wraps, leaving "A" alone | 220 px text column, one line with normal spaces; auto-wrap broke before the last word. | Grades on their own line, "Best grade: A" with non-breaking spaces (and "·" between several). | C++, unbuilt |
| 7 | Japanese pick-the-sentence hint: "The phrase has 4 words" | `SubmitRecognition` grades with `SubmittedTokens = 0` and `FirstWrongIndex = 0`, so `ChooseHint` fell through to the tile-building length hint ("words" also wrong for unspaced chunk tiles). | `ChooseHint` takes the scaffold level and the language's word spacing: Recognize gets its own hint (no token hints), unspaced languages count tiles. Test `LinguaSpy.Feedback.HintLevels`. | C++, unbuilt |
| 8 | Tally "0 mistakes, 2 translations peeked" after 1 wrong attempt + 1 peek | **Confirmed.** By design a pass after the handler's hint counts as help (it was added to `Peeks`), so the debrief reported the hint-assisted retry as a "translation peeked"; "0 mistakes" is correct (only a final miss is a mistake). | `FMissionGrade.Retries` (save v4, no-op migration step); `UFluencyScoring::ClassifyAnswer` (pure) buckets each final answer; grade = `ComputeGrade(C, M, Peeks + Retries)`, i.e. **unchanged**; debrief now reads "0 mistakes, 1 fixed after a hint, 1 translation peeked". Test `LinguaSpy.FluencyTally`. | C++, unbuilt |

## 8. To finish (needs the student)

1. Reinstall the **Desktop development with C++** workload in Visual Studio 2026 (MSVC 14.5x + Windows 11 SDK).
2. Close the editor, build: `Build.bat PersonalEditor Win64 Development -Project=... -WaitMutex`. Fix any compile
   error (the code was reviewed by hand but never compiled).
3. Run `UnrealEditor-Cmd.exe Personal.uproject -ExecCmds="Automation RunTests LinguaSpy; Quit" -unattended -nullrhi`:
   expect the 19 existing tests plus `LinguaSpy.Look.Blend`, `LinguaSpy.Briefing.QuizStep`, `LinguaSpy.FluencyTally`,
   `LinguaSpy.Feedback.HintLevels`.
4. Play Paris from a fresh profile: briefing over the correct top-down view; complete First Contact: debrief, then
   the grade banner, then the Croissant briefing (no overlap).
5. Art direction: approve or retune the four moods (Tokyo is the boldest) and the outline strength.
