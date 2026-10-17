#pragma once

#include "Animation/AnimInstance.h"
#include "WotCharacterAnimInstance.generated.h"

class UWotAnimSet;
class UAnimSequenceBase;
class UAnimMontage;

/**
 * Anim instance shared by the player and every humanoid NPC (Template_AnimBP and its children). The state machine in the
 * Anim Blueprint handles idle / walk / run (blend space on Speed), jump and the light attack; everything else is played
 * from the UWotAnimSet as a dynamic montage in the DefaultSlot: heavy attack, hit and death are one-shots, channeling,
 * riding and climbing are looping poses that follow the character's state.
 */
UCLASS(Transient, Blueprintable, HideCategories = AnimInstance, BlueprintType)
class VOXELRPG_API UWotCharacterAnimInstance : public UAnimInstance
{
  GENERATED_BODY()

public:

  UWotCharacterAnimInstance();

  /** Variables declaration */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  float Speed = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  bool bIsClimbing = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  bool bIsInAir = false;

  /** True while the character channels the One Power (arms raised). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacking")
  bool bIsChanneling = false;

  /** True while the character is riding a mount (play a seated pose). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  bool bIsRiding = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacking")
  bool bIsAttacking = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacking")
  bool bIsHeavyAttack = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacking")
  float BowAim = 0.0f;

  /** True once PlayDeath() ran; the death pose is held. */
  UPROPERTY(BlueprintReadOnly, Category = "State")
  bool bIsDead = false;

  /** Clips for the montage-driven states. Falls back to /Game/Wot/Characters/Anims/DA_HumanoidAnims when unset. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  TObjectPtr<UWotAnimSet> AnimSet;

  /** Slot node in the Anim Blueprint the override clips play in. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  FName OverrideSlot = TEXT("DefaultSlot");

  UFUNCTION(BlueprintCallable, Category = "Attacking")
  bool LightAttack();

  /** Plays the heavy attack clip; bIsAttacking / bIsHeavyAttack stay set until it ends. */
  UFUNCTION(BlueprintCallable, Category = "Attacking")
  bool HeavyAttack();

  /** Unarmed / magic attack (one-shot); falls back to the light attack when the set has no Cast clip. */
  UFUNCTION(BlueprintCallable, Category = "Attacking")
  bool PlayCast();

  /** A short flinch (one-shot). */
  UFUNCTION(BlueprintCallable, Category = "State")
  void PlayHit();

  /** Falls and holds the last frame until the mesh ragdolls or the character is reset. */
  UFUNCTION(BlueprintCallable, Category = "State")
  void PlayDeath();

  UFUNCTION(BlueprintCallable, Category = "State")
  void ResetDeath();

  virtual void NativeInitializeAnimation() override;
  virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
  UFUNCTION()
  void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

  UAnimMontage* PlayOverride(UAnimSequenceBase* Clip, bool bLoop, float BlendIn = 0.15f, float BlendOut = 0.2f, float Rate = 1.0f);
  void UpdateLoopingOverride();

  /** The looping clip currently playing (channel / ride / climb), if any. */
  UPROPERTY(Transient)
  TObjectPtr<UAnimSequenceBase> LoopingClip;

  UPROPERTY(Transient)
  TObjectPtr<UAnimMontage> LoopingMontage;

  UPROPERTY(Transient)
  TObjectPtr<UAnimMontage> HeavyMontage;

  UPROPERTY(Transient)
  TObjectPtr<UAnimMontage> CastMontage;

  UPROPERTY(Transient)
  TObjectPtr<UAnimMontage> DeathMontage;
};
