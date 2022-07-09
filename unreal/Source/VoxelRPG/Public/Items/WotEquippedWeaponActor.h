// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/WotItemActor.h"
#include "WotEquippedWeaponActor.generated.h"

/*
 * 	Subclass of AWotItemActor which also implements the logic for
 * 	Primary/Secondary Attack Start/Stop. The logic for these events can be
 * 	overridden in BP.
 */
UCLASS( ClassGroup=(Custom) )
class VOXELRPG_API AWotEquippedWeaponActor : public AWotItemActor
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	AWotEquippedWeaponActor();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void PrimaryAttackStart();
    virtual void PrimaryAttackStart_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void PrimaryAttackStop();
    virtual void PrimaryAttackStop_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void SecondaryAttackStart();
    virtual void SecondaryAttackStart_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void SecondaryAttackStop();
    virtual void SecondaryAttackStop_Implementation();
};
