# World generation: towns, PCG graphs and the shared subgraphs

The eleven town levels (`/Game/Wot/Maps/<Town>`) are generated, not hand-built. A Python **spec** per town describes
streets, plazas, walls, landmarks and NPCs; a deterministic **relief** pass turns that into a stepped voxel terrain with
carved roads, stairs and placement rules; and one **PCG graph** per town spawns everything as instanced meshes. The
point-processing part of every town graph lives in five **shared subgraphs**, so a fix in one of them improves every
level on its next rebuild. This document explains what the pieces do, how the graphs work, and how to build with them.

Images: rendered diagrams live in `docs/images/worldgen/` (regenerate with `pcg_graph_render.py`); the editor
screenshots are also attached to PR #139 (links in the *Gallery* section at the end).

## 1. The pipeline at a glance

```
specs_*.py  ──►  relief.py (terrain, roads, stairs, placement)  ──►  town_builder.py  ──►  PCG graph + hand-placed actors
   │                       │                                              │
   │                       └── house slots, street lines, plaza            └── Create Points data + Static Mesh Spawners
   │                           furniture, terrain boxes, lattice               + the shared PCGS_* subgraphs
   └── roads, plazas, walls, props, NPCs, dialogue, foliage bands
```

Everything runs through the editor's MCP server (launch the editor with `Tools/playtest/editor.sh start`):

| command | what it does |
|---|---|
| `python3 Tools/towns/run_town.py --subgraphs` | (re)build the five shared `PCGS_*` subgraphs in place |
| `python3 Tools/towns/run_town.py --kit` | rebuild the merged voxel kit meshes (`SM_*`: houses, stalls, lamp posts, stairs, foliage) |
| `python3 Tools/towns/run_town.py <Town> ...` / `--all` | regenerate towns: clean the level, build its PCG graph, place props and NPCs, write dialogue rows, execute the graph, save |
| `python3 Tools/towns/run_town.py --dry [Town]` | build the spec and relief locally (no editor) and print counts: boxes, lattice, stairs, lines, furniture |
| `python3 Tools/playtest/nav_rebuild_all.py [Town ...]` | rebuild and save every town's navmesh (always after a town rebuild: the runtime navmesh keeps stale saved tiles) |
| `python3 Tools/playtest/pietest.py <Town> [N] [Seconds] [Interact]` | play-test: walk to the N nearest NPCs, report REACHED / STUCK / NOPATH, talk |
| `python3 Tools/towns/plaza_debug.py <Town> [plaza]` | offline render of a plaza's placement (roads, mouths, footprints, furniture) |
| `python3 Tools/towns/pcg_graph_render.py <graph> <out.png>` | diagram of any PCG graph from its live structure |

A town rebuild takes one to four minutes; `--all` about forty. The CLI stops at the first failing town and leaves that
level open and unsaved for inspection.

## 2. Specs (`specs_a.py`, `specs_b.py`, `specs_common.py`)

A spec is a plain dict built by a function such as `caemlyn_spec()`:

* `roads`: polylines with `width` (lanes of 100 cm tiles), `kind` (`road`, `wall`, `water`, `nobuild`), `closed`,
  `linear`, `keep` (gap ratio), `houses` / `spacing` / `setback` / `sides` / `set` for house slots along it, `lines`
  (street furniture: `lamp_line`, `tree_line`, fences, reeds) and `towers` for walls. Helpers: `grid_streets`,
  `radial_streets`, `circle`, `rect`, `wiggle`, `pier_roads`.
* `plazas`: closed outlines with a pavement material and a `scatter` list `(mesh, ratio, scaleMin, scaleMax, materials)`
  that the placement rules turn into orderly furniture (stalls, lamp posts, monuments, trees).
* `houses` / `house_sets`: mesh/material entries, scale range, keep ratio, footprint; `exclusions`: boxes nothing is
  placed in; `pads`: flattened areas; `props`: hand-placed meshes, actors, NPCs (`npc`, `hero`), chests, potions,
  Waygates (`waygate`); `dialogue`: rows for the dialogue table (`lines`); `foliage`: tree lists and density bands
  (`base_foliage`, `boulders_extra`).
* The terrain: a `Relief` with hills, ridges, valleys, slopes, noise, plateaus (`peaks` for mountain ranges), finished by
  `relief_apply(spec, R, palette)`.

## 3. Relief (`relief.py`): terrain, roads, stairs, placement

`Relief` is a 250 cm cell heightfield quantised to 40 cm steps (one step is always walkable). `relief_apply` runs, in order:

1. hand-placed props **register their footprints** (`register_props`: radius by label/asset, buildings as oriented
   rectangles, landmarks with their own pads none, NPCs only their spawn capsule);
2. `carve_roads`: every road gets a walkable profile (one step per cell), is cut into the terrain and painted into
   `cellmat`; where it meets a terrace edge a **stair flight** is placed (snapped to the real step, shared between roads
   crossing the same edge, slid out of other flights) and both its ground and the upper landing are fixed by
   `finalize_stairs`; walls register as occupied ground and every **gate** (road crossing a wall) is kept clear;
3. `carve_plazas`, `house_pads`: plazas are flattened and painted, house slots get level pads;
4. `road_props`: **house slots** (per set) and **street lines** from the carved profiles, thinned against pavement, stairs,
   doorways and the registry; lines start half a step along the road so nothing sits on a junction corner;
5. `plaza_furniture`: lamp posts at the inset corners / along the edges (or around a round plaza's rim) and around a
   central feature; **market stalls** in rows along the edges facing the middle, back-to-back rows down wide plazas, a
   ring around a well or monument, alternating colour variants; an obelisk in the middle; other scatter on a jittered
   grid. A seat is taken only when free: not on a road through the plaza, not within reach of a street mouth, not on
   another footprint;
6. terrain **boxes** (run-merged cubes per palette material) and the foliage **lattice** (every unpaved cell clear of
   slots, lines, stairs and footprints) are attached to the spec for the graph.

Tunables per spec: `slot_clear`, `slot_spacing`, `slot_stair_clear`, `line_clear`, `line_house_clear`. `plaza_debug.py`
draws one plaza with all of the above so a rule can be tuned without the editor.

## 4. Town graphs (`town_builder.py`): data + spawners

A town graph holds nothing but **data** and **spawners**, laid out in boxed, commented sections and described in its
graph settings ("generated by run_town.py <Town> ... do not edit by hand"):

| section | data nodes | processing | spawners |
|---|---|---|---|
| Exclusions and plazas | hand exclusion boxes, plaza outlines | `PCGS_Plaza` → Surface (exclusion), Tiles | plaza tiles only when asked (the relief paints pavement) |
| Roads, water, walls | road paths, one tile row per lane count | `PCGS_Road` → Tiles, Spline, Samples; Merge per material/kind; `RoadExcl` (tall thin boxes); `WallCut` Difference | piers/bridges/walls spawn tiles; painted roads only feed exclusions |
| Street lines, furniture, towers | relief-placed points | none | one spawner per set |
| Houses | relief house slots | `PCGS_Houses` with the set's jitter/scale/footprint, excluded by roads, plazas, walls, hand boxes and earlier sets | one spawner per set (`bApplyMeshBoundsToPoints`) |
| Terrain | run-merged boxes | none | one spawner per palette material |
| Lattice and foliage | relief lattice, town centre | `PCGS_Lattice` → Ground; one `PCGS_Scatter` per band | one spawner per band |

Because the processing is in subgraphs, a town graph is small (100–240 nodes, most of them Create Points) and every
town reads the same way. Rebuilding a town (`run_town.py <Town>`) recreates its graph from the spec.

![Baerlon town graph](images/worldgen/pcg_baerlon_diagram.png)

## 5. The shared subgraphs (`pcg_subgraphs.py`, `/Game/Wot/Towns/Common/PCG`)

Each subgraph is built by `pcg_lib.SubG`: extra **input/output pins** on the graph's Input/Output nodes, and **graph
parameters** that a `Get Graph Parameter` node feeds into a node's *Overrides* pin as an attribute named after the
property it overrides (so a parameter overriding `BoundsMin` on two nodes is declared twice with `attr="BoundsMin"`). The
town graph passes values per use as overrides on the subgraph node (`G.sub(graph, name, {param: value})`). Rebuilding a
subgraph happens **in place** (nodes and parameters are cleared and recreated), so town graphs keep their reference and
pick the change up on their next generation.

| subgraph | in | out | parameters |
|---|---|---|---|
| `PCGS_Plaza` | Outline | Surface, Tiles | bLinear |
| `PCGS_Road` | Path, Row | Tiles, Spline, Samples | bClosedLoop, bLinear, DistanceIncrement, KeepRatio |
| `PCGS_Houses` | Slots, Exclusions | Houses | OffsetMin/Max, RotationMin/Max, ScaleMin/Max, Ratio, CoreMin/Max, FootMin/Max |
| `PCGS_Lattice` | Lattice, Center, Exclusions | Ground | OffsetMin/Max, MaximumDistance |
| `PCGS_Scatter` | Ground | Points | LowerBound, UpperBound, Ratio, RotationMax, ScaleMin/Max, BoundsMin/Max |

### PCGS_Plaza
Outline points → closed `Create Spline` (bLinear) → `Create Surface From Spline` (the **Surface** output, used as an
exclusion by houses and foliage) and an interior `Spline Sampler` every 100 cm flattened to 6 cm slabs (**Tiles**).

![PCGS_Plaza](images/worldgen/pcgs_plaza_diagram.png)
![PCGS_Plaza in the editor](images/worldgen/pcgs_plaza_editor.png)

### PCGS_Road
Path points → `Create Spline` (bClosedLoop, bLinear) → `Spline Sampler` every DistanceIncrement → `Copy Points` of the
Row (one tile per lane) onto every sample → `Random Choice` KeepRatio for gaps. Outputs **Tiles**, the **Spline** and the
**Samples**.

![PCGS_Road](images/worldgen/pcgs_road_diagram.png)
![PCGS_Road in the editor](images/worldgen/pcgs_road_editor.png)

### PCGS_Houses
Slots → `Transform Points` jitter and uniform scale → `Random Choice` Ratio → `Bounds Modifier` core → `Difference`
(binary) against the Exclusions → `Bounds Modifier` footprint → `Self Pruning`. The output points carry the footprint,
so later sets and the foliage treat them as exclusions.

![PCGS_Houses](images/worldgen/pcgs_houses_diagram.png)
![PCGS_Houses in the editor](images/worldgen/pcgs_houses_editor.png)

### PCGS_Lattice
Lattice → jitter → `Distance` to the town centre written as density (0 at the centre, 1 at MaximumDistance) → `Difference`
(minimum density) against the Exclusions → **Ground**. Density bands then select where each kind of foliage grows.

![PCGS_Lattice](images/worldgen/pcgs_lattice_diagram.png)
![PCGS_Lattice in the editor](images/worldgen/pcgs_lattice_editor.png)

### PCGS_Scatter
Ground → `Density Filter` LowerBound..UpperBound → `Random Choice` Ratio → `Transform Points` (random yaw, uniform scale)
→ `Bounds Modifier` (the prune radius) → `Self Pruning` → **Points** for a spawner in the town graph.

![PCGS_Scatter](images/worldgen/pcgs_scatter_diagram.png)
![PCGS_Scatter in the editor](images/worldgen/pcgs_scatter_editor.png)

## 6. Collision, navigation and the kit

PCG spawner descriptors default to no collision; `pcg_lib.collision_descriptor` gives every blocking mesh `BlockAll`
and real navigation geometry (navmesh on terrain cubes, tiles, stairs and bridges, carved around buildings and trees;
ground cover stays non-colliding). Stairs meshes use complex-as-simple collision so the navmesh follows the steps. The
kit (`kit_builder.py`, `run_town.py --kit`) assembles houses, stalls, wells, lamp posts, gatehouses, stairs, bridges and
foliage from voxel primitives into merged static meshes under `/Game/Wot/Towns/Common/Buildings` with palette material
instances under `.../Materials`.

## 7. Building with it: recipes

* **Change a rule for every town** (stall spacing, lamp clearance, house jitter): edit `relief.py` (placement) or the
  subgraph in `pcg_subgraphs.py` (point processing), run `run_town.py --subgraphs` if a subgraph changed, then
  `run_town.py --all` and `nav_rebuild_all.py`. Check with `--dry` first; it prints counts in a few seconds.
* **Change one town**: edit its spec, `run_town.py <Town>`, `nav_rebuild_all.py <Town>`, `pietest.py <Town>`.
* **Add a town**: write `<town>_spec()` next to the others, add it to `CITY_SPECS` and `TOWNS`, create the level from
  the template (see `run_cities.py`), then build as above.
* **Add a kit piece**: a function in `kit_builder.py` returning voxel boxes, registered in its table; `run_town.py --kit`.
* **Add a plaza furniture kind**: `plaza_furniture` keys on the mesh name (`MarketStall`, `LampPost`, `Obelisk`, other);
  add a layout branch there and the footprint radius in `FOOT_RADIUS`.
* **Debug placement**: `plaza_debug.py <Town> <plaza>` and `audit_stairs.py <Town> -v`; **debug a graph**: open it in the
  editor (sections are boxed and commented) or `pcg_graph_render.py`.
* **Never** patch PCG `MeshEntries` in place from `get_properties`: unloaded soft material references read back as None and
  wipe the material overrides. Change `pcg_lib` / the spec and rebuild.

## 8. Gallery (PR #139 attachments)

The same images, hosted on the PR for anyone without a checkout:

Gallery comment: https://github.com/Well-known-Game-Studio/wot_a_good_game/pull/139#issuecomment-6234831370

* [Baerlon town graph (editor)](https://github.com/user-attachments/assets/17c007f8-1612-4957-9ad0-fcb18ff3ed43)
* [Baerlon town graph (diagram)](https://github.com/user-attachments/assets/e1a54baf-6a65-4ea8-b36b-d708ad9d329e)
* [PCGS_Plaza (editor)](https://github.com/user-attachments/assets/8b03f10b-bf0c-4357-b3d2-33db10fe4803)
* [PCGS_Plaza (diagram)](https://github.com/user-attachments/assets/19977370-fe4d-46ff-b08b-51fffef4039a)
* [PCGS_Road (editor)](https://github.com/user-attachments/assets/68f79cd8-86bf-4231-a45d-9e1d45c45ac4)
* [PCGS_Road (diagram)](https://github.com/user-attachments/assets/61e548f4-23de-4bf6-aa77-416f57a425bb)
* [PCGS_Houses (editor)](https://github.com/user-attachments/assets/41db6219-7a55-464a-8e2c-0a6a94739dff)
* [PCGS_Houses (diagram)](https://github.com/user-attachments/assets/c5247649-cde5-4d2f-acbe-bac2da76f67c)
* [PCGS_Lattice (editor)](https://github.com/user-attachments/assets/38feb342-0197-451b-9bb2-1d4f5d6c19cd)
* [PCGS_Lattice (diagram)](https://github.com/user-attachments/assets/34c239e8-dd52-4f61-a8e2-5d3791c03d69)
* [PCGS_Scatter (editor)](https://github.com/user-attachments/assets/cbead8de-13d1-4aac-9dc1-c0b91cae3757)
* [PCGS_Scatter (diagram)](https://github.com/user-attachments/assets/942c5cfc-03a1-4411-a7c1-bc6bde2a1366)

