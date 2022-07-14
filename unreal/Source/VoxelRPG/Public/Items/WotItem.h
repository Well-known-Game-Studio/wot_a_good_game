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
class AWotItemActor;

UCLASS( Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class VOXELRPG_API UWotItem : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItem();

	// Sets default values for this component's properties
	UWotItem(const UWotItem* Other);

	virtual void Copy(const UWotItem* Other);

    UFUNCTION(BlueprintCallable)
    virtual UWotItem* Clone(UObject* Outer, const UWotItem* Item) PURE_VIRTUAL(UWotItem::Clone, return nullptr;);

    // Allows spawning of objects and things in BP as long as we can get
    // reference to world
    virtual UWorld* GetWorld() const override;

    UPROPERTY(Transient)
    UWorld* World;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    TSubclassOf<AWotItemActor> ItemActorClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    AWotItemActor* ItemActor;

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

    UFUNCTION(BlueprintCallable)
    bool CanBeUsedBy(ACharacter* Character);

    UFUNCTION(BlueprintCallable)
    bool UseAddedToInventory(ACharacter* Character);

    UFUNCTION(BlueprintCallable)
    virtual void Use(ACharacter* Character) PURE_VIRTUAL(UWotItem::Use, );

    UFUNCTION(BlueprintCallable)
    virtual void Drop(FVector Location, int DropCount = 1);

    UFUNCTION(BlueprintImplementableEvent)
    void OnUse(ACharacter* Character);

    UFUNCTION(BlueprintImplementableEvent)
    void OnDrop(ACharacter* Character);

    bool operator==(const UWotItem& rhs);
    bool operator!=(const UWotItem& rhs);
};
