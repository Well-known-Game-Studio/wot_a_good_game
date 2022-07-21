// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WotEquippedWeaponActor.h"
#include "WotEquippedWeaponMeleeActor.generated.h"

class USoundBase;
class UAudioComponent;

/*
 * 	Subclass of AWotItemActor which also implements the logic for
 * 	Primary/Secondary Attack Start/Stop. The logic for these events can be
 * 	overridden in BP.
 */
UCLASS( ClassGroup=(Custom) )
class VOXELRPG_API AWotEquippedWeaponMeleeActor : public AWotEquippedWeaponActor
{
	GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float HitDelay = 0.3f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float AttackRange = 200.0f; // meters

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector HitBoxHalfExtent{20.0f, 100.0f, 50.0f};

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* HitSound;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects")
    UAudioComponent* EffectAudioComp;

	// Sets default values for this component's properties
	AWotEquippedWeaponMeleeActor();

    virtual void AttackSweep();

    virtual void PrimaryAttackStart_Implementation() override;

    virtual void PrimaryAttackStop_Implementation() override;

    virtual void SecondaryAttackStart_Implementation() override;

    virtual void SecondaryAttackStop_Implementation() override;
};
