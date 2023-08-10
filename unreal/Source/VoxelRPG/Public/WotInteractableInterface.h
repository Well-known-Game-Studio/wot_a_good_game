// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WotInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWotInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *   Interface for interactable objects
 */
class VOXELRPG_API IWotInteractableInterface
{
    GENERATED_BODY()

    // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void Interact(APawn* InstigatorPawn, FHitResult Hit);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void GetInteractionText(APawn* InstigatorPawn, FHitResult Hit, FText& OutText);
};
