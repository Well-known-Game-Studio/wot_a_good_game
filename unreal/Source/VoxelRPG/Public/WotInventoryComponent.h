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

    UFUNCTION(BlueprintCallable)
    bool AddItem(UWotItem* Item);

    UFUNCTION(BlueprintCallable)
    bool RemoveItem(UWotItem* Item, int RemoveCount);

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdated OnInventoryUpdated;

protected:

    UPROPERTY(EditAnywhere, Instanced)
    TArray<UWotItem*> DefaultItems;
    // TMap<TSubclassOf<UWotItem>, int> DefaultItems;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items")
    TArray<UWotItem*> Items;
    // TMap<TSubclassOf<UWotItem>, int> Items;
};
