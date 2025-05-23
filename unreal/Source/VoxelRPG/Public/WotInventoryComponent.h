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

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    static UWotInventoryComponent* GetInventory(AActor* FromActor);

	// Sets default values for this component's properties
	UWotInventoryComponent();

    // Called when the game starts
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    UWotItem* FindItem(TSubclassOf<UWotItem> ItemClass);

    UFUNCTION(BlueprintCallable)
    int32 AddItem(UWotItem* Item);

    UFUNCTION(BlueprintCallable)
    bool RemoveItem(UWotItem* Item, int RemoveCount);

    UFUNCTION(BlueprintCallable)
    void DeleteItem(UWotItem* Item);

    UFUNCTION(BlueprintCallable)
    void DropAll();

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdated OnInventoryUpdated;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
    TArray<UWotItem*> DefaultItems;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items")
    TArray<UWotItem*> Items;
};
