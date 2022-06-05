#include "AI/WotAIController.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"

void AWotAIController::BeginPlay()
{
  Super::BeginPlay();

  RunBehaviorTree(BehaviorTree);

  /*
  auto PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
  if (PlayerPawn) {
    GetBlackboardComponent()->SetValueAsObject("TargetActor", PlayerPawn);
  }
  */
}
