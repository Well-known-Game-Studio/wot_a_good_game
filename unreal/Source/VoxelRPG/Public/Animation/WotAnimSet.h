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
 * /Game/Wot/Characters/Anims/DA_HumanoidAnims (built by wot.Anim.Build); an Anim Blueprint or character can point at
 * another set.
 */
UCLASS(BlueprintType)
class VOXELRPG_API UWotAnimSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Idle / walk / run blend space on Speed (cm/s); used by wot.Anim.Build to fill the Anim Blueprint override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UBlendSpace> Locomotion;

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
