// Copyright

#include "BasicWallTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshTransforms.h"
#include "Generators/GridBoxMeshGenerator.h"
#include "Operations/MeshBoolean.h"

ABasicWallTool::ABasicWallTool()
{
    // Set default values if needed
}

void ABasicWallTool::OnRebuildMesh(UDynamicMesh* TargetMesh)
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->Reset();
    FVector HalfDimensions = Dimensions * 0.5f;

    TargetMesh->EditMesh([&](FDynamicMesh3& WallMesh)
    {
        // Create a box mesh generator for the wall, centered at the origin
        FGridBoxMeshGenerator WallGen;
        WallGen.Box = FOrientedBox3d(FVector3d::Zero(), FVector3d(HalfDimensions));
        WallGen.Grid = FIndex3i(FMath::Max(1, Dimensions.X / VoxelSize),
                               FMath::Max(1, Dimensions.Y / VoxelSize),
                               FMath::Max(1, Dimensions.Z / VoxelSize));
        WallGen.Generate(&WallMesh);

        auto SubtractMesh = [&](FDynamicMesh3& BaseMesh, const FVector& Pos, const FVector& Dims)
        {
            FDynamicMesh3 SubMesh;
            FGridBoxMeshGenerator SubGen;
            FVector HalfSubDims = Dims * 0.5f;
            SubGen.Box = FOrientedBox3d(FVector3d(Pos), FVector3d(HalfSubDims));
            SubGen.Grid = FIndex3i(FMath::Max(1, Dims.X / VoxelSize),
                                   FMath::Max(1, Dims.Y / VoxelSize),
                                   FMath::Max(1, Dims.Z / VoxelSize));
            SubGen.Generate(&SubMesh);

            FDynamicMesh3 ResultMesh;
            FMeshBoolean Boolean(&BaseMesh, FTransform::Identity, &SubMesh, FTransform::Identity);
            if (Boolean.ComputeResult(ResultMesh, FMeshBoolean::EBooleanOp::Difference))
            {
                BaseMesh = MoveTemp(ResultMesh);
            }
        };

        if (bHasDoor)
        {
            SubtractMesh(WallMesh, DoorPosition, DoorDimensions);
        }

        if (bHasWindow)
        {
            SubtractMesh(WallMesh, WindowPosition, WindowDimensions);
        }
    });
} 