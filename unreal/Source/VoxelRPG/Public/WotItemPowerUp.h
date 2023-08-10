// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotInteractableInterface.h"
#include "WotItemPowerUp.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotItemPowerUp : public AActor, public IWotInteractableInterface
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    float CooldownTime = 10.0f;

    virtual void Interact_Implementation(APawn* InstigatorPawn, FHitResult HitResult) override;

    virtual void GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult HitResult, FText& OutText) override;

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BaseMesh;

    virtual void SetPowerupState(bool bNewIsInteractable);

	FTimerHandle TimerHandle_Cooldown;

public:

    UFUNCTION()
    virtual void ShowPowerup();

    UFUNCTION()
    virtual void HideAndCooldownPowerup();

    // Sets default values for this actor's properties
    AWotItemPowerUp();
};
