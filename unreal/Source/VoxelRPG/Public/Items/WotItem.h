// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Math/MathFwd.h"
#include "UObject/NoExportTypes.h"
#include "WotItem.generated.h"

class UStaticMesh;
class UTexture2D;
class UWotInventoryComponent;
class ACharacter;
class UWorld;
class AWotItemActor;

USTRUCT(BlueprintType)
struct VOXELRPG_API FWotItemSpawnInfo
{
    GENERATED_BODY()

    // Minimum amount to spawn if spawned. Note: will be limited by MaxCount of
    // item itself. If MaxCount == 0 and MinCount == 0, It will use the item's
    // Count when spawned.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0))
    int32 MinCount = 0;

    // Maximum amount to spawn if spawned. Note: will be limited by MaxCount of
    // item itself. If MaxCount == 0 and MinCount == 0, It will use the item's
    // Count when spawned.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0))
    int32 MaxCount = 0;

    // Probability of spawning this item. Note: this governs whether or not the
    // item will be spawned. If it is spawned, the amount will be determined by
    // selecting a random number between MinCount and MaxCount.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
    float Probability = 1.0f;
};

UCLASS( Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class VOXELRPG_API UWotItem : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotItem();

	// Sets default values for this component's properties
	UWotItem(const UWotItem* Other);

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FWotItemSpawnInfo SpawnInfo;

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
