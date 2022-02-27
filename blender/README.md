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

Another good video is [this video](https://www.youtube.com/watch?v=QBGGeAPWgug)
which goes into how to make a minecraft like mesh, armature, and animation from
start to finish.

There is also [this video](https://www.youtube.com/watch?v=AoiJ1xM1m9o) which
talks specifically about making a reusable custom humanoid skeleton for Unreal
in Blender.

If you load the UE4_Mannequin into Blender, when you import (under armature) you
need to set the primary axis to be the `-X Axis` and the secondary axis to be
the `Y Axis`.

However, for the models we are using, I suggest (at least initially) to create a
simple skeleton / armature that doesn't have IK or anything fancy.

### Video Transcript for Rigging

1. Go to front orthographic view (numpad 1).
2. Bring up the Add menu (Shift+A). Select "Armature".
3. A single bone will appear. Drag it up to the model's torso.
4. You need to be able to see the bones at all times, so go to the Armature
   settings and under Viewport Display check the In Front box. It's also
   somewhat helpful to change the display type to be "Stick" instead of
   "Octahedral"
5. Now go into edit mode and bring the bone up to the guy's neck. Make sure to
   lock the axis when you're moving your bones, by pressing the equivalent axis
   button, after you've pressed G to move the bone.
6. Go to side view (numpad 3) and pull the bone to the center of the torso.
7. Go back to front view.
8. Extrude one bone straight up for the neck, and another one for the head.
9. Press Shift+A to add another bone.
10. Rotate the new bone by 180 degrees on the Y axis and place it over the left thigh.
11. Extrude one bone straight down, and then another one at a 90 degree angle for the foot.
12. Add another bone, place it over the upper arm, and extrude two more for the forearm and the hand.
13. Parent the upper arm bone and the thigh bone to the torso.
14. Time to name our bones. This is a VERY important step that will cut our work in half.
15. Name the torso, neck and head bones accordingly.
16. For the bones on the left hand side of the model, be sure to add ".l" at the end of their name.
17. Time to apply some inverse kinematics. Though not necessary, this step will make animating our model a lot easier.
18. Go to top view (numpad 7) and drag the elbow a little bit towards the back.
19. Extrude two bones, one from the elbow and one from the wrist.
20. Select both bones and press Alt+P to clear their parents.
21. Select each bone seperately and uncheck "Deform".
22. Name the bones "elbowIK.l" and "handIK.l" respectively.
23. Go to Pose mode. Select handIK.l and then shift select 4arm.l.
24. Press Shift+I and then "To Active Bone".
25. Under "Pole Target", select "Armature" and under "Bone" select "elbowIK.l".
26. Set the chain length to 2.
27. If the elbow isn't pointing at the elbowIK bone, adjust the pole angle accordingly. Usually a value of 90, -90 or 180 does the trick.
28. Now let's do the same for the leg. First, move the knee joint slightly forward.
29. Extrude two bones, one forward from the knee and one backward from the heel.
30. Shift click both new bones, Alt + P and clear parents.
31. Select each bone seperately and uncheck "Deform".
32. Name the bones "kneeIK.l" and "footIK.l" respectively.
33. Go to Pose mode. Select footIK.l and then shift select shin.l.
34. Press Shift+I and then "To Active Bone".
35. Under "Pole Target", select "Armature" and under "Bone" select "kneeIK.l".
36. Set the chain length to 2.
37. If the knee isn't pointing at the kneeIK bone, adjust the pole angle as before.
38. Go to Edit mode. Select the footIK bone and then shift-select the foot bone.
39. Press Ctrl+P --> Keep Offset.
40. Back in Pose mode, select the foot bone.
41. Go to bone constraints and add a copy location constraint.
42. Target --> Armature. Bone --> shin.l.
43. And now for the right side. Go into edit mode.
44. Drag-select all left-hand side bones.
45. Right click --> Symmetrize. Boom! You're done.
46. Just make sure that the right-hand side bones bend properly. If not, adjust their pole angles.
47. Before applying the armature to our model, it would be a great idea to back it up.
48. That way, you can skip all these annoying steps, in case you need to rig a model with identical or similar analogies in the future.
49. Now select the armature in object mode, press Ctrl+A --> All Transforms
50. First select the model and THEN the armature.
51. In general, when you're parenting two objects together, remember to always select the child object first. It helps to memorize the phrase "children first".
52. Press Ctrl+P --> With Automatic Weights.
53. Success! However, we're far from finished.
54. As you can see, when we deform our model, these little squares appear.
55. To get rid of them, right click on your model in object mode and click on Shade Smooth.
56. The squares are gone, but we also lost that distinctive voxel look. To get it back, add an Edge Split modifier.
57. Unfortunately, we're still not done :(
58. As you can see, when we move the arm, a large part of the torso gets deformed as well.
59. To fix this, we need to go through the most tedious part of this whole process: weight painting.
60. While in object mode, select the armature and THEN the model.
61. Go to weight paint mode. Ctrl+left click the left upper arm bone.
62. Go to viewport shading and select wireframe view.
63. Right click anywhere in the workspace and set the Weight to zero.
64. Hold down the left mouse button and drag the circle over the parts that you don't want affected by that bone.
65. Bear in mind that weight painting affects vertices; not the space in between them.
66. So don't get frustrated if nothing happens when you click on a face.
67. Just click over the vertices and you'll be done in no time.
68. After you've think you're done, press G and move the bone around a bit.
69. This way you'll spot any areas that you may have missed.
70. Repeat the same process for the other arm and for the head.

## Animating

For a good example, see [this video](https://www.youtube.com/watch?v=ZHpleRn9q3o&t=0s)


## Importing into Unreal

[This video](https://www.youtube.com/watch?v=QBGGeAPWgug) is a good video with custom skeleton / mesh animated, rigged, and imported into Unreal.

Take a look at [this video](https://youtu.be/5yG4sGhz4RE) which goes through a
custom character with custom skeleton / armature for use in Unreal.