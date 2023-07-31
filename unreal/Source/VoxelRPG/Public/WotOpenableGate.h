// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WotOpenable.h"
#include "WotOpenableGate.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotOpenableGate : public AWotOpenable
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    FRotator TargetRotation;

    virtual void Open_Implementation(APawn* InstigatorPawn) override;

    virtual void Close_Implementation(APawn* InstigatorPawn) override;

protected:

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    UStaticMeshComponent* LeftMesh;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    UStaticMeshComponent* RightMesh;

public:
    // Sets default values for this actor's properties
    AWotOpenableGate();
};
