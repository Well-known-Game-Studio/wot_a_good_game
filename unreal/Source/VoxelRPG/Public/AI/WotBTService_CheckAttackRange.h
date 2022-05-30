#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WotBTService_CheckAttackRange.generated.h"

UCLASS()
class VOXELRPG_API UWotBTService_CheckAttackRange : public UBTService
{
  GENERATED_BODY()

protected:

  UPROPERTY(EditAnywhere, Category = "AI")
  float AttackRange = 2000.0f;

  UPROPERTY(EditAnywhere, Category = "AI")
  FBlackboardKeySelector AttackRangeKey;

  virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
