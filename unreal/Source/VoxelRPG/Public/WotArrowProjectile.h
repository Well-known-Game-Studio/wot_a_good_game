// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotProjectile.h"
#include "WotArrowState.h"
#include "WotArrowProjectile.generated.h"

class UWotItem;

UCLASS()
class VOXELRPG_API AWotArrowProjectile : public AWotProjectile
{
  GENERATED_BODY()

public:

  AWotArrowProjectile();

protected:

	UPROPERTY(EditAnywhere, Category = "Item")
	TSubclassOf<UWotItem> ItemClass;

  UPROPERTY(VisibleAnywhere)
  EWotArrowState CurrentState;

  UPROPERTY(EditAnywhere)
  AActor* Shooter;

  UPROPERTY(EditAnywhere)
  float BowCharge;

  UPROPERTY(EditAnywhere)
  float MinVelocity = 100.0f;

  UPROPERTY(EditAnywhere)
  float MaxVelocity = 4000.0f;

  UPROPERTY(EditAnywhere)
  float PenetrationDepth = 50.0f;

  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  virtual void PostInitializeComponents() override;

  virtual void Explode_Implementation() override;

  UFUNCTION(BlueprintCallable)
  void SetArrowState(EWotArrowState NewArrowState);

  UFUNCTION(BlueprintCallable)
  void Fire(AActor* NewShooter, float NewBowCharge);

  UFUNCTION()
  void HandleCollision(AActor* OtherActor, const FHitResult& SweepResult);

  UFUNCTION()
  virtual void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

  UFUNCTION()
  virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

  UFUNCTION()
  void OnStateBegin(EWotArrowState BeginArrowState);

  UFUNCTION()
  void OnStateEnd(EWotArrowState EndArrowState);

};
