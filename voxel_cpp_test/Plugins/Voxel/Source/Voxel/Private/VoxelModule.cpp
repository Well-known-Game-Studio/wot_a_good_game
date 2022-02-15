// Copyright 2020 Phyronnaz

#include "VoxelModule.h"

#include "VoxelValue.h"
#include "VoxelMaterial.h"
#include "VoxelMessages.h"
#include "IVoxelPool.h"
#include "VoxelUtilities/VoxelSerializationUtilities.h"

#include "Containers/Ticker.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/MessageDialog.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"

void FVoxelModule::StartupModule()
{
	LOG_VOXEL(Log, TEXT("VOXEL_DEBUG=%d"), VOXEL_DEBUG);
	
	{
		check((32 << (FVoxelUtilities::GetDepthFromSize<32>(1023) - 1)) < 1023);
		check((32 << FVoxelUtilities::GetDepthFromSize<32>(1023)) >= 1023);
		check((32 << (FVoxelUtilities::GetDepthFromSize<32>(1025) - 1)) < 1025);
		check((32 << FVoxelUtilities::GetDepthFromSize<32>(1025)) >= 1025);
		
		FVoxelMaterial Material(ForceInit);
		const auto CheckUV = [&](int32 Tex)
		{
			for (int32 Value = 0; Value < 256; Value++)
			{
#define checkBytesEqual(A, B) checkf(A == B, TEXT("%d == %d"), A, B);
				
				Material.SetU(Tex, Value);
				checkBytesEqual(Material.GetU(Tex), Value);

				Material.SetV(Tex, 255 - Value);
				checkBytesEqual(Material.GetV(Tex), 255 - Value);

				Material.SetV(Tex, Value);
				checkBytesEqual(Material.GetV(Tex), Value);
				
				Material.SetU(Tex, 255 - Value);
				checkBytesEqual(Material.GetU(Tex), 255 - Value);

				Material.SetU_AsFloat(Tex, 0.5f);
				check(Material.GetU_AsFloat(Tex) == FVoxelUtilities::UINT8ToFloat(FVoxelUtilities::FloatToUINT8(0.5f)));

				Material.SetV_AsFloat(Tex, 0.5f);
				check(Material.GetV_AsFloat(Tex) == FVoxelUtilities::UINT8ToFloat(FVoxelUtilities::FloatToUINT8(0.5f)));

#undef checkBytesEqual

				Material.SetUV_AsFloat(Tex, FVector2D(1.f, 1.f));
				checkf(Material.GetUV_AsFloat(Tex) == FVector2D(1.f, 1.f), TEXT("%s"), *Material.GetUV_AsFloat(Tex).ToString());
			}
		};
#if VOXEL_MATERIAL_ENABLE_UV0
		CheckUV(0);
#endif
#if VOXEL_MATERIAL_ENABLE_UV1
		CheckUV(1);
#endif
#if VOXEL_MATERIAL_ENABLE_UV2
		CheckUV(2);
#endif
#if VOXEL_MATERIAL_ENABLE_UV3
		CheckUV(3);
#endif
	}

	FVoxelSerializationUtilities::TestCompression(128, EVoxelCompressionLevel::BestSpeed);
	FVoxelSerializationUtilities::TestCompression(128, EVoxelCompressionLevel::BestCompression);
	
	//FVoxelSerializationUtilities::TestCompression(1llu << 32, EVoxelCompressionLevel::BestSpeed);

	{
		const auto Plugin = IPluginManager::Get().FindPlugin(VOXEL_PLUGIN_NAME);

		// This is needed to correctly share content across Pro and Free
		FPackageName::UnRegisterMountPoint(TEXT("/") VOXEL_PLUGIN_NAME TEXT("/"), Plugin->GetContentDir());
		FPackageName::RegisterMountPoint("/Voxel/", Plugin->GetContentDir());

		const FString PluginBaseDir = Plugin.IsValid() ? FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir()) : "";

		const FString PluginShaderDir = FPaths::Combine(PluginBaseDir, TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/Voxel"), PluginShaderDir);

#if USE_EMBREE_VOXEL
		const FString EmbreeBase = PluginBaseDir / "Source" / "ThirdParty" / "Embree3/";

		bool bSuccess = true;

#if PLATFORM_WINDOWS
		{
			const FString Folder = EmbreeBase / "Win64" / "lib";
			LOG_VOXEL(Log, TEXT("Loading embree3.dll from %s"), *Folder);

			FPlatformProcess::PushDllDirectory(*Folder);
			auto* Handle = FPlatformProcess::GetDllHandle(TEXT("embree3.dll"));
			FPlatformProcess::PopDllDirectory(*Folder);

			bSuccess = Handle != nullptr;
		}
#elif PLATFORM_MAC
		{
			const FString Folder = EmbreeBase / "MacOSX" / "lib";

			const TArray<FString> Libs =
			{
				"libtbb.dylib",
				"libtbbmalloc.dylib",
				"libembree3.3.dylib"
			};

			for (auto& Lib : Libs)
			{
				const FString Path = Folder / Lib;
				LOG_VOXEL(Log, TEXT("Loading %s"), *Path);
				auto* Handle = FPlatformProcess::GetDllHandle(*Path);
				bSuccess &= Handle != nullptr;
			}
		}
#endif

		if (!bSuccess)
		{
			FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Ok, VOXEL_LOCTEXT("Voxel Plugin: embree not found, voxel spawners won't work."));
		}
#endif
	}
		
	FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float)
	{
		int32 VoxelPluginVersion = 0;
		GConfig->GetInt(TEXT("VoxelPlugin"), TEXT("VoxelPluginVersion"), VoxelPluginVersion, GEditorPerProjectIni);
		
		const auto OpenLink = [=](const FString& Url)
		{
			FString Error;
			FPlatformProcess::LaunchURL(*Url, nullptr, &Error);
			if (!Error.IsEmpty())
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to open " + Url + "\n:" + Error));
			}
		};

		constexpr int32 LatestVoxelPluginVersion = 2;
		if (VoxelPluginVersion < LatestVoxelPluginVersion)
		{
			const auto Close = [=]()
			{
				GConfig->SetInt(TEXT("VoxelPlugin"), TEXT("VoxelPluginVersion"), LatestVoxelPluginVersion, GEditorPerProjectIni);
			};

			FVoxelMessages::FNotification Notification;
			Notification.UniqueId = OBJECT_LINE_ID();
			Notification.Message = "Voxel Plugin has been updated to 1.2!";
			Notification.Duration = 1e6f;
			Notification.OnClose = FSimpleDelegate::CreateLambda(Close);
			
			auto& Button = Notification.Buttons.Emplace_GetRef();
			Button.Text = "Show Release Notes";
			Button.Tooltip = "See the latest plugin release notes";
			Button.OnClick = FSimpleDelegate::CreateLambda([=]() 
			{
				OpenLink("https://releases.voxelplugin.com");
				Close();
			});
			
			FVoxelMessages::ShowNotification(Notification);
		}

		
		return false;
	}));
}

void FVoxelModule::ShutdownModule()
{
	IVoxelPool::Shutdown();
}

IMPLEMENT_MODULE(FVoxelModule, Voxel)