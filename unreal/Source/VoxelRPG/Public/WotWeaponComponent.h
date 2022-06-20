// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotWeaponComponent.generated.h"

class UWotItemWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    static UWotWeaponComponent* GetWeapon(AActor* FromActor);

	// Sets default values for this component's properties
	UWotWeaponComponent();

    UFUNCTION(BlueprintCallable)
	void SetItem(UWotItemWeapon* NewItemWeapon);

    UFUNCTION(BlueprintCallable)
    bool PrimaryAttack();

    UFUNCTION(BlueprintCallable)
    bool SecondaryAttack();

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Items")
    UWotItemWeapon* ItemWeapon;

};
