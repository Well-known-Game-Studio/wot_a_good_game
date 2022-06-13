// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WotItem.h"
#include "WotItemFood.generated.h"

UCLASS()
class VOXELRPG_API UWotItemFood : public UWotItem
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItemFood();

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = 0.0))
    float HealingAmount;

protected:

    virtual void Use(ACharacter* Character) override;

};
