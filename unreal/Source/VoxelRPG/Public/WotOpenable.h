// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotGameplayInterface.h"
#include "WotOpenable.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOpened, AActor*, InstigatorActor, AActor*, OpenableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClosed, AActor*, InstigatorActor, AActor*, OpenableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStateChanged, AActor*, InstigatorActor, AActor*, OpenableActor, bool, NewState);

class USceneComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotOpenable : public AActor, public IWotGameplayInterface
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Openable")
    bool bIsOpen = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Openable")
    bool bCanBeOpened = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Openable")
    bool bCanBeClosed = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* OpenSound;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* CloseSound;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects")
    UAudioComponent* EffectAudioComp;

    virtual void Interact_Implementation(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Openable")
    void Open(APawn* InstigatorPawn);

    virtual void Open_Implementation(APawn* InstigatorPawn);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Openable")
    void Close(APawn* InstigatorPawn);

    virtual void Close_Implementation(APawn* InstigatorPawn);

    UPROPERTY(BlueprintAssignable)
    FOnOpened OnOpened;

    UPROPERTY(BlueprintAssignable)
    FOnClosed OnClosed;

    UPROPERTY(BlueprintAssignable)
    FOnStateChanged OnStateChanged;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* BaseSceneComp;

public:
    // Sets default values for this actor's properties
    AWotOpenable();
};
