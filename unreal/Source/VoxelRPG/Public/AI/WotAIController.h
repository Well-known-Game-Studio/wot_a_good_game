#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WotAIController.generated.h"

UCLASS()
class VOXELRPG_API AWotAIController : public AAIController
{
  GENERATED_BODY()

protected:

  UPROPERTY(EditDefaultsOnly, Category = "AI")
  UBehaviorTree* BehaviorTree;

  virtual void BeginPlay() override;
};
