// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotCharacter.generated.h"

class UAnimMontage;
class UCineCameraComponent;
class USpringArmComponent;
class UWotInteractionComponent;
class UWotAttributeComponent;

UCLASS()
class VOXELRPG_API AWotCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "View Camera")
	float CameraDistance = 2000.0f;

	UPROPERTY(VisibleAnywhere, Category = "Attack")
	FName HandSocketName;

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

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotAttributeComponent* AttributeComp;

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

	UFUNCTION(BlueprintCallable)
	void HitFlash();

	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta);

	virtual void PostInitializeComponents() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
