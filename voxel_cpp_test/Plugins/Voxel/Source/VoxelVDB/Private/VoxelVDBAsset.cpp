// Copyright 2020 Phyronnaz

#include "VoxelVDBAsset.h"
#include "VoxelVDBInclude.h"

#include "VoxelMessages.h"
#include "VoxelObjectArchive.h"
#include "VoxelFeedbackContext.h"
#include "VoxelGenerators/VoxelGeneratorHelpers.h"
#include "VoxelGenerators/VoxelTransformableGeneratorHelper.h"
#include "VoxelGenerators/VoxelGeneratorInstance.inl"
#include "VoxelGenerators/VoxelEmptyGenerator.h"

#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

struct FVoxelVDBAssetDataChannel
{
public:
	const EVoxelVDBChannel Channel;
	float Min = 0;
	float Max = 1;
	FVoxelIntBox Bounds;

	explicit FVoxelVDBAssetDataChannel(EVoxelVDBChannel Channel)
		: Channel(Channel)
	{
	}
	
	bool IsValid() const { return bool(Grid); }
	const openvdb::FloatGrid& GetGrid() const { return *Grid; }
	const openvdb::FloatGrid::Ptr& GetGridPtr() const { return Grid; }
	const openvdb::tools::FindActiveValues<openvdb::FloatTree>& GetFindActiveValues() const { return *FindActiveValues; }

public:
	void SetGrid(const openvdb::FloatGrid::Ptr& NewGrid)
	{
		Grid = NewGrid;
		if (Grid)
		{
			FindActiveValues = MakeUnique<openvdb::tools::FindActiveValues<openvdb::FloatTree>>(Grid->tree());
		}
		else
		{
			FindActiveValues = nullptr;
		}
	}

public:
	void Save(FArchive& Ar)
	{
		check(Ar.IsSaving());

		EVoxelVDBChannel ChannelCopy = Channel;
		Ar << ChannelCopy;
		Ar << Min;
		Ar << Max;
		Ar << Bounds;
		
		std::ostringstream StringStream(std::ios_base::binary);
		openvdb::io::Stream(StringStream).write({ Grid });

		const std::string String = StringStream.str();
		int64 Size = String.size();

		Ar << Size;
		Ar.Serialize(const_cast<char*>(String.c_str()), Size);
	}
	static TUniquePtr<FVoxelVDBAssetDataChannel> Load(FArchive& Ar)
	{
		check(Ar.IsLoading());
		
		EVoxelVDBChannel Channel;
		Ar << Channel;

		auto Result = MakeUnique<FVoxelVDBAssetDataChannel>(Channel);
		
		Ar << Result->Min;
		Ar << Result->Max;
		Ar << Result->Bounds;
		
		int64 Size = 0;
		Ar << Size;

		const std::string String(Size, 0);
		Ar.Serialize(const_cast<char*>(String.c_str()), Size);

		std::istringstream StringStream(String, std::ios_base::binary);

		openvdb::io::Stream Stream(StringStream);
		const auto Grids = Stream.getGrids();

		check(Grids->size() == 1);
		const auto FloatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(Grids->at(0));
		check(FloatGrid);

		Result->SetGrid(FloatGrid);

		return MoveTemp(Result);
	}

private:
	openvdb::FloatGrid::Ptr Grid;
	TUniquePtr<openvdb::tools::FindActiveValues<openvdb::FloatTree>> FindActiveValues;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelVDBAssetData::FVoxelVDBAssetData()
{
	for (uint32 Index = 0; Index < Channels.Num(); Index++)
	{
		Channels[Index] = MakeUnique<FVoxelVDBAssetDataChannel>(EVoxelVDBChannel(Index));
	}
}

FVoxelVDBAssetData::~FVoxelVDBAssetData()
{
}

bool FVoxelVDBAssetData::LoadVDB(const FString& Path, FString& OutError, const TFunction<bool(TMap<FName, FVoxelVDBImportChannelConfig>&)>& GetChannelConfigs)
{
	VOXEL_FUNCTION_COUNTER();

	try
	{
		openvdb::io::File File(TCHAR_TO_UTF8(*Path));

		File.open();

		const auto Grids = File.getGrids();
		if (Grids->size() == 0)
		{
			OutError = "No grids";
			return false;
		}

		TMap<FName, FVoxelVDBImportChannelConfig> ChannelConfigs;
		TMap<FName, openvdb::FloatGrid::Ptr> NamesToGrids;
		for (auto& Grid : *Grids)
		{
			FName Name;

			try
			{
				const auto StdName = Grid->metaValue<std::string>("name");
				Name = *FString(StdName.c_str());
			}
			catch (openvdb::LookupError&)
			{
				Name = TEXT("UNAMED GRID");
			}
			catch (openvdb::TypeError&)
			{
				Name = TEXT("UNAMED GRID (Type Error)");
			}
			
			const auto FloatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(Grid);
			if (!FloatGrid)
			{
				LOG_VOXEL(Log, TEXT("Skipping grid named %s, as it's not a float grid"), *Name.ToString());
				continue;
			}
			
			while (ChannelConfigs.Contains(Name))
			{
				Name.SetNumber(Name.GetNumber() + 1);
			}

			NamesToGrids.Add(Name, FloatGrid);
			
			LOG_VOXEL(Log, TEXT("Found float grid named %s"), *Name.ToString());
			ChannelConfigs.Add(Name);
		}

		if (GetChannelConfigs && !GetChannelConfigs(ChannelConfigs))
		{
			OutError = "Cancelled";
			return false;
		}

		for (auto& It : ChannelConfigs)
		{
			const auto Grid = NamesToGrids.FindRef(It.Key);
			if (!Grid)
			{
				OutError = "Missing channel from vdb: " + It.Key.ToString();
				return false;
			}

			float Min = MAX_flt;
			float Max = -MAX_flt;
			FVoxelIntBoxWithValidity Bounds;
			for (openvdb::FloatGrid::ValueOnCIter Iterator = Grid->cbeginValueOn(); Iterator.test(); ++Iterator) 
			{
				const float& Value = *Iterator;

				Min = FMath::Min(Min, Value);
				Max = FMath::Max(Max, Value);
				
				openvdb::CoordBBox BoundingBox;
				Iterator.getBoundingBox(BoundingBox);

				Bounds += FVoxelIntBox
				{
					FIntVector
					{
						BoundingBox.min().x(),
						BoundingBox.min().z(),
						BoundingBox.min().y(),
					},
					FIntVector
					{
						BoundingBox.max().x() + 1,
						BoundingBox.max().z() + 1,
						BoundingBox.max().y() + 1,
					}
				};
			}

			if (Bounds.IsValid()) // Else grid is empty
			{
				const auto& Channel = Channels[int32(It.Value.TargetChannel)];
				Channel->SetGrid(Grid);
				Channel->Min = It.Value.bAutoMinMax ? Min : It.Value.Min;
				Channel->Max = It.Value.bAutoMinMax ? Max : It.Value.Max;
				Channel->Bounds = Bounds.GetBox();
			}
		}

		File.close();

		return true;
	}
	catch (openvdb::IoError& IoError)
	{
		OutError = IoError.what();
		return false;
	}
	catch (openvdb::LookupError& LookupError)
	{
		OutError = LookupError.what();
		return false;
	}
}

bool FVoxelVDBAssetData::SaveVDB(const FString& Path, FString& OutError) const
{
	VOXEL_FUNCTION_COUNTER();

	try
	{
		openvdb::io::File File(TCHAR_TO_UTF8(*Path));

		openvdb::GridCPtrVec Grids;
		for (auto& Channel : Channels)
		{
			if (Channel->IsValid())
			{
				const auto Grid = Channel->GetGridPtr();
				Grid->insertMeta("name", openvdb::StringMetadata(TCHAR_TO_UTF8(*StaticEnum<EVoxelVDBChannel>()->GetNameStringByValue(int64(Channel->Channel)))));
				Grids.push_back(Grid);
			}
		}
		
		File.write(Grids);
		File.close();
		
		return true;
	}
	catch (openvdb::IoError& IoError)
	{
		OutError = IoError.what();
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelVDBAssetData::IsValid() const
{
	for (auto& Channel : Channels)
	{
		if (Channel->IsValid())
		{
			return true;
		}
	}
	return false;
}

void FVoxelVDBAssetData::Save(TArray<uint8>& OutData) const
{
	FMemoryWriter Writer(OutData);
	
	int32 NumChannels = 0;
	for (auto& Channel : Channels)
	{
		if (Channel->IsValid())
		{
			NumChannels++;
		}
	}
	Writer << NumChannels;

	for (auto& Channel : Channels)
	{
		if (Channel->IsValid())
		{
			Channel->Save(Writer);
		}
	}
}

void FVoxelVDBAssetData::Load(const TArray<uint8>& Data)
{
	FMemoryReader Reader(Data);
	
	int32 NumChannels = 0;
	Reader << NumChannels;
	
	for (int32 Index = 0; Index < NumChannels; Index++)
	{
		auto Channel = FVoxelVDBAssetDataChannel::Load(Reader);
		Channels[int32(Channel->Channel)] = MoveTemp(Channel);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelIntBoxWithValidity FVoxelVDBAssetData::GetBounds() const
{
	FVoxelIntBoxWithValidity Bounds;
	for (auto& Channel : Channels)
	{
		if (Channel->IsValid())
		{
			Bounds += Channel->Bounds;
		}
	}
	return Bounds;
}

float FVoxelVDBAssetData::GetValue(double X, double Y, double Z) const
{
	const auto& DensityChannel = Channels[int32(EVoxelVDBChannel::Density)];
	if (!DensityChannel->IsValid())
	{
		return 1.f;
	}
	
	const auto& Tree = DensityChannel->GetGrid().constTree();
	
	const openvdb::Vec3R Position(X, Z, Y);
	
	// Compute the value of the grid at ijk via nearest-neighbor (zero-order)
	// interpolation.
	//float v0 = openvdb::tools::PointSampler::sample(Tree, Position);
	// Compute the value via triquadratic (second-order) interpolation.
	//float v2 = openvdb::tools::QuadraticSampler::sample(Tree, Position);

	return openvdb::tools::BoxSampler::sample(Tree, Position);
}

FVoxelMaterial FVoxelVDBAssetData::GetMaterial(double X, double Y, double Z) const
{
	const openvdb::Vec3R Position(X, Z, Y);
	
	FVoxelMaterial Material{ ForceInit };

#define CHANNEL(Name) \
	{ \
		const auto& Channel = Channels[int32(EVoxelVDBChannel::Name)]; \
		if (Channel->IsValid()) \
		{ \
			const auto& Tree = Channel->GetGrid().constTree(); \
			const float Value = openvdb::tools::BoxSampler::sample(Tree, Position); \
			Material.Set##Name##_AsFloat((Value - Channel->Min) / (Channel->Max - Channel->Min)); \
		} \
	}
	
	CHANNEL(R);
	CHANNEL(G);
	CHANNEL(B);
	CHANNEL(A);
	CHANNEL(U0);
	CHANNEL(U1);
	CHANNEL(U2);
	CHANNEL(U3);
	CHANNEL(V0);
	CHANNEL(V1);
	CHANNEL(V2);
	CHANNEL(V3);

#undef CHANNEL

	return Material;
}

TVoxelRange<float> FVoxelVDBAssetData::GetValueRange(const FVoxelIntBox& Bounds) const
{
	const auto& DensityChannel = Channels[int32(EVoxelVDBChannel::Density)];
	if (!DensityChannel->IsValid())
	{
		return 1.f;
	}
	
	const openvdb::CoordBBox Box(
		{
			Bounds.Min.X,
			Bounds.Min.Z,
			Bounds.Min.Y
		},
		{
			Bounds.Max.X,
			Bounds.Max.Z,
			Bounds.Max.Y
		});

	if (DensityChannel->GetFindActiveValues().any(Box))
	{
		return { -1, 1 };
	}
	else
	{
		return 1.f;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelVDBAssetInstance : public TVoxelGeneratorInstanceHelper<FVoxelVDBAssetInstance, UVoxelVDBAsset>
{
public:
	using Super = TVoxelGeneratorInstanceHelper<FVoxelVDBAssetInstance, UVoxelVDBAsset>;
	
	const TVoxelSharedPtr<const FVoxelVDBAssetData> Data;

public:
	explicit FVoxelVDBAssetInstance(UVoxelVDBAsset& Asset)
		: Super(&Asset)
		, Data(Asset.GetData())
	{
	}

	//~ Begin FVoxelGeneratorInstance Interface
	v_flt GetValueImpl(v_flt X, v_flt Y, v_flt Z, int32 LOD, const FVoxelItemStack& Items) const
	{
		return Data->GetValue(X, Y, Z);
	}
	
	FVoxelMaterial GetMaterialImpl(v_flt X, v_flt Y, v_flt Z, int32 LOD, const FVoxelItemStack& Items) const
	{
		return Data->GetMaterial(X, Y, Z);
	}
	
	TVoxelRange<v_flt> GetValueRangeImpl(const FVoxelIntBox& Bounds, int32 LOD, const FVoxelItemStack& Items) const
	{
		return TVoxelRange<v_flt>(Data->GetValueRange(Bounds));
	}
	FVector GetUpVector(v_flt X, v_flt Y, v_flt Z) const override final
	{
		return FVector::UpVector;
	}
	//~ End FVoxelGeneratorInstance Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelIntBox UVoxelVDBAsset::GetBounds() const
{
	return Bounds;
}

TVoxelSharedRef<FVoxelGeneratorInstance> UVoxelVDBAsset::GetInstance()
{
	return GetInstanceImpl();
}

TVoxelSharedRef<FVoxelTransformableGeneratorInstance> UVoxelVDBAsset::GetTransformableInstance()
{
	const bool bSubtractiveAsset = false;
	return MakeVoxelShared<TVoxelTransformableGeneratorHelper<FVoxelVDBAssetInstance>>(GetInstanceImpl(), bSubtractiveAsset);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelSharedRef<const FVoxelVDBAssetData> UVoxelVDBAsset::GetData()
{
	TryLoad();
	return Data;
}

void UVoxelVDBAsset::SetData(const TVoxelSharedRef<FVoxelVDBAssetData>& InData)
{
	Data = InData;
	Save();
	
	const auto DataBounds = Data->GetBounds();
	Bounds = DataBounds.IsValid() ? DataBounds.GetBox() : FVoxelIntBox();

	MemorySizeInMB = CompressedData.Num() / double(1 << 20);
}

TVoxelSharedRef<FVoxelVDBAssetInstance> UVoxelVDBAsset::GetInstanceImpl()
{
	return MakeVoxelShared<FVoxelVDBAssetInstance>(*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelVDBAsset::Save()
{
	VOXEL_FUNCTION_COUNTER();
	
	FVoxelScopedSlowTask Saving(1.f);

	Modify();

	VoxelCustomVersion = FVoxelVDBAssetDataVersion::LatestVersion;
	Data->Save(CompressedData);
}


void UVoxelVDBAsset::Load()
{
	VOXEL_FUNCTION_COUNTER();
	
	if (CompressedData.Num() == 0)
	{
		// Nothing to load
		return;
	}
	
	Data->Load(CompressedData);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelVDBAsset::TryLoad()
{
	if (!Data->IsValid())
	{
		// Seems invalid, try to load
		Load();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelVDBAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if ((Ar.IsLoading() || Ar.IsSaving()) && !Ar.IsTransacting())
	{
		CompressedData.BulkSerialize(Ar);
	}
}
