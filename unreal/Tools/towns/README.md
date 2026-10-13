# Town generation tooling

These scripts drive the Unreal 5.8 editor through its built-in **Model Context Protocol** server
(plugins `ModelContextProtocol`, `EditorToolset`, `PCGToolset`, enabled in `VoxelRPG.uproject`).
Launch the editor with `-ModelContextProtocolStartServer -ModelContextProtocolPort=8000`.

* `mcp.py` – JSON-RPC client for the editor's MCP endpoint (`list`, `schema <tool>`, `call call_tool '<json>'`).
* `pcg_lib.py` – helpers for building PCG graphs (nodes, edges, weighted mesh entries, points).
* `town_builder.py` – turns a *spec* into one PCG graph per town: tiled roads, walls with towers and
  gates (cut where roads cross, optional gaps), fences, piers, canals, plazas with scattered props,
  street furniture lines (lamps, tree rows, reeds), houses along streets or at hand-placed slots with
  per-district house sets (cramped alleys vs grand avenues), distance-banded foliage plus extra
  scatter bands (boulders, snow, cacti, rubble). Then places landmark props, wires the level's
  portal/player-start tags and scales the level/nav bounds.
* `kit_builder.py` – procedural voxel-style building and foliage kit assembled from primitives and merged
  into static meshes under `/Game/Wot/Towns/Common/Buildings` (SM_GableHouse, SM_TowerHouse,
  SM_DomeHouse, SM_CourtyardHouse, SM_Keep, SM_Longhouse, SM_MarketStall, SM_StoneWell, SM_LampPost,
  SM_Obelisk, SM_GateHouse, SM_RuinWall, SM_BridgeArch, SM_Pine, SM_Palm, SM_DeadTree, SM_Boulder,
  SM_Cactus, SM_Reeds, SM_FlowerShrub, SM_SnowPatch, SM_Birch, SM_Stairs200…600, SM_Ladder600/900, SM_WhiteBridge). Material slots follow the first-appearance
  order of the part colours; `kit_entry(name, {"P_Plaster":"P_White"})` recolours per town.
* `relief.py` – deterministic voxel heightfield per town. A `Relief` is a sum of continuous features (hills, ridges,
  valleys/rivers, slopes, value noise) plus plateaus (flat terraces with optional rim blending; rim 0 = cliff),
  quantised to 40 cm steps (the character's MaxStepHeight is 45) on a 250 cm cell grid. Roads are then *carved*: each
  road gets a walkable profile (at most one step per cell, so hills become ramps cut into or built up from the ground),
  and wherever a road meets a terrace edge of 160–600 cm a kit staircase is placed automatically (rise snapped to
  SM_Stairs200…600). Water-kind roads become channels (filled by the per-town water plane, or drawn with water tiles when
  there is none), walls just follow the ground, piers/bridges keep their own z (`nocarve`, `rel`). Plazas and house slots
  (same spacing/setback as the PCG samplers) are flattened into pads, landmark props get `pads`. Output: run-merged terrain
  boxes spawned by the PCG graph (grass/rock/sand/snow palette, rock columns under cliff tops), a height-aware foliage
  lattice, and `zat(x,y)` which lifts every prop, NPC and portal onto the ground. Also the NPC/dialogue/chest helpers.
* `specs_common.py`, `specs_a.py` (Emond's Field, Baerlon, Whitebridge, Shadar Logoth, Fal Dara), `specs_b.py`
  (Caemlyn, Cairhien, Tear, Tar Valon, Tanchico, Illian) – the town specs: roads, districts (dense alley sets vs grand
  avenues), plazas, house kits, foliage, props, NPCs with dialogue rows (written into `DT_HybridRPG_Dialogue`;
  `unreal/DT_HybridRPG_Dialogue.csv` mirrors them) and the per-town relief.
* `ow_fit.py` – fits the Randland reference map (`maps/westlands-national-borders-and-cities.png`) onto the Overworld
  heightmap (`blender/overworld-full.png`, which the landscape was sculpted from) and writes `ow_sites.json`; `ow_portals.py`
  places/moves the Overworld portals, player starts and landmarks there. Overworld axes: +X east, +Y south.
* `run_town.py <Town>...|--all|--dry|--kit|--overworld` – regenerate towns, dry-run the relief locally, rebuild the kit,
  or re-place the Overworld portals.

Portals: the Portal blueprint now has a `ToLevel` string (used when set, otherwise the old
`ToMap` enum). Overworld portals are named `Overworld<Town>` and each town's return portal
`<Town>Overworld`; the arriving level picks the PlayerStart whose tag matches the portal name.
Palette material instances (`P_*`, `MI_Ground_*`) under `/Game/Wot/Towns/Common/Materials` take
linear colour values (display colour ^ 2.2).

## Collision, navigation and play-testing

PCG spawner descriptors default to **NoCollision** and `bForceNavigationObstacle`; `pcg_lib.collision_descriptor`
now gives every blocking mesh `BlockAll` collision and real navigation geometry (navmesh on top of terrain cubes,
tiles, stairs and bridges, carved around buildings and trees; ground cover such as grass/reeds/flowers stays
non-colliding). The kit meshes carry convex simple collision (`StaticMeshTools.generate_convex_collisions`) so the
navmesh sees them. `DefaultEngine.ini` raises the Recast agent step height to 45 cm to match the 40 cm terraces.
When relief terrain is present the flat `TownGround` plane is no longer placed.

The C++ module has a play-test harness (`Source/VoxelRPG/.../Testing/WotPlayTestSubsystem`). Because the MCP
server has no console-exec tool, the subsystem polls `Saved/PlayTest/commands.txt` (PIE/game worlds) and
`Saved/PlayTest/commands_editor.txt` (editor world) every half second and runs each line as a console command:

* `wot.PlayTest.Spawn` – where the player spawned, ground under it, NPC ground clearance, navmesh state.
* `wot.PlayTest.Walk [N] [Seconds] [Interact]` – path (navmesh) to the N nearest NPCs, jump when wedged, report
  REACHED / UNREACHED / STUCK (with what is around the capsule), talk through dialogue lines when Interact=1.
* `wot.PlayTest.Goto X Y`, `wot.PlayTest.Stop`, `wot.PlayTest.Status`
* `wot.Nav.Rebuild`, `wot.Nav.Status`, `wot.Nav.Probe X Y ...` – rebuild / inspect the navmesh.

Everything is logged under `LogWotTest` with a `[PT]` prefix and read back through `LogsToolset.GetLogEntries`.
`EditorAppToolset.StartPIE/StopPIE` drive the session (see the scratch `pietest.py` flow: load level → StartPIE →
write commands → poll the log for `[PT] SUMMARY`).

## Main cast, quests and mounts

* `relief.hero(label, name, row, x, y, yaw, radius, tags)` places a main-character NPC Blueprint from
  `/Game/Wot/NPCs/Main/<Label>_NPC` (Moiraine, Egwene, Nynaeve, Mat, Perrin, Thom; meshes made in Blender on the
  Rand medium-flat rig under `/Game/Wot/Characters/Main`). Actor tags `QuestStart=<QuestId>` and `QuestName=<Name>`
  are read by the quest subsystem.
* Quests are `UWotQuestDefinition` data assets under `/Game/Wot/Quests` (objectives: TalkTo, Reach, Kill, Collect,
  Custom; chained with `NextQuestId`). `UWotQuestSubsystem` (GameInstance subsystem) tracks progress, saves it to the
  `WotQuests` slot, shows the C++ tracker widget and toasts; console: `wot.Quest.List/Start/Advance/Reset`.
* Horses carry `UWotMountComponent` (interact = ride, interact again = dismount; rider input steers the horse,
  rider plays the Blender `ride_idle` / `ride_walk` poses). Animals without an AnimBP graph use
  `UWotAnimalAnimationComponent` with idle/walk/run sequences imported from `scratch anims/`.
