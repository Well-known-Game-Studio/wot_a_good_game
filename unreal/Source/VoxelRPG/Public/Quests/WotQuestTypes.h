#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/SaveGame.h"
#include "WotQuestTypes.generated.h"

UENUM(BlueprintType)
enum class EWotObjectiveType : uint8
{
	/** Talk to an NPC whose Name (or class name) matches Target. */
	TalkTo,
	/** Reach Location (within Radius) in level LevelName (any level when empty). */
	Reach,
	/** Kill RequiredCount actors whose Name / class name / tag matches Target. */
	Kill,
	/** Collect RequiredCount items; reported from Blueprint via ReportProgress(Collect, Target). */
	Collect,
	/** Anything else; advanced from Blueprint / C++ via AdvanceObjective or ReportProgress(Custom, Target). */
	Custom
};

UENUM(BlueprintType)
enum class EWotQuestStatus : uint8
{
	Inactive,
	Active,
	Completed,
	Failed
};

USTRUCT(BlueprintType)
struct VOXELRPG_API FWotQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName ObjectiveId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	EWotObjectiveType Type = EWotObjectiveType::Custom;

	/** NPC name, enemy class/tag, item name, or custom key. Empty matches any target of this type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = 1))
	int32 RequiredCount = 1;

	/** Level (short map name, e.g. "EmondsField") the objective applies to; empty = any. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FName LevelName;

	/** Reach objectives: world location to reach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	FVector Location = FVector::ZeroVector;

	/** Reach objectives: horizontal radius around Location. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	float Radius = 400.0f;

	/** Optional objectives are not required to complete the quest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
	bool bOptional = false;
};

/**
 * One quest. Author these as data assets under /Game/Wot/Quests; the quest
 * subsystem loads every UWotQuestDefinition it finds there.
 */
UCLASS(BlueprintType)
class VOXELRPG_API UWotQuestDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FWotQuestObjective> Objectives;

	/** When true only the first incomplete objective is active; otherwise all are. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bObjectivesInOrder = true;

	/** Started automatically when the quest system first runs (new game). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bAutoStart = false;

	/** Quest started automatically when this one completes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName NextQuestId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = true))
	FText CompletionText;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("WotQuest"), QuestId.IsNone() ? GetFName() : QuestId);
	}
};

USTRUCT(BlueprintType)
struct VOXELRPG_API FWotQuestProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	EWotQuestStatus Status = EWotQuestStatus::Inactive;

	/** Progress count per objective, same order as the definition. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Quest")
	TArray<int32> ObjectiveCounts;
};

UCLASS()
class VOXELRPG_API UWotQuestSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	TArray<FWotQuestProgress> Quests;

	UPROPERTY(SaveGame)
	bool bAutoStartDone = false;
};
