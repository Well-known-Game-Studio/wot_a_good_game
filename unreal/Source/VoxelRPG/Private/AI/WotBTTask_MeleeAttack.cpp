#include "AI/WotBTTask_MeleeAttack.h"
#include "AI/WotAICharacter.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "WotAttributeComponent.h"

UWotBTTask_MeleeAttack::UWotBTTask_MeleeAttack()
{
}

EBTNodeResult::Type UWotBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
  AAIController* MyController = OwnerComp.GetAIOwner();
  if (ensure(MyController)) {
    ACharacter* MyPawn = Cast<ACharacter>(MyController->GetPawn());
    if (MyPawn == nullptr) {
      UE_LOG(LogTemp, Warning, TEXT("Invalid AI Pawn!"));
      return EBTNodeResult::Failed;
    }

    AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject("TargetActor"));
    if (TargetActor == nullptr) {
      UE_LOG(LogTemp, Warning, TEXT("No target to attack!"));
      return EBTNodeResult::Failed;
    }

    if (!UWotAttributeComponent::IsActorAlive(TargetActor)) {
      UE_LOG(LogTemp, Warning, TEXT("Target is not alive!"));
      return EBTNodeResult::Failed;
    }

    AWotAICharacter* MyAIPawn = Cast<AWotAICharacter>(MyPawn);
    if (MyAIPawn == nullptr) {
      UE_LOG(LogTemp, Warning, TEXT("Invalid AI Pawn"));
      return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Warning, TEXT("MeleeAttack!"));
    // call the melee attack on the class
    MyAIPawn->PrimaryAttack(TargetActor);

    return EBTNodeResult::Succeeded;
  }
  return EBTNodeResult::Failed;
}
