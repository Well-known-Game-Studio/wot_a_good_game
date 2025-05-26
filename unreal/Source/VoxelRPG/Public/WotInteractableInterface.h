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

public:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void Interact(APawn* InstigatorPawn, FHitResult Hit);
    virtual void Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit) {}

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    bool ShowNextLine();
    virtual bool ShowNextLine_Implementation() { return false; }

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void FinishInteraction(APawn* InstigatorPawn, FHitResult Hit);
    virtual void FinishInteraction_Implementation(APawn* InstigatorPawn, FHitResult Hit) {}

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void GetInteractionText(APawn* InstigatorPawn, FHitResult Hit, FText& OutText);
    virtual void GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult Hit, FText& OutText) { OutText = FText::GetEmpty(); }
};
