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
    static bool GetClosestInteractibleInBox(AActor* InstigatorActor, FVector BoxHalfExtent, FVector Origin, FVector End, AActor* &ClosestActor, UActorComponent* &ClosestComponent, FHitResult &ClosestHit);

    UFUNCTION(BlueprintCallable, Category = "Debug")
    static void DrawHitPointAndBounds(AActor* HitActor, const FHitResult& Hit);

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    static bool ApplyDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    static bool ApplyDirectionalDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount, const FHitResult&HitResult);

    static FString GetFloatAsStringWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero=true);
    static FText GetFloatAsTextWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero=true);

    static FString GetIntAsString(int TheNumber);
    static FText GetIntAsText(int TheNumber);

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    static void GetAllCppSubclasses(UClass* BaseClass, TArray<UClass*>& ClassArray);

    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    static void GetAllBlueprintSubclasses(UClass* BaseClass, TArray<UClass*>& ClassArray);

};
