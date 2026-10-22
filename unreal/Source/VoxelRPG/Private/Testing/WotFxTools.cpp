// Editor-only console commands to inspect and trim Niagara systems from the playtest harness:
//   wot.FX.Dump   <system>                              emitters, sim target and every module with its enabled state
//   wot.FX.Module <system> <module substring> 0|1 [emitter substring]
//   wot.FX.BindUser <system> <module substring> <input name> <User.Param> <true|false>
//                                                       drive a bool module input from a (new) user parameter, e.g. the
//                                                       Collision module's "Collision Enabled" from User.CollisionEnabled
//   wot.FX.Status [substring]                        every Niagara component in this world: owner, asset, active, visible
//                                                       enable / disable matching modules, recompile and save the asset
// (used to switch off the per-particle scene-trace Collision module of the portal effect: with 23 portals in a level it
// issued ~115k raycasts per frame and held the editor and PIE to ~3 fps)
#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/SavePackage.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/PackageName.h"
#include "NiagaraComponent.h"
#include "NiagaraScript.h"
#include "NiagaraCommon.h"
#include "NiagaraParameterStore.h"
#include "NiagaraParameterMapHistory.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"

namespace
{
	UNiagaraSystem* LoadSystem(const FString& Arg)
	{
		const FString Path = Arg.Contains(TEXT(".")) ? Arg : Arg + TEXT(".") + FPackageName::GetShortName(Arg);
		UNiagaraSystem* Sys = Cast<UNiagaraSystem>(FSoftObjectPath(Path).TryLoad());
		if (!Sys) { UE_LOG(LogTemp, Warning, TEXT("[FX] no Niagara system at %s"), *Path); }
		return Sys;
	}

	UNiagaraGraph* GraphOf(const FNiagaraEmitterHandle& H)
	{
		FVersionedNiagaraEmitterData* Data = H.GetEmitterData();
		UNiagaraScriptSource* Src = Data ? Cast<UNiagaraScriptSource>(Data->GraphSource) : nullptr;
		return Src ? Src->NodeGraph : nullptr;
	}

	bool SaveAsset(UObject* Obj)
	{
		UPackage* Pkg = Obj->GetOutermost(); Pkg->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Pkg, Obj, *File, Args);
	}
}

static FAutoConsoleCommand GWotFxDump(
	TEXT("wot.FX.Dump"), TEXT("List a Niagara system's emitters and modules: wot.FX.Dump <system>"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		if (Args.Num() < 1) { UE_LOG(LogTemp, Warning, TEXT("[FX] usage: wot.FX.Dump <system>")); return; }
		UNiagaraSystem* Sys = LoadSystem(Args[0]); if (!Sys) { return; }
		UE_LOG(LogTemp, Display, TEXT("[FX] %s: %d emitters, effect type %s"), *Sys->GetName(), Sys->GetEmitterHandles().Num(), *GetNameSafe(Sys->GetEffectType()));
		for (const FNiagaraEmitterHandle& H : Sys->GetEmitterHandles())
		{
			FVersionedNiagaraEmitterData* Data = H.GetEmitterData();
			UE_LOG(LogTemp, Display, TEXT("[FX]  emitter %s enabled=%d sim=%s"), *H.GetName().ToString(), H.GetIsEnabled() ? 1 : 0,
				Data ? (Data->SimTarget == ENiagaraSimTarget::GPUComputeSim ? TEXT("GPU") : TEXT("CPU")) : TEXT("?"));
			UNiagaraGraph* Graph = GraphOf(H); if (!Graph) { continue; }
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UNiagaraNodeFunctionCall* Fn = Cast<UNiagaraNodeFunctionCall>(Node))
				{
					UE_LOG(LogTemp, Display, TEXT("[FX]    module %-40s %s  script=%s"), *Fn->GetFunctionName(), LexToString(Fn->GetDesiredEnabledState()), *GetNameSafe(Fn->FunctionScript));
				}
			}
		}
	}));

static FAutoConsoleCommand GWotFxModule(
	TEXT("wot.FX.Module"), TEXT("Enable/disable Niagara modules: wot.FX.Module <system> <module substring> 0|1 [emitter substring]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("[FX] usage: wot.FX.Module <system> <module substring> 0|1 [emitter substring]")); return; }
		UNiagaraSystem* Sys = LoadSystem(Args[0]); if (!Sys) { return; }
		const bool bEnable = FCString::Atoi(*Args[2]) != 0;
		const FString EmitterFilter = Args.Num() > 3 ? Args[3] : FString();
		int32 Changed = 0;
		for (const FNiagaraEmitterHandle& H : Sys->GetEmitterHandles())
		{
			if (!EmitterFilter.IsEmpty() && !H.GetName().ToString().Contains(EmitterFilter)) { continue; }
			UNiagaraGraph* Graph = GraphOf(H); if (!Graph) { continue; }
			bool bGraphChanged = false;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UNiagaraNodeFunctionCall* Fn = Cast<UNiagaraNodeFunctionCall>(Node);
				if (!Fn || !Fn->GetFunctionName().Contains(Args[1])) { continue; }
				const ENodeEnabledState Want = bEnable ? ENodeEnabledState::Enabled : ENodeEnabledState::Disabled;
				if (Fn->GetDesiredEnabledState() == Want) { continue; }
				Fn->Modify(); Fn->SetEnabledState(Want, true); Fn->MarkNodeRequiresSynchronization(TEXT("wot.FX.Module"), true); ++Changed; bGraphChanged = true;
				UE_LOG(LogTemp, Display, TEXT("[FX] %s / %s: module %s -> %s"), *Sys->GetName(), *H.GetName().ToString(), *Fn->GetFunctionName(), LexToString(Want));
			}
			if (bGraphChanged) { Graph->NotifyGraphChanged(); }
		}
		if (Changed == 0) { UE_LOG(LogTemp, Display, TEXT("[FX] %s: no module matching '%s' needed a change"), *Sys->GetName(), *Args[1]); return; }
		Sys->RequestCompile(true);
		Sys->WaitForCompilationComplete(false, false);
		const bool bSaved = SaveAsset(Sys);
		UE_LOG(LogTemp, Display, TEXT("[FX] %s: %d modules changed, recompiled, saved=%d"), *Sys->GetName(), Changed, bSaved ? 1 : 0);
	}));
static FAutoConsoleCommandWithWorldAndArgs GWotFxStatus(
	TEXT("wot.FX.Status"), TEXT("List Niagara components in this world: wot.FX.Status [name substring]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (!World) { return; }
		const FString Filter = Args.Num() > 0 ? Args[0] : FString();
		int32 N = 0, Active = 0;
		for (TObjectIterator<UNiagaraComponent> It; It; ++It)
		{
			UNiagaraComponent* C = *It;
			if (!IsValid(C) || C->GetWorld() != World || !C->IsRegistered()) { continue; }
			const FString Owner = GetNameSafe(C->GetOwner());
			if (!Filter.IsEmpty() && !Owner.Contains(Filter) && !GetNameSafe(C->GetAsset()).Contains(Filter)) { continue; }
			++N; if (C->IsActive()) { ++Active; }
			FString User;
			for (const FNiagaraVariableWithOffset& V : C->GetOverrideParameters().ReadParameterVariables())
			{
				if (V.GetType() != FNiagaraTypeDefinition::GetBoolDef()) { continue; }
				const uint8* D = C->GetOverrideParameters().GetParameterData(V);
				User += FString::Printf(TEXT(" %s=%d"), *V.GetName().ToString(), D ? (*reinterpret_cast<const int32*>(D) != 0 ? 1 : 0) : -1);
			}
			UE_LOG(LogTemp, Display, TEXT("[FX]   %-28s %-26s active=%d visible=%d autoActivate=%d%s"), *Owner, *GetNameSafe(C->GetAsset()), C->IsActive() ? 1 : 0, C->IsVisible() ? 1 : 0, C->bAutoActivate ? 1 : 0, *User);
		}
		UE_LOG(LogTemp, Display, TEXT("[FX] %s: %d Niagara components, %d active"), *World->GetMapName(), N, Active);
	}));
// wot.FX.Inputs <system> <module substring> : a module's inputs (type), its static switch pins, the override pins that
// feed it (default values or linked parameters) and the rapid-iteration constants that hold its plain default values
static FAutoConsoleCommand GWotFxInputs(
	TEXT("wot.FX.Inputs"), TEXT("List a Niagara module's inputs and current values: wot.FX.Inputs <system> <module substring>"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		if (Args.Num() < 2) { UE_LOG(LogTemp, Warning, TEXT("[FX] usage: wot.FX.Inputs <system> <module substring>")); return; }
		UNiagaraSystem* Sys = LoadSystem(Args[0]); if (!Sys) { return; }
		for (const FNiagaraEmitterHandle& H : Sys->GetEmitterHandles())
		{
			UNiagaraGraph* Graph = GraphOf(H); FVersionedNiagaraEmitterData* Data = H.GetEmitterData(); if (!Graph || !Data) { continue; }
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UNiagaraNodeFunctionCall* Fn = Cast<UNiagaraNodeFunctionCall>(Node);
				if (!Fn || !Fn->GetFunctionName().Contains(Args[1])) { continue; }
				UE_LOG(LogTemp, Display, TEXT("[FX] %s / %s / module %s (%s)"), *Sys->GetName(), *H.GetName().ToString(), *Fn->GetFunctionName(), *GetPathNameSafe(Fn->FunctionScript));
				FCompileConstantResolver Resolver(H.GetInstance(), ENiagaraScriptUsage::ParticleUpdateScript);
				TArray<FNiagaraVariable> Inputs;
				FNiagaraStackGraphUtilities::GetStackFunctionInputs(*Fn, Inputs, Resolver, FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);
				for (const FNiagaraVariable& V : Inputs) { UE_LOG(LogTemp, Display, TEXT("[FX]    input  %-50s %s"), *V.GetName().ToString(), *V.GetType().GetName()); }
				TArray<UEdGraphPin*> SwitchPins; TSet<UEdGraphPin*> Hidden;
				FNiagaraStackGraphUtilities::GetStackFunctionStaticSwitchPins(*Fn, SwitchPins, Hidden, Resolver);
				for (UEdGraphPin* P : SwitchPins) { UE_LOG(LogTemp, Display, TEXT("[FX]    switch %-50s = %s%s"), *P->PinName.ToString(), *P->DefaultValue, Hidden.Contains(P) ? TEXT(" (hidden)") : TEXT("")); }
				const FString Prefix = Fn->GetFunctionName() + TEXT(".");
				for (UEdGraphNode* N2 : Graph->Nodes)
				{
					if (!N2 || N2->GetClass()->GetName() != TEXT("NiagaraNodeParameterMapSet")) { continue; }
					for (UEdGraphPin* P : N2->Pins)
					{
						if (P->Direction != EGPD_Input || !P->PinName.ToString().StartsWith(Prefix)) { continue; }
						FString Linked;
						for (UEdGraphPin* L : P->LinkedTo) { Linked += FString::Printf(TEXT(" <- %s:%s"), *L->GetOwningNode()->GetClass()->GetName(), *L->PinName.ToString()); }
						UE_LOG(LogTemp, Display, TEXT("[FX]    override %-48s = '%s'%s"), *P->PinName.ToString(), *P->DefaultValue, *Linked);
					}
				}
				if (UNiagaraScript* Script = Data->UpdateScriptProps.Script)
				{
					TArray<FNiagaraVariable> Consts; Script->RapidIterationParameters.GetParameters(Consts);
					for (const FNiagaraVariable& C : Consts)
					{
						if (!C.GetName().ToString().Contains(Prefix)) { continue; }
						const uint8* D = Script->RapidIterationParameters.GetParameterData(C);
						FString Val = TEXT("?");
						if (D && C.GetType() == FNiagaraTypeDefinition::GetFloatDef()) { Val = FString::SanitizeFloat(*reinterpret_cast<const float*>(D)); }
						else if (D && (C.GetType() == FNiagaraTypeDefinition::GetIntDef() || C.GetType() == FNiagaraTypeDefinition::GetBoolDef() || C.GetType().IsEnum())) { Val = FString::FromInt(*reinterpret_cast<const int32*>(D)); }
						else if (D && C.GetType() == FNiagaraTypeDefinition::GetVec3Def()) { const FVector3f* V3 = reinterpret_cast<const FVector3f*>(D); Val = FString::Printf(TEXT("(%g %g %g)"), V3->X, V3->Y, V3->Z); }
						UE_LOG(LogTemp, Display, TEXT("[FX]    const  %-50s %s = %s"), *C.GetName().ToString(), *C.GetType().GetName(), *Val);
					}
				}
			}
		}
	}));
static FAutoConsoleCommand GWotFxBindUser(
	TEXT("wot.FX.BindUser"), TEXT("Drive a bool module input from a user parameter: wot.FX.BindUser <system> <module substring> <input name> <User.Param> <true|false>"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		if (Args.Num() < 5) { UE_LOG(LogTemp, Warning, TEXT("[FX] usage: wot.FX.BindUser <system> <module substring> <input name> <User.Param> <true|false>")); return; }
		UNiagaraSystem* Sys = LoadSystem(Args[0]); if (!Sys) { return; }
		const FString InputName = Args[2].Replace(TEXT("_"), TEXT(" "));
		const bool bDefault = Args[4].ToBool();
		const FNiagaraVariable UserVar(FNiagaraTypeDefinition::GetBoolDef(), FName(*Args[3]));
		Sys->Modify();
		FNiagaraParameterStore& Store = Sys->GetExposedParameters();
		if (Store.IndexOf(UserVar) == INDEX_NONE) { Store.AddParameter(UserVar, true, true); UE_LOG(LogTemp, Display, TEXT("[FX] %s: added user parameter %s"), *Sys->GetName(), *Args[3]); }
		Store.SetParameterValue(FNiagaraBool(bDefault), UserVar);
		int32 Bound = 0;
		for (const FNiagaraEmitterHandle& H : Sys->GetEmitterHandles())
		{
			UNiagaraGraph* Graph = GraphOf(H); if (!Graph) { continue; }
			TArray<UNiagaraNodeFunctionCall*> Matches;
			for (UEdGraphNode* Node : Graph->Nodes) { if (UNiagaraNodeFunctionCall* Fn = Cast<UNiagaraNodeFunctionCall>(Node)) { if (Fn->GetFunctionName().Contains(Args[1])) { Matches.Add(Fn); } } }
			for (UNiagaraNodeFunctionCall* Fn : Matches)
			{
				const FNiagaraParameterHandle Handle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FName(*(TEXT("Module.") + InputName)), FName(*Fn->GetFunctionName()));
				UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Fn, Handle, FNiagaraTypeDefinition::GetBoolDef(), FGuid(), FGuid());
				if (OverridePin.LinkedTo.Num() > 0) { UE_LOG(LogTemp, Warning, TEXT("[FX] %s / %s: input %s already has an override (%s); not changed"), *H.GetName().ToString(), *Fn->GetFunctionName(), *InputName, *OverridePin.LinkedTo[0]->PinName.ToString()); continue; }
				FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(OverridePin, UserVar, TSet<FNiagaraVariableBase>(), ENiagaraDefaultMode::Value);
				Fn->MarkNodeRequiresSynchronization(TEXT("wot.FX.BindUser"), true); ++Bound;
				UE_LOG(LogTemp, Display, TEXT("[FX] %s / %s: input '%s' <- %s (default %d)"), *H.GetName().ToString(), *Fn->GetFunctionName(), *InputName, *Args[3], bDefault ? 1 : 0);
			}
			if (Matches.Num() > 0) { Graph->NotifyGraphChanged(); }
		}
		Sys->RequestCompile(true);
		Sys->WaitForCompilationComplete(false, false);
		const bool bSaved = SaveAsset(Sys);
		UE_LOG(LogTemp, Display, TEXT("[FX] %s: %d inputs bound, recompiled, saved=%d"), *Sys->GetName(), Bound, bSaved ? 1 : 0);
	}));
#endif
