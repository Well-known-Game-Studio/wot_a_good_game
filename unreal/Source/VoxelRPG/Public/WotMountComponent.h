#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotMountComponent.generated.h"

class AWotCharacter;
class AAIController;
class UAnimSequenceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWotOnMountChanged, UWotMountComponent*, Mount, AWotCharacter*, Rider);

/**
 * Makes the owning character (a horse, for example) rideable.
 *
 * The rider is attached to the mount (to the saddle socket when the mesh has
 * one, else to the root component), its own collision and movement are
 * switched off, and its movement input is forwarded to the mount's character
 * movement component. The mount's AI controller is suspended while ridden and
 * re-possesses the animal on dismount.
 */
UCLASS(ClassGroup = (Wot), meta = (BlueprintSpawnableComponent))
class VOXELRPG_API UWotMountComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWotMountComponent();

	/** Shown in the interaction prompt, e.g. "Ride horse". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount")
	FText MountDisplayName;

	/** Socket on the mount's skeletal mesh to seat the rider on. Falls back to the root component when missing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount")
	FName SaddleSocketName = TEXT("saddle");

	/** Rider offset relative to the attach point (socket or root). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount")
	FVector RiderOffset = FVector(-10.0f, 0.0f, 70.0f);

	/** Walk speed of the mount while ridden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount")
	float MountedMaxWalkSpeed = 900.0f;

	/** Walk speed of the mount while ridden and sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount")
	float MountedSprintSpeed = 1500.0f;

	/** How far to the side of the mount the rider is placed on dismount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount")
	float DismountSideOffset = 160.0f;

	/** Seated pose played on the rider while the mount stands still (single-node animation; the rider's AnimBP is restored on dismount). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount|Rider")
	TObjectPtr<UAnimSequenceBase> RiderIdleAnim;

	/** Seated pose played on the rider while the mount moves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mount|Rider")
	TObjectPtr<UAnimSequenceBase> RiderMoveAnim;

	UPROPERTY(BlueprintReadOnly, Category = "Mount")
	TObjectPtr<AWotCharacter> Rider;

	UPROPERTY(BlueprintAssignable, Category = "Mount")
	FWotOnMountChanged OnMounted;

	UPROPERTY(BlueprintAssignable, Category = "Mount")
	FWotOnMountChanged OnDismounted;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mount")
	bool IsMounted() const { return Rider != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Mount")
	bool Mount(AWotCharacter* NewRider);

	UFUNCTION(BlueprintCallable, Category = "Mount")
	bool Dismount();

	/** Forward a rider's movement input to the mount. */
	UFUNCTION(BlueprintCallable, Category = "Mount")
	void AddRiderMovementInput(FVector WorldDirection, float Scale);

	UFUNCTION(BlueprintCallable, Category = "Mount")
	void SetSprinting(bool bSprinting);

	/** Returns the mount component of an actor, or null when the actor is not rideable. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mount")
	static UWotMountComponent* FindMount(AActor* Actor);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY()
	TObjectPtr<AAIController> SuspendedAIController;

	float SavedMaxWalkSpeed = 0.0f;
	bool bSavedOrientRotationToMovement = true;
	bool bSavedRunPhysicsWithNoController = false;
	bool bSavedUseControllerRotationYaw = false;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> CurrentRiderAnim;
	void PlayRiderAnim(UAnimSequenceBase* Anim);
};
