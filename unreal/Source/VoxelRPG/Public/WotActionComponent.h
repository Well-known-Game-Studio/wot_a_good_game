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

    UFUNCTION(BlueprintCallable, Category = "Actions")
    void AddAction(TSubclassOf<UWotAction> Action);

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool StartActionByName(AActor* Instigator, FName ActionName);

    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool StopActionByName(AActor* Instigator, FName ActionName);

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
