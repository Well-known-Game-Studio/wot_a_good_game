// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotGameplayInterface.h"
#include "WotItemDoor.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotItemDoor : public AActor, public IWotGameplayInterface
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    FRotator TargetRotation;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool bIsOpen = false;

    void Interact_Implementation(APawn* InstigatorPawn);

protected:

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    UStaticMeshComponent* DoorMesh;

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Sets default values for this actor's properties
    AWotItemDoor();
};
