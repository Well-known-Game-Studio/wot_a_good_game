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

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
    float Weight;

    UPROPERTY()
    UWotInventoryComponent* OwningInventory;

    virtual void Use(ACharacter* Character) PURE_VIRTUAL(UWotItem, );

    UFUNCTION(BlueprintImplementableEvent)
    void OnUse(ACharacter* Character);
};