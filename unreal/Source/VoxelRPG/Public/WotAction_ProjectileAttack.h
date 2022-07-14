// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WotAction.h"
#include "WotAction_ProjectileAttack.generated.h"

class UAnimMontage;
class UNiagaraSystem;

UCLASS(Blueprintable)
class VOXELRPG_API UWotAction_ProjectileAttack : public UWotAction
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotAction_ProjectileAttack();

protected:

	UPROPERTY(EditAnywhere, Category = "Attack")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(VisibleAnywhere, Category = "Attack")
	FName HandSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackAnimDelay;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* AttackAnim;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UNiagaraSystem* CastingNiagaraSystem;

	UFUNCTION()
	void AttackDelay_TimerElapsed(ACharacter* InstigatorCharacter);

public:

    virtual void Start_Implementation(AActor* Instigator) override;

    virtual void Stop_Implementation(AActor* Instigator) override;
};
