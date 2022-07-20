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

    virtual void Interact_Implementation(APawn* InstigatorPawn) override;

protected:

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    UStaticMeshComponent* LeftMesh;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    UStaticMeshComponent* RightMesh;

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Sets default values for this actor's properties
    AWotOpenableGate();
};
