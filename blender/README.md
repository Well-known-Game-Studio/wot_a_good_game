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
    Unreal; it's a good idea to have a light or something positioned in Blender at XYZ 1,-1,2 to give an idea of scale when scaling the model.)

### Materials

To work with materials, you must enable nodes - **but enabling nodes will lose color information, so follow the steps below** and make sure to __copy the color HEX code before clicking the "Use Nodes" button__:

> Note: the steps below are from the video and will give the character /
> materials a **Plastic Look**. Of course, if you don’t like that look, you can
> simply paste the color code onto the Base Color.

1. Paste the color code onto the Subsurface Color value, instead of the Base
   Color one
2. Turn the Subsurface value all the way up and increase the Subsurface Radius
   up to 3.5
3. Turn the Roughness value all the way up.
4. Repeat the exact same process for all materials that you want to be plastic.
5. For anything you don't want to look very plastic (e.g. glass), turn the
   roughness all the way down.
6. For anything you want to look metallic, well, turn Metallic all the way up.
7. Finally, it would be a great idea to rename all materials according to what
   they are used for.

### Cleanup

Check for internal vertices by pressing _alt+z_ in `Edit Mode`. If you see many
internal vertices (you will), follow these steps:

1. With the model selected, go into edit mode.
2. Make sure that all vertices are selected (A).
3. Make sure that X-ray mode is off.
4. Press C to enable circle select
5. Sweep the cursor over your model while holding the middle mouse button (or
   shift) down.
6. It is VERY IMPORTANT that you unselect all visible vertices - if not, you
   will end up with holes in your model, so be sure to check into every nook and
   cranny.
7. After you’re done, press X --> Vertices.

## Rigging

For a good example, see [this video](https://www.youtube.com/watch?v=SHNDQaF2ae4&t=0s)

## Animating

For a good example, see [this video](https://www.youtube.com/watch?v=ZHpleRn9q3o&t=0s)
