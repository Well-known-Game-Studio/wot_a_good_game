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
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
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
		if (!M) { MT_LOG("usage: wot.Mat.Dump /Game/Path/Material (not found)"); return; }
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
#endif
