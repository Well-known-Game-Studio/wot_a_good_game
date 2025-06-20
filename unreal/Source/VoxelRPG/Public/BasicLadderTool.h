// Copyright

#pragma once

#include "CoreMinimal.h"
#include "GeneratedMeshActor.h"
#include "BasicLadderTool.generated.h"

UCLASS()
class VOXELRPG_API ABasicLadderTool : public AGeneratedMeshActor
{
    GENERATED_BODY()

public:
    ABasicLadderTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder Parameters")
    int NumRungs = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder Parameters")
    float RungSpacing = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder Parameters")
    FVector RungDimensions = FVector(50.f, 5.f, 5.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder Parameters")
    FVector SideRailDimensions = FVector(5.f, 5.f, 240.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder Parameters")
    int VoxelSize = 5;

protected:
    virtual void OnRebuildMesh(UDynamicMesh* TargetMesh) override;
}; 