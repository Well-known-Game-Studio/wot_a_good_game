// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WotGameplayFunctionLibrary.generated.h"

UCLASS()
class VOXELRPG_API UWotGameplayFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    static bool ApplyDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    static bool ApplyDirectionalDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount, const FHitResult&HitResult);
};
