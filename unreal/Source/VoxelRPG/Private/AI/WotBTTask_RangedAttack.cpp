#include "AI/WotBTTask_RangedAttack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "WotAttributeComponent.h"

UWotBTTask_RangedAttack::UWotBTTask_RangedAttack()
{
  MaxProjectileSpread = 2.0f;
}

EBTNodeResult::Type UWotBTTask_RangedAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
  AAIController* MyController = OwnerComp.GetAIOwner();
  if (ensure(MyController)) {
    ACharacter* MyPawn = Cast<ACharacter>(MyController->GetPawn());
    if (MyPawn == nullptr) {
      return EBTNodeResult::Failed;
    }

    FVector MuzzleLocation = MyPawn->GetMesh()->GetSocketLocation("Muzzle_01");
    AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject("TargetActor"));
    if (TargetActor == nullptr) {
      return EBTNodeResult::Failed;
    }

    if (!UWotAttributeComponent::IsActorAlive(TargetActor)) {
      return EBTNodeResult::Failed;
    }

    FVector Direction = TargetActor->GetActorLocation() - MuzzleLocation;
    FRotator MuzzleRotation = Direction.Rotation();

    // Don't want them to shoot down (into the floor), it looks bad
    MuzzleRotation.Pitch += FMath::RandRange(0.0f, MaxProjectileSpread);
    MuzzleRotation.Yaw += FMath::RandRange(-MaxProjectileSpread, MaxProjectileSpread);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Set the instigator so the projectile doesn't interact / damage the owner pawn
    Params.Instigator = MyPawn;

    AActor* NewProj = GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, MuzzleRotation, Params);

    return NewProj ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
  }
  return EBTNodeResult::Failed;
}
