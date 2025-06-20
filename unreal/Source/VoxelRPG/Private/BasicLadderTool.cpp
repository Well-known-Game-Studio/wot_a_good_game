// Copyright

#include "BasicLadderTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Generators/GridBoxMeshGenerator.h"
#include "Operations/MeshBoolean.h"

ABasicLadderTool::ABasicLadderTool()
{
}

void ABasicLadderTool::OnRebuildMesh(UDynamicMesh* TargetMesh)
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->Reset();

    TargetMesh->EditMesh([&](FDynamicMesh3& LadderMesh)
    {
        // Adjust side rail height based on number of rungs and spacing
        float TotalHeight = NumRungs * RungSpacing;
        SideRailDimensions.Z = TotalHeight;
        FVector HalfSideRailDimensions = SideRailDimensions * 0.5f;

        // Generate side rails
        auto GenerateRail = [&](float XOffset)
        {
            FDynamicMesh3 RailMesh;
            FGridBoxMeshGenerator BoxGen;
            FVector RailPosition = FVector(XOffset, 0, TotalHeight * 0.5f);
            BoxGen.Box = FOrientedBox3d(RailPosition, FVector3d(HalfSideRailDimensions));
            BoxGen.Grid = FIndex3i(
                FMath::Max(1, SideRailDimensions.X / VoxelSize),
                FMath::Max(1, SideRailDimensions.Y / VoxelSize),
                FMath::Max(1, SideRailDimensions.Z / VoxelSize));
            BoxGen.Generate(&RailMesh);
            return RailMesh;
        };

        FDynamicMesh3 LeftRail = GenerateRail(-RungDimensions.X * 0.5f);
        FDynamicMesh3 RightRail = GenerateRail(RungDimensions.X * 0.5f);

        // Merge side rails
        FMeshBoolean Boolean(&LeftRail, FTransform::Identity, &RightRail, FTransform::Identity);
        Boolean.ComputeResult(LadderMesh, FMeshBoolean::EBooleanOp::Union);

        // Generate and merge rungs
        FVector HalfRungDimensions = RungDimensions * 0.5f;
        for (int32 i = 0; i < NumRungs; ++i)
        {
            FDynamicMesh3 RungMesh;
            FGridBoxMeshGenerator BoxGen;
            FVector RungPosition = FVector(0, 0, (i + 0.5f) * RungSpacing);
            BoxGen.Box = FOrientedBox3d(RungPosition, FVector3d(HalfRungDimensions));
            BoxGen.Grid = FIndex3i(
                FMath::Max(1, RungDimensions.X / VoxelSize),
                FMath::Max(1, RungDimensions.Y / VoxelSize),
                FMath::Max(1, RungDimensions.Z / VoxelSize));
            BoxGen.Generate(&RungMesh);

            FDynamicMesh3 ResultMesh;
            FMeshBoolean RungBoolean(&LadderMesh, FTransform::Identity, &RungMesh, FTransform::Identity);
            if (RungBoolean.ComputeResult(ResultMesh, FMeshBoolean::EBooleanOp::Union))
            {
                LadderMesh = MoveTemp(ResultMesh);
            }
        }
    });
} 