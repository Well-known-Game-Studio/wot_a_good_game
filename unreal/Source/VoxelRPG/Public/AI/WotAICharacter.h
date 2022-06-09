#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotAICharacter.generated.h"

class UPawnSensingComponent;
class UWotAttributeComponent;
class UWotDeathEffectComponent;

UCLASS()
class VOXELRPG_API AWotAICharacter : public ACharacter
{
  GENERATED_BODY()

public:
  // Sets default values for this character's properties
  AWotAICharacter();

protected:

	UFUNCTION(BlueprintCallable)
	void HitFlash();

	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta);

	UFUNCTION()
	void OnKilled(AActor* InstigatorActor, UWotAttributeComponent* OwningComp);

  virtual void PostInitializeComponents() override;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotAttributeComponent* AttributeComp;

  UPROPERTY(VisibleAnywhere, Category = "Components")
  UPawnSensingComponent* PawnSensingComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotDeathEffectComponent* DeathEffectComp;

  UFUNCTION()
  void OnPawnSeen(APawn* Pawn);

	float KilledDestroyDelay = 2.0f;
	FTimerHandle TimerHandle_Destroy;
	void Destroy_TimeElapsed();
};
