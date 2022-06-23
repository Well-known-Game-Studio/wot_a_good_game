// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WotItem.h"
#include "WotItemEquipment.generated.h"

UCLASS()
class VOXELRPG_API UWotItemEquipment : public UWotItem
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItemEquipment();

    virtual void Copy(const UWotItem* OtherItem) override;

    virtual UWotItem* Clone(UObject* Outer, const UWotItem* Item);

    UPROPERTY(VisibleAnywhere, Category = "Equipment")
    FName EquipSocketName;

    UPROPERTY(VisibleAnywhere, Category = "Equipment")
    bool IsEquipped;

protected:

    virtual void Use(ACharacter* Character) override;

};
