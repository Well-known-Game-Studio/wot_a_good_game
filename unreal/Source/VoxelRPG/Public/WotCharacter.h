// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class VOXELRPG_API AWotCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ProjectileClass;

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

	void MoveForward(float value);
	void MoveRight(float value);

	// Movement
	void HandleMovementInput();
	void Jump();

	// Attacking
	void PrimaryAttack();
	void LightAttack();
	void HeavyAttack();

	// Interaction
	void Interact();
	void Drop();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
