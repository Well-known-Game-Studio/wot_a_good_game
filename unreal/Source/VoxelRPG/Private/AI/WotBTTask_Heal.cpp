#include "AI/WotBTTask_Heal.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "WotAttributeComponent.h"

EBTNodeResult::Type UWotBTTask_Heal::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
  AAIController* MyController = OwnerComp.GetAIOwner();
  if (ensure(MyController)) {
    ACharacter* MyPawn = Cast<ACharacter>(MyController->GetPawn());
    if (MyPawn == nullptr) {
      return EBTNodeResult::Failed;
    }
    // get the attribute component of the AI pawn
    UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(MyPawn);
    // if there's no attribute component, fail
    if (!AttributeComp) {
      return EBTNodeResult::Failed;
    }

    if (AttributeComp->IsFullHealth()) {
      return EBTNodeResult::Failed;
    }

    return AttributeComp->ApplyHealthChange(HealAmount) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
  }
  return EBTNodeResult::Failed;
}
