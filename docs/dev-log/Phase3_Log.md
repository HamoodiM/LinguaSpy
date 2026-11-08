# Phase 3 log: decisions made during execution

- **Characters:** segmented "figurine" style, where each chunky low-poly part is rigidly bound to one mannequin bone. This avoids auto-weighting problems, and all mannequin animations work unchanged. There are 7 meshes (spy, barista, vendor, elder, young, tourist, worker), with colors in vertex colors.
- **FBX scale bug:** the first character import had a ×100 scale on the root bone, which made characters huge while walking. It was fixed with the "Armature + real root bone + cm units" export recipe, which is documented in CLAUDE.md.
- **Kits:** whole-building meshes (not facade modules) with tintable walls (vertex color alpha = 1). One mesh gives many pastel variations through `MI_Tint_<HEX>`.
- **Toon look:** a post-process (`PP_Toon`) does the cel bands and outlines, and a single master material (`M_ToonMaster`) is used for all art. Occlusion fade uses Custom Primitive Data, so there are no dynamic material instances.
- **Maps** are generated from `Tools/Greybox/cities.py` layouts, so they're reproducible. `Lvl_Paris` replaces `Lvl_Paris_Greybox`. The Seine was raised above the sidewalks to fix z-fighting, and the bridge deck was raised above the water.
- **Paris:** First Contact stays as the tutorial; Opération Croissant is mission 2. Camille and Marcel switch dialogue per mission via `RequiredMission`.
- **Madrid, Tokyo, Rome:** one 3-conversation mission each (bar/shop → directions → reach a marker → code phrase with a contact). The metro arch mesh from the Paris kit is reused in Madrid.
- **Audio:** procedural MetaSounds only. UI sounds are enveloped oscillators; ambience is low-passed noise plus a quiet chord pad per city; the menu has a pad. There is no separate music track per city yet.
- **Verified:** scripted PIE runs complete every mission (both Croissant branches, plus a mission-failed restart), and the menu shows 4 playable cities.
