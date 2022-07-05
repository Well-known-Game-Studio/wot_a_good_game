#include "WotCharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

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
  // update the falling / air state
  bIsInAir = MovementComp->IsFalling();
  // update the speed variable
  Speed = Pawn->GetVelocity().Length();
}
