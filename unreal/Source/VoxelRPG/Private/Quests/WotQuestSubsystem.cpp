#include "Quests/WotQuestSubsystem.h"
#include "Quests/WotQuestTrackerWidget.h"
#include "WotCharacter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotQuest, Log, All);

namespace
{
	FName StripBlueprintSuffix(const FString& ClassName)
	{
		FString Name = ClassName;
		Name.RemoveFromEnd(TEXT("_C"));
		return FName(*Name);
	}

	bool NamesMatch(FName A, FName B)
	{
		return A.IsNone() || A.IsEqual(B, ENameCase::IgnoreCase);
	}
}

UWotQuestSubsystem* UWotQuestSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) {
		return nullptr;
	}
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) {
		return nullptr;
	}
	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UWotQuestSubsystem>() : nullptr;
}

void UWotQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadDefinitions();
	LoadProgress();
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UWotQuestSubsystem::TickReach), 0.5f);
	UE_LOG(LogWotQuest, Log, TEXT("Quest subsystem ready: %d definitions, %d with progress"), Definitions.Num(), Progress.Num());
}

void UWotQuestSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

void UWotQuestSubsystem::LoadDefinitions()
{
	Definitions.Empty();
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Registry.ScanPathsSynchronous({ QuestContentPath }, true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UWotQuestDefinition::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Data : Assets) {
		const UWotQuestDefinition* Def = Cast<UWotQuestDefinition>(Data.GetAsset());
		if (!Def) {
			continue;
		}
		FName Id = Def->QuestId.IsNone() ? Def->GetFName() : Def->QuestId;
		if (Definitions.Contains(Id)) {
			UE_LOG(LogWotQuest, Warning, TEXT("Duplicate quest id %s (%s)"), *Id.ToString(), *Data.GetObjectPathString());
			continue;
		}
		Definitions.Add(Id, Def);
	}
}

void UWotQuestSubsystem::ReloadDefinitions()
{
	LoadDefinitions();
	if (Tracker) {
		Tracker->Refresh();
	}
}

void UWotQuestSubsystem::LoadProgress()
{
	Progress.Empty();
	bAutoStartDone = false;
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0)) {
		if (UWotQuestSaveGame* Save = Cast<UWotQuestSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0))) {
			Progress = Save->Quests;
			bAutoStartDone = Save->bAutoStartDone;
		}
	}
	// keep objective count arrays in sync with the definitions
	for (FWotQuestProgress& P : Progress) {
		if (const UWotQuestDefinition* Def = GetQuestDefinition(P.QuestId)) {
			P.ObjectiveCounts.SetNum(Def->Objectives.Num());
		}
	}
}

void UWotQuestSubsystem::SaveProgress()
{
	UWotQuestSaveGame* Save = Cast<UWotQuestSaveGame>(UGameplayStatics::CreateSaveGameObject(UWotQuestSaveGame::StaticClass()));
	if (!Save) {
		return;
	}
	Save->Quests = Progress;
	Save->bAutoStartDone = bAutoStartDone;
	UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void UWotQuestSubsystem::ResetAllQuests()
{
	Progress.Empty();
	bAutoStartDone = false;
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);
	UE_LOG(LogWotQuest, Log, TEXT("Quest progress reset"));
	AutoStartIfNeeded();
	if (Tracker) {
		Tracker->Refresh();
	}
}

void UWotQuestSubsystem::AutoStartIfNeeded()
{
	// a save written before any quest existed must not block auto-start forever
	if (bAutoStartDone && Progress.Num() > 0) {
		return;
	}
	bAutoStartDone = true;
	for (const auto& Pair : Definitions) {
		if (Pair.Value && Pair.Value->bAutoStart && GetQuestStatus(Pair.Key) == EWotQuestStatus::Inactive) {
			StartQuest(Pair.Key);
		}
	}
	SaveProgress();
}

// ---------------------------------------------------------------- queries

const UWotQuestDefinition* UWotQuestSubsystem::GetQuestDefinition(FName QuestId) const
{
	const TObjectPtr<const UWotQuestDefinition>* Found = Definitions.Find(QuestId);
	return Found ? Found->Get() : nullptr;
}

FWotQuestProgress* UWotQuestSubsystem::FindProgress(FName QuestId)
{
	return Progress.FindByPredicate([QuestId](const FWotQuestProgress& P) { return P.QuestId == QuestId; });
}

const FWotQuestProgress* UWotQuestSubsystem::FindProgress(FName QuestId) const
{
	return Progress.FindByPredicate([QuestId](const FWotQuestProgress& P) { return P.QuestId == QuestId; });
}

EWotQuestStatus UWotQuestSubsystem::GetQuestStatus(FName QuestId) const
{
	const FWotQuestProgress* P = FindProgress(QuestId);
	return P ? P->Status : EWotQuestStatus::Inactive;
}

TArray<FName> UWotQuestSubsystem::GetActiveQuestIds() const
{
	TArray<FName> Ids;
	for (const FWotQuestProgress& P : Progress) {
		if (P.Status == EWotQuestStatus::Active) {
			Ids.Add(P.QuestId);
		}
	}
	return Ids;
}

TArray<FName> UWotQuestSubsystem::GetAllQuestIds() const
{
	TArray<FName> Ids;
	Definitions.GetKeys(Ids);
	return Ids;
}

bool UWotQuestSubsystem::GetQuestProgress(FName QuestId, FWotQuestProgress& OutProgress) const
{
	if (const FWotQuestProgress* P = FindProgress(QuestId)) {
		OutProgress = *P;
		return true;
	}
	return false;
}

int32 UWotQuestSubsystem::GetObjectiveCount(FName QuestId, FName ObjectiveId) const
{
	const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
	const FWotQuestProgress* P = FindProgress(QuestId);
	if (!Def || !P) {
		return 0;
	}
	for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
		if (Def->Objectives[i].ObjectiveId == ObjectiveId) {
			return P->ObjectiveCounts.IsValidIndex(i) ? P->ObjectiveCounts[i] : 0;
		}
	}
	return 0;
}

bool UWotQuestSubsystem::IsObjectiveDone(const UWotQuestDefinition& Def, const FWotQuestProgress& P, int32 Index) const
{
	if (!Def.Objectives.IsValidIndex(Index)) {
		return false;
	}
	const int32 Count = P.ObjectiveCounts.IsValidIndex(Index) ? P.ObjectiveCounts[Index] : 0;
	return Count >= Def.Objectives[Index].RequiredCount;
}

bool UWotQuestSubsystem::IsObjectiveComplete(FName QuestId, FName ObjectiveId) const
{
	const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
	const FWotQuestProgress* P = FindProgress(QuestId);
	if (!Def || !P) {
		return false;
	}
	for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
		if (Def->Objectives[i].ObjectiveId == ObjectiveId) {
			return IsObjectiveDone(*Def, *P, i);
		}
	}
	return false;
}

bool UWotQuestSubsystem::AreRequiredObjectivesDone(const UWotQuestDefinition& Def, const FWotQuestProgress& P) const
{
	for (int32 i = 0; i < Def.Objectives.Num(); ++i) {
		if (!Def.Objectives[i].bOptional && !IsObjectiveDone(Def, P, i)) {
			return false;
		}
	}
	return true;
}

void UWotQuestSubsystem::CollectCurrentObjectiveIndices(const UWotQuestDefinition& Def, const FWotQuestProgress& P, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	for (int32 i = 0; i < Def.Objectives.Num(); ++i) {
		if (IsObjectiveDone(Def, P, i)) {
			continue;
		}
		OutIndices.Add(i);
		if (Def.bObjectivesInOrder && !Def.Objectives[i].bOptional) {
			break;
		}
	}
}

TArray<FWotQuestObjective> UWotQuestSubsystem::GetCurrentObjectives(FName QuestId) const
{
	TArray<FWotQuestObjective> Out;
	const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
	const FWotQuestProgress* P = FindProgress(QuestId);
	if (!Def || !P || P->Status != EWotQuestStatus::Active) {
		return Out;
	}
	TArray<int32> Indices;
	CollectCurrentObjectiveIndices(*Def, *P, Indices);
	for (int32 i : Indices) {
		Out.Add(Def->Objectives[i]);
	}
	return Out;
}

// ---------------------------------------------------------------- mutations

bool UWotQuestSubsystem::StartQuest(FName QuestId)
{
	const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
	if (!Def) {
		UE_LOG(LogWotQuest, Warning, TEXT("StartQuest: unknown quest %s"), *QuestId.ToString());
		return false;
	}
	FWotQuestProgress* P = FindProgress(QuestId);
	if (P && P->Status != EWotQuestStatus::Inactive) {
		return false;
	}
	if (!P) {
		P = &Progress.AddDefaulted_GetRef();
		P->QuestId = QuestId;
	}
	P->Status = EWotQuestStatus::Active;
	P->ObjectiveCounts.Init(0, Def->Objectives.Num());
	UE_LOG(LogWotQuest, Log, TEXT("Quest started: %s"), *QuestId.ToString());
	ShowToast(FText::Format(NSLOCTEXT("Wot", "QuestStarted", "New quest: {0}"), Def->Title));
	OnQuestStarted.Broadcast(QuestId);
	SaveProgress();
	if (Tracker) {
		Tracker->Refresh();
	}
	// a quest with no objectives completes immediately
	if (Def->Objectives.Num() == 0) {
		CompleteQuest(QuestId);
	}
	return true;
}

bool UWotQuestSubsystem::CompleteQuest(FName QuestId)
{
	const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
	FWotQuestProgress* P = FindProgress(QuestId);
	if (!Def || !P || P->Status != EWotQuestStatus::Active) {
		return false;
	}
	P->Status = EWotQuestStatus::Completed;
	UE_LOG(LogWotQuest, Log, TEXT("Quest completed: %s"), *QuestId.ToString());
	ShowToast(Def->CompletionText.IsEmpty()
		? FText::Format(NSLOCTEXT("Wot", "QuestCompleted", "Quest complete: {0}"), Def->Title)
		: Def->CompletionText);
	OnQuestCompleted.Broadcast(QuestId);
	SaveProgress();
	if (Tracker) {
		Tracker->Refresh();
	}
	if (!Def->NextQuestId.IsNone()) {
		StartQuest(Def->NextQuestId);
	}
	return true;
}

bool UWotQuestSubsystem::FailQuest(FName QuestId)
{
	FWotQuestProgress* P = FindProgress(QuestId);
	if (!P || P->Status != EWotQuestStatus::Active) {
		return false;
	}
	P->Status = EWotQuestStatus::Failed;
	SaveProgress();
	if (Tracker) {
		Tracker->Refresh();
	}
	return true;
}

bool UWotQuestSubsystem::AdvanceObjectiveIndex(const UWotQuestDefinition& Def, FWotQuestProgress& P, int32 Index, int32 Count)
{
	if (!Def.Objectives.IsValidIndex(Index) || P.Status != EWotQuestStatus::Active) {
		return false;
	}
	if (P.ObjectiveCounts.Num() != Def.Objectives.Num()) {
		P.ObjectiveCounts.SetNum(Def.Objectives.Num());
	}
	const FWotQuestObjective& Obj = Def.Objectives[Index];
	int32& Current = P.ObjectiveCounts[Index];
	if (Current >= Obj.RequiredCount) {
		return false;
	}
	Current = FMath::Clamp(Current + Count, 0, Obj.RequiredCount);
	UE_LOG(LogWotQuest, Log, TEXT("Objective %s/%s: %d/%d"), *P.QuestId.ToString(), *Obj.ObjectiveId.ToString(), Current, Obj.RequiredCount);
	OnObjectiveUpdated.Broadcast(P.QuestId, Obj.ObjectiveId, Current, Obj.RequiredCount);
	if (Current >= Obj.RequiredCount) {
		ShowToast(FText::Format(NSLOCTEXT("Wot", "ObjectiveDone", "Done: {0}"), Obj.Description));
	}
	const FName QuestId = P.QuestId;
	if (AreRequiredObjectivesDone(Def, P)) {
		CompleteQuest(QuestId);
	} else {
		SaveProgress();
		if (Tracker) {
			Tracker->Refresh();
		}
	}
	return true;
}

bool UWotQuestSubsystem::AdvanceObjective(FName QuestId, FName ObjectiveId, int32 Count)
{
	const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
	FWotQuestProgress* P = FindProgress(QuestId);
	if (!Def || !P) {
		return false;
	}
	for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
		if (Def->Objectives[i].ObjectiveId == ObjectiveId) {
			return AdvanceObjectiveIndex(*Def, *P, i, Count);
		}
	}
	return false;
}

int32 UWotQuestSubsystem::ReportProgress(EWotObjectiveType Type, FName Target, int32 Count)
{
	return ReportProgressAny(Type, { Target }, Count);
}

int32 UWotQuestSubsystem::ReportProgressAny(EWotObjectiveType Type, const TArray<FName>& Targets, int32 Count)
{
	int32 Advanced = 0;
	// copy the ids first: advancing may complete a quest and start another
	TArray<FName> Active = GetActiveQuestIds();
	for (FName QuestId : Active) {
		const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
		FWotQuestProgress* P = FindProgress(QuestId);
		if (!Def || !P || P->Status != EWotQuestStatus::Active) {
			continue;
		}
		TArray<int32> Indices;
		CollectCurrentObjectiveIndices(*Def, *P, Indices);
		for (int32 i : Indices) {
			const FWotQuestObjective& Obj = Def->Objectives[i];
			if (Obj.Type != Type) {
				continue;
			}
			bool bMatch = Obj.Target.IsNone();
			for (FName T : Targets) {
				if (NamesMatch(Obj.Target, T)) {
					bMatch = true;
					break;
				}
			}
			if (!bMatch) {
				continue;
			}
			// re-find: the array may have been reallocated by a nested StartQuest
			P = FindProgress(QuestId);
			if (P && AdvanceObjectiveIndex(*Def, *P, i, Count)) {
				++Advanced;
			}
			if (!P || P->Status != EWotQuestStatus::Active) {
				break;
			}
		}
	}
	return Advanced;
}

TArray<FName> UWotQuestSubsystem::GetActorQuestNames(const AActor* Actor)
{
	TArray<FName> Names;
	if (!Actor) {
		return Names;
	}
	for (const FName& Tag : Actor->Tags) {
		FString TagStr = Tag.ToString();
		if (TagStr.StartsWith(TEXT("QuestName="))) {
			Names.Add(FName(*TagStr.RightChop(10)));
		}
	}
	// Blueprint "Name" variable on NPC_Base_BP (string, text or name)
	if (FProperty* Prop = Actor->GetClass()->FindPropertyByName(TEXT("Name"))) {
		const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Actor);
		FString Value;
		if (const FStrProperty* StrProp = CastField<FStrProperty>(Prop)) {
			Value = StrProp->GetPropertyValue(ValuePtr);
		} else if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop)) {
			Value = TextProp->GetPropertyValue(ValuePtr).ToString();
		} else if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop)) {
			Value = NameProp->GetPropertyValue(ValuePtr).ToString();
		}
		Value.TrimStartAndEndInline();
		if (!Value.IsEmpty()) {
			Names.Add(FName(*Value));
		}
	}
	Names.Add(StripBlueprintSuffix(Actor->GetClass()->GetName()));
	for (const FName& Tag : Actor->Tags) {
		Names.AddUnique(Tag);
	}
#if WITH_EDITOR
	Names.AddUnique(FName(*Actor->GetActorLabel()));
#endif
	return Names;
}

void UWotQuestSubsystem::NotifyTalkedTo(APawn* Player, AActor* NPC)
{
	if (!NPC) {
		return;
	}
	// tags can start quests
	for (const FName& Tag : NPC->Tags) {
		FString TagStr = Tag.ToString();
		if (TagStr.StartsWith(TEXT("QuestStart="))) {
			StartQuest(FName(*TagStr.RightChop(11)));
		}
	}
	TArray<FName> Names = GetActorQuestNames(NPC);
	const int32 Advanced = ReportProgressAny(EWotObjectiveType::TalkTo, Names, 1);
	UE_LOG(LogWotQuest, Verbose, TEXT("TalkedTo %s (%s): %d objectives advanced"), *GetNameSafe(NPC), Names.Num() ? *Names[0].ToString() : TEXT(""), Advanced);
}

void UWotQuestSubsystem::NotifyKilled(AActor* Killer, AActor* Victim)
{
	if (!Victim) {
		return;
	}
	ReportProgressAny(EWotObjectiveType::Kill, GetActorQuestNames(Victim), 1);
}

APawn* UWotQuestSubsystem::GetLocalPawn() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) {
		return nullptr;
	}
	APlayerController* PC = GI->GetFirstLocalPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

bool UWotQuestSubsystem::TickReach(float DeltaTime)
{
	APawn* Pawn = GetLocalPawn();
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World || !World->HasBegunPlay()) {
		return true;
	}
	const FName LevelName(*UGameplayStatics::GetCurrentLevelName(World, true));
	const FVector Loc = Pawn->GetActorLocation();
	TArray<FName> Active = GetActiveQuestIds();
	for (FName QuestId : Active) {
		const UWotQuestDefinition* Def = GetQuestDefinition(QuestId);
		FWotQuestProgress* P = FindProgress(QuestId);
		if (!Def || !P || P->Status != EWotQuestStatus::Active) {
			continue;
		}
		TArray<int32> Indices;
		CollectCurrentObjectiveIndices(*Def, *P, Indices);
		for (int32 i : Indices) {
			const FWotQuestObjective& Obj = Def->Objectives[i];
			if (Obj.Type != EWotObjectiveType::Reach) {
				continue;
			}
			if (!Obj.LevelName.IsNone() && !Obj.LevelName.IsEqual(LevelName, ENameCase::IgnoreCase)) {
				continue;
			}
			if (FVector::Dist2D(Loc, Obj.Location) <= Obj.Radius) {
				P = FindProgress(QuestId);
				if (P) {
					AdvanceObjectiveIndex(*Def, *P, i, Obj.RequiredCount);
				}
				break;
			}
		}
	}
	return true;
}

void UWotQuestSubsystem::ShowToast(const FText& Text) const
{
	if (AWotCharacter* Player = Cast<AWotCharacter>(GetLocalPawn())) {
		Player->ShowPopupWidget(Text, 3.0f, true);
	}
}

void UWotQuestSubsystem::EnsureTracker(AWotCharacter* Player)
{
	APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PC) {
		PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	}
	if (!PC) {
		return;
	}
	if (Tracker && Tracker->GetOwningPlayer() == PC && Tracker->IsInViewport()) {
		Tracker->Refresh();
		return;
	}
	if (Tracker) {
		Tracker->RemoveFromParent();
		Tracker = nullptr;
	}
	Tracker = CreateWidget<UWotQuestTrackerWidget>(PC, UWotQuestTrackerWidget::StaticClass());
	if (Tracker) {
		Tracker->AddToViewport(5);
		Tracker->Refresh();
	}
}

void UWotQuestSubsystem::OnPlayerBeginPlay(AWotCharacter* Player)
{
	AutoStartIfNeeded();
	EnsureTracker(Player);
}

FString UWotQuestSubsystem::DescribeState() const
{
	FString Out;
	TArray<FName> Ids = GetAllQuestIds();
	Ids.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	for (FName Id : Ids) {
		const UWotQuestDefinition* Def = GetQuestDefinition(Id);
		const FWotQuestProgress* P = FindProgress(Id);
		const TCHAR* Status = TEXT("inactive");
		if (P) {
			switch (P->Status) {
			case EWotQuestStatus::Active: Status = TEXT("ACTIVE"); break;
			case EWotQuestStatus::Completed: Status = TEXT("completed"); break;
			case EWotQuestStatus::Failed: Status = TEXT("failed"); break;
			default: break;
			}
		}
		Out += FString::Printf(TEXT("%s [%s] %s\n"), *Id.ToString(), Status, Def ? *Def->Title.ToString() : TEXT(""));
		if (Def && P) {
			for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
				const FWotQuestObjective& Obj = Def->Objectives[i];
				const int32 Count = P->ObjectiveCounts.IsValidIndex(i) ? P->ObjectiveCounts[i] : 0;
				Out += FString::Printf(TEXT("    %s %s (%d/%d)\n"), IsObjectiveDone(*Def, *P, i) ? TEXT("[x]") : TEXT("[ ]"), *Obj.Description.ToString(), Count, Obj.RequiredCount);
			}
		}
	}
	return Out;
}

// ---------------------------------------------------------------- console

static void WotQuestConsole(const TArray<FString>& Args, UWorld* World, void (*Fn)(UWotQuestSubsystem&, const TArray<FString>&))
{
	UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(World);
	if (!Quests) {
		UE_LOG(LogWotQuest, Warning, TEXT("No quest subsystem for this world"));
		return;
	}
	Fn(*Quests, Args);
}

static FAutoConsoleCommandWithWorldAndArgs GWotQuestList(
	TEXT("wot.Quest.List"), TEXT("List every quest and its progress"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		WotQuestConsole(Args, World, [](UWotQuestSubsystem& Q, const TArray<FString>&) {
			UE_LOG(LogWotQuest, Display, TEXT("Quests:\n%s"), *Q.DescribeState());
		});
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotQuestStart(
	TEXT("wot.Quest.Start"), TEXT("Start a quest: wot.Quest.Start <QuestId>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		WotQuestConsole(Args, World, [](UWotQuestSubsystem& Q, const TArray<FString>& A) {
			if (A.Num() > 0) {
				Q.StartQuest(FName(*A[0]));
			}
		});
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotQuestAdvance(
	TEXT("wot.Quest.Advance"), TEXT("Advance an objective: wot.Quest.Advance <QuestId> <ObjectiveId> [Count]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		WotQuestConsole(Args, World, [](UWotQuestSubsystem& Q, const TArray<FString>& A) {
			if (A.Num() > 1) {
				Q.AdvanceObjective(FName(*A[0]), FName(*A[1]), A.Num() > 2 ? FCString::Atoi(*A[2]) : 1);
			}
		});
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotQuestReset(
	TEXT("wot.Quest.Reset"), TEXT("Clear all quest progress and re-run auto-start"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		WotQuestConsole(Args, World, [](UWotQuestSubsystem& Q, const TArray<FString>&) {
			Q.ResetAllQuests();
		});
	}));
