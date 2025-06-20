// Copyright

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeneratedMeshActor.generated.h"

class UDynamicMeshComponent;
class UDynamicMesh;

UCLASS()
class VOXELRPG_API AGeneratedMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AGeneratedMeshActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDynamicMeshComponent* DynamicMeshComponent;

	UFUNCTION(BlueprintCallable, Category = "Mesh Generation")
	virtual void RebuildMesh();

protected:
	UFUNCTION(BlueprintCallable, Category = "Mesh Generation")
	virtual void OnRebuildMesh(UDynamicMesh* TargetMesh);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
}; 