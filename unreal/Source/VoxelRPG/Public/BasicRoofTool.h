// Copyright

#pragma once

#include "CoreMinimal.h"
#include "GeneratedMeshActor.h"
#include "BasicRoofTool.generated.h"

UCLASS()
class VOXELRPG_API ABasicRoofTool : public AGeneratedMeshActor
{
    GENERATED_BODY()

public:
    ABasicRoofTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Parameters")
    FVector Dimensions = FVector(200.f, 200.f, 100.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Parameters")
    int VoxelSize = 10;

protected:
    virtual void OnRebuildMesh(UDynamicMesh* TargetMesh) override;
}; 