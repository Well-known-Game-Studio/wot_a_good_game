#include "AI/WotBTService_CheckHidden.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

void UWotBTService_CheckHidden::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
  Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

  bool bWithinRange = false;
  bool bHasLineOfSight = false;

  // check distance between AI pawn and target actor
  UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
  if (ensure(BlackboardComp)) {
    AActor* ActorToHideFrom = Cast<AActor>(BlackboardComp->GetValueAsObject(ActorToHideFromKey.SelectedKeyName));
    if (ActorToHideFrom) {
      AAIController* MyController = OwnerComp.GetAIOwner();
      if (ensure(MyController)) {
        APawn* AIPawn = MyController->GetPawn();
        if (ensure(AIPawn)) {
          float DistanceTo = FVector::Distance(ActorToHideFrom->GetActorLocation(), AIPawn->GetActorLocation());
          bWithinRange = DistanceTo < MaxHideRange;

          if (bWithinRange) {
            bHasLineOfSight = MyController->LineOfSightTo(ActorToHideFrom);
          }
        }
      }
    }
  }
  BlackboardComp->SetValueAsBool(HiddenKey.SelectedKeyName, (!bWithinRange || !bHasLineOfSight));
}
