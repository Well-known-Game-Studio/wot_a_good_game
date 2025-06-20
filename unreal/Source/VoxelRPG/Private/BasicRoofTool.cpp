// Copyright

#include "BasicRoofTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Generators/GridBoxMeshGenerator.h"
#include "Operations/MeshBoolean.h"
#include "DynamicMesh/MeshTransforms.h"

ABasicRoofTool::ABasicRoofTool()
{
}

void ABasicRoofTool::OnRebuildMesh(UDynamicMesh* TargetMesh)
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->Reset();
    FVector HalfDimensions = Dimensions * 0.5f;

    TargetMesh->EditMesh([&](FDynamicMesh3& RoofMesh)
    {
        // Create a box for the basic roof shape
        FGridBoxMeshGenerator BoxGen;
        BoxGen.Box = FOrientedBox3d(FVector3d::Zero(), FVector3d(HalfDimensions));
        BoxGen.Grid = FIndex3i(
            FMath::Max(1, Dimensions.X / VoxelSize),
            FMath::Max(1, Dimensions.Y / VoxelSize),
            FMath::Max(1, Dimensions.Z / VoxelSize));
        BoxGen.Generate(&RoofMesh);

        // Create a cutting volume
        FDynamicMesh3 CuttingVolume;
        FGridBoxMeshGenerator CutGen;
        CutGen.Box = FOrientedBox3d(FVector3d(0, HalfDimensions.Y/2, HalfDimensions.Z/2), FVector3d(HalfDimensions.X, HalfDimensions.Y/2, HalfDimensions.Z/2));
        CutGen.Grid = FIndex3i(
            FMath::Max(1, Dimensions.X / VoxelSize),
            FMath::Max(1, Dimensions.Y / (VoxelSize*2)),
            FMath::Max(1, Dimensions.Z / (VoxelSize*2)));
        CutGen.Generate(&CuttingVolume);

        FTransformSRT3d Transform(FQuaterniond(FVector3d(1,0,0), -45.0f * FMath::DegreesToRadians), FVector3d(0,0,HalfDimensions.Z));
        MeshTransforms::ApplyTransform(CuttingVolume, Transform);

        FDynamicMesh3 ResultMesh;
        FMeshBoolean Boolean(&RoofMesh, FTransform::Identity, &CuttingVolume, FTransform::Identity);
        if (Boolean.ComputeResult(ResultMesh, FMeshBoolean::EBooleanOp::Difference))
        {
            RoofMesh = MoveTemp(ResultMesh);
        }
    });
} 