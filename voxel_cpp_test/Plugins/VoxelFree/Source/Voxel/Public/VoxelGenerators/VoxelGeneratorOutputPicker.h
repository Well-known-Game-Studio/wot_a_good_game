// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelGeneratorOutputPicker.generated.h"

USTRUCT(BlueprintType)
struct FVoxelGeneratorOutputPicker
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	FName Name;

	FVoxelGeneratorOutputPicker() = default;
	FVoxelGeneratorOutputPicker(FName Name)
		: Name(Name)
	{
	}
	FVoxelGeneratorOutputPicker(const ANSICHAR* Name)
		: Name(Name)
	{
	}

	operator FName() const
	{
		return Name;
	}
	bool IsNone() const
	{
		return Name.IsNone();
	}

	friend bool operator==(const FVoxelGeneratorOutputPicker& Lhs, const FVoxelGeneratorOutputPicker& Rhs)
	{
		return Lhs.Name == Rhs.Name;
	}
};