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

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    float InteractionRange = 200.0f; // meters

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    FVector InteractionBoxQueryHalfExtent{32.0f, 32.0f, 100.0f};

    void PrimaryInteract();

public:

    // Sets default values for this actor's properties
    UWotInteractionComponent();

    virtual void BeginPlay() override;
};
