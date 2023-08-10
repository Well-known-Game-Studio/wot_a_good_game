// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotInteractableInterface.h"
#include "WotInteractionComponent.generated.h"

UCLASS()
class VOXELRPG_API UWotInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    float InteractionRange = 200.0f; // meters

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    FVector InteractionBoxQueryHalfExtent{32.0f, 32.0f, 100.0f};

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool IsInteractableInRange() const;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool GetInteractableInRange(AActor*& OutActor, UActorComponent*& OutComponent, FHitResult& OutHitResult) const;

    void PrimaryInteract();

public:

    // Sets default values for this actor's properties
    UWotInteractionComponent();

    virtual void BeginPlay() override;
};
