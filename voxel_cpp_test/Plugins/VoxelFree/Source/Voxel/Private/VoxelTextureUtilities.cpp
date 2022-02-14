// Copyright 2021 Phyronnaz

#include "VoxelUtilities/VoxelTextureUtilities.h"
#include "VoxelMinimal.h"
#include "Engine/Texture2D.h"

void FVoxelTextureUtilities::UpdateColorTexture(UTexture2D*& Texture, const FIntPoint& Size, const TArray<FColor>& Colors)
{
	VOXEL_FUNCTION_COUNTER();
	
	check(Colors.Num() == Size.X * Size.Y);
	
	if (!Texture || Texture->GetSizeX() != Size.X || Texture->GetSizeY() != Size.Y)
	{
		Texture = UTexture2D::CreateTransient(Size.X, Size.Y);
		if (!ensure(Texture))
		{
			return;
		}
		// TODO why not TC_VectorDisplacementmap
		Texture->CompressionSettings = TC_HDR;
		Texture->SRGB = false;
	}
	FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
	{
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		if (ensure(Data))
		{
			FMemory::Memcpy(Data, Colors.GetData(), Colors.Num() * sizeof(FColor));
		}
		Mip.BulkData.Unlock();
	}
	Texture->UpdateResource();
}