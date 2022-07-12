// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WotAction.generated.h"

UCLASS(Blueprintable)
class VOXELRPG_API UWotAction : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotAction();

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Action")
    FName ActionName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Action", meta = (MultiLine = true))
    FName ActionDescription;

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void Start(AActor* Instigator);

    UFUNCTION(BlueprintNativeEvent, Category = "Action")
    void Stop(AActor* Instigator);
};
