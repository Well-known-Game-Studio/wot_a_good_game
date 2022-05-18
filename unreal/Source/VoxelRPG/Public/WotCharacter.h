// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotCharacter.generated.h"

class UCineCameraComponent;
class USpringArmComponent;
class UWotInteractionComponent;
class UAnimMontage;

UCLASS()
class VOXELRPG_API AWotCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "Attack")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* AttackAnim;

	FTimerHandle TimerHandle_PrimaryAttack;

public:
	// Sets default values for this character's properties
	AWotCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UCineCameraComponent* CineCameraComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UWotInteractionComponent* InteractionComp;

	void SetupSpringArm();
	void SetupCineCamera();

	void MoveForward(float value);
	void MoveRight(float value);

	// Movement
	void HandleMovementInput();

	// Attacking
	UFUNCTION(BlueprintCallable)
	void PrimaryAttack();
	void PrimaryAttack_TimeElapsed();

	UFUNCTION(BlueprintCallable)
	void LightAttack();
	UFUNCTION(BlueprintCallable)
	void HeavyAttack();

	// Interaction
	UFUNCTION(BlueprintCallable)
	void PrimaryInteract();
	UFUNCTION(BlueprintCallable)
	void Drop();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
