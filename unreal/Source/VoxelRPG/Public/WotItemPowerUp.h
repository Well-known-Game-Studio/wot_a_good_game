// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotGameplayInterface.h"
#include "WotInteractableInterface.h"
#include "WotItemPowerUp.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotItemPowerUp : public AActor, public IWotInteractableInterface, public IWotGameplayInterface
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    float CooldownTime = 10.0f;

    virtual void Interact_Implementation(APawn* InstigatorPawn, FHitResult HitResult) override;

    virtual void GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult HitResult, FText& OutText) override;

    virtual void Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration) override;

    virtual void Unhighlight_Implementation(FHitResult Hit) override;

    virtual void SetHighlightEnabled(int HighlightValue, bool Enabled);

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BaseMesh;

    virtual void SetPowerupState(bool bNewIsInteractable);

	FTimerHandle TimerHandle_Cooldown;

    FTimerHandle HighlightTimerHandle;
    void OnHighlightTimerExpired();

public:

    UFUNCTION()
    virtual void ShowPowerup();

    UFUNCTION()
    virtual void HideAndCooldownPowerup();

    // Sets default values for this actor's properties
    AWotItemPowerUp();
};
