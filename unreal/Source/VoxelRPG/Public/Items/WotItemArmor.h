// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WotItemEquipment.h"
#include "WotItemArmor.generated.h"

UCLASS()
class VOXELRPG_API UWotItemArmor : public UWotItemEquipment
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItemArmor();

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Armor", meta = (ClampMin = 0.0))
    float ArmorAmount;
};
