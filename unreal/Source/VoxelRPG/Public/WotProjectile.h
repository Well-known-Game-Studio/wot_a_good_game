// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotProjectile.generated.h"
class UAudioComponent;
class UCameraShakeBase;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
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

  UFUNCTION()
  virtual void OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
  USphereComponent* SphereComp;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
  float Damage = -20.0f;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
  float LifeSpan = 1.0f;

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Movement")
  UProjectileMovementComponent* MovementComp;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
  USoundBase* EffectSound;

  UPROPERTY()
  UAudioComponent* EffectAudioComp;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
  USoundBase* ImpactSound;

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Visual Effects")
  UStaticMeshComponent* StaticMeshComp;

  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects", meta = (AllowPrivateAccess = "true"))
  UNiagaraSystem* EffectNiagaraSystem;

  UPROPERTY()
  UNiagaraComponent* EffectNiagaraComp;

  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects", meta = (AllowPrivateAccess = "true"))
  UNiagaraSystem* ImpactNiagaraSystem;

  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects", meta = (AllowPrivateAccess = "true"))
  TSubclassOf<UCameraShakeBase> CameraShakeEffect;

  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects")
  float CameraShakeInnerRadius;
  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects")
  float CameraShakeOuterRadius;
  UPROPERTY(EditDefaultsOnly, Category = "Visual Effects")
  float CameraShakeFalloff;

  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  virtual void PostInitializeComponents() override;

  // BlueprintNativeEvent = C++ base implementation, can be expanded in Blueprints
  // BlueprintCallable to allow child classes to trigger explosions
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  void Explode();

public:
  // called every frame
  virtual void Tick(float DeltaTime) override;
};
