#pragma once

#include "Animation/AnimInstance.h"
#include "WotCharacterAnimInstance.generated.h"

/**
 *
 */
UCLASS(Transient, Blueprintable, HideCategories = AnimInstance, BlueprintType)
class VOXELRPG_API UWotCharacterAnimInstance : public UAnimInstance
{
  GENERATED_BODY()

public:

  UWotCharacterAnimInstance();

  /** Variables declaration */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  float Speed = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
  bool bIsInAir = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacking")
  bool bIsAttacking = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacking")
  float BowAim = 0.0f;

  UFUNCTION(BlueprintCallable, Category = "Attacking")
  bool LightAttack();

  virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};
