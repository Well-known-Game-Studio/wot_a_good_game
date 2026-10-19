# Shared humanoid animation (Tools/anims)

All humanoids (the player's Rand medium-flat rig, Tam, Lan, the Fade, the Trolloc, the main cast and the villagers) share
one six-bone rig layout (`root`, `head`, `arm_r`, `arm_l`, `leg_l`, `leg_r`, each with an `_end` leaf), so they share one
animation set:

* **Clips** are generated, not hand-animated: `wot.Anim.Build` (editor console; `Source/.../Testing/WotAnimBuilder.cpp`)
  turns pose-key tables (mesh space: forward +Y, right -X, up +Z) into `A_Humanoid_*` sequences, the
  `BS_Humanoid_Locomotion` blend space (idle 0 / walk 260 / run 750 cm/s) and the `DA_HumanoidAnims` clip set. Edit the
  tables, rerun the command, every character picks the new clips up.
* **One set per skeleton.** The tables are shared; the assets are not. The reference rig (Rand, `rand_medium_flat_sk`)
  gets its set at `/Game/Wot/Characters/Anims` and keeps its hand-made idle / walk / light attack / jump / bow clips;
  every other humanoid skeleton (Moiraine's, which the whole main cast uses, Tam, Lan, Fade, Trolloc) gets a native copy
  under `/Game/Wot/Characters/Anims/<SkeletonName>/`: the generated clips built against that skeleton's reference pose,
  plus the hand-made Rand clips *rebased* onto it (rotations copied per bone, translation and scale taken from the target
  reference pose, the root's travel carried over in component space). `UWotCharacterAnimInstance::ResolveAnimSet` picks
  the set by the mesh's skeleton and swaps the Anim Blueprint's clip variables (blend space, jump, light attack, bow) to
  it, so the shared Anim Blueprint needs no per-rig children.
* **The shared Anim Blueprint is a template** (`bIsTemplate`, no target skeleton; `wot.Anim.MakeTemplateABP <ABP>`).
  `USkeletalMeshComponent` only creates an anim instance when every mesh bone, the root included, exists in the Anim
  Blueprint's target skeleton, and the voxel rigs name their top bone after the character (`tam_medium_flat`,
  `fade_medium_flat`, `armature_medium_flat`), so a Rand-skeleton Anim Blueprint silently refused Tam's mesh
  (`wot.Anim.Status` showed `instance=None`). With no target skeleton each mesh is validated against its own skeleton.
  Why not play one clip on every rig through compatible skeletons: the voxel rigs put the 100x Blender unit scale on
  different bones. Rand, Tam, Lan, Fade and the Trolloc have an extra top bone (`rigged_medium_flat`, `tam_medium_flat`,
  …, scale 100-596) above `root` (scale 1, offsets in metres); the cast meshes have `root` itself at the top with scale
  100. Scale is never retargeted, and the engine maps bone 0 onto bone 0 regardless of names, so a Rand clip on a cast
  mesh writes scale 1 and a 0.6 cm offset into a root that needs scale 100 and 57 cm: the character collapses to a dot
  or sinks to the waist depending on which clip is blended in (the "Nynaeve shrinks while she walks" bug).
* **Playback**: the shared Anim Blueprint (`rand_medium_flat_animbp`, child of `Template_AnimBP`) drives idle / walk /
  run, jump and the light attack from its state machine; `UWotCharacterAnimInstance` plays everything else from the clip
  set as dynamic montages in `DefaultSlot`: `HeavyAttack()`, `PlayCast()` (unarmed / magic attack), `PlayHit()`,
  `PlayDeath()` one-shots, and the channeling, riding (idle / move) and climbing loops that follow the character state.
  The mount component leaves a rider with this anim instance on its Anim Blueprint.
* **Wiring**: `setup_humanoids.py` makes the skeletons compatible with each other, points every humanoid Blueprint at the
  shared Anim Blueprint, installs the locomotion blend space, gives villagers (`NPC_Villager_BP`, which has no mesh) a
  body from `RandomLooks` (cast meshes wearing each other's palettes, Tam, Lan; chosen per actor name) and sets bone
  translation retargeting to Skeleton (harmless now that no clip crosses rigs). Run after `wot.Anim.Build`.
* **Checks**: `Tools/playtest/anim_probe.py <Town> [NPC ...]` logs `wot.Anim.Status` for the player (idle, running,
  attacking) and named NPCs; `Tools/playtest/rig_probe.py [Town] [NPC] [samples] [RigSkeleton]` dumps the humanoid
  skeletons and locomotion clips (`wot.Anim.Dump`), then samples one NPC's live pose while it wanders: `pose … rootCS`,
  `boneZ=[min..max]` and `boneScale` must stay the same between idle and walking (full height is ~194 for a medium
  rig). `record_clips.py anim` records the set in Emond's Field.

Why the player stopped animating: the Rand skeleton asset had lost its top bone (`rigged_medium_flat`) when the ride clips
were imported onto it, the mesh/skeleton compatibility check then failed and the engine silently never created an anim
instance. `wot.Anim.Status` shows `instance=None` / `compatMesh=0` in that case.
