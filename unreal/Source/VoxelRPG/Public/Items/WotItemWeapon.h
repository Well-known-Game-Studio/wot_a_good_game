// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WotItemEquipment.h"
#include "WotItemWeapon.generated.h"

UCLASS()
class VOXELRPG_API UWotItemWeapon : public UWotItemEquipment
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItemWeapon();

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = 0.0))
    float DamageAmount;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool PrimaryAttackStart();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool PrimaryAttackStop();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool SecondaryAttackStart();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool SecondaryAttackStop();
};
