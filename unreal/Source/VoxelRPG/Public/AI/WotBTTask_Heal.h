#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WotBTTask_Heal.generated.h"

UCLASS()
class VOXELRPG_API UWotBTTask_Heal : public UBTTaskNode
{
  GENERATED_BODY()

  virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:

  UPROPERTY(EditAnywhere, Category = "Healing")
  float HealAmount = 20.0f;
};
