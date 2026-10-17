#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotGameplayInterface.h"
#include "WotInteractableInterface.h"
#include "WotAICharacter.generated.h"

class UAIPerceptionComponent;
class UWotInventoryComponent;
class UWotAttributeComponent;
class UWotActionComponent;
class UWotEquipmentComponent;
class UWotDeathEffectComponent;
class UWotUWHealthBar;
class UWotUWPopupNumber;

class USkeletalMesh;
class UMaterialInterface;

/** One body a generic NPC can wear: a humanoid mesh and its palette materials. */
USTRUCT(BlueprintType)
struct FWotLook
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
  TSoftObjectPtr<USkeletalMesh> Mesh;

  /** Per-slot material overrides (empty entries keep the mesh's own material). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
  TArray<TSoftObjectPtr<UMaterialInterface>> Materials;
};

UCLASS()
class VOXELRPG_API AWotAICharacter : public ACharacter, public IWotInteractableInterface, public IWotGameplayInterface
{
  GENERATED_BODY()

public:
  // Sets default values for this character's properties
  AWotAICharacter();

  /** Bodies for NPCs whose Blueprint sets no mesh (villagers): one is picked per actor name, so a town keeps the same faces. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
  TArray<FWotLook> RandomLooks;

  /** Anim Blueprint applied with a random look (the shared humanoid Anim Blueprint). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
  TSubclassOf<UAnimInstance> RandomLookAnimClass;

  /** Gives the mesh a body from RandomLooks when it has none. Returns the look index or -1. */
  UFUNCTION(BlueprintCallable, Category = "Look")
  int32 ApplyRandomLook();

  // Attacking
  UFUNCTION(BlueprintCallable)
  void PrimaryAttack(AActor* TargetActor);

  UFUNCTION(BlueprintCallable)
  void PrimaryAttackStop();

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Knockback")
  float KnockbackFactor = 500.0f;

  virtual void Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration) override;

  virtual void Unhighlight_Implementation(FHitResult Hit) override;

  virtual void SetHighlightEnabled(int HighlightValue, bool Enabled);

protected:

    FTimerHandle HighlightTimerHandle;
    void OnHighlightTimerExpired();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  void Knockback(const FVector &Direction, float Amount=5000.0f);

  UFUNCTION(BlueprintCallable)
  void GetKnockbackVectorFromActor(AActor* FromActor, FVector& OutDirection, float& OutAmount);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void HitFlash();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
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
	TObjectPtr<UWotAttributeComponent> AttributeComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	TObjectPtr<UWotActionComponent> ActionComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	TObjectPtr<UWotEquipmentComponent> EquipmentComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	TObjectPtr<UWotInventoryComponent> InventoryComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComp;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Components")
	TObjectPtr<UWotDeathEffectComponent> DeathEffectComp;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWHealthBar> HealthBarWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWPopupNumber> PopupWidgetClass;

	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	float DamageActorForgetDelay = 5.0f;
	FTimerHandle TimerHandle_ForgetDamageActor;
	void ForgetDamageActor_TimeElapsed();

	float KilledDestroyDelay = 2.0f;
	FTimerHandle TimerHandle_Destroy;
	void Destroy_TimeElapsed();
};
