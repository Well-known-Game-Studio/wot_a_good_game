#include "WotCharacterAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/WotAnimSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WotCharacter.h"
#include "Channeling/WotChannelComponent.h"

UWotCharacterAnimInstance::UWotCharacterAnimInstance()
{

}

void UWotCharacterAnimInstance::NativeInitializeAnimation()
{
  Super::NativeInitializeAnimation();
  if (!AnimSet) {
    AnimSet = LoadObject<UWotAnimSet>(nullptr, TEXT("/Game/Wot/Characters/Anims/DA_HumanoidAnims.DA_HumanoidAnims"));
  }
  OnMontageEnded.RemoveDynamic(this, &UWotCharacterAnimInstance::HandleMontageEnded);
  OnMontageEnded.AddDynamic(this, &UWotCharacterAnimInstance::HandleMontageEnded);
}

void UWotCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
  Super::NativeUpdateAnimation(DeltaSeconds);
  APawn* Pawn = TryGetPawnOwner();
  if (!Pawn) {
    return;
  }
  UPawnMovementComponent* MovementComp = Pawn->GetMovementComponent();
  if (!MovementComp) {
    return;
  }
  FVector Velocity = Pawn->GetVelocity();
  if (AWotCharacter* WotCharacter = Cast<AWotCharacter>(Pawn)) {
    bIsClimbing = WotCharacter->IsClimbing();
    bIsRiding = WotCharacter->IsMounted();
    bIsChanneling = WotCharacter->GetChannelComponent() && WotCharacter->GetChannelComponent()->IsChanneling();
    if (bIsRiding) {
      // a seated rider is attached to the mount: its own movement component is idle, the mount carries it
      if (AActor* Mount = WotCharacter->GetAttachParentActor()) {
        Velocity = Mount->GetVelocity();
      }
    }
  }
  // update the falling / air state
  bIsInAir = MovementComp->IsFalling() && !bIsRiding;
  // update the speed variable
  Speed = Velocity.Length();
  UpdateLoopingOverride();
}

void UWotCharacterAnimInstance::UpdateLoopingOverride()
{
  // one looping pose at a time, by priority; the state machine shows through when none applies
  UAnimSequenceBase* Wanted = nullptr;
  if (!bIsDead && AnimSet) {
    if (bIsClimbing) {
      Wanted = AnimSet->Climb;
    } else if (bIsRiding) {
      Wanted = (Speed > 30.0f && AnimSet->RideMove) ? AnimSet->RideMove.Get() : AnimSet->RideIdle.Get();
    } else if (bIsChanneling && !bIsAttacking) {
      Wanted = AnimSet->Channel;
    }
  }
  if (Wanted == LoopingClip) {
    if (LoopingMontage && Wanted && !Montage_IsPlaying(LoopingMontage)) {
      LoopingMontage = PlayOverride(Wanted, true);   // the loop ran out (long rides): start it again
    }
    return;
  }
  if (LoopingMontage && Montage_IsPlaying(LoopingMontage)) {
    Montage_Stop(0.2f, LoopingMontage);
  }
  LoopingMontage = nullptr;
  LoopingClip = Wanted;
  if (Wanted) {
    LoopingMontage = PlayOverride(Wanted, true);
  }
}

UAnimMontage* UWotCharacterAnimInstance::PlayOverride(UAnimSequenceBase* Clip, bool bLoop, float BlendIn, float BlendOut, float Rate)
{
  if (!Clip) {
    return nullptr;
  }
  return PlaySlotAnimationAsDynamicMontage(Clip, OverrideSlot, BlendIn, BlendOut, Rate, bLoop ? 1000 : 1);
}

void UWotCharacterAnimInstance::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
  if (Montage && (Montage == HeavyMontage || Montage == CastMontage)) {
    HeavyMontage = nullptr; CastMontage = nullptr;
    bIsAttacking = false;
    bIsHeavyAttack = false;
  }
  if (Montage && Montage == LoopingMontage) {
    LoopingMontage = nullptr;
  }
}

bool UWotCharacterAnimInstance::LightAttack()
{
  if (bIsAttacking || bIsDead) {
    // we're already attacking, so we did not attack again
    return false;
  } else {
    // ensure we are not in a heavy attack
    bIsHeavyAttack = false;
    // we aren't attacking, so we can attack
    bIsAttacking = true;
    return true;
  }
}

bool UWotCharacterAnimInstance::HeavyAttack()
{
  if (bIsAttacking || bIsDead) {
    // we're already attacking, so we did not attack again
    return false;
  }
  bIsHeavyAttack = true;
  bIsAttacking = true;
  if (AnimSet && AnimSet->HeavyAttack) {
    if (LoopingMontage) {
      Montage_Stop(0.1f, LoopingMontage);
      LoopingMontage = nullptr; LoopingClip = nullptr;
    }
    HeavyMontage = PlayOverride(AnimSet->HeavyAttack, false, 0.1f, 0.2f);
  }
  return true;
}

bool UWotCharacterAnimInstance::PlayCast()
{
  if (bIsAttacking || bIsDead) {
    return false;
  }
  if (!AnimSet || !AnimSet->Cast) {
    return LightAttack();
  }
  bIsAttacking = true; bIsHeavyAttack = false;
  CastMontage = PlayOverride(AnimSet->Cast, false, 0.08f, 0.2f);
  return true;
}

void UWotCharacterAnimInstance::PlayHit()
{
  if (bIsDead || !AnimSet || !AnimSet->Hit) {
    return;
  }
  PlayOverride(AnimSet->Hit, false, 0.05f, 0.15f);
}

void UWotCharacterAnimInstance::PlayDeath()
{
  if (bIsDead) {
    return;
  }
  bIsDead = true;
  bIsAttacking = false; bIsHeavyAttack = false;
  if (LoopingMontage) {
    Montage_Stop(0.1f, LoopingMontage);
    LoopingMontage = nullptr; LoopingClip = nullptr;
  }
  if (AnimSet && AnimSet->Death) {
    DeathMontage = PlayOverride(AnimSet->Death, false, 0.1f, 0.0f);
  }
}

void UWotCharacterAnimInstance::ResetDeath()
{
  bIsDead = false;
  if (DeathMontage) {
    Montage_Stop(0.2f, DeathMontage);
    DeathMontage = nullptr;
  }
}
