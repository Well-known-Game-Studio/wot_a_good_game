#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WotBTTask_RangedAttack.generated.h"

UCLASS()
class VOXELRPG_API UWotBTTask_RangedAttack : public UBTTaskNode
{
  GENERATED_BODY()

  virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:

  UPROPERTY(EditAnywhere, Category = "AI")
  float MaxProjectileSpread;

  UPROPERTY(EditAnywhere, Category = "AI")
  FName SpawnSocketName;

  UPROPERTY(EditAnywhere, Category = "AI")
  TSubclassOf<AActor> ProjectileClass;

public:

  UWotBTTask_RangedAttack();
};
