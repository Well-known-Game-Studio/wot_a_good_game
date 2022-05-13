// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotGameplayInterface.h"
#include "WotInteractionComponent.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API UWotInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    void PrimaryInteract();

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Sets default values for this actor's properties
    UWotInteractionComponent();
};
