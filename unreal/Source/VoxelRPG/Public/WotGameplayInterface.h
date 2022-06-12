// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WotGameplayInterface.generated.h"

class UCameraComponent;
class USpringArmComponent;

UINTERFACE(MinimalAPI, Blueprintable)
class UWotGameplayInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *   [summary]
 */
class VOXELRPG_API IWotGameplayInterface
{
    GENERATED_BODY()

    // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    UFUNCTION(BlueprintNativeEvent)
    void Interact(APawn* InstigatorPawn);
};
