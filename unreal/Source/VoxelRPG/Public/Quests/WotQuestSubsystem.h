#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Quests/WotQuestTypes.h"
#include "WotQuestSubsystem.generated.h"

class AWotCharacter;
class UWotQuestTrackerWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWotOnQuestEvent, FName, QuestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FWotOnObjectiveEvent, FName, QuestId, FName, ObjectiveId, int32, Count, int32, Required);

/**
 * Quest / story state for the whole game session. Lives on the GameInstance
 * so it survives level transitions through portals; progress is also written
 * to the "WotQuests" save slot.
 *
 * Progress arrives through ReportProgress (generic), NotifyTalkedTo (called by
 * the player character after a conversation), NotifyKilled (game mode) and
 * the periodic Reach check against the local player's location.
 *
 * NPC actor tags understood: "QuestStart=<QuestId>" starts that quest when the
 * NPC is talked to; "QuestName=<Name>" overrides the name used for TalkTo /
 * Kill matching.
 */
UCLASS()
class VOXELRPG_API UWotQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UWotQuestSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FWotOnQuestEvent OnQuestStarted;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FWotOnQuestEvent OnQuestCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FWotOnObjectiveEvent OnObjectiveUpdated;

	/** Content folder scanned for UWotQuestDefinition assets. */
	UPROPERTY(EditDefaultsOnly, Category = "Quest")
	FString QuestContentPath = TEXT("/Game/Wot/Quests");

	UPROPERTY(EditDefaultsOnly, Category = "Quest")
	FString SaveSlotName = TEXT("WotQuests");

	// --- queries
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	const UWotQuestDefinition* GetQuestDefinition(FName QuestId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	EWotQuestStatus GetQuestStatus(FName QuestId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsQuestActive(FName QuestId) const { return GetQuestStatus(QuestId) == EWotQuestStatus::Active; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsQuestCompleted(FName QuestId) const { return GetQuestStatus(QuestId) == EWotQuestStatus::Completed; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TArray<FName> GetActiveQuestIds() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TArray<FName> GetAllQuestIds() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool GetQuestProgress(FName QuestId, FWotQuestProgress& OutProgress) const;

	/** Count for one objective (0 when unknown). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	int32 GetObjectiveCount(FName QuestId, FName ObjectiveId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsObjectiveComplete(FName QuestId, FName ObjectiveId) const;

	/** Objectives the player can currently make progress on. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	TArray<FWotQuestObjective> GetCurrentObjectives(FName QuestId) const;

	// --- mutations
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool StartQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CompleteQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool FailQuest(FName QuestId);

	/** Add Count to one objective of one quest. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AdvanceObjective(FName QuestId, FName ObjectiveId, int32 Count = 1);

	/** Add Count to every current objective of the given type whose Target matches (or is empty). Returns the number of objectives advanced. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	int32 ReportProgress(EWotObjectiveType Type, FName Target, int32 Count = 1);

	/** Same as ReportProgress but tries each candidate name (first match per objective wins). */
	int32 ReportProgressAny(EWotObjectiveType Type, const TArray<FName>& Targets, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void NotifyTalkedTo(APawn* Player, AActor* NPC);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void NotifyKilled(AActor* Killer, AActor* Victim);

	/** Called from AWotCharacter::BeginPlay for the local player; shows the tracker and auto-starts quests. */
	void OnPlayerBeginPlay(AWotCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ResetAllQuests();

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SaveProgress();

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ReloadDefinitions();

	/** Names used to match an actor against TalkTo / Kill targets: QuestName tag, Name property, class name, label. */
	static TArray<FName> GetActorQuestNames(const AActor* Actor);

	/** One-line text summary of every quest (used by the wot.Quest.List console command). */
	FString DescribeState() const;

protected:
	UPROPERTY()
	TMap<FName, TObjectPtr<const UWotQuestDefinition>> Definitions;

	UPROPERTY()
	TArray<FWotQuestProgress> Progress;

	UPROPERTY()
	TObjectPtr<UWotQuestTrackerWidget> Tracker;

	bool bAutoStartDone = false;
	FTSTicker::FDelegateHandle TickHandle;

	FWotQuestProgress* FindProgress(FName QuestId);
	const FWotQuestProgress* FindProgress(FName QuestId) const;
	bool IsObjectiveDone(const UWotQuestDefinition& Def, const FWotQuestProgress& P, int32 Index) const;
	bool AreRequiredObjectivesDone(const UWotQuestDefinition& Def, const FWotQuestProgress& P) const;
	void CollectCurrentObjectiveIndices(const UWotQuestDefinition& Def, const FWotQuestProgress& P, TArray<int32>& OutIndices) const;
	bool AdvanceObjectiveIndex(const UWotQuestDefinition& Def, FWotQuestProgress& P, int32 Index, int32 Count);
	void LoadProgress();
	void LoadDefinitions();
	void AutoStartIfNeeded();
	bool TickReach(float DeltaTime);
	void ShowToast(const FText& Text) const;
	APawn* GetLocalPawn() const;
	void EnsureTracker(AWotCharacter* Player);
};
