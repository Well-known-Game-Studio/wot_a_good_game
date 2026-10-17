# Shared humanoid animation (Tools/anims)

All humanoids (the player's Rand medium-flat rig, Tam, Lan, the Fade, the Trolloc, the main cast and the villagers) share
one six-bone rig layout (`root`, `head`, `arm_r`, `arm_l`, `leg_l`, `leg_r`, each with an `_end` leaf), so they share one
animation set:

* **Clips** are generated, not hand-animated: `wot.Anim.Build` (editor console; `Source/.../Testing/WotAnimBuilder.cpp`)
  turns pose-key tables (mesh space: forward +Y, right -X, up +Z) into `A_Humanoid_*` sequences on the Rand skeleton,
  the `BS_Humanoid_Locomotion` blend space (idle 0 / walk 260 / run 750 cm/s) and the `DA_HumanoidAnims` clip set under
  `/Game/Wot/Characters/Anims`. Edit the tables, rerun the command, every character picks the new clips up.
* **Playback**: the shared Anim Blueprint (`rand_medium_flat_animbp`, child of `Template_AnimBP`) drives idle / walk /
  run, jump and the light attack from its state machine; `UWotCharacterAnimInstance` plays everything else from the clip
  set as dynamic montages in `DefaultSlot`: `HeavyAttack()`, `PlayCast()` (unarmed / magic attack), `PlayHit()`,
  `PlayDeath()` one-shots, and the channeling, riding (idle / move) and climbing loops that follow the character state.
  The mount component leaves a rider with this anim instance on its Anim Blueprint.
* **Wiring**: `setup_humanoids.py` makes the skeletons compatible with each other, points every humanoid Blueprint at the
  shared Anim Blueprint, installs the locomotion blend space, gives villagers (`NPC_Villager_BP`, which has no mesh) a
  body from `RandomLooks` (cast meshes wearing each other's palettes, Tam, Lan; chosen per actor name) and sets bone
  translation retargeting to Skeleton so each mesh keeps its own proportions. Run after `wot.Anim.Build`.
* **Checks**: `Tools/playtest/anim_probe.py <Town> [NPC ...]` logs `wot.Anim.Status` for the player (idle, running,
  attacking) and named NPCs; `record_clips.py anim` records the set in Emond's Field.

Why the player stopped animating: the Rand skeleton asset had lost its top bone (`rigged_medium_flat`) when the ride clips
were imported onto it, the mesh/skeleton compatibility check then failed and the engine silently never created an anim
instance. `wot.Anim.Status` shows `instance=None` / `compatMesh=0` in that case.
