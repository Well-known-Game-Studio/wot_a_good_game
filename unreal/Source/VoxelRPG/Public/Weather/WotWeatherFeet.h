#pragma once

#include "CoreMinimal.h"

class ACharacter;

/**
 * Footprints in snow / dust and ripples in puddles: Ultra Dynamic Weather's "DLWE Interaction" components, one per foot
 * bone (any bone whose name contains "foot"), attached when a character starts play. They only do anything on levels
 * whose UDW has "Enable Dynamic Landscape Weather Effects" on, and only on surfaces whose material carries the
 * project's ground physical material (Tools/playtest/weather.py lists it on UDW). Characters without foot bones get one
 * component at the capsule's base.
 */
namespace WotWeatherFeet
{
	VOXELRPG_API void Attach(ACharacter* Character, float FootSize = 35.f);

	/**
	 * Collision shapes do not follow a component's override material, so the towns' ground voxels (engine cubes in PCG
	 * instanced meshes, material MI_Ground_* -> voxel_ground_base with PM_Ground) would report the default physical
	 * material to UDW's interaction trace. Once per world: every instanced mesh whose material 0 resolves to PM_Ground
	 * gets that physical material as its body override. Called from Attach.
	 */
	VOXELRPG_API void TagGround(UWorld* World);
}
