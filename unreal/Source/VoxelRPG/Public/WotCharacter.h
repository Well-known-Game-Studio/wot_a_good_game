// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CineCameraComponent.h"
#include "WotCharacter.generated.h"

class UAnimMontage;
class UCineCameraComponent;
class USpringArmComponent;
class UWotInteractionComponent;
class UWotAttributeComponent;
class UWotEquipmentComponent;
class UWotInventoryComponent;
class UWotDeathEffectComponent;
class UWotActionComponent;
class UUserWidget;
class UWotUWInventoryPanel;
class UWotUWHealthBar;
class UWotUWPopupNumber;
class UWotItem;
class USoundBase;
class UAudioComponent;

UCLASS()
class VOXELRPG_API AWotCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "View Camera")
	bool bUseSquareAspectRatio = true;

	UPROPERTY(EditAnywhere, Category = "View Camera")
	float CameraDistance = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "View Camera")
	float CurrentFocalLength = 500.0f;

	UPROPERTY(EditAnywhere, Category = "View Camera")
	float CurrentAperture = 1.2f;

	UPROPERTY(EditAnywhere, Category = "View Camera")
	FCameraLensSettings CameraLensSettings;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWInventoryPanel> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWHealthBar> HealthBarWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWPopupNumber> PopupWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> ActionTextWidgetClass;

public:
	// Sets default values for this character's properties
	AWotCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UCineCameraComponent* CineCameraComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UWotInteractionComponent* InteractionComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotAttributeComponent* AttributeComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotEquipmentComponent* EquipmentComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UWotInventoryComponent* InventoryComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotDeathEffectComponent* DeathEffectComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotActionComponent* ActionComp;

	// Sound effect for when the character gets something
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* GetSound;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects")
    UAudioComponent* EffectAudioComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Actions")
	bool bCanOpenMenu;

	void SetupSpringArm();
	void SetupCineCamera();

	// Movement
	void HandleMovementInput();

	DECLARE_DELEGATE_OneParam(FActionDelegate, FName);

	UFUNCTION(BlueprintCallable)
	void ActionStart(FName ActionName);

	UFUNCTION(BlueprintCallable)
	void ActionStop(FName ActionName);

	// Attacking
	UFUNCTION(BlueprintCallable)
	void PrimaryAttack();

	UFUNCTION(BlueprintCallable)
	void PrimaryAttackStop();

	// Interaction
	UFUNCTION(BlueprintCallable)
	void PrimaryInteract();

	UFUNCTION(BlueprintCallable)
	void HitFlash();

	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta);

	UFUNCTION()
	void OnKilled(AActor* InstigatorActor, UWotAttributeComponent* OwningComp);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void RotateCamera();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowInventoryWidget();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowHealthBarWidget(float NewHealth, float Delta, float Duration);

	float KilledDestroyDelay = 2.0f;
	FTimerHandle TimerHandle_Destroy;
	void Destroy_TimeElapsed();

	UFUNCTION(Exec)
	void HealSelf(float Amount = 100.0f);

	virtual void PostInitializeComponents() override;

	virtual FVector GetPawnViewLocation() const override;

public:

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowPopupWidget(const FText& Text, float Duration);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowPopupWidgetNumber(int Number, float Duration);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowActionTextWidget(FString Text, float Duration);

	UFUNCTION(BlueprintCallable, Category = "SFX")
	void PlaySoundGet();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
