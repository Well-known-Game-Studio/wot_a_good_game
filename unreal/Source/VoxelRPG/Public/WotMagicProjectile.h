// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotMagicProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystemComponent;

UCLASS()
class VOXELRPG_API AWotMagicProjectile : public AActor
{
  GENERATED_BODY()

public:

  AWotMagicProjectile();

protected:

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
  USphereComponent* SphereComp;

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
  UProjectileMovementComponent* MovementComp;

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
  UParticleSystemComponent* EffectComp;

  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // called every frame
  virtual void Tick(float DeltaTime) override;
};
