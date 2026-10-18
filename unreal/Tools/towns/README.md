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
  SM_Cactus, SM_Reeds, SM_FlowerShrub, SM_SnowPatch, SM_Birch, SM_Stairs200…600, SM_Ladder600/900, SM_WhiteBridge,
  SM_RandHouse = the imported `rand_house` re-merged with a half turn). Material slots follow the first-appearance
  order of the part colours; `kit_entry(name, {"P_Plaster":"P_White"})` recolours per town. Every building's door is
  on -X and house slots point +X away from the street, so doors face the road; `wot.Mesh.Dump <Mesh>` (editor console)
  prints per-section vertex centroids to check a mesh's door side.
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
  heightmap (`blender/overworld-full.png`, which the landscape was sculpted from) and writes `ow_sites.json` (all 11 towns);
  `ow_portals.py` places or moves a portal and a tagged PlayerStart for every town in that file, plus a generic landmark
  mesh for each town except Tar Valon and Tear, whose hand-placed landmarks (the White Tower, Dragonmount, the Stone)
  are moved to the fitted sites instead. Overworld axes: +X east, +Y south.
* `run_town.py <Town>...|--all|--dry|--kit [SM_Name ...]|--overworld` – regenerate towns, dry-run the relief locally, rebuild the kit,
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

## Shared PCG subgraphs (pcg_subgraphs.py)

The full picture (pipeline, specs, relief, town graphs, subgraphs, recipes, diagrams) is in
[`docs/world_gen.md`](../../../docs/world_gen.md).

Every town graph is **data + spawners only**: `Create Points` nodes carry the town's outlines, paths, slots, lines,
furniture, terrain boxes and lattice, and `Static Mesh Spawner` nodes carry its mesh/material entries. All point
processing lives in five shared subgraphs under `/Game/Wot/Towns/Common/PCG`, built once with
`run_town.py --subgraphs` and referenced by every town, so fixing one subgraph fixes every level on its next rebuild:

| subgraph | in | out | parameters (overridden per subgraph node) |
|---|---|---|---|
| `PCGS_Plaza` | Outline | Surface (exclusion shape), Tiles | bLinear |
| `PCGS_Road` | Path, Row | Tiles, Spline, Samples | bClosedLoop, bLinear, DistanceIncrement, KeepRatio |
| `PCGS_Houses` | Slots, Exclusions | Houses (with footprint bounds) | OffsetMin/Max, RotationMin/Max, ScaleMin/Max, Ratio, CoreMin/Max, FootMin/Max |
| `PCGS_Lattice` | Lattice, Center, Exclusions | Ground (density = distance) | OffsetMin/Max, MaximumDistance |
| `PCGS_Scatter` | Ground | Points | LowerBound, UpperBound, Ratio, RotationMax, ScaleMin/Max, BoundsMin/Max (prune) |

Graphs are rebuilt **in place** (`pcg_lib.create_graph` clears nodes, recorded comment boxes and parameters): deleting a
graph a loaded level references and recreating it under the same name leaves the new asset unsaved. After any rebuild
check that `Content/Wot/Towns/<Town>/PCG_<Town>.uasset` exists. `pcg_graph_render.py <graph> <out.png> [scale]` draws a
graph from its live structure (used for `docs/world_gen.md`).

Inside a subgraph a parameter is a `Get Graph Parameter` node wired to the target node's **Overrides** pin, emitting an
attribute named after the property it overrides (so a parameter that overrides `BoundsMin` on two different nodes is
declared twice with `attr="BoundsMin"`). `pcg_lib.SubG` wraps that (`inputs`, `outputs`, `param`, `bind`). Town graphs
are laid out in boxed, commented sections (exclusions & plazas, roads/water/walls, street lines & furniture & towers,
houses, terrain, lattice & foliage) and carry a description naming the spec and command that generated them.

## Placement rules (relief.py)

Roads, plazas and water are painted into `Relief.cellmat`, and `road_props` / `lattice` use it as the single source of
truth: street furniture is dropped on pavement, inside a house slot or near a staircase and thinned to half its spacing;
house slots whose footprint touches a crossing road are dropped and thinned against other streets; the foliage lattice
skips paved cells and their neighbours, doorways, lamp spots and stairs. Tunables per spec: `slot_clear`, `slot_spacing`,
`slot_stair_clear`, `line_clear`, `line_house_clear`. Never patch PCG `MeshEntries` in place from `get_properties`
(unloaded soft material references read back as None and wipe the overrides) — change `pcg_lib` and rebuild.

Everything that takes up ground goes through one **occupancy registry** (`Relief.occupy` / `_occupied`), in this order:

1. hand-placed props claim their footprint first (`register_props`: radius by label/asset via `FOOT_RADIUS`, buildings an
   oriented rectangle, landmarks that already have pads/exclusions none, NPCs only their spawn capsule);
2. roads, plazas, stairs (keep-outs) and Waygate pads are carved;
3. house slots and street lines (lamp posts, tree rows, fences) are thinned against pavement, stairs, doorways and the
   registry; lines start half a step along the road so nothing sits on a junction corner;
4. **plaza furniture** (`plaza_furniture`) replaces the old random `scatter`: lamp posts at the inset corners / along the
   edges (or around the rim of a round plaza) and around a central feature; market stalls (`SM_MarketStall`, 250 deep,
   400 wide, open front on -X) in rows along the edges facing the middle, back-to-back rows down the middle of wide
   plazas, and a ring around a well / monument in the middle, alternating the colour variants listed in `scatter`; an
   obelisk in the middle; anything else (ruin walls, boulders, birches) on a jittered grid. A seat is taken only when its
   footprint is free: not on a road running through the plaza, not within reach of a street mouth (where a road crosses
   the plaza outline), not on another footprint. The stall `ratio` in `scatter` only sets how tightly rows are packed;
5. the foliage lattice skips every registered footprint.

The rules are shared by every town; a plaza with a street through it or a prop cluster in it simply seats fewer stalls.
`plaza_debug.py <Town> [plaza index]` renders a plaza (roads, mouths, footprints, slots, line props, keep-outs and the
furniture placed) to `<Town>_plaza<i>.png` for tuning without the editor; `run_town.py --dry` prints the counts.

## Save / load and menus (C++)

`Save/WotSaveSubsystem` writes `UWotGameSave` slots (level, player transform, health, inventory, quests, story flags,
time of day, playtime) and applies a loaded slot once the player pawn is possessed; portals autosave. The menus
(`UI/WotMenuWidget`, `UI/WotGameMenus`: pause, save/load, main) and the inventory screen (`UI/WotInventoryScreen`) are
built in C++ with a shared style; Esc / P / gamepad Start opens the pause menu. Console: `wot.Save`, `wot.Load`,
`wot.Saves`, `wot.NewGame`, `wot.Menu.Pause|Main|Save|Load|Inventory|CloseAll`.

## Stairs and placement rules (shared by every town)

All of this lives in `relief.py` and applies to every level built from a spec; specs only describe roads, terrain and
hand-placed props. Stairs are placed in `carve_roads` where a road meets a terrace edge (a jump of at least 4 steps):

- the climb axis is the axis along which the ground actually steps, and the top edge snaps to the cell boundary where
  it does (searching three cells either side of the road's crossing), so the top of a flight is flush with the wall;
- two roads crossing the same edge within a flight's width share one flight; a flight whose footprint would land on
  another flight (terrace corners) slides along the edge until it is clear, or the road becomes a ramp;
- `finalize_stairs` runs after every carve (roads, plazas, house pads): the ground under the run is forced to the
  lower level and the cells just past the top edge to the upper level, so a flight can sink into the floor but never
  into a wall and always reaches its terrace;
- every flight registers a keep-out (`keepout`) so the lattice (trees, bushes, grass), house slots and street lines
  (lamp posts, tree lines) stay out of it, and an axis-aligned PCG exclusion that also opens the wall tiles where a
  flight crosses a city wall.

The stairs meshes carry no simple collision and use complex-as-simple: a merged kit mesh's navigation collision only
exports its convex hull (gathered when the mesh was created, i.e. empty), so with a hull the navmesh ran flat under every
flight and nobody could path up a terrace; with the step triangles both physics and the navmesh climb them. After any town
rebuild run `Tools/playtest/nav_rebuild_all.py`: the runtime navmesh starts from the saved tiles and does not regenerate
stale ones.

`audit_stairs.py [Town ...] [-v]` rebuilds the specs locally (no editor) and reports, per flight, ground above the lower
level under the run, a missing upper level past the top, and lattice / house-slot / street-line / wall-road overlaps.
