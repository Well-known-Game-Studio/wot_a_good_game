// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelEngineVersionHelpers.h"

struct VOXEL_API FVoxelDebug
{
	template<typename T>
#if VOXEL_ENGINE_VERSION < 426
	using TDelegate = TMulticastDelegate<void, FName, const FIntVector&, TArrayView<const T>>;
#else
	using TDelegate = TMulticastDelegate<void(FName, const FIntVector&, TArrayView<const T>)>;
#endif
	
	template<typename T>
	static TDelegate<T>& GetDelegate();
	
	template<typename T>
	static void Broadcast(FName Name, const FIntVector& Size, TArrayView<T> Data)
	{
		GetDelegate<typename TRemoveConst<T>::Type>().Broadcast(Name, Size, Data);
	}
	template<typename T>
	static void Broadcast(FName Name, const FIntVector& Size, const TArray<T>& Data)
	{
		Broadcast(Name, Size, TArrayView<const T>(Data));
	}
};