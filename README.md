# wot_a_good_game
WOT: A Good Game - Wotxels; the first of its name

![image](https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/d10bae3d-c8df-445d-88c4-9c6ab06c48c2)

- Youtube: https://www.youtube.com/@wellknowngamestudio9054
- Twitter: https://twitter.com/WellKnownGames

## Overview

Here is an overview picture of the models that have been made for the prototype
so far and how (in the modeling software) they were built together to get a
rough idea of how well they look together and work together.

![Model Overview](./images/model_overview.jpeg)

Here is a zoomed-in view of the character sizes, starting with the largest on
the left, with two options for medium models (medium and medium-flat), and then
the smallest model (which is simply a few small blocks of certain colors/traits).

![Character Sizes](./images/character_sizes.jpeg)

Here is an example drawing showing the different level sizes (rough
approximation) that could be used for different parts of the world, broken down
into 3 main categories:

1. Overworld - This is the most zoomed out, at which point we'd be using the
   smallest character models, that would only be a few blocks tall. The
   Overworld allows the player to traverse the whole world and enter points of
   interest. Within the overworld the player can see the points of interest
   rendered as smaller models, and can even see other smaller versions of NPCs
   such as enemies and friendlies (but likely not animals and other
   environmental objects).
2. Point of Interest - This is the medium model - at which point we'd be using
   the medium / medium-flat models. This would be used for towns or other places
   that the character goes to in the overworld. Within points of interest there
   will be NPCs including animals, as well as interactibles such as buildings,
   chests, collectables, and such. If a player enters a building / cave, etc.
   they will transition to the interior model (largest).
3. Interior - This is the most zoomed in model of a specific place which uses
   the largest / most detailed character model. This might be used for the
   inside of a house or cave for instance.

![Level Sizes](./images/level_sizes.jpeg)

Here is how they currently look:

Interior (Rand House):
<img width="1309" alt="CleanShot 2023-07-29 at 10 27 47@2x" src="https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/f0e09084-4adf-4c96-87a1-53e62e596c3e">

Overworld:
<img width="1263" alt="CleanShot 2023-07-29 at 10 28 32@2x" src="https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/9da33ba1-2203-4e02-b7c8-a591d7531174">

Video:

https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/f148ddb8-2a70-4761-8087-b54520e2f074


### Controls:

![CleanShot 2025-05-20 at 12 48 39](https://github.com/user-attachments/assets/1e83a215-a19c-4571-9589-40265ba4981d)

### Opening

https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/07de2f99-19bc-4531-b0dd-976531cbe2f2

### Herding

https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/e68240f3-9376-4ee2-af0b-171f75ccd8f2

### Attack

https://github.com/Well-known-Game-Studio/wot_a_good_game/assets/213467/b3bde112-3e55-42ee-baf5-fece5317146d

## Development

### Programming

Here's a useful [Unreal Engine Style
Guide](https://github.com/Allar/ue5-style-guide) that can help answer some
questions about why and how to do certain things within UE5.

### Modeling Workflow

All modeling is done with `VoxelMax`, make sure in the settings to select
`Optimize Mesh` and to configure the `Minimum Texture Size` to at least
1024x1024.

If you have exported from `VoxelMax` and get multiple materials in blender, then
you can fix that by baking those materials into a single texture following [this
guide](https://blender.stackexchange.com/questions/218406/how-to-bake-multiple-materials-on-a-single-texture)
or [this
guide](https://gamefromscratch.com/baking-blender-materials-to-texture-to-make-them-usable-in-a-game-engine/)

> :warning: NOTE: you need to be in the object edit mode when you export assets
> from VoxelMax, otherwise the origin / pivot point of the mesh will not be the
> local origin of the model, but will instead be the origin of the scene and
> you'll have to import with tralsation, rotation as well as scale.

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
5. After importing, make sure to enable `CPU Access` in the skeletal mesh's settings in UE
   
#### Props, Foliage, and Non-Rigged Static Mesh Assets

1. Model the object in `VoxelMax`
2. Export as `FBX`
3. Import into UE5, with import scale to be `25`
4. You will likely have to import with a translation as well to get the pivot to
   be in the correct place on the object.
