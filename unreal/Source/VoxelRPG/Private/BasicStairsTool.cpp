// Copyright

#include "BasicStairsTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Generators/GridBoxMeshGenerator.h"
#include "Operations/MeshBoolean.h"

ABasicStairsTool::ABasicStairsTool()
{
}

void ABasicStairsTool::OnRebuildMesh(UDynamicMesh* TargetMesh)
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->Reset();
    FVector HalfStepDimensions = StepDimensions * 0.5f;

    TargetMesh->EditMesh([&](FDynamicMesh3& StairsMesh)
    {
        for (int32 i = 0; i < NumSteps; ++i)
        {
            FVector StepPosition = FVector(0, i * StepDimensions.Y, i * StepDimensions.Z);

            FDynamicMesh3 StepMesh;
            FGridBoxMeshGenerator BoxGen;
            BoxGen.Box = FOrientedBox3d(StepPosition, FVector3d(HalfStepDimensions));
            BoxGen.Grid = FIndex3i(
                FMath::Max(1, StepDimensions.X / VoxelSize),
                FMath::Max(1, StepDimensions.Y / VoxelSize),
                FMath::Max(1, StepDimensions.Z / VoxelSize));
            BoxGen.Generate(&StepMesh);

            if (i == 0)
            {
                StairsMesh = MoveTemp(StepMesh);
            }
            else
            {
                FDynamicMesh3 ResultMesh;
                FMeshBoolean Boolean(&StairsMesh, FTransform::Identity, &StepMesh, FTransform::Identity);
                if (Boolean.ComputeResult(ResultMesh, FMeshBoolean::EBooleanOp::Union))
                {
                    StairsMesh = MoveTemp(ResultMesh);
                }
            }
        }
    });
} 