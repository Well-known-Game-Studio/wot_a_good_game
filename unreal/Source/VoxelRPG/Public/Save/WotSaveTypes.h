#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Quests/WotQuestTypes.h"
#include "WotSaveTypes.generated.h"

USTRUCT(BlueprintType)
struct VOXELRPG_API FWotSavedItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FSoftClassPath ItemClass;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	int32 Count = 0;
};

/** What a save slot shows in the load menu before the whole file is used. */
USTRUCT(BlueprintType)
struct VOXELRPG_API FWotSaveHeader
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FString SlotName;

	/** Short map name, e.g. EmondsField. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FString LevelName;

	/** Long package path used to open the level. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FString LevelPath;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FDateTime SavedAt;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	float PlaytimeSeconds = 0.0f;

	/** Title of the most recent active quest, for the slot list. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FText CurrentQuest;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	bool bAutosave = false;
};

/** One complete game state. Slot files live in Saved/SaveGames/<slot>.sav. */
UCLASS(BlueprintType)
class VOXELRPG_API UWotGameSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FWotSaveHeader Header;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	bool bHasPlayerTransform = false;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FName LastPortalName;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	float TimeOfDaySeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	float Health = -1.0f;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	TArray<FWotSavedItem> Inventory;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	TArray<FWotQuestProgress> Quests;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	bool bQuestAutoStartDone = false;

	/** Free-form story flags set from Blueprint / quests (e.g. "SheepHerded"). */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	TArray<FName> StoryFlags;
};
