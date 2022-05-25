// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotGameplayInterface.h"
#include "WotItemHealthPotion.generated.h"

class UStaticMeshComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotItemHealthPotion : public AActor, public IWotGameplayInterface
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    float HealingAmount = 20.0f;

    UPROPERTY(EditAnywhere)
    float CooldownTime = 10.0f;

    void Interact_Implementation(APawn* InstigatorPawn);

protected:

    UPROPERTY()
    bool bIsInteractible = true;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BaseMesh;

	FTimerHandle TimerHandle_Cooldown;

	void Cooldown_TimeElapsed();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Sets default values for this actor's properties
    AWotItemHealthPotion();
};
