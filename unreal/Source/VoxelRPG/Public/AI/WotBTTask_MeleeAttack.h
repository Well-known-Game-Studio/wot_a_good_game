#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WotBTTask_MeleeAttack.generated.h"

UCLASS()
class VOXELRPG_API UWotBTTask_MeleeAttack : public UBTTaskNode
{
  GENERATED_BODY()

  virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:

public:

  UWotBTTask_MeleeAttack();
};
