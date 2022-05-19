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

    // HealthMax, Stamina, Strength

public:

    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    bool ApplyHealthChange(float Delta);

};
