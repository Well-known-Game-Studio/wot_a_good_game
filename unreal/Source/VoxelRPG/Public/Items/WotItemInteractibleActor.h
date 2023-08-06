// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WotItemActor.h"
#include "WotGameplayInterface.h"
#include "WotItemInteractibleActor.generated.h"

class APawn;

/*
 * 	Subclass of AWotItemActor which also implements IWotGameplayInterface -
 * 	allowing the "Use" function of the Item to be called. Can be overridden for
 * 	custom implementation.
 */
UCLASS()
class VOXELRPG_API AWotItemInteractibleActor : public AWotItemActor, public IWotGameplayInterface
{
	GENERATED_BODY()

public:

    void Interact_Implementation(APawn* InstigatorPawn, FHitResult HitResult) override;

	AWotItemInteractibleActor();
};
