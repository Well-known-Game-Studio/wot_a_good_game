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

    virtual void Copy(const UWotItem* OtherItem) override;

    virtual UWotItem* Clone(UObject* Outer, const UWotItem* Item);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wewapon", meta = (ClampMin = 0.0))
    float DamageAmount;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    FName EquipSocketName;

protected:

    virtual void Use(ACharacter* Character) override;

};
