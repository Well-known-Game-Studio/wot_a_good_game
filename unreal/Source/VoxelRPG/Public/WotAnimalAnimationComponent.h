#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotAnimalAnimationComponent.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;

/**
 * Speed-driven locomotion animation for animals without an animation
 * Blueprint graph. Plays Idle / Walk / Run sequences on the owner's skeletal
 * mesh (single-node animation mode) and switches between them by ground
 * speed, so a new animal only needs three sequences assigned in its
 * Blueprint defaults.
 */
UCLASS(ClassGroup = (Wot), meta = (BlueprintSpawnableComponent))
class VOXELRPG_API UWotAnimalAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWotAnimalAnimationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> WalkAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> RunAnim;

	/** Speeds (cm/s) above which Walk and Run play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float WalkSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float RunSpeed = 450.0f;

	/** Walk cycle authored for this ground speed; play rate scales with actual speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float WalkReferenceSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float RunReferenceSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bScalePlayRateWithSpeed = true;

	/** Take over the mesh even if it has an animation Blueprint assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bOverrideAnimBlueprint = true;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> CurrentAnim;

	USkeletalMeshComponent* GetMesh() const;
	void Play(UAnimSequenceBase* Anim, float Rate);
};
