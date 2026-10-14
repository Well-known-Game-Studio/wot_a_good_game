# The Ways

Procedural generator for the Ways (`/Game/Wot/Maps/Ways/Ways`): dark stone islands floating in a void, linked by
decayed bridges, lit only by the Guidings and the Waygates, haunted by Machin Shin. Design follows the Amazon series
(see `docs/ways_and_channeling_design.md`).

    python3 run_ways.py mats        # P_WayStone, P_WayStoneDark, P_GuidingPale, P_GuidingScript (emissive)
    python3 run_ways.py kit         # SM_WayIsland(1-3), SM_WayBridge(1-2), SM_Guiding, SM_Waygate, SM_WayPillar(1-2), SM_WayRubble
    python3 run_ways.py level [seed] [islands]   # (re)generate the level: islands on a 3-D random walk, bridges, Guidings
                                                 # with lights, pillars, rubble, entry/exit Waygates with Portal + PlayerStart,
                                                 # black fog, fixed-exposure post process, Machin Shin, nav/level bounds

Kit pieces live in `towns/kit_builder.py` (`way_island`, `way_bridge`, `guiding`, `waygate`, `way_pillar`, `way_rubble`);
the level logic is `ways_builder.py` and runs inside the editor's ProgrammaticToolset like the towns (the sandbox has no
`random`, so `WotRandom` is a small deterministic LCG).

Walkability rules (checked in PIE with the `wot.PlayTest.Goto` harness, see `Tools/playtest/ways_nav.py` and
`ways_test.py`): islands and bridge spans get a single convex hull so the walking surface is one flat plane; every bridge is a
level landing that starts under the island's crumbled band (`0.86 r - 260`) and reaches past its outermost rim voxel, a
pitched middle span (rise/run at most `MAX_SLOPE` = 0.25; the island spacing in `island_layout` leaves room for it) and a
level landing on the far island, 3 cm above the island top; segments are scaled to their 3-D length so pitched decks leave
no gaps. Guidings, pillars and rubble are placed with `spot()`, which keeps them 4.6 m clear of every bridge centreline.

Entry: the Waygate in Shadar Logoth (`relief.waygate` entries in `specs_a.py`; portal `ShadarLogothWays`, arrival tag
`WaysShadarLogoth`). Exit: the far island's Waygate to Fal Dara (`WaysFalDara` / `FalDaraWays`).
`AWotMachinShin` (`Source/VoxelRPG/Ways`) is an instanced-cube murmuration animated in C++ that patrols the island centres,
hunts the player within hearing range, and drains / throws back on contact. The void has a kill-Z of -6000.
