// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WotItem.generated.h"

class UStaticMesh;
class UTexture2D;
class UWotInventoryComponent;
class ACharacter;
class UWorld;

UCLASS( Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class VOXELRPG_API UWotItem : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItem();

    virtual UWorld* GetWorld() const { return World; }

    UPROPERTY(Transient)
    UWorld* World;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    FText UseActionText;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    UStaticMesh* PickupMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UTexture2D* Thumbnail;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    FText ItemDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
    FText ItemDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0))
    int Count;

    // MaxCount == 0 implies no limit
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0))
    int MaxCount;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
    float Weight;

    UPROPERTY()
    UWotInventoryComponent* OwningInventory;

    // returns how many we were able to add
    UFUNCTION(BlueprintCallable)
    int Add(int AddedCount);

    // returns how many we were able to remove
    UFUNCTION(BlueprintCallable)
    int Remove(int RemovedCount);

    bool UseAddedToInventory(ACharacter* Character);

    UFUNCTION(BlueprintCallable)
    virtual void Use(ACharacter* Character) PURE_VIRTUAL(UWotItem::Use, );

    UFUNCTION(BlueprintCallable)
    virtual void Drop(ACharacter* Character, int DropCount = 1) PURE_VIRTUAL(UWotItem::Drop, );

    UFUNCTION(BlueprintImplementableEvent)
    void OnUse(ACharacter* Character);

    UFUNCTION(BlueprintImplementableEvent)
    void OnDrop(ACharacter* Character);

    bool operator==(const UWotItem& rhs);
    bool operator!=(const UWotItem& rhs);
};
