// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WotItemActor.h"
#include "WotInteractableInterface.h"
#include "WotItemInteractableActor.generated.h"

class APawn;

/*
 * 	Subclass of AWotItemActor which also implements IWotInteractableInterface -
 * 	allowing the "Use" function of the Item to be called. Can be overridden for
 * 	custom implementation.
 */
UCLASS()
class VOXELRPG_API AWotItemInteractableActor : public AWotItemActor, public IWotInteractableInterface
{
	GENERATED_BODY()

public:

    virtual void Interact_Implementation(APawn* InstigatorPawn, FHitResult HitResult) override;

    virtual void GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult HitResult, FText& OutText) override;

	AWotItemInteractableActor();
};
