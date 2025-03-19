#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotGameplayInterface.h"
#include "WotInteractableInterface.h"
#include "WotAICharacter.generated.h"

// class UDialogueBuilderObject;
class UPawnSensingComponent;
class UWotInventoryComponent;
class UWotAttributeComponent;
class UWotActionComponent;
class UWotEquipmentComponent;
class UWotDeathEffectComponent;
class UWotUWHealthBar;
class UWotUWPopupNumber;

UCLASS()
class VOXELRPG_API AWotAICharacter : public ACharacter, public IWotInteractableInterface, public IWotGameplayInterface
{
  GENERATED_BODY()

public:
  // Sets default values for this character's properties
  AWotAICharacter();

  // Attacking
  UFUNCTION(BlueprintCallable)
  void PrimaryAttack(AActor* TargetActor);

  UFUNCTION(BlueprintCallable)
  void PrimaryAttackStop();

  virtual void Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration) override;

  virtual void Unhighlight_Implementation(FHitResult Hit) override;

  virtual void SetHighlightEnabled(int HighlightValue, bool Enabled);

protected:

    FTimerHandle HighlightTimerHandle;
    void OnHighlightTimerExpired();

	UFUNCTION(BlueprintCallable)
	void HitFlash();

	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta);

	UFUNCTION()
	void OnKilled(AActor* InstigatorActor, UWotAttributeComponent* OwningComp);

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetBlackboardActor(const FString BlackboardKeyName, AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowHealthBarWidget(float NewHealth, float Delta, float Duration);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowPopupWidget(const FText& Text, float Duration);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowPopupWidgetNumber(int Number, float Duration);

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotAttributeComponent* AttributeComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotActionComponent* ActionComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotEquipmentComponent* EquipmentComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UWotInventoryComponent* InventoryComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UPawnSensingComponent* PawnSensingComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	UWotDeathEffectComponent* DeathEffectComp;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWHealthBar> HealthBarWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWPopupNumber> PopupWidgetClass;

	// UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dialog")
	// UDialogueBuilderObject* DialogObject;

	UFUNCTION()
	void OnPawnSeen(APawn* Pawn);

	float DamageActorForgetDelay = 5.0f;
	FTimerHandle TimerHandle_ForgetDamageActor;
	void ForgetDamageActor_TimeElapsed();

	float KilledDestroyDelay = 2.0f;
	FTimerHandle TimerHandle_Destroy;
	void Destroy_TimeElapsed();
};
