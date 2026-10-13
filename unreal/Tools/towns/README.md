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
  SM_Cactus, SM_Reeds, SM_FlowerShrub, SM_SnowPatch, SM_Birch). Material slots follow the first-appearance
  order of the part colours; `kit_entry(name, {"P_Plaster":"P_White"})` recolours per town.
* `city_specs.py` – specs for Caemlyn, Cairhien, Tear, Tar Valon, Fal Dara, Tanchico, Illian,
  Shadar Logoth, Whitebridge, Baerlon. `ef_spec.py` – Emond's Field.
* `city_terrain.py` – per-town terrain layer: terraces (stacked slabs with heights), stairs and BP_Ladder
  placement, water pools/rivers/moats, chests and potions on upper levels, wandering villager NPCs
  (`NPC_Villager_BP`, a child of `NPC_Base_BP` that periodically sets the blackboard MoveToLocation) and
  their dialogue rows (written into `DT_HybridRPG_Dialogue`; `unreal/DT_HybridRPG_Dialogue.csv` mirrors them).
  Heights are deterministic: roads get explicit z with vertical jumps at terrace edges, the foliage
  lattice is split per terrace, props carry their own z.
* `ow_portals.py` – Overworld portal/player-start/landmark placement for every town.
* `run_town.py <Town>|--kit|--overworld` – regenerate a town, the kit, or the Overworld portals.

Portals: the Portal blueprint now has a `ToLevel` string (used when set, otherwise the old
`ToMap` enum). Overworld portals are named `Overworld<Town>` and each town's return portal
`<Town>Overworld`; the arriving level picks the PlayerStart whose tag matches the portal name.
Palette material instances (`P_*`, `MI_Ground_*`) under `/Game/Wot/Towns/Common/Materials` take
linear colour values (display colour ^ 2.2).
