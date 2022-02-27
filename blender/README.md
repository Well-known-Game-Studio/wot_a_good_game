# Blender + MagicaVoxel

## Setup

Install the [magicavoxel importer blender plugin](https://github.com/well-known-game-studio/magicavoxel-vox-importer).

This is a fork which has enabled support for importing MagicaVoxel 0.99 vox
files.

## Importing

For a good example, see [this video](https://www.youtube.com/watch?v=YB1k6-QR7xc)

1. Use the plugin above and keep the scale / spacing set to 1.0.
2. Shift left click on a cube and press Ctrl+J to join them all together.
3. Go into edit mode, press M to bring up the Merge menu and then select "By
   Distance", this will get rid of all double vertices.

### Origin to Character Feet

1. Select two inner vertices in the middle
2. Press Shift+S to bring up the move menu
3. Press 2 to bring the cursor right between the selected vertices
4. Press TAB to go into object mode.
5. Right click on the character and go to Set Origin --> Origin to 3D cursor
6. Press Shift+S
7. Press 1 to move the cursor back to the World Origin
8. With the object selected, press Shift+S once more
9. Press 8 to bring the model to the World Origin
10. Scale the model down to a reasonable size (1 unit in Blender = 1 unit in
    Unreal)

## Rigging

For a good example, see [this video](https://www.youtube.com/watch?v=SHNDQaF2ae4&t=0s)

## Animating

For a good example, see [this video](https://www.youtube.com/watch?v=ZHpleRn9q3o&t=0s)
