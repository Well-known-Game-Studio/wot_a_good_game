// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotGameplayInterface.h"
#include "WotOpenable.generated.h"

class USceneComponent;
class APawn;

UCLASS()
class VOXELRPG_API AWotOpenable : public AActor, public IWotGameplayInterface
{
	GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Openable")
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

protected:

    UPROPERTY(VisibleAnywhere)
    USceneComponent* BaseSceneComp;

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Sets default values for this actor's properties
    AWotOpenable();
};
