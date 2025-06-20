// Copyright

#pragma once

#include "CoreMinimal.h"
#include "GeneratedMeshActor.h"
#include "BasicStairsTool.generated.h"

UCLASS()
class VOXELRPG_API ABasicStairsTool : public AGeneratedMeshActor
{
    GENERATED_BODY()

public:
    ABasicStairsTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs Parameters")
    int NumSteps = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs Parameters")
    FVector StepDimensions = FVector(100.f, 30.f, 15.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stairs Parameters")
    int VoxelSize = 5;

protected:
    virtual void OnRebuildMesh(UDynamicMesh* TargetMesh) override;
}; 