# The Ways and the One Power — design notes (2026-10-14)

## The Ways (show depiction as the starting point)
Sources: the Amazon series' production design ([Cinesite breakdown](https://cinesite.com/the-wheel-of-time-2/),
[Looper behind-the-scenes](https://looper.com/637078/get-a-look-behind-the-scenes-of-amazons-wheel-of-time-series),
[wotseries episode 7 review](https://www.wotseries.com/2021/12/18/full-spoiler-review-of-episode-7-the-dark-along-the-ways/)).

* An area outside reality: **floating islands of stone** hanging in a black void, linked by **suspended bridges**
  with no railings. Everything was once holy, built by Ogier and Aes Sedai after the Breaking, and now looks
  "touched by a terminal illness": cracked, pitted, blackened, moss-dark, sagging.
* **Guidings**: tall slabs of pale stone on every island, carved with Ogier script, the only bright surfaces.
* **Oppressive darkness**: light (torches, the Waygate glow) reaches only a few metres; fog and particulate.
* **Machin Shin**, the Black Wind: a cloud of synchronised flying dark stones and debris, murmuration-like,
  that hunts along the bridges; it drains the mind, so it must be fled, not fought.
* **Waygates**: ornate carved stone doors with a leaf (Avendesora) key; the entrance and exit of each Ways level.

### Procedural plan (`unreal/Tools/ways/`)
1. A Python generator (same MCP/PCG tooling as the towns) builds one Ways level per run from a seed:
   a graph of 6–12 islands laid out on a 3-D random walk (islands rise/fall by whole terrace steps so bridges
   can slope), each island a voxel disc of dark stone with a Guiding slab, rubble and a broken pillar or two;
   bridges as kit meshes (`SM_WayBridge`, straight deck with crumbled edges, 1–2 missing slabs per span).
2. Kit meshes made with `kit_builder` primitives: `SM_WayIsland` (variants), `SM_WayBridge`, `SM_Guiding`
   (with an emissive script decal), `SM_Waygate`, `SM_WayPillar`, `SM_WayRubble`; palette `P_WayStone`,
   `P_WayStoneDark`, `P_GuidingPale`, emissive `M_GuidingScript`.
3. Lighting: no sky; a black fog volume (exponential height fog, very short view distance), a single cold point
   light at each Guiding and a warm one at the Waygate; post-process with heavy vignette.
4. Machin Shin: a Niagara system (dark cube particles with a flocking/curl-noise field, ~2 000 particles) on
   an actor that patrols the bridge graph and chases the player when within hearing; contact drains the
   player's "sanity" (new attribute) and pushes them back.
5. Gameplay: a Ways level is entered from a Waygate placed outside a town (first one at Shadar Logoth's
   Waygate, then Fal Dara), the exit Waygate leads to the next town; navmesh covers islands and bridges, the
   void kills (kill-Z) with a respawn at the last Guiding passed.

## Channeling (saidin / saidar)
* One Power as a new `UWotAction` family on the existing action component: `Action_Channel_<Weave>`
  (Air push, Fire bolt, Earth shield, Water heal, Spirit shield-of-light) selected from a radial.
* Visual language from the show: **saidar** = cool white-blue threads that spiral in from the air around the
  channeler's hands and form glowing weaves before release; **saidin** = the same threads but with a dark,
  turbulent "taint" shimmer around the hands and a brief dark flash. Men and women see each other's weaves
  only when of the same half.
* Implementation: a `UWotChannelComponent` on characters (gender/half, strength, current weave); Niagara
  systems `NS_Weave_Saidar` and `NS_Weave_Saidin` (ribbon threads converging on a socket + glow sprites),
  a release effect per weave (fire bolt projectile reuses `WotProjectile`, air push is a radial impulse), and
  anim-instance flags `bIsChanneling` for the arm pose. Trollocs/Fades get a reaction (knockback) to Air.
