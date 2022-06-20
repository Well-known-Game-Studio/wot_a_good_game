// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotGameplayInterface.h"
#include "WotItemInteractible.generated.h"

class UStaticMeshComponent;
class APawn;
class UWotItem;

UCLASS()
class VOXELRPG_API AWotItemInteractible : public AActor, public IWotGameplayInterface
{
	GENERATED_BODY()

public:

    void Interact_Implementation(APawn* InstigatorPawn);

	void SetItem(UWotItem* NewItem);

	AWotItemInteractible();

protected:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Items")
	UWotItem* Item;

};
