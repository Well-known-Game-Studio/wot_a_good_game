// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotItemActor.generated.h"

class UStaticMeshComponent;
class UWotItem;

/*
 * 	Simple container of UWotItem and UStaticMeshComponent which can be
 * 	placed in the world. Also allows physics / collision to be enabled so
 * 	that the item can interact with the world.
 */
UCLASS()
class VOXELRPG_API AWotItemActor : public AActor
{
	GENERATED_BODY()

public:

    virtual void SetItem(UWotItem* NewItem);

    UFUNCTION(BlueprintCallable)
	void SetPhysicsAndCollision(FName CollisionProfileName, bool EnablePhysics, bool EnableCollision);

	AWotItemActor();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Items")
	UWotItem* Item;

};
