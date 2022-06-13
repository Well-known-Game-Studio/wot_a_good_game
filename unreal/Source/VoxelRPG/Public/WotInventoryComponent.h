// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotInventoryComponent.generated.h"

class UWotItem;

// Blueprints will bind to this to update the UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotInventoryComponent();

    // Called when the game starts
    virtual void BeginPlay() override;

    bool AddItem(UWotItem* Item);
    bool RemoveItem(UWotItem* Item);

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdated OnInventoryUpdated;

protected:

    UPROPERTY(EditDefaultsOnly, Instanced)
    TArray<UWotItem*> DefaultItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = 0))
    int Capacity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items")
    TArray<UWotItem*> Items;
};
