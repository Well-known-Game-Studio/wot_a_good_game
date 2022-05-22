# wot_a_good_game
WOT: A Good Game - Wotxels; the first of its name

## Overview

## Development

### Modeling Workflow

All modeling is done with `VoxelMax`, make sure in the settings to select
`Optimize Mesh` and to configure the `Minimum Texture Size` to at least
1024x1024.

If you have exported from `VoxelMax` and get multiple materials in blender, then
you can fix that by baking those materials into a single texture following [this
guide](https://blender.stackexchange.com/questions/218406/how-to-bake-multiple-materials-on-a-single-texture)
or [this
guide](https://gamefromscratch.com/baking-blender-materials-to-texture-to-make-them-usable-in-a-game-engine/)

#### Land and other Voxel / Destructable Assets

1. Model the land / object in `VoxelMax`
2. Export as `VOX`
3. Import into UE5
4. Create `VoxelWorld` object and set the voxel size to be 20 cm

#### Characters, NPCs, and Assets Requiring Rigging

1. Model the character(s) in `VoxelMax`
2. Export as `GLTF`
3. Scale the object to `0.25`
4. Import into `Blender` to rig - see the
   [./blender/README.md](./blender/README.md) for further instructions on
   rigging and export as FBX into UE5.
   
#### Props, Foliage, and Non-Rigged Static Mesh Assets

1. Model the object in `VoxelMax`
2. Export as `FBX`
3. Import into UE5, with import scale to be `20`
4. You will likely have to import with a translation as well to get the pivot to
   be in the correct place on the object.
