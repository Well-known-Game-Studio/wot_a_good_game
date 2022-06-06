#include "AI/WotBTService_CheckLowHealth.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "WotAttributeComponent.h"

void UWotBTService_CheckLowHealth::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
  Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

  bool bIsLowHealth = false;

  // check distance between AI pawn and target actor
  UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
  if (ensure(BlackboardComp)) {
    AAIController* MyController = OwnerComp.GetAIOwner();
    if (ensure(MyController)) {
      APawn* AIPawn = MyController->GetPawn();
      if (ensure(AIPawn)) {
        // get the attribute component of the AI pawn
        UWotAttributeComponent* AttributeComp =
          Cast<UWotAttributeComponent>(AIPawn->GetComponentByClass(UWotAttributeComponent::StaticClass()));
        // if there's no attribute component, return
        if (!AttributeComp) {
          return;
        }
        const auto PawnHealth = AttributeComp->GetHealth();
        bIsLowHealth = PawnHealth <= LowHealth;
      }
    }
  }
  BlackboardComp->SetValueAsBool(LowHealthKey.SelectedKeyName, bIsLowHealth);
}
