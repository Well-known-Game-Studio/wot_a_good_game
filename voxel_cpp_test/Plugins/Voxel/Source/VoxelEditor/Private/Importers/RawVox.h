// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"

struct FVoxelDataAssetData;

namespace RawVox
{
	bool ImportToAsset(const FString& File, FVoxelDataAssetData& Asset);
}

