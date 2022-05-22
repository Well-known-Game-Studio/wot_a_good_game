// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChanged, AActor*, InstigatorActor, UWotAttributeComponent*, OwningComp, float, NewHealth, float, Delta);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotAttributeComponent();

protected:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float Health;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float HealthMax = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float Stamina;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float StaminaMax = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float Strength;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
    float StrengthMax = 100.0f;

public:

    UFUNCTION(BlueprintCallable)
    bool IsAlive() const;

    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    bool ApplyHealthChange(float Delta);

};
