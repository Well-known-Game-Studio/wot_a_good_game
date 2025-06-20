// Copyright

#include "BasicPlatformTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Generators/GridBoxMeshGenerator.h"

ABasicPlatformTool::ABasicPlatformTool()
{
}

void ABasicPlatformTool::OnRebuildMesh(UDynamicMesh* TargetMesh)
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->Reset();
    FVector HalfDimensions = Dimensions * 0.5f;

    TargetMesh->EditMesh([&](FDynamicMesh3& PlatformMesh)
    {
        FGridBoxMeshGenerator BoxGen;
        BoxGen.Box = FOrientedBox3d(FVector3d::Zero(), FVector3d(HalfDimensions));
        BoxGen.Grid = FIndex3i(
            FMath::Max(1, Dimensions.X / VoxelSize),
            FMath::Max(1, Dimensions.Y / VoxelSize),
            FMath::Max(1, Dimensions.Z / VoxelSize));
        BoxGen.Generate(&PlatformMesh);
    });
} 