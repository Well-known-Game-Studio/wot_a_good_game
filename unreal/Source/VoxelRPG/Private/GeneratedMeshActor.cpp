// Copyright

#include "GeneratedMeshActor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Components/DynamicMeshComponent.h"

AGeneratedMeshActor::AGeneratedMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
	SetRootComponent(DynamicMeshComponent);
}

void AGeneratedMeshActor::RebuildMesh()
{
	if (DynamicMeshComponent)
	{
		DynamicMeshComponent->ModifyMesh([&](FDynamicMesh3& Mesh)
		{
			// This is not a great way to do this, but it's the easiest way to
			// get a clean mesh to work with.
			Mesh.Clear();
			OnRebuildMesh(DynamicMeshComponent->GetDynamicMesh());
		});
	}
}

void AGeneratedMeshActor::OnRebuildMesh(UDynamicMesh* TargetMesh)
{
	// Base implementation does nothing, to be overridden by subclasses
}

void AGeneratedMeshActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// Don't rebuild the mesh while the user is dragging a slider or gizmo
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		RebuildMesh();
	}
} 