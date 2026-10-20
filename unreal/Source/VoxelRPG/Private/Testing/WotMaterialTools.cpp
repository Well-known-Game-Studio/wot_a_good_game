// Editor-only material tooling for the weather pass:
//   wot.Mat.Dump <MaterialPath>        the expressions of a material and what feeds each material property
//   wot.Mat.AddWeather <MaterialPath>  routes Base Color / Roughness / Normal through Ultra Dynamic Weather's
//                                      "Snow And Dust Weather Effects" and "Wet Weather Effects" functions (idempotent),
//                                      so every instance of the material gathers snow / dust and gets wet with the weather
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "StaticParameterSet.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotMat, Log, All);
#define MT_LOG(Fmt, ...) UE_LOG(LogWotMat, Display, TEXT("[MAT] ") Fmt, ##__VA_ARGS__)

namespace WotMat
{
	UMaterial* Load(const FString& Arg)
	{
		const FString Path = Arg.Contains(TEXT(".")) ? Arg : Arg + TEXT(".") + FPackageName::GetShortName(Arg);
		return Cast<UMaterial>(FSoftObjectPath(Path).TryLoad());
	}

	bool Save(UObject* Obj)
	{
		UPackage* Pkg = Obj->GetOutermost(); Pkg->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Pkg, Obj, *File, Args);
	}

	const TCHAR* PropName(EMaterialProperty P)
	{
		switch (P) {
		case MP_BaseColor: return TEXT("BaseColor"); case MP_Roughness: return TEXT("Roughness"); case MP_Normal: return TEXT("Normal");
		case MP_Metallic: return TEXT("Metallic"); case MP_Specular: return TEXT("Specular"); case MP_EmissiveColor: return TEXT("Emissive");
		case MP_Opacity: return TEXT("Opacity"); case MP_OpacityMask: return TEXT("OpacityMask"); case MP_SubsurfaceColor: return TEXT("SubsurfaceColor");
		case MP_WorldPositionOffset: return TEXT("WPO"); case MP_AmbientOcclusion: return TEXT("AO"); default: return TEXT("?");
		}
	}

	/** A function call expression wired as: Material property P <- Fn.<OutName>  and  Fn.<InName> <- whatever fed P before. */
	UMaterialExpressionMaterialFunctionCall* InsertFunction(UMaterial* M, UMaterialFunction* Fn, int32 X, int32 Y)
	{
		UMaterialExpressionMaterialFunctionCall* Call = Cast<UMaterialExpressionMaterialFunctionCall>(UMaterialEditingLibrary::CreateMaterialExpression(M, UMaterialExpressionMaterialFunctionCall::StaticClass(), X, Y));
		if (!Call) { return nullptr; }
		Call->SetMaterialFunction(Fn);
		Call->UpdateFromFunctionResource();
		return Call;
	}

	bool AlreadyUses(UMaterial* M, const UMaterialFunction* Fn)
	{
		for (UMaterialExpression* E : M->GetExpressions()) {
			if (const UMaterialExpressionMaterialFunctionCall* C = Cast<UMaterialExpressionMaterialFunctionCall>(E)) {
				if (C->MaterialFunction == Fn) { return true; }
			}
		}
		return false;
	}

	/** Re-routes property P through Call: Call.InName <- old source of P (if any), P <- Call.OutName. */
	void Route(UMaterial* M, UMaterialExpressionMaterialFunctionCall* Call, EMaterialProperty P, const FString& InName, const FString& OutName)
	{
		UMaterialExpression* Src = UMaterialEditingLibrary::GetMaterialPropertyInputNode(M, P);
		const FString SrcOut = UMaterialEditingLibrary::GetMaterialPropertyInputNodeOutputName(M, P);
		if (Src) {
			const bool bIn = UMaterialEditingLibrary::ConnectMaterialExpressions(Src, SrcOut, Call, InName);
			MT_LOG("  %s: %s.%s -> fn.%s (%d)", PropName(P), *Src->GetName(), *SrcOut, *InName, bIn ? 1 : 0);
		} else {
			MT_LOG("  %s: nothing fed it; fn.%s left at its default", PropName(P), *InName);
		}
		const bool bOut = UMaterialEditingLibrary::ConnectMaterialProperty(Call, OutName, P);
		MT_LOG("  %s <- fn.%s (%d)", PropName(P), *OutName, bOut ? 1 : 0);
	}
}

static FAutoConsoleCommand GWotMatDump(
	TEXT("wot.Mat.Dump"), TEXT("Log a material's expressions and property inputs: wot.Mat.Dump /Game/Path/Material"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		UMaterial* M = Args.Num() > 0 ? Load(Args[0]) : nullptr;
		if (!M && Args.Num() > 0) {   // a material function: its inputs (with defaults), outputs and expressions
			const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
			if (UMaterialFunction* Fn = Cast<UMaterialFunction>(FSoftObjectPath(Path).TryLoad())) {
				MT_LOG("function %s expressions=%d", *Fn->GetName(), Fn->GetExpressions().Num());
				for (UMaterialExpression* E : Fn->GetExpressions()) {
					if (const UMaterialExpressionFunctionInput* In = Cast<UMaterialExpressionFunctionInput>(E)) {
						MT_LOG("  input '%s' type=%d preview=(%.2f %.2f %.2f %.2f) usePreviewAsDefault=%d desc='%s'", *In->InputName.ToString(), (int32)In->InputType, In->PreviewValue.X, In->PreviewValue.Y, In->PreviewValue.Z, In->PreviewValue.W, In->bUsePreviewValueAsDefault ? 1 : 0, *In->Description.Left(120));
					}
					else if (const UMaterialExpressionFunctionOutput* Out = Cast<UMaterialExpressionFunctionOutput>(E)) { MT_LOG("  output '%s'", *Out->OutputName.ToString()); }
					else if (const UMaterialExpressionStaticSwitchParameter* SW = Cast<UMaterialExpressionStaticSwitchParameter>(E)) { MT_LOG("  static switch param '%s' default=%d", *SW->ParameterName.ToString(), SW->DefaultValue ? 1 : 0); }
					else if (const UMaterialExpressionScalarParameter* SP = Cast<UMaterialExpressionScalarParameter>(E)) { MT_LOG("  scalar param '%s' default=%.2f", *SP->ParameterName.ToString(), SP->DefaultValue); }
					else if (const UMaterialExpressionCollectionParameter* CP = Cast<UMaterialExpressionCollectionParameter>(E)) { MT_LOG("  collection param '%s' of %s", *CP->ParameterName.ToString(), *GetNameSafe(CP->Collection)); }
					else if (const UMaterialExpressionMaterialFunctionCall* C = Cast<UMaterialExpressionMaterialFunctionCall>(E)) { MT_LOG("  call %s", *GetNameSafe(C->MaterialFunction)); }
				}
				return;
			}
		}
		if (!M) { MT_LOG("usage: wot.Mat.Dump /Game/Path/Material|Function (not found)"); return; }
		MT_LOG("%s domain=%d blend=%d shading=%d expressions=%d", *M->GetName(), (int32)M->MaterialDomain, (int32)M->BlendMode, (int32)M->GetShadingModels().GetFirstShadingModel(), M->GetExpressions().Num());
		for (UMaterialExpression* E : M->GetExpressions()) {
			FString Extra;
			if (const UMaterialExpressionMaterialFunctionCall* C = Cast<UMaterialExpressionMaterialFunctionCall>(E)) { Extra = FString::Printf(TEXT(" fn=%s"), *GetNameSafe(C->MaterialFunction)); }
			TArray<UMaterialExpression*> Ins = UMaterialEditingLibrary::GetInputsForMaterialExpression(M, E);
			TArray<FString> InNames; for (UMaterialExpression* I : Ins) { InNames.Add(GetNameSafe(I)); }
			MT_LOG("  %s (%s)%s desc='%s' inputs=[%s]", *E->GetName(), *E->GetClass()->GetName(), *Extra, *E->Desc, *FString::Join(InNames, TEXT(",")));
		}
		const EMaterialProperty Props[] = { MP_BaseColor, MP_Metallic, MP_Specular, MP_Roughness, MP_Normal, MP_EmissiveColor, MP_Opacity, MP_OpacityMask, MP_SubsurfaceColor, MP_WorldPositionOffset, MP_AmbientOcclusion };
		for (EMaterialProperty P : Props) {
			UMaterialExpression* Src = UMaterialEditingLibrary::GetMaterialPropertyInputNode(M, P);
			if (Src) { MT_LOG("  property %s <- %s.%s", PropName(P), *Src->GetName(), *UMaterialEditingLibrary::GetMaterialPropertyInputNodeOutputName(M, P)); }
		}
	}));

static FAutoConsoleCommand GWotMatAddWeather(
	TEXT("wot.Mat.AddWeather"), TEXT("Route a material's Base Color / Roughness / Normal through Ultra Dynamic Weather's snow+dust and wet effects: wot.Mat.AddWeather /Game/Path/Material [nowet]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		UMaterial* M = Args.Num() > 0 ? Load(Args[0]) : nullptr;
		if (!M) { MT_LOG("usage: wot.Mat.AddWeather /Game/Path/Material (not found)"); return; }
		UMaterialFunction* SnowFn = Cast<UMaterialFunction>(FSoftObjectPath(TEXT("/Game/UltraDynamicSky/Materials/Weather/Snow_And_Dust_Weather_Effects.Snow_And_Dust_Weather_Effects")).TryLoad());
		UMaterialFunction* WetFn = Cast<UMaterialFunction>(FSoftObjectPath(TEXT("/Game/UltraDynamicSky/Materials/Weather/Wet_Weather_Effects.Wet_Weather_Effects")).TryLoad());
		if (!SnowFn || !WetFn) { MT_LOG("UDW weather material functions not found"); return; }
		const bool bWet = !(Args.Num() > 1 && Args[1] == TEXT("nowet"));
		M->Modify();
		// a constant Height of 0.5: coverage builds evenly over the whole surface (the voxel materials have no height map)
		int32 Added = 0;
		if (!AlreadyUses(M, SnowFn)) {
			UMaterialExpressionMaterialFunctionCall* Snow = InsertFunction(M, SnowFn, 200, -200);
			if (Snow) {
				MT_LOG("snow+dust: inputs=[%s] outputs=[%s]", *FString::JoinBy(Snow->FunctionInputs, TEXT(","), [](const FFunctionExpressionInput& I) { return I.Input.InputName.ToString(); }),
					*FString::JoinBy(Snow->FunctionOutputs, TEXT(","), [](const FFunctionExpressionOutput& O) { return O.Output.OutputName.ToString(); }));
				Route(M, Snow, MP_BaseColor, TEXT("Base Color"), TEXT("Base Color"));
				Route(M, Snow, MP_Roughness, TEXT("Roughness"), TEXT("Roughness"));
				Route(M, Snow, MP_Normal, TEXT("Normal"), TEXT("Normal"));
				UMaterialExpressionConstant* H = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(M, UMaterialExpressionConstant::StaticClass(), -100, 0));
				if (H) { H->R = 0.5f; UMaterialEditingLibrary::ConnectMaterialExpressions(H, TEXT(""), Snow, TEXT("Height")); }
				++Added;
			}
		} else { MT_LOG("snow+dust already present"); }
		if (bWet && !AlreadyUses(M, WetFn)) {
			UMaterialExpressionMaterialFunctionCall* Wet = InsertFunction(M, WetFn, 500, -200);
			if (Wet) {
				Route(M, Wet, MP_BaseColor, TEXT("Base Color"), TEXT("Base Color"));
				Route(M, Wet, MP_Roughness, TEXT("Roughness"), TEXT("Roughness"));
				Route(M, Wet, MP_Normal, TEXT("Normal"), TEXT("Normal"));
				UMaterialExpressionConstant* H = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(M, UMaterialExpressionConstant::StaticClass(), 300, 0));
				if (H) { H->R = 0.5f; UMaterialEditingLibrary::ConnectMaterialExpressions(H, TEXT(""), Wet, TEXT("Height")); }
				++Added;
			}
		} else if (bWet) { MT_LOG("wet already present"); }
		if (Added > 0) {
			UMaterialEditingLibrary::LayoutMaterialExpressions(M);
			TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(M);
			for (const FString& E : Errors) { MT_LOG("  compile: %s", *E); }
			MT_LOG("%s: %d weather function(s) added, saved=%d, errors=%d", *M->GetName(), Added, Save(M) ? 1 : 0, Errors.Num());
		}
	}));

// wot.Mat.SetColor <MaterialInstancePath> r g b [ParamName=Color] : set a palette instance's colour through the material
// editing library (the reflection path writes the array but the instance keeps rendering its parent's colour) and save
static FAutoConsoleCommand GWotMatSetColor(
	TEXT("wot.Mat.SetColor"), TEXT("Set a material instance's vector parameter and save it: wot.Mat.SetColor /Game/Path/MI r g b [ParamName]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 4) { MT_LOG("usage: wot.Mat.SetColor /Game/Path/MI r g b [ParamName]"); return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(FSoftObjectPath(Path).TryLoad());
		if (!MI) { MT_LOG("material instance not found: %s", *Path); return; }
		const FName Param = Args.Num() > 4 ? FName(*Args[4]) : FName(TEXT("Color"));
		const FLinearColor C(FCString::Atof(*Args[1]), FCString::Atof(*Args[2]), FCString::Atof(*Args[3]), 1.0f);
		MI->Modify();
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MI, Param, C);
		UMaterialEditingLibrary::UpdateMaterialInstance(MI);
		MT_LOG("%s: %s = (%.2f %.2f %.2f) saved=%d", *MI->GetName(), *Param.ToString(), C.R, C.G, C.B, Save(MI) ? 1 : 0);
	}));

// wot.Mat.DumpMI <MaterialInstancePath> : parent chain, each level's vector overrides, and how the instance resolves Color
static FAutoConsoleCommand GWotMatDumpMI(
	TEXT("wot.Mat.DumpMI"), TEXT("Log a material instance's parent chain and parameter resolution: wot.Mat.DumpMI /Game/Path/MI [ParamName]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 1) { return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UMaterialInstance* MI = Cast<UMaterialInstance>(FSoftObjectPath(Path).TryLoad());
		if (!MI) { MT_LOG("material instance not found: %s", *Path); return; }
		const FName Param = Args.Num() > 1 ? FName(*Args[1]) : FName(TEXT("Color"));
		UMaterialInterface* Cur = MI; int32 Depth = 0;
		while (Cur && Depth < 8) {
			if (UMaterialInstance* I = Cast<UMaterialInstance>(Cur)) {
				TArray<FString> Vals;
				for (const FVectorParameterValue& V : I->VectorParameterValues) { Vals.Add(FString::Printf(TEXT("%s=(%.2f %.2f %.2f) assoc=%d guid=%s"), *V.ParameterInfo.Name.ToString(), V.ParameterValue.R, V.ParameterValue.G, V.ParameterValue.B, (int32)V.ParameterInfo.Association, *V.ExpressionGUID.ToString().Left(8))); }
				MT_LOG("%*s%s (%s) parent=%s vectors=[%s] hasStaticPermutation=%d", Depth * 2, TEXT(""), *I->GetName(), *I->GetClass()->GetName(), *GetNameSafe(I->Parent), *FString::Join(Vals, TEXT("; ")), I->HasStaticParameters() ? 1 : 0);
				Cur = I->Parent;
			} else {
				UMaterial* M = Cast<UMaterial>(Cur);
				TArray<FMaterialParameterInfo> Infos; TArray<FGuid> Guids; M->GetAllVectorParameterInfo(Infos, Guids);
				TArray<FString> Names; for (int32 i = 0; i < Infos.Num(); ++i) { Names.Add(FString::Printf(TEXT("%s guid=%s"), *Infos[i].Name.ToString(), *Guids[i].ToString().Left(8))); }
				MT_LOG("%*s%s (Material) vector params=[%s]", Depth * 2, TEXT(""), *M->GetName(), *FString::Join(Names, TEXT("; ")));
				break;
			}
			++Depth;
		}
		FLinearColor Resolved; const bool bOk = MI->GetVectorParameterValue(FMaterialParameterInfo(Param), Resolved);
		MT_LOG("%s resolves %s -> ok=%d (%.2f %.2f %.2f)", *MI->GetName(), *Param.ToString(), bOk ? 1 : 0, Resolved.R, Resolved.G, Resolved.B);
	}));

// wot.Mat.MakeFoliage <BaseMaterialPath> <NewMaterialPath> : a copy of the base voxel material whose World Position
// Offset runs UDW's "Foliage Wind Movement", so leaves, grass and reeds sway with the weather's wind while houses stay put
static FAutoConsoleCommand GWotMatMakeFoliage(
	TEXT("wot.Mat.MakeFoliage"), TEXT("Duplicate a material and add UDW foliage wind to its World Position Offset: wot.Mat.MakeFoliage /Game/Path/Base /Game/Path/NewFoliageMaterial"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 2) { MT_LOG("usage: wot.Mat.MakeFoliage /Game/Path/Base /Game/Path/New"); return; }
		UMaterial* M = Load(Args[1]);
		if (!M) {
			if (!UEditorAssetLibrary::DuplicateAsset(Args[0], Args[1])) { MT_LOG("could not duplicate %s -> %s", *Args[0], *Args[1]); return; }
			M = Load(Args[1]);
		}
		if (!M) { MT_LOG("new material not found: %s", *Args[1]); return; }
		UMaterialFunction* WindFn = Cast<UMaterialFunction>(FSoftObjectPath(TEXT("/Game/UltraDynamicSky/Materials/Weather/Foliage_Wind_Movement.Foliage_Wind_Movement")).TryLoad());
		if (!WindFn) { MT_LOG("Foliage_Wind_Movement not found"); return; }
		M->Modify();
		UMaterialExpressionMaterialFunctionCall* Wind = nullptr;
		for (UMaterialExpression* E : M->GetExpressions()) {   // rerunning completes an earlier attempt instead of adding a second call
			if (UMaterialExpressionMaterialFunctionCall* C = Cast<UMaterialExpressionMaterialFunctionCall>(E)) { if (C->MaterialFunction == WindFn) { Wind = C; } }
		}
		if (!Wind) { Wind = InsertFunction(M, WindFn, 200, 400); }
		if (!Wind) { MT_LOG("could not add the function"); return; }
		// "Small Movement Mask" (which vertices get the leaf flutter) is the one required input: every vertex, a constant 1.
		// The medium / large masks have auto-mask defaults that are left alone.
		for (const FFunctionExpressionInput& I : Wind->FunctionInputs) {
			const FString Name = I.Input.InputName.ToString();
			if (Name == TEXT("Small Movement Mask") && !I.Input.Expression) {
				UMaterialExpressionConstant* One = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(M, UMaterialExpressionConstant::StaticClass(), -100, 400));
				if (One) { One->R = 1.0f; UMaterialEditingLibrary::ConnectMaterialExpressions(One, TEXT(""), Wind, Name); MT_LOG("  %s <- 1", *Name); }
			}
		}
		MT_LOG("foliage wind: inputs=[%s] outputs=[%s]", *FString::JoinBy(Wind->FunctionInputs, TEXT(","), [](const FFunctionExpressionInput& I) { return I.Input.InputName.ToString(); }),
			*FString::JoinBy(Wind->FunctionOutputs, TEXT(","), [](const FFunctionExpressionOutput& O) { return O.Output.OutputName.ToString(); }));
		const FString Out = Wind->FunctionOutputs.Num() > 0 ? Wind->FunctionOutputs[0].Output.OutputName.ToString() : FString();
		const bool bOut = UMaterialEditingLibrary::ConnectMaterialProperty(Wind, Out, MP_WorldPositionOffset);
		UMaterialEditingLibrary::LayoutMaterialExpressions(M);
		TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(M);
		for (const FString& E : Errors) { MT_LOG("  compile: %s", *E); }
		MT_LOG("%s: WPO <- wind.%s (%d), saved=%d, errors=%d", *M->GetName(), *Out, bOut ? 1 : 0, Save(M) ? 1 : 0, Errors.Num());
	}));

// wot.Mat.SetParent <MaterialInstancePath> <MaterialPath> : re-parent an instance (keeps its parameter overrides) and save
static FAutoConsoleCommand GWotMatSetParent(
	TEXT("wot.Mat.SetParent"), TEXT("Re-parent a material instance: wot.Mat.SetParent /Game/Path/MI /Game/Path/Material"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 2) { return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(FSoftObjectPath(Path).TryLoad());
		const FString PPath = Args[1].Contains(TEXT(".")) ? Args[1] : Args[1] + TEXT(".") + FPackageName::GetShortName(Args[1]);
		UMaterialInterface* Parent = Cast<UMaterialInterface>(FSoftObjectPath(PPath).TryLoad());
		if (!MI || !Parent) { MT_LOG("instance or parent not found: %s / %s", *Path, *PPath); return; }
		MI->Modify();
		UMaterialEditingLibrary::SetMaterialInstanceParent(MI, Parent);
		UMaterialEditingLibrary::UpdateMaterialInstance(MI);
		MT_LOG("%s parent -> %s saved=%d", *MI->GetName(), *Parent->GetName(), Save(MI) ? 1 : 0);
	}));

// wot.Mat.AddLandscapeWeather <MaterialPath> : route a ground material through UDW's "Dynamic Landscape Weather
// Effects" (snow / dust depth with trails, puddles with ripples): Base Color, Metallic, Specular, Roughness, Normal,
// World Position Offset and Pixel Depth Offset. Materials that end in one Material Attributes pin must be broken first.
static FAutoConsoleCommand GWotMatAddLandscapeWeather(
	TEXT("wot.Mat.AddLandscapeWeather"), TEXT("Route a ground material through UDW's Dynamic Landscape Weather Effects: wot.Mat.AddLandscapeWeather /Game/Path/Material [/Game/Path/NewCopy]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		UMaterial* M = Args.Num() > 1 ? Load(Args[1]) : nullptr;   // with a second path: a copy of the first gets the effects
		if (!M && Args.Num() > 1) {
			if (!UEditorAssetLibrary::DuplicateAsset(Args[0], Args[1])) { MT_LOG("could not duplicate %s -> %s", *Args[0], *Args[1]); return; }
			M = Load(Args[1]);
		}
		if (!M && Args.Num() > 0) { M = Load(Args[0]); }
		if (!M) { MT_LOG("usage: wot.Mat.AddLandscapeWeather /Game/Path/Material [/Game/Path/NewCopy] (not found)"); return; }
		UMaterialFunction* Fn = Cast<UMaterialFunction>(FSoftObjectPath(TEXT("/Game/UltraDynamicSky/Materials/Weather/Dynamic_Landscape_Weather_Effects.Dynamic_Landscape_Weather_Effects")).TryLoad());
		if (!Fn) { MT_LOG("Dynamic_Landscape_Weather_Effects not found"); return; }
		if (AlreadyUses(M, Fn)) { MT_LOG("%s already has landscape weather", *M->GetName()); return; }
		if (M->bUseMaterialAttributes) { MT_LOG("%s uses a single Material Attributes output: break it into property pins first", *M->GetName()); return; }
		M->Modify();
		// the prop-style snow / wet functions (wot.Mat.AddWeather) would sit under the landscape snow and hide the
		// footprints: unwire them (each property goes back to what fed the function, through however many are chained)
		// and drop the calls
		TArray<UMaterialFunction*> OldFns;
		for (const TCHAR* FnPath : { TEXT("/Game/UltraDynamicSky/Materials/Weather/Snow_And_Dust_Weather_Effects.Snow_And_Dust_Weather_Effects"), TEXT("/Game/UltraDynamicSky/Materials/Weather/Wet_Weather_Effects.Wet_Weather_Effects") }) {
			if (UMaterialFunction* Old = Cast<UMaterialFunction>(FSoftObjectPath(FnPath).TryLoad())) { if (AlreadyUses(M, Old)) { OldFns.Add(Old); } }
		}
		const EMaterialProperty Props[] = { MP_BaseColor, MP_Metallic, MP_Specular, MP_Roughness, MP_Normal, MP_WorldPositionOffset, MP_EmissiveColor, MP_AmbientOcclusion };
		for (int32 Pass = 0; Pass < 8 && OldFns.Num() > 0; ++Pass) {
			bool bChanged = false;
			for (EMaterialProperty P : Props) {
				FExpressionInput* In = M->GetExpressionInputForProperty(P);
				UMaterialExpressionMaterialFunctionCall* Call = In ? Cast<UMaterialExpressionMaterialFunctionCall>(In->Expression) : nullptr;
				if (!Call || !OldFns.Contains(Call->MaterialFunction)) { continue; }
				const FString OutName = Call->FunctionOutputs.IsValidIndex(In->OutputIndex) ? Call->FunctionOutputs[In->OutputIndex].Output.OutputName.ToString() : FString();
				const FFunctionExpressionInput* Src = Call->FunctionInputs.FindByPredicate([&OutName](const FFunctionExpressionInput& I) { return I.Input.InputName.ToString().Replace(TEXT(" "), TEXT("")).Equals(OutName.Replace(TEXT(" "), TEXT("")), ESearchCase::IgnoreCase); });
				if (Src && Src->Input.Expression) { In->Connect(Src->Input.OutputIndex, Src->Input.Expression); MT_LOG("  %s <- %s again (bypassing %s)", PropName(P), *Src->Input.Expression->GetName(), *Call->MaterialFunction->GetName()); }
				else { In->Expression = nullptr; MT_LOG("  %s unwired (bypassing %s)", PropName(P), *Call->MaterialFunction->GetName()); }
				bChanged = true;
			}
			if (!bChanged) { break; }
		}
		TArray<UMaterialExpressionMaterialFunctionCall*> Dead;
		for (UMaterialExpression* E : M->GetExpressions()) { if (UMaterialExpressionMaterialFunctionCall* C = Cast<UMaterialExpressionMaterialFunctionCall>(E)) { if (OldFns.Contains(C->MaterialFunction)) { Dead.Add(C); } } }
		for (UMaterialExpressionMaterialFunctionCall* C : Dead) { UMaterialEditingLibrary::DeleteMaterialExpression(M, C); }
		UMaterialExpressionMaterialFunctionCall* Call = InsertFunction(M, Fn, 400, -400);
		if (!Call) { MT_LOG("could not add the function"); return; }
		MT_LOG("landscape weather: inputs=[%s] outputs=[%s]", *FString::JoinBy(Call->FunctionInputs, TEXT(","), [](const FFunctionExpressionInput& I) { return I.Input.InputName.ToString(); }),
			*FString::JoinBy(Call->FunctionOutputs, TEXT(","), [](const FFunctionExpressionOutput& O) { return O.Output.OutputName.ToString(); }));
		auto Has = [Call](const FString& Name, bool bOut) {
			if (bOut) { for (const FFunctionExpressionOutput& O : Call->FunctionOutputs) { if (O.Output.OutputName.ToString().Equals(Name, ESearchCase::IgnoreCase)) { return true; } } }
			else { for (const FFunctionExpressionInput& I : Call->FunctionInputs) { if (I.Input.InputName.ToString().Equals(Name, ESearchCase::IgnoreCase)) { return true; } } }
			return false;
		};
		struct FPin { EMaterialProperty P; const TCHAR* Name; };
		const FPin Pins[] = { {MP_BaseColor, TEXT("Base Color")}, {MP_Metallic, TEXT("Metallic")}, {MP_Specular, TEXT("Specular")}, {MP_Roughness, TEXT("Roughness")}, {MP_Normal, TEXT("Normal")}, {MP_WorldPositionOffset, TEXT("World Position Offset")} };
		for (const FPin& Pin : Pins) {
			if (Has(Pin.Name, false) && Has(Pin.Name, true)) { Route(M, Call, Pin.P, Pin.Name, Pin.Name); }
			else { MT_LOG("  %s: no such pin pair on the function, skipped", Pin.Name); }
		}
		if (Has(TEXT("Pixel Depth Offset"), true)) { UMaterialEditingLibrary::ConnectMaterialProperty(Call, TEXT("Pixel Depth Offset"), MP_PixelDepthOffset); }
		UMaterialEditingLibrary::LayoutMaterialExpressions(M);
		TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(M);
		for (const FString& E : Errors) { MT_LOG("  compile: %s", *E); }
		MT_LOG("%s: landscape weather added, saved=%d, errors=%d", *M->GetName(), Save(M) ? 1 : 0, Errors.Num());
	}));

// wot.Mat.CloneMI <SourceMI> <NewPath> <ParentMaterial> [PhysMaterial] : copy a palette instance (keeps its colour) under
// another base, e.g. the ground variant of a palette colour, optionally tagging it with a physical material
static FAutoConsoleCommand GWotMatCloneMI(
	TEXT("wot.Mat.CloneMI"), TEXT("Copy a material instance under a new parent: wot.Mat.CloneMI /Game/Path/P_Grass /Game/Path/G_Grass /Game/Path/voxel_ground_base [/Game/Path/PM_Ground]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 3) { MT_LOG("usage: wot.Mat.CloneMI /Src/MI /New/MI /Parent/Material [/Phys/Material]"); return; }
		auto Obj = [](const FString& Arg) { const FString Path = Arg.Contains(TEXT(".")) ? Arg : Arg + TEXT(".") + FPackageName::GetShortName(Arg); return FSoftObjectPath(Path).TryLoad(); };
		UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(Obj(Args[1]));
		if (!MI) {
			if (!UEditorAssetLibrary::DuplicateAsset(Args[0], Args[1])) { MT_LOG("could not duplicate %s -> %s", *Args[0], *Args[1]); return; }
			MI = Cast<UMaterialInstanceConstant>(Obj(Args[1]));
		}
		UMaterialInterface* Parent = Cast<UMaterialInterface>(Obj(Args[2]));
		if (!MI || !Parent) { MT_LOG("instance or parent not found"); return; }
		MI->Modify();
		UMaterialEditingLibrary::SetMaterialInstanceParent(MI, Parent);
		if (Args.Num() > 3) { MI->PhysMaterial = Cast<UPhysicalMaterial>(Obj(Args[3])); }
		UMaterialEditingLibrary::UpdateMaterialInstance(MI);
		MT_LOG("%s parent -> %s phys=%s saved=%d", *MI->GetName(), *Parent->GetName(), MI->PhysMaterial ? *MI->PhysMaterial->GetName() : TEXT("none"), Save(MI) ? 1 : 0);
	}));

// wot.Mat.MakePhysMat <Path> : a plain physical material asset (the tag UDW uses to know which meshes take footprints)
static FAutoConsoleCommand GWotMatMakePhysMat(
	TEXT("wot.Mat.MakePhysMat"), TEXT("Create a physical material asset: wot.Mat.MakePhysMat /Game/Path/PM_Ground"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 1) { MT_LOG("usage: wot.Mat.MakePhysMat /Game/Path/PM_Name"); return; }
		const FString PkgName = Args[0], Name = FPackageName::GetShortName(Args[0]);
		if (FSoftObjectPath(PkgName + TEXT(".") + Name).TryLoad()) { MT_LOG("%s exists", *PkgName); return; }
		UPackage* Pkg = CreatePackage(*PkgName);
		UPhysicalMaterial* PM = NewObject<UPhysicalMaterial>(Pkg, *Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(PM);
		MT_LOG("%s created saved=%d", *PkgName, Save(PM) ? 1 : 0);
	}));

// wot.Mat.Delete <AssetPath> : delete an asset the tools made (references in memory are cleared; re-parent instances after)
static FAutoConsoleCommand GWotMatDelete(
	TEXT("wot.Mat.Delete"), TEXT("Delete an asset: wot.Mat.Delete /Game/Path/Asset"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		if (Args.Num() < 1) { MT_LOG("usage: wot.Mat.Delete /Game/Path/Asset"); return; }
		MT_LOG("%s deleted=%d", *Args[0], UEditorAssetLibrary::DeleteAsset(Args[0]) ? 1 : 0);
	}));

// wot.Mat.SetMeshPhysMat <StaticMesh> <PhysMaterial|None> : the physical material on a project mesh's collision body
// (the towns' ground voxels are engine cubes, so those get PM_Ground at runtime from their material: WotWeatherFeet::TagGround).
static FAutoConsoleCommand GWotMatSetMeshPhysMat(
	TEXT("wot.Mat.SetMeshPhysMat"), TEXT("Set a static mesh's collision physical material: wot.Mat.SetMeshPhysMat /Game/Path/SM_Cube /Game/Path/PM_Ground"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 2) { MT_LOG("usage: wot.Mat.SetMeshPhysMat /Game/Path/Mesh /Game/Path/PhysMat|None"); return; }
		auto Obj = [](const FString& Arg) { const FString Path = Arg.Contains(TEXT(".")) ? Arg : Arg + TEXT(".") + FPackageName::GetShortName(Arg); return FSoftObjectPath(Path).TryLoad(); };
		UStaticMesh* SM = Cast<UStaticMesh>(Obj(Args[0]));
		UPhysicalMaterial* PM = Args[1] == TEXT("None") ? nullptr : Cast<UPhysicalMaterial>(Obj(Args[1]));
		if (!SM || !SM->GetBodySetup() || (!PM && Args[1] != TEXT("None"))) { MT_LOG("mesh / body setup / physical material not found"); return; }
		SM->Modify(); SM->GetBodySetup()->Modify();
		SM->GetBodySetup()->PhysMaterial = PM;
		MT_LOG("%s phys material -> %s saved=%d", *SM->GetName(), PM ? *PM->GetName() : TEXT("None"), Save(SM) ? 1 : 0);
	}));

// wot.Mat.SetSwitch <MaterialInstance> <StaticSwitchName> <0|1> : a static switch parameter on an instance (also the ones
// that live inside UDW's weather functions, e.g. "Apply Weather Mask Brushes to Snow/Dust"), then save
static FAutoConsoleCommand GWotMatSetSwitch(
	TEXT("wot.Mat.SetSwitch"), TEXT("Set a static switch parameter on a material instance: wot.Mat.SetSwitch /Game/Path/MI \"Switch Name\" 0|1"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 3) { MT_LOG("usage: wot.Mat.SetSwitch /Game/Path/MI \"Switch Name\" 0|1"); return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(FSoftObjectPath(Path).TryLoad());
		if (!MI) { MT_LOG("instance not found: %s", *Args[0]); return; }
		const FString Name = Args[1].TrimQuotes();
		MI->Modify();
		const bool bVal = Args[2] == TEXT("1");
		bool bOk = UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MI, *Name, bVal);
		if (!bOk) {   // the library refuses switches that live inside functions: edit the static permutation directly
			FStaticParameterSet Set = MI->GetStaticParameters();
			bool bFound = false;
			for (FStaticSwitchParameter& P : Set.StaticSwitchParameters) { if (P.ParameterInfo.Name == *Name) { P.Value = bVal; P.bOverride = true; bFound = true; } }
			if (!bFound) {
				FStaticSwitchParameter NewP; NewP.ParameterInfo = FMaterialParameterInfo(*Name); NewP.Value = bVal; NewP.bOverride = true;
				bool bCur = false; MI->GetStaticSwitchParameterValue(NewP.ParameterInfo, bCur, NewP.ExpressionGUID);
				Set.StaticSwitchParameters.Add(NewP);
			}
			MI->UpdateStaticPermutation(Set);
			bool bNow = !bVal; FGuid G; MI->GetStaticSwitchParameterValue(FMaterialParameterInfo(*Name), bNow, G);
			bOk = (bNow == bVal);
		}
		UMaterialEditingLibrary::UpdateMaterialInstance(MI);
		if (!bOk) {
			TArray<FMaterialParameterInfo> Infos; TArray<FGuid> Ids; MI->GetAllStaticSwitchParameterInfo(Infos, Ids);
			TArray<FString> Names; for (const FMaterialParameterInfo& I : Infos) { Names.Add(FString::Printf(TEXT("'%s'(assoc=%d idx=%d)"), *I.Name.ToString(), (int32)I.Association, I.Index)); }
			MT_LOG("  static switches on %s: %s", *MI->GetName(), *FString::Join(Names, TEXT(", ")));
		}
		MT_LOG("%s: '%s' = %s ok=%d saved=%d", *MI->GetName(), *Name, *Args[2], bOk ? 1 : 0, Save(MI) ? 1 : 0);
	}));

// wot.Mat.WireConst <Material> <FunctionName> <"Input Name"> <v> [v2 v3 v4] : feed a constant into one input of the first call
// of that function in the material (a scalar, or a vector / colour with 3 or 4 numbers), recompile and save
static FAutoConsoleCommand GWotMatWireConst(
	TEXT("wot.Mat.WireConst"), TEXT("Wire a constant into a function call input: wot.Mat.WireConst /Game/Path/Material Snow_And_Dust_Weather_Effects \"Z Mask Offset\" -1"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 4) { MT_LOG("usage: wot.Mat.WireConst /Game/Path/Material FunctionName \"Input\" v [v2 v3 v4]"); return; }
		UMaterial* M = Load(Args[0]);
		if (!M) { MT_LOG("material not found: %s", *Args[0]); return; }
		UMaterialExpressionMaterialFunctionCall* Call = nullptr;
		for (UMaterialExpression* E : M->GetExpressions()) { if (UMaterialExpressionMaterialFunctionCall* C = Cast<UMaterialExpressionMaterialFunctionCall>(E)) { if (C->MaterialFunction && C->MaterialFunction->GetName() == Args[1]) { Call = C; break; } } }
		if (!Call) { MT_LOG("no call of %s in %s", *Args[1], *M->GetName()); return; }
		const FString InName = Args[2].TrimQuotes();
		M->Modify();
		UMaterialExpression* Src = nullptr;
		if (Args.Num() >= 6) {
			UMaterialExpressionConstant4Vector* V = Cast<UMaterialExpressionConstant4Vector>(UMaterialEditingLibrary::CreateMaterialExpression(M, UMaterialExpressionConstant4Vector::StaticClass(), -400, 600));
			if (V) { V->Constant = FLinearColor(FCString::Atof(*Args[3]), FCString::Atof(*Args[4]), FCString::Atof(*Args[5]), Args.Num() >= 7 ? FCString::Atof(*Args[6]) : 1.f); Src = V; }
		}
		else {
			UMaterialExpressionConstant* C = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(M, UMaterialExpressionConstant::StaticClass(), -400, 600));
			if (C) { C->R = FCString::Atof(*Args[3]); Src = C; }
		}
		const bool bOk = Src && UMaterialEditingLibrary::ConnectMaterialExpressions(Src, TEXT(""), Call, InName);
		TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(M);
		for (const FString& E : Errors) { MT_LOG("  compile: %s", *E); }
		MT_LOG("%s: %s.'%s' <- const ok=%d saved=%d errors=%d", *M->GetName(), *Args[1], *InName, bOk ? 1 : 0, Save(M) ? 1 : 0, Errors.Num());
	}));

// wot.Mat.SetScalar <MaterialInstance> <Param> <v> : a scalar parameter on an instance, then save
static FAutoConsoleCommand GWotMatSetScalar(
	TEXT("wot.Mat.SetScalar"), TEXT("Set a scalar parameter on a material instance: wot.Mat.SetScalar /Game/Path/MI Snowy 1"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotMat;
		if (Args.Num() < 3) { MT_LOG("usage: wot.Mat.SetScalar /Game/Path/MI Param v"); return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(FSoftObjectPath(Path).TryLoad());
		if (!MI) { MT_LOG("instance not found: %s", *Args[0]); return; }
		MI->Modify();
		const bool bOk = UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MI, *Args[1].TrimQuotes(), FCString::Atof(*Args[2]));
		UMaterialEditingLibrary::UpdateMaterialInstance(MI);
		MT_LOG("%s: %s = %s ok=%d saved=%d", *MI->GetName(), *Args[1], *Args[2], bOk ? 1 : 0, Save(MI) ? 1 : 0);
	}));
#endif
