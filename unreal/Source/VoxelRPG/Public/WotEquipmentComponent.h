// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotEquipmentComponent.generated.h"

class AWotEquippedArmorActor;
class AWotEquippedWeaponActor;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    static UWotEquipmentComponent* GetEquipment(AActor* FromActor);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    static UWotItemWeapon* GetEquippedWeaponFromActor(AActor* FromActor);

	// Sets default values for this component's properties
	UWotEquipmentComponent();

    virtual void InitializeComponent() override;

    UFUNCTION(BlueprintCallable)
    void EquipItem(UWotItemEquipment* NewItemEquipment);

    UFUNCTION(BlueprintCallable)
    void UnequipItem(UWotItemEquipment* NewItemEquipment);

    UFUNCTION(BlueprintCallable)
	void EquipArmor(UWotItemArmor* NewItemArmor);

    UFUNCTION(BlueprintCallable)
	void EquipWeapon(UWotItemWeapon* NewItemWeapon);

    UFUNCTION(BlueprintCallable)
    void UnequipAll();

    UFUNCTION(BlueprintCallable)
	void UnequipArmor(UWotItemArmor* NewItemArmor);

    UFUNCTION(BlueprintCallable)
	void UnequipWeapon(UWotItemWeapon* NewItemWeapon);

    UFUNCTION(BlueprintCallable)
	UWotItemWeapon* GetEquippedWeapon();

protected:

    UPROPERTY(EditAnywhere, Category = "Equipment")
    TArray<FName> ArmorSocketNames;

    UPROPERTY(EditAnywhere, Category = "Equipment")
    TArray<FName> WeaponSocketNames;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Equipment")
    TMap<FName, UWotItemArmor*> ArmorItems;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Equipment")
    TMap<FName, UWotItemWeapon*> WeaponItems;
};
