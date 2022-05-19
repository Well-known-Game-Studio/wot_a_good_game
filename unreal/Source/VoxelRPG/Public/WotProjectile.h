// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotProjectile.generated.h"
class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class VOXELRPG_API AWotProjectile : public AActor
{
  GENERATED_BODY()

public:

  AWotProjectile();

protected:

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
  USphereComponent* SphereComp;

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Movement")
  UProjectileMovementComponent* MovementComp;

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Visual Effects")
  UStaticMeshComponent* StaticMeshComp;

  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects", meta = (AllowPrivateAccess = "true"))
  UNiagaraSystem* NiagaraSystem;

  UPROPERTY()
  UNiagaraComponent* NiagaraComp;

  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // called every frame
  virtual void Tick(float DeltaTime) override;
};
