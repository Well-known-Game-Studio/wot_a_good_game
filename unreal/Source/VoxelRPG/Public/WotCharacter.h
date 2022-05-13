// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotCharacter.generated.h"

class UCameraComponent;
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

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere)
	UWotInteractionComponent* InteractionComp;

	void MoveForward(float value);
	void MoveRight(float value);

	// Movement
	void HandleMovementInput();
	void Jump();

	// Attacking
	void PrimaryAttack();
	void PrimaryAttack_TimeElapsed();
	void LightAttack();
	void HeavyAttack();

	// Interaction
	void PrimaryInteract();
	void Drop();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
