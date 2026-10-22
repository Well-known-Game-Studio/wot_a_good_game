// Editor-only console commands to inspect and trim Niagara systems from the playtest harness:
//   wot.FX.Dump   <system>                              emitters, sim target and every module with its enabled state
//   wot.FX.Module <system> <module substring> 0|1 [emitter substring]
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
			UE_LOG(LogTemp, Display, TEXT("[FX]   %-28s %-26s active=%d visible=%d autoActivate=%d"), *Owner, *GetNameSafe(C->GetAsset()), C->IsActive() ? 1 : 0, C->IsVisible() ? 1 : 0, C->bAutoActivate ? 1 : 0);
		}
		UE_LOG(LogTemp, Display, TEXT("[FX] %s: %d Niagara components, %d active"), *World->GetMapName(), N, Active);
	}));
#endif
