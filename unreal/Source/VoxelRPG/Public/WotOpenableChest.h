// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WotOpenable.h"
#include "WotOpenableChest.generated.h"

class UStaticMeshComponent;
class APawn;
class UWotInventoryComponent;
class UWotUWInventoryPanel;

UCLASS()
class VOXELRPG_API AWotOpenableChest : public AWotOpenable
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    float TargetPitch = 110.0f;

    virtual void Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit) override;

    virtual void GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult Hit, FText& OutText) override;

    virtual void SetHighlightEnabled(int HighlightValue, bool Enabled) override;

protected:

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWotUWInventoryPanel> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	FName InventoryPanelTitle = "Chest Loot";

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BaseMesh;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* LidMesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UWotInventoryComponent* InventoryComp;

public:
    // Sets default values for this actor's properties
    AWotOpenableChest();
};
