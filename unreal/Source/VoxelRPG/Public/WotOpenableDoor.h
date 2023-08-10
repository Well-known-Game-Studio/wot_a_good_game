// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WotOpenable.h"
#include "WotOpenableDoor.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotOpenableDoor : public AWotOpenable
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    FRotator TargetRotation;

    virtual void SetHighlightEnabled(int HighlightValue, bool Enabled) override;

    virtual void Open_Implementation(APawn* InstigatorPawn) override;

    virtual void Close_Implementation(APawn* InstigatorPawn) override;

protected:

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    UStaticMeshComponent* DoorMesh;

public:
    // Sets default values for this actor's properties
    AWotOpenableDoor();
};
