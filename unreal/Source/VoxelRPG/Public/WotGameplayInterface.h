// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WotGameplayInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWotGameplayInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *   Base interface for all objects.
 */
class VOXELRPG_API IWotGameplayInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Gameplay")
    void Highlight(FHitResult Hit, int HighlightValue, float Duration=0);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Gameplay")
    void Unhighlight(FHitResult Hit);
};
