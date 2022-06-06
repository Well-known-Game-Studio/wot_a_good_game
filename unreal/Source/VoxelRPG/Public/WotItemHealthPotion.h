// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WotItemPowerUp.h"
#include "WotItemHealthPotion.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotItemHealthPotion : public AWotItemPowerUp
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    float HealingAmount = 20.0f;

    void Interact_Implementation(APawn* InstigatorPawn);
};
