// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WotDeathEffectComponent.generated.h"

class USoundBase;
class UNiagaraSystem;
class UMaterialInterface;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOXELRPG_API UWotDeathEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWotDeathEffectComponent();

protected:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* EffectSound;

    UPROPERTY(EditDefaultsOnly, Category = "Visual Effects", meta = (AllowPrivateAccess = "true"))
    UNiagaraSystem* EffectNiagaraSystem;

    UPROPERTY(EditDefaultsOnly, Category = "Visual Effects")
    UMaterialInterface* EffectMaterialBase;

    UPROPERTY(EditDefaultsOnly, Category = "Visual Effects")
    FName TextureParameterName = "Color Texture";

public:

    UFUNCTION(BlueprintCallable)
    void Play();
};
