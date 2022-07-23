// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "WotAction.generated.h"

class UWorld;
class UWotActionComponent;

UCLASS(Blueprintable)
class VOXELRPG_API UWotAction : public UObject
{
	GENERATED_BODY()

protected:

    UFUNCTION(BlueprintCallable, Category = "Action")
    UWotActionComponent* GetOwningComponent() const;

    UPROPERTY(EditDefaultsOnly, Category = "Tags")
    FGameplayTagContainer GrantsTags;

    UPROPERTY(EditDefaultsOnly, Category = "Tags")
    FGameplayTagContainer BlockedTags;

    UPROPERTY(VisibleAnywhere, Category = "Action")
    bool bIsRunning;

public:
	// Sets default values for this component's properties
	UWotAction();

    UFUNCTION(BlueprintCallable, Category = "Action")
    bool IsRunning() const;

    UFUNCTION(BlueprintCallable, Category = "Action")
    bool CanStart(AActor* Instigator);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Action")
    void Start(AActor* Instigator);

    virtual void Start_Implementation(AActor* Instigator);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Action")
    void Stop(AActor* Instigator);

    virtual void Stop_Implementation(AActor* Instigator);

    // Allows spawning of objects and things in BP as long as we can get
    // reference to world
    virtual UWorld* GetWorld() const override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Action")
    FName ActionName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Action", meta = (MultiLine = true))
    FName ActionDescription;
};
