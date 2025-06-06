// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WotActionComponent.generated.h"

class UWotAction;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Actions")
    static UWotActionComponent* GetActions(AActor* FromActor);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
    FGameplayTagContainer ActiveGameplayTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
    bool bEnableActions = true;

    UFUNCTION(BlueprintCallable, Category = "Actions")
    void AddAction(TSubclassOf<UWotAction> Action);

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool StartActionByName(FName ActionName, AActor* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool StopActionByName(FName ActionName, AActor* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool StopAllActions(AActor* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool IsActionRunning(FName ActionName) const;

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool IsAnyActionRunning() const;

	// Sets default values for this component's properties
	UWotActionComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, Category = "Actions")
    TArray<TSubclassOf<UWotAction>> DefaultActions;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actions")
    TArray<UWotAction*> Actions;
};
