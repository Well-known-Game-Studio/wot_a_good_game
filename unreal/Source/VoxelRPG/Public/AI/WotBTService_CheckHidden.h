#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WotBTService_CheckHidden.generated.h"

UCLASS()
class VOXELRPG_API UWotBTService_CheckHidden : public UBTService
{
  GENERATED_BODY()

protected:

  UPROPERTY(EditAnywhere, Category = "AI")
  float MaxHideRange = 2000.0f;

  UPROPERTY(EditAnywhere, Category = "AI")
  FBlackboardKeySelector ActorToHideFromKey;

  UPROPERTY(EditAnywhere, Category = "AI")
  FBlackboardKeySelector HiddenKey;

  virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
