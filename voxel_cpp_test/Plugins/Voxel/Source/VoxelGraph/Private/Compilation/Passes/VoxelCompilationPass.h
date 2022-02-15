// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"

class FVoxelGraphCompiler;

#define VOXEL_PASS_BODY(Class) static const TCHAR* GetName() { sizeof(Class); return TEXT(#Class); }

