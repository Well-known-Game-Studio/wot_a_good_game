// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotItemChest.generated.h"

class UCameraComponent;
class USpringArmComponent;

class UStaticMeshComponent;

UCLASS()
class VOXELRPG_API AWotItemChest : public AActor
{
	GENERATED_BODY()

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BaseMesh;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* LidMesh;

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Sets default values for this actor's properties
    AWotItemChest();
};
