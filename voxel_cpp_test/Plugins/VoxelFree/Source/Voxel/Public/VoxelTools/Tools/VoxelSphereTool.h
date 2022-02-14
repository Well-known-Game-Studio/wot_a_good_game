// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelInterval.h"
#include "VoxelTools/Tools/VoxelSphereToolBase.h"
#include "VoxelSphereTool.generated.h"

UCLASS()
class VOXEL_API UVoxelSphereTool : public UVoxelSphereToolBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Tool Preview Settings", EditAnywhere, BlueprintReadWrite, meta = (HideInPanel))
	UMaterialInterface* OverlayMaterial = nullptr;

public:
	UPROPERTY(Category = "Tool Settings", EditAnywhere, BlueprintReadWrite)
	bool bSculpt = true;

	UPROPERTY(Category = "Tool Settings", EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle))
	bool bEnableIndexInterval = false;

	// Will only edit voxels with an index contained in this interval
	// Only works in the single index edit mode, and only when removing voxels
	// Mainly intended for the cubic mode
	UPROPERTY(Category = "Tool Settings", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableIndexInterval"))
	FVoxelInt32Interval IndexInterval = { 0, 10 };
	
	UPROPERTY(Category = "Paint Settings", EditAnywhere, BlueprintReadWrite)
	bool bPaint = false;
	
	UPROPERTY(Category = "Paint Settings", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bPaint", UIMin = "0", UIMax = "1"))
	float PaintStrength = 0.5f;

	// Paint falloff type
	UPROPERTY(Category = "Paint Settings", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bPaint && !bSculpt"))
	EVoxelFalloff FalloffType = EVoxelFalloff::Smooth;

	// Paint falloff
	UPROPERTY(Category = "Paint Settings", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bPaint && !bSculpt", UIMin = 0, UIMax = 1))
	float Falloff = 0.5f;

public:
	UVoxelSphereTool();
	
	//~ Begin UVoxelToolBase Interface
	virtual void GetToolConfig(FVoxelToolBaseConfig& OutConfig) const override;
	virtual void UpdateRender(UMaterialInstanceDynamic* OverlayMaterialInstance, UMaterialInstanceDynamic* MeshMaterialInstance) override;
	virtual FVoxelIntBoxWithValidity DoEdit() override;
	//~ End UVoxelToolBase Interface
};