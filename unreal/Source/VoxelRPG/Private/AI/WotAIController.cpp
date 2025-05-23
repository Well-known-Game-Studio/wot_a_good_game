#include "AI/WotAIController.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"

void AWotAIController::BeginPlay()
{
  Super::BeginPlay();

  if (ensureMsgf(BehaviorTree, TEXT("BehaviorTree is nullptr! Please assign BehaviorTree in your AI Controller!"))) {
    RunBehaviorTree(BehaviorTree);
  }
}
