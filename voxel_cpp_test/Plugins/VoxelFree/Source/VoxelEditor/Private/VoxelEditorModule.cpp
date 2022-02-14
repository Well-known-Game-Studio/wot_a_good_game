// Copyright 2021 Phyronnaz

#include "VoxelEditorModule.h"
#include "VoxelMinimal.h"

#include "Interfaces/IPluginManager.h"
#include "IPlacementModeModule.h"
#include "PropertyEditorModule.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "EditorModeRegistry.h"
#include "EditorSupportDelegates.h"
#include "LevelEditor.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "MessageLogModule.h"
#include "EditorReimportHandler.h"
#include "Framework/Commands/Commands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#include "VoxelGraphGenerator.h"
#include "VoxelPool.h"
#include "VoxelWorld.h"
#include "VoxelTexture.h"
#include "VoxelMessages.h"
#include "VoxelBoolVector.h"
#include "VoxelMessagesEditor.h"
#include "VoxelEditorDelegates.h"
#include "VoxelOpenAssetsOnStartup.h"
#include "VoxelConvertLandscapeMaterial.h"
#include "VoxelCreateStaticMeshFromProcMesh.h"
#include "VoxelTools/VoxelPaintMaterial.h"
#include "VoxelNodes/VoxelOptimizationNodes.h"
#include "VoxelFoliage.h"
#include "VoxelPlaceableItems/Actors/VoxelAssetActor.h"
#include "VoxelPlaceableItems/Actors/VoxelDisableEditsBox.h"

#include "AssetTools/AssetTypeActions_VoxelDataAsset.h"
#include "AssetTools/AssetTypeActions_VoxelHeightmapAsset.h"
#include "AssetTools/AssetTypeActions_VoxelGraphGenerator.h"
#include "AssetTools/AssetTypeActions_VoxelGraphMacro.h"
#include "AssetTools/AssetTypeActions_VoxelFoliageBiomeType.h"

#include "Thumbnails/VoxelGraphGeneratorThumbnailRenderer.h"
#include "Thumbnails/VoxelDataAssetThumbnailRenderer.h"
#include "Thumbnails/VoxelHeightmapAssetThumbnailRenderer.h"
#include "Thumbnails/VoxelFoliageThumbnailRenderer.h"

#include "DataAssetEditor/VoxelDataAssetEditorToolkit.h"
#include "EdMode/VoxelEdMode.h"
#include "VoxelToolsCommands.h"

#include "ActorFactoryVoxelWorld.h"
#include "ActorFactoryVoxelPlaceableItems.h"
#include "ActorFactoryVoxelMeshImporter.h"

#include "Details/VoxelWorldDetails.h"
#include "Details/VoxelLandscapeImporterDetails.h"
#include "Details/VoxelMeshImporterDetails.h"
#include "Details/VoxelAssetActorDetails.h"
#include "Details/VoxelGeneratorPickerCustomization.h"
#include "Details/RangeAnalysisDebuggerDetails.h"
#include "Details/VoxelPaintMaterialCustomization.h"
#include "Details/VoxelFoliageScaleCustomization.h"
#include "Details/VoxelGeneratorOutputPickerCustomization.h"
#include "Details/VoxelFoliageDensityCustomization.h"
#include "Details/VoxelGraphOutputCustomization.h"
#include "Details/VoxelIntervalCustomization.h"
#include "Details/VoxelBoolVectorCustomization.h"
#include "Details/VoxelLandscapeCollectionDetails.h"
#include "Details/VoxelDistanceCustomization.h"
#include "Details/VoxelFoliageCustomDataCustomization.h"

#include "VoxelImporters/VoxelMeshImporter.h"
#include "VoxelImporters/VoxelLandscapeImporter.h"
#include "VoxelComponents/VoxelInvokerComponent.h"

#include "VoxelEditorDetailsUtilities.h"
#include "VoxelDebugEditor.h"
#include "VoxelScopedTransaction.h"
#include "VoxelWorldEditorControls.h"
#include "Factories/VoxelFactories.h"
#include "VoxelImporters/VoxelMagicaVoxImporter.h"
#include "VoxelUtilities/VoxelSystemUtilities.h"

const FVector2D Icon14x14(14.0f, 14.0f);
const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);
const FVector2D Icon512x512(512.0f, 512.0f);

class FVoxelEditorCommands : public TCommands<FVoxelEditorCommands>
{
public:
	FVoxelEditorCommands()
		: TCommands<FVoxelEditorCommands>
		(
		"VoxelEditor", // Context name for icons
		VOXEL_LOCTEXT("Voxel Editor"), // Localized context name for displaying
		NAME_None, // Parent
		"VoxelStyle" // Icon Style Set
		)
	{
	}
	
	TSharedPtr<FUICommandInfo> RefreshVoxelWorlds;

#define LOCTEXT_NAMESPACE "Voxel"
	virtual void RegisterCommands() override
	{
		UI_COMMAND(
			RefreshVoxelWorlds, 
			"Retoggle", 
			"Retoggle the voxel worlds", 
			EUserInterfaceActionType::Button, 
			FInputChord(EModifierKey::Control, EKeys::F5));
	}
#undef LOCTEXT_NAMESPACE
};

static void RefreshVoxelWorlds_Execute(UObject* MatchingGenerator = nullptr)
{
	FViewport* Viewport = GEditor->GetActiveViewport();
	if (Viewport)
	{
		FViewportClient* Client = Viewport->GetClient();
		if (Client)
		{
			UWorld* World = Client->GetWorld();
			if (World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview))
			{
				for (TActorIterator<AVoxelWorld> It(World); It; ++It)
				{
					if (It->IsCreated() && (!MatchingGenerator || It->Generator.GetObject() == MatchingGenerator))
					{
						It->Toggle();
						It->Toggle();
					}
				}
				for (TActorIterator<AVoxelAssetActor> It(World); It; ++It)
				{
					It->UpdatePreview();
				}
			}
		}
	}
}

static void BindEditorDelegates(IVoxelEditorDelegatesInterface* Interface, UObject* Object)
{
	check(Interface && Object);
	
	if (!FEditorDelegates::PreSaveWorld.IsBoundToObject(Object))
	{
		FEditorDelegates::PreSaveWorld.AddWeakLambda(Object, [=](uint32 SaveFlags, UWorld* World) { Interface->OnPreSaveWorld(SaveFlags, World); });
	}
	if (!FEditorDelegates::PreBeginPIE.IsBoundToObject(Object))
	{
		FEditorDelegates::PreBeginPIE.AddWeakLambda(Object, [=](bool bIsSimulating) { Interface->OnPreBeginPIE(bIsSimulating); });
	}
	if (!FEditorDelegates::EndPIE.IsBoundToObject(Object))
	{
		FEditorDelegates::EndPIE.AddWeakLambda(Object, [=](bool bIsSimulating) { Interface->OnEndPIE(bIsSimulating); });
	}
	if (!FEditorDelegates::OnApplyObjectToActor.IsBoundToObject(Object))
	{
		FEditorDelegates::OnApplyObjectToActor.AddWeakLambda(Object, [=](UObject* InObject, AActor* InActor) { Interface->OnApplyObjectToActor(InObject, InActor); });
	}
	if (!FEditorSupportDelegates::PrepareToCleanseEditorObject.IsBoundToObject(Object))
	{
		FEditorSupportDelegates::PrepareToCleanseEditorObject.AddWeakLambda(Object, [=](UObject* InObject) { Interface->OnPrepareToCleanseEditorObject(InObject); });
	}
	if (!FCoreDelegates::OnPreExit.IsBoundToObject(Object))
	{
		FCoreDelegates::OnPreExit.AddWeakLambda(Object, [=]() { Interface->OnPreExit(); });
	}
}

class FVoxelWorldEditor : public IVoxelWorldEditor
{
public:
	FVoxelWorldEditor() = default;

	virtual UVoxelWorldSaveObject* CreateSaveObject() override
	{
		return Cast<UVoxelWorldSaveObject>(FVoxelEditorUtilities::CreateAssetWithDialog(UVoxelWorldSaveObject::StaticClass(), NewObject<UVoxelWorldSaveObjectFactory>()));
	}

	virtual UClass* GetVoxelWorldEditorClass() override
	{
		return AVoxelWorldEditorControls::StaticClass();
	}

	virtual void RegisterTransaction(AVoxelWorld* VoxelWorld, FName Name) override
	{
		FVoxelScopedTransaction Transaction(VoxelWorld, Name, EVoxelChangeType::Edit);
	}
};

/**
 * Implements the VoxelEditor module.
 */
class FVoxelEditorModule : public IVoxelEditorModule
{
public:
	virtual void StartupModule() override
	{
#if 0
		FVoxelSystemUtilities::DelayedCall([]()
		{
			FString String;
			for (TObjectIterator<UField> ClassIt; ClassIt; ++ClassIt)
			{
				UField* Field = *ClassIt;
				if (Field->GetOuter()->GetName() == "/Script/VoxelLegacySpawners")
				{
					const FString Prefix =
						Cast<UScriptStruct>(Field)
						? "StructRedirects"
						: Cast<UClass>(Field)
						? "ClassRedirects"
						: Cast<UEnum>(Field)
						? "EnumRedirects"
						: "";
					if (!Prefix.IsEmpty())
					{
						String += FString::Printf(TEXT("+%s=(OldName=\"/Script/Voxel.%s\", NewName=\"/Script/VoxelLegacySpawners.%s\")\n"), *Prefix, *Field->GetName(), *Field->GetName());
					}
				}
			}
			UE_DEBUG_BREAK();
		});
#endif
		
		UVoxelOpenAssetsOnStartup::Init();
		FVoxelConvertLandscapeMaterial::Init();
		FVoxelCreateStaticMeshFromProcMesh::Init();
		
		// Voxel World Editor
		if (!IVoxelWorldEditor::GetVoxelWorldEditor())
		{
			IVoxelWorldEditor::SetVoxelWorldEditor(MakeShared<FVoxelWorldEditor>());
		}

		FVoxelEditorDelegates::FixVoxelLandscapeMaterial.AddStatic(&FVoxelConvertLandscapeMaterial::ConvertMaterial);
		FVoxelEditorDelegates::CreateStaticMeshFromProcMesh.BindStatic(&FVoxelCreateStaticMeshFromProcMesh::Create);

		// Destroy global pool on end PIE
		FEditorDelegates::EndPIE.AddLambda([](bool bIsSimulating)
		{
			// Don't use huge amount of memory
			FVoxelTextureHelpers::ClearIdCache();
		});
		
		// Clear texture cache on reimport
		FReimportManager::Instance()->OnPostReimport().AddLambda([](UObject*, bool) { FVoxelTextureHelpers::ClearCache(); });
		
		// Global commands
		FVoxelEditorCommands::Register();
		FVoxelToolsCommands::Register();

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetGlobalLevelEditorActions()->MapAction(
			FVoxelEditorCommands::Get().RefreshVoxelWorlds,
			FExecuteAction::CreateStatic(&RefreshVoxelWorlds_Execute, (UObject*)nullptr),
			FCanExecuteAction());

		IVoxelEditorDelegatesInterface::BindEditorDelegatesDelegate.AddStatic(&BindEditorDelegates);

		// Blueprint errors
		// Delay them by one frame to work on startup
		FVoxelMessages::LogMessageDelegate.AddLambda([](const TSharedRef<FTokenizedMessage>& Message, EVoxelShowNotification ShowNotification)
		{
			FVoxelSystemUtilities::DelayedCall([=]()
			{
				FVoxelMessagesEditor::LogMessage(Message, ShowNotification);
			});
		});
		FVoxelMessages::ShowNotificationDelegate.AddStatic([](const FVoxelMessages::FNotification& Notification)
		{
			FVoxelSystemUtilities::DelayedCall([=]()
			{
				FVoxelMessagesEditor::ShowNotification(Notification);
			});
		});

		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		FMessageLogInitializationOptions InitOptions;
		InitOptions.bShowFilters = true;
		InitOptions.bShowPages = false;
		InitOptions.bAllowClear = true;
		MessageLogModule.RegisterLogListing("Voxel", VOXEL_LOCTEXT("Voxel"), InitOptions);

		// Voxel asset category
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		VoxelAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory("Voxel", VOXEL_LOCTEXT("Voxel"));

		RegisterPlacementModeExtensions();
		RegisterCustomClassLayouts();
		RegisterAssetTools();
		
		// Thumbnails
		auto& ThumbnailManager = UThumbnailManager::Get();
		ThumbnailManager.RegisterCustomRenderer(UVoxelGraphGenerator::StaticClass(), UVoxelGraphGeneratorThumbnailRenderer::StaticClass());
		ThumbnailManager.RegisterCustomRenderer(UVoxelDataAsset     ::StaticClass(), UVoxelDataAssetThumbnailRenderer     ::StaticClass());
		ThumbnailManager.RegisterCustomRenderer(UVoxelHeightmapAsset::StaticClass(), UVoxelHeightmapAssetThumbnailRenderer::StaticClass());
		ThumbnailManager.RegisterCustomRenderer(UVoxelFoliage       ::StaticClass(), UVoxelFoliageThumbnailRenderer       ::StaticClass());

		// Icons
		{
			IPlugin& Plugin = FVoxelSystemUtilities::GetPlugin();
			FString ContentDir = Plugin.GetContentDir() + "/";

			StyleSet = MakeShareable(new FSlateStyleSet("VoxelStyle"));
			StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
			StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

			// For menus
			StyleSet->Set("VoxelIcon"                                        , new FSlateImageBrush(ContentDir + TEXT("Editor/UIIcons/mode_40.png"), Icon16x16));

			// VoxelWorld
			StyleSet->Set("ClassThumbnail.VoxelWorld"                        , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/World_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelWorld"                             , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/World_16x.png"), Icon16x16));
																		     
			// Voxel Material Collection								     
			StyleSet->Set("ClassThumbnail.VoxelMaterialCollectionBase"       , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/MaterialCollection_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelMaterialCollectionBase"            , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/MaterialCollection_16x.png"), Icon16x16));

			// Importers
			StyleSet->Set("ClassThumbnail.VoxelLandscapeImporter"            , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Import_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelLandscapeImporter"                 , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Import_16x.png"), Icon16x16));
			StyleSet->Set("ClassThumbnail.VoxelMeshImporter"                 , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Import_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelMeshImporter"                      , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Import_16x.png"), Icon16x16));
					     
			// Voxel Graph												     
			StyleSet->Set("ClassThumbnail.VoxelGraphGenerator"               , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/VoxelGraph_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelGraphGenerator"                    , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/VoxelGraph_16x.png"), Icon16x16));
																		     
			// Data Asset												     
			StyleSet->Set("ClassThumbnail.VoxelDataAsset"                    , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/DataAsset_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelDataAsset"                         , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/DataAsset_16x.png"), Icon16x16));
																		     
			// Landscape asset											     
			StyleSet->Set("ClassThumbnail.VoxelLandscapeAsset"               , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Landscape_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelLandscapeAsset"                    , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Landscape_16x.png"), Icon16x16));	
																		     
			// Data Asset Editor										     
			StyleSet->Set("VoxelDataAssetEditor.InvertDataAsset"             , new FSlateImageBrush(ContentDir + TEXT("Editor/UIIcons/InvertDataAsset_40x.png"), Icon40x40));
			StyleSet->Set("VoxelDataAssetEditor.InvertDataAsset.Small"       , new FSlateImageBrush(ContentDir + TEXT("Editor/UIIcons/InvertDataAsset_16x.png"), Icon16x16));
																		     
			// Voxel Editor Tools										     
			StyleSet->Set("VoxelTools.Tab"                                   , new FSlateImageBrush(ContentDir + TEXT("Editor/UIIcons/mode_40.png"), Icon40x40));
			StyleSet->Set("VoxelTools.Tab.Small"                             , new FSlateImageBrush(ContentDir + TEXT("Editor/UIIcons/mode_40.png"), Icon16x16));
																		     
			// Generator											     
			StyleSet->Set("ClassThumbnail.VoxelGenerator"                    , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Generator_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelGenerator"                         , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/Generator_16x.png"), Icon16x16));
																		     
			// Voxel World Object Save									     
			StyleSet->Set("ClassThumbnail.VoxelWorldSaveObject"              , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/VoxelWorldSaveObject_64x.png"), Icon64x64));
			StyleSet->Set("ClassIcon.VoxelWorldSaveObject"                   , new FSlateImageBrush(ContentDir + TEXT("Editor/AssetIcons/VoxelWorldSaveObject_16x.png"), Icon16x16));

			// Tools
			const auto AddTool = [&](const FString& Name)
			{
				StyleSet->Set(*("VoxelTools." + Name), new FSlateImageBrush(ContentDir + "Editor/UIIcons/Tools/" + Name + "_40.png", Icon40x40));
				StyleSet->Set(*("VoxelTools." + Name + ".Small"), new FSlateImageBrush(ContentDir + "Editor/UIIcons/Tools/" + Name + "_40.png", Icon16x16));
			};
			AddTool("FlattenTool");
			AddTool("LevelTool");
			AddTool("MeshTool");
			AddTool("RevertTool");
			AddTool("SmoothTool");
			AddTool("SphereTool");
			AddTool("SurfaceTool");
			AddTool("TrimTool");
			
			FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
		}

		{
			FTabSpawnerEntry& SpawnerEntry = FGlobalTabmanager::Get()->RegisterNomadTabSpawner("VoxelDebug", FOnSpawnTab::CreateStatic(&FVoxelDebugEditor::CreateTab))
				.SetDisplayName(VOXEL_LOCTEXT("Voxel Debug"))
				.SetIcon(FSlateIcon(StyleSet->GetStyleSetName(), "VoxelIcon"));
			SpawnerEntry.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
		}

		// Voxel Editor Tools
		FEditorModeRegistry::Get().RegisterMode<FEdModeVoxel>(FEdModeVoxel::EM_Voxel, VOXEL_LOCTEXT("Voxels"), FSlateIcon("VoxelStyle", "VoxelTools.Tab", "VoxelTools.Tab.Small"), true);
	}

	virtual void ShutdownModule() override
	{
		FEditorModeRegistry::Get().UnregisterMode(FEdModeVoxel::EM_Voxel);

		if (UObjectInitialized())
		{
			auto& ThumbnailManager = UThumbnailManager::Get();
			ThumbnailManager.UnregisterCustomRenderer(UVoxelGraphGenerator::StaticClass());
			ThumbnailManager.UnregisterCustomRenderer(UVoxelDataAsset::StaticClass());
			ThumbnailManager.UnregisterCustomRenderer(UVoxelHeightmapAsset::StaticClass());
		}

		UnregisterPlacementModeExtensions();
		UnregisterClassLayout();
		UnregisterAssetTools();

		if (StyleSet.IsValid())
		{
			FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
			StyleSet.Reset();
		}
	}

	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}

private:
	template<typename T>
	void RegisterPlacementModeExtension(IPlacementModeModule& PlacementModeModule, UActorFactory* Factory = nullptr)
	{
		PlacementModeModule.RegisterPlaceableItem(PlacementCategoryInfo.UniqueHandle, MakeShared<FPlaceableItem>(Factory, FAssetData(T::StaticClass())));
	}
	void RegisterPlacementModeExtensions()
	{
		IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();
		PlacementModeModule.RegisterPlacementCategory(PlacementCategoryInfo);

		RegisterPlacementModeExtension<AVoxelWorld          >(PlacementModeModule, GetMutableDefault<UActorFactoryVoxelWorld          >());
		RegisterPlacementModeExtension<AVoxelDisableEditsBox>(PlacementModeModule, GetMutableDefault<UActorFactoryVoxelDisableEditsBox>());
		RegisterPlacementModeExtension<AVoxelAssetActor     >(PlacementModeModule, GetMutableDefault<UActorFactoryVoxelAssetActor     >());
		RegisterPlacementModeExtension<AVoxelMeshImporter   >(PlacementModeModule, GetMutableDefault<UActorFactoryVoxelMeshImporter   >());

		RegisterPlacementModeExtension<AVoxelLODVolume>(PlacementModeModule);
		RegisterPlacementModeExtension<AVoxelLandscapeImporter>(PlacementModeModule);

		PlacementModeModule.RegenerateItemsForCategory(FBuiltInPlacementCategories::AllClasses());
	}
	void UnregisterPlacementModeExtensions()
	{
		if (IPlacementModeModule::IsAvailable())
		{
			IPlacementModeModule::Get().UnregisterPlacementCategory(PlacementCategoryInfo.UniqueHandle);
		}
	}

private:
	template<typename TDetails, typename TClass>
	void RegisterCustomClassLayout()
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		const FName Name = TClass::StaticClass()->GetFName();
		PropertyModule.RegisterCustomClassLayout(Name, FOnGetDetailCustomizationInstance::CreateLambda([]() { return MakeShared<TDetails>(); }));
		RegisteredCustomClassLayouts.Add(Name);
	}
	template<typename TDetails, typename TStruct>
	void RegisterCustomPropertyLayout()
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		const FName Name = TStruct::StaticStruct()->GetFName();
		PropertyModule.RegisterCustomPropertyTypeLayout(Name, FOnGetPropertyTypeCustomizationInstance::CreateLambda([]() { return MakeShared<TDetails>(); }));
		RegisteredCustomPropertyLayouts.Add(Name);
	}

	void RegisterCustomClassLayouts()
	{
		RegisterCustomClassLayout<FVoxelWorldDetails              , AVoxelWorld                          >();
		RegisterCustomClassLayout<FVoxelLandscapeImporterDetails  , AVoxelLandscapeImporter              >();
		RegisterCustomClassLayout<FVoxelMeshImporterDetails       , AVoxelMeshImporter                   >();
		RegisterCustomClassLayout<FVoxelAssetActorDetails         , AVoxelAssetActor                     >();
		RegisterCustomClassLayout<FRangeAnalysisDebuggerDetails   , UVoxelNode_RangeAnalysisDebuggerFloat>();
		RegisterCustomClassLayout<FVoxelLandscapeCollectionDetails, UVoxelLandscapeMaterialCollection    >();

		RegisterCustomPropertyLayout<FVoxelGeneratorPickerCustomization					       , FVoxelGeneratorPicker                        >();
		RegisterCustomPropertyLayout<FVoxelGeneratorPickerCustomization					       , FVoxelTransformableGeneratorPicker           >();
		RegisterCustomPropertyLayout<FVoxelPaintMaterialCustomization                          , FVoxelPaintMaterial                          >();
		RegisterCustomPropertyLayout<FVoxelPaintMaterial_MaterialCollectionChannelCustomization, FVoxelPaintMaterial_MaterialCollectionChannel>();
		RegisterCustomPropertyLayout<FVoxelBoolVectorCustomization                             , FVoxelBoolVector                             >();
		RegisterCustomPropertyLayout<FVoxelFoliageScaleCustomization                           , FVoxelFoliageScale                           >();
		RegisterCustomPropertyLayout<FVoxelDistanceCustomization                               , FVoxelDistance                               >();
		RegisterCustomPropertyLayout<FVoxelGeneratorOutputPickerCustomization                  , FVoxelGeneratorOutputPicker                  >();
		RegisterCustomPropertyLayout<FVoxelFoliageDensityCustomization                         , FVoxelFoliageDensity                         >();
		RegisterCustomPropertyLayout<FVoxelFoliageCustomDataCustomization                      , FVoxelFoliageCustomData                      >();
		RegisterCustomPropertyLayout<FVoxelGraphOutputCustomization                            , FVoxelGraphOutput                            >();
		RegisterCustomPropertyLayout<FVoxelIntervalCustomization                               , FVoxelInt32Interval                          >();
		RegisterCustomPropertyLayout<FVoxelIntervalCustomization                               , FVoxelFloatInterval                          >();
		
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	void UnregisterClassLayout()
	{
		if (auto* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			for (auto& Name : RegisteredCustomClassLayouts)
			{
				PropertyModule->UnregisterCustomClassLayout(Name);
			}
			for (auto& Name : RegisteredCustomPropertyLayouts)
			{
				PropertyModule->UnregisterCustomPropertyTypeLayout(Name);
			}
			PropertyModule->NotifyCustomizationModuleChanged();
		}
	}
	
private:
	template<typename T>
	void RegisterAssetTypeAction()
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		const auto Action = MakeShared<T>(VoxelAssetCategoryBit);
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	template<typename T>
	void RegisterAssetTypeAction(const FText& Name, FColor Color)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		const auto Action = MakeShared<FAssetTypeActions_Voxel>(VoxelAssetCategoryBit, Name, Color, T::StaticClass());
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

	void RegisterAssetTools()
	{
		RegisterAssetTypeAction<FAssetTypeActions_VoxelDataAsset>();
		RegisterAssetTypeAction<FAssetTypeActions_VoxelHeightmapAsset>();
		RegisterAssetTypeAction<FAssetTypeActions_VoxelGraphGenerator>();
		RegisterAssetTypeAction<FAssetTypeActions_VoxelGraphMacro>();
		RegisterAssetTypeAction<FAssetTypeActions_VoxelFoliageBiomeType>();

		const FColor Orange = FColor(255, 140, 0);
		const FColor DarkGreen = FColor(0, 192, 0);
		const FColor LightGreen = FColor(128, 255, 128);
		const FColor Blue = FColor(0, 175, 255);

		RegisterAssetTypeAction<UVoxelWorldSaveObject>(VOXEL_LOCTEXT("Voxel World Save Object"), Orange);
		RegisterAssetTypeAction<UVoxelMagicaVoxScene >(VOXEL_LOCTEXT("Voxel Magica Vox Scene"), Orange);

		RegisterAssetTypeAction<UVoxelBasicMaterialCollection             >(VOXEL_LOCTEXT("Voxel Basic Material Collection"              ), DarkGreen);
		RegisterAssetTypeAction<UVoxelBlueprintMaterialCollection         >(VOXEL_LOCTEXT("Voxel Blueprint Material Collection"          ), DarkGreen);
		RegisterAssetTypeAction<UVoxelInstancedMaterialCollectionTemplates>(VOXEL_LOCTEXT("Voxel Instanced Material Collection Templates"), DarkGreen);
		RegisterAssetTypeAction<UVoxelInstancedMaterialCollection         >(VOXEL_LOCTEXT("Voxel Instanced Material Collection"          ), DarkGreen);
		RegisterAssetTypeAction<UVoxelInstancedMaterialCollectionInstance >(VOXEL_LOCTEXT("Voxel Instanced Material Collection Instance" ), DarkGreen);
		RegisterAssetTypeAction<UVoxelLandscapeMaterialCollection         >(VOXEL_LOCTEXT("Voxel Landscape Material Collection"          ), DarkGreen);

		RegisterAssetTypeAction<UVoxelFoliageCollection>(VOXEL_LOCTEXT("Voxel Foliage Collection"), LightGreen);
		RegisterAssetTypeAction<UVoxelFoliage          >(VOXEL_LOCTEXT("Voxel Foliage"           ), LightGreen);
		RegisterAssetTypeAction<AVoxelFoliageActor     >(VOXEL_LOCTEXT("Voxel Foliage Actor"     ), LightGreen);
		RegisterAssetTypeAction<UVoxelFoliageBiome     >(VOXEL_LOCTEXT("Voxel Foliage Biome"     ), LightGreen);

		RegisterAssetTypeAction<UVoxelGraphDataItemConfig>(VOXEL_LOCTEXT("Voxel Graph Data Item Config"), Blue);
	}

	void UnregisterAssetTools()
	{
		if (auto* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools"))
		{
			IAssetTools& AssetTools = AssetToolsModule->Get();
			for (auto& Action : RegisteredAssetTypeActions)
			{
				AssetTools.UnregisterAssetTypeActions(Action);
			}
		}
	}
	
public:
	virtual void CreateVoxelDataAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UVoxelDataAsset* DataAsset) override
	{
		TSharedRef<FVoxelDataAssetEditorToolkit> NewVoxelEditor(new FVoxelDataAssetEditorToolkit());
		NewVoxelEditor->InitVoxelEditor(Mode, InitToolkitHost, DataAsset);
	}

	virtual void RefreshVoxelWorlds(UObject* MatchingGenerator) override
	{
		RefreshVoxelWorlds_Execute(MatchingGenerator);
	}

	virtual EAssetTypeCategories::Type GetVoxelAssetTypeCategory() const override
	{
		return VoxelAssetCategoryBit;
	}

private:
	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
	TArray<FName> RegisteredCustomClassLayouts;
	TArray<FName> RegisteredCustomPropertyLayouts;

	EAssetTypeCategories::Type VoxelAssetCategoryBit = EAssetTypeCategories::None;
	FPlacementCategoryInfo PlacementCategoryInfo = FPlacementCategoryInfo(VOXEL_LOCTEXT("Voxel"), "Voxel", TEXT("PMVoxel"), 25);
	TSharedPtr<FSlateStyleSet> StyleSet;
};

IMPLEMENT_MODULE(FVoxelEditorModule, VoxelEditor);