#include "WotCharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WotCharacter.h"

UWotCharacterAnimInstance::UWotCharacterAnimInstance()
{

}

void UWotCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
  APawn* Pawn = TryGetPawnOwner();
  if (!Pawn) {
    return;
  }
  UPawnMovementComponent* MovementComp = Pawn->GetMovementComponent();
  if (!MovementComp) {
    return;
  }
  if (Pawn->IsA(AWotCharacter::StaticClass())) {
    AWotCharacter* WotCharacter = Cast<AWotCharacter>(Pawn);
    if (WotCharacter) {
      bIsClimbing = WotCharacter->IsClimbing();
    }
  }
  // update the falling / air state
  bIsInAir = MovementComp->IsFalling();
  // update the speed variable
  Speed = Pawn->GetVelocity().Length();
}

bool UWotCharacterAnimInstance::LightAttack()
{
  if (bIsAttacking) {
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
  if (bIsAttacking) {
    // we're already attacking, so we did not attack again
    return false;
  } else {
    // ensure we are in a heavy attack
    bIsHeavyAttack = true;
    // we aren't attacking, so we can attack
    bIsAttacking = true;
    return true;
  }
}
