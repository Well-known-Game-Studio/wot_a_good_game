#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WotBTService_CheckLowHealth.generated.h"

UCLASS()
class VOXELRPG_API UWotBTService_CheckLowHealth : public UBTService
{
  GENERATED_BODY()

protected:

  UPROPERTY(EditAnywhere, Category = "AI")
  float LowHealth = 30.0f;

  UPROPERTY(EditAnywhere, Category = "AI")
  FBlackboardKeySelector LowHealthKey;

  virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
