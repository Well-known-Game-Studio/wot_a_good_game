#include "WotCharacterAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/WotAnimSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WotCharacter.h"
#include "Channeling/WotChannelComponent.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UnrealType.h"

static const TCHAR* GWotAnimSetFolder = TEXT("/Game/Wot/Characters/Anims");

UWotCharacterAnimInstance::UWotCharacterAnimInstance()
{

}

void UWotCharacterAnimInstance::NativeInitializeAnimation()
{
  Super::NativeInitializeAnimation();
  ResolveAnimSet();
  OnMontageEnded.RemoveDynamic(this, &UWotCharacterAnimInstance::HandleMontageEnded);
  OnMontageEnded.AddDynamic(this, &UWotCharacterAnimInstance::HandleMontageEnded);
}

void UWotCharacterAnimInstance::ResolveAnimSet()
{
  UWotAnimSet* Default = LoadObject<UWotAnimSet>(nullptr, *FString::Printf(TEXT("%s/DA_HumanoidAnims.DA_HumanoidAnims"), GWotAnimSetFolder));
  if (!AnimSet) {
    AnimSet = Default;
  }
  bUsesRigSet = false;
  if (!bPickAnimSetByRig || !Default) {
    return;
  }
  USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
  USkeletalMesh* SkM = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
  USkeleton* Skel = SkM ? SkM->GetSkeleton() : nullptr;
  USkeleton* DefaultSkel = Default->Locomotion ? Default->Locomotion->GetSkeleton() : nullptr;
  if (!Skel || Skel == DefaultSkel) {
    return;
  }
  // this mesh is on another rig: use the set wot.Anim.Build made for that skeleton
  const FString Path = FString::Printf(TEXT("%s/%s/DA_HumanoidAnims.DA_HumanoidAnims"), GWotAnimSetFolder, *Skel->GetName());
  UWotAnimSet* RigSet = LoadObject<UWotAnimSet>(nullptr, *Path);
  if (!RigSet) {
    UE_LOG(LogAnimation, Warning, TEXT("%s: no clip set for skeleton %s (%s); run wot.Anim.Build. Clips of the default rig will distort this mesh."), *GetNameSafe(GetOwningActor()), *Skel->GetName(), *Path);
    return;
  }
  AnimSet = RigSet;
  bUsesRigSet = true;
  // the Anim Blueprint's own clip variables (locomotion blend space, jump, light attack, bow) hold the default set's
  // assets: swap each one for this rig's copy
  TMap<UObject*, UObject*> Swap;
  for (TFieldIterator<FObjectProperty> It(UWotAnimSet::StaticClass()); It; ++It) {
    UObject* From = It->GetObjectPropertyValue_InContainer(Default);
    UObject* To = It->GetObjectPropertyValue_InContainer(RigSet);
    if (From && To && From != To) {
      Swap.Add(From, To);
    }
  }
  for (TFieldIterator<FObjectProperty> It(GetClass()); It; ++It) {
    if (!It->PropertyClass || !It->PropertyClass->IsChildOf(UAnimationAsset::StaticClass())) {
      continue;
    }
    UObject* Current = It->GetObjectPropertyValue_InContainer(this);
    if (UObject* const* To = Current ? Swap.Find(Current) : nullptr) {
      It->SetObjectPropertyValue_InContainer(this, *To);
    }
  }
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
