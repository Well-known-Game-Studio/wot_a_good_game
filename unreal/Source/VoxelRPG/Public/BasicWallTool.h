// Copyright

#pragma once

#include "CoreMinimal.h"
#include "GeneratedMeshActor.h"
#include "BasicWallTool.generated.h"

UCLASS()
class VOXELRPG_API ABasicWallTool : public AGeneratedMeshActor
{
    GENERATED_BODY()

public:
    ABasicWallTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters")
    FVector Dimensions = FVector(100.f, 20.f, 200.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters")
    int VoxelSize = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters|Door")
    bool bHasDoor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters|Door", meta = (EditCondition = "bHasDoor"))
    FVector DoorPosition = FVector(0.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters|Door", meta = (EditCondition = "bHasDoor"))
    FVector DoorDimensions = FVector(80.f, 20.f, 180.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters|Window")
    bool bHasWindow = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters|Window", meta = (EditCondition = "bHasWindow"))
    FVector WindowPosition = FVector(0.f, 0.f, 100.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Parameters|Window", meta = (EditCondition = "bHasWindow"))
    FVector WindowDimensions = FVector(60.f, 20.f, 90.f);

protected:
    virtual void OnRebuildMesh(UDynamicMesh* TargetMesh) override;
}; 