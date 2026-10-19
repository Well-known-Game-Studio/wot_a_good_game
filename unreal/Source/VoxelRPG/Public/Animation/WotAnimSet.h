#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WotAnimSet.generated.h"

class UAnimSequenceBase;
class UBlendSpace;

/**
 * The clips a humanoid plays outside the locomotion state machine: one-shots (heavy attack, hit, death) and looping
 * poses (channeling, riding, climbing). UWotCharacterAnimInstance plays them as dynamic montages in the Anim Blueprint's
 * DefaultSlot, so every character whose Anim Blueprint derives from Template_AnimBP shares them. The default set is
 * /Game/Wot/Characters/Anims/DA_HumanoidAnims (built by wot.Anim.Build for the Rand skeleton); every other humanoid
 * skeleton gets its own copy of the set under /Game/Wot/Characters/Anims/<SkeletonName>/, built from the same tables,
 * which the anim instance picks by the mesh's skeleton. Clips never cross rigs: the voxel rigs put the 100x import
 * scale on different bones (Rand: an extra top bone above root; the cast: root itself), so a clip authored on one rig
 * shrinks or sinks another.
 */
UCLASS(BlueprintType)
class VOXELRPG_API UWotAnimSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Idle / walk / run blend space on Speed (cm/s); used by wot.Anim.Build to fill the Anim Blueprint override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UBlendSpace> Locomotion;

	/** The Anim Blueprint's own state-machine clips, rebuilt for this rig. The anim instance swaps the Blueprint's clip
	 *  variables from the default set's assets to these when the mesh is on another skeleton. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State machine")
	TObjectPtr<UAnimSequenceBase> LightAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State machine")
	TObjectPtr<UAnimSequenceBase> Jump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State machine")
	TObjectPtr<UAnimSequenceBase> HoldingBow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One-shots")
	TObjectPtr<UAnimSequenceBase> HeavyAttack;

	/** Unarmed / magic attack: a throw from the right hand (the projectile leaves 0.2 s in). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One-shots")
	TObjectPtr<UAnimSequenceBase> Cast;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One-shots")
	TObjectPtr<UAnimSequenceBase> Hit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One-shots")
	TObjectPtr<UAnimSequenceBase> Death;

	/** Arms raised, swaying: looped while the character channels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loops")
	TObjectPtr<UAnimSequenceBase> Channel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loops")
	TObjectPtr<UAnimSequenceBase> RideIdle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loops")
	TObjectPtr<UAnimSequenceBase> RideMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loops")
	TObjectPtr<UAnimSequenceBase> Climb;
};
