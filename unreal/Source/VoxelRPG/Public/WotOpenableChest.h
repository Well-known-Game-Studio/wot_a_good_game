// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WotOpenable.h"
#include "WotOpenableChest.generated.h"

class UStaticMeshComponent;
class APawn;
class UWotInventoryComponent;

UCLASS()
class VOXELRPG_API AWotOpenableChest : public AWotOpenable
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    float TargetPitch = 110.0f;

    virtual void Interact_Implementation(APawn* InstigatorPawn) override;

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BaseMesh;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* LidMesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UWotInventoryComponent* InventoryComp;

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Sets default values for this actor's properties
    AWotOpenableChest();
};
