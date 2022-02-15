// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"

struct FVoxelDataAssetData;

namespace MagicaVox
{
	bool ImportToAsset(const FString& Filename, FVoxelDataAssetData& Asset, bool bUsePalette);
}
