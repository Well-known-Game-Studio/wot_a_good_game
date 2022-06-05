#include "AI/WotBTService_CheckAttackRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

void UWotBTService_CheckAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
  Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

  bool bWithinRange = false;
  bool bHasLineOfSight = false;

  // check distance between AI pawn and target actor
  UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
  if (ensure(BlackboardComp)) {
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject("TargetActor"));
    if (TargetActor) {
      AAIController* MyController = OwnerComp.GetAIOwner();
      if (ensure(MyController)) {
        APawn* AIPawn = MyController->GetPawn();
        if (ensure(AIPawn)) {
          float DistanceTo = FVector::Distance(TargetActor->GetActorLocation(), AIPawn->GetActorLocation());
          bWithinRange = DistanceTo < AttackRange;

          if (bWithinRange) {
            bHasLineOfSight = MyController->LineOfSightTo(TargetActor);
          }
        }
      }
    }
  }
  BlackboardComp->SetValueAsBool(AttackRangeKey.SelectedKeyName, (bWithinRange && bHasLineOfSight));
}
