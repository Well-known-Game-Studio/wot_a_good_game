// Copyright

#pragma once

#include "CoreMinimal.h"
#include "GeneratedMeshActor.h"
#include "BasicPlatformTool.generated.h"

UCLASS()
class VOXELRPG_API ABasicPlatformTool : public AGeneratedMeshActor
{
    GENERATED_BODY()

public:
    ABasicPlatformTool();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Parameters")
    FVector Dimensions = FVector(150.f, 100.f, 10.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Parameters")
    int VoxelSize = 10;

protected:
    virtual void OnRebuildMesh(UDynamicMesh* TargetMesh) override;
}; 