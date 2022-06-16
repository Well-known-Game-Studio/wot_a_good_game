// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WotItem.h"
#include "WotItemWeapon.generated.h"

UCLASS()
class VOXELRPG_API UWotItemWeapon : public UWotItem
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItemWeapon();

protected:

    virtual void Use(ACharacter* Character) override;

    virtual void Drop(ACharacter* Character, int DropCount) override;

};
