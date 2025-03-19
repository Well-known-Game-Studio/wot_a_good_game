// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKilled, AActor*, InstigatorActor, UWotAttributeComponent*, OwningComp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChanged, AActor*, InstigatorActor, UWotAttributeComponent*, OwningComp, float, NewHealth, float, Delta);

UCLASS( ClassGroup=(Custom), EditInlineNew, meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    static UWotAttributeComponent* GetAttributes(AActor* FromActor);

    UFUNCTION(BlueprintCallable, Category = "Attributes", meta = (DisplayName = "IsAlive"))
    static bool IsActorAlive(AActor* Actor);

	// Sets default values for this component's properties
	UWotAttributeComponent();

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Health;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float HealthMax = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    bool bIsStunned = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float StunDuration = 1.0f;

	FTimerHandle TimerHandle_Stunned;

    void Stunned_TimeElapsed();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float Stamina;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float StaminaMax = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float Strength;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float StrengthMax = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float Magic;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float MagicMax = 100.0f;

public:

    UFUNCTION(BlueprintCallable)
    bool Kill(AActor* InstigatorActor);

    UFUNCTION(BlueprintCallable)
    float GetHealth() const;

    UFUNCTION(BlueprintCallable)
    float GetHealthMax() const;

    UFUNCTION(BlueprintCallable)
    bool IsAlive() const;

    UFUNCTION(BlueprintCallable)
    bool IsStunned() const;

    UFUNCTION(BlueprintCallable)
    bool IsFullHealth() const;

    UFUNCTION(BlueprintCallable)
    float GetMagic() const;

    UFUNCTION(BlueprintCallable)
    float GetMagicMax() const;

    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable)
    FOnKilled OnKilled;

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    bool ApplyHealthChange(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    bool ApplyHealthChangeInstigator(AActor* InstigatorActor, float Delta);

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    bool ApplyMagicChange(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    bool ApplyMagicChangeInstigator(AActor* InstigatorActor, float Delta);
};
