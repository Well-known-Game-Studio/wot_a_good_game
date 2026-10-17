#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Save/WotSaveTypes.h"
#include "WotSaveSubsystem.generated.h"

class AWotCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWotOnSaveEvent, const FString&, SlotName);

/**
 * Whole-game save / load. Captures the local player's level, transform,
 * inventory and health, the quest subsystem's progress, story flags and the
 * time of day into a UWotGameSave slot; loading opens the saved level and
 * applies the state once the player pawn is possessed.
 *
 * Also keeps the "carry-over" state between levels in one session (portals):
 * the inventory and health captured on level transition are re-applied to
 * the new pawn, so the Blueprint-side inventory hand-off is no longer needed.
 */
UCLASS()
class VOXELRPG_API UWotSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UWotSaveSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FWotOnSaveEvent OnGameSaved;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FWotOnSaveEvent OnGameLoaded;

	/** Level opened by New Game. */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FString NewGameLevel = TEXT("/Game/Wot/Maps/EmondsField/EmondsField");

	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FString AutosaveSlot = TEXT("autosave");

	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FString QuickSlot = TEXT("quicksave");

	/** Autosave when the player leaves a level through a portal. */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	bool bAutosaveOnLevelTransition = true;

	// --- slots
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveToSlot(const FString& SlotName, bool bAutosave = false);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadFromSlot(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool QuickSave() { return SaveToSlot(QuickSlot); }

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool QuickLoad() { return LoadFromSlot(QuickSlot); }

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool DeleteSlot(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SlotExists(const FString& SlotName) const;

	/** Headers of every slot on disk, newest first. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FWotSaveHeader> ListSlots() const;

	/** Most recently written slot, if any. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool GetLatestSlot(FWotSaveHeader& OutHeader) const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	void NewGame();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void QuitToMainMenu();

	// --- story flags
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SetStoryFlag(FName Flag, bool bValue = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save")
	bool HasStoryFlag(FName Flag) const { return StoryFlags.Contains(Flag); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save")
	float GetPlaytimeSeconds() const { return PlaytimeSeconds; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save")
	bool IsLoadPending() const { return PendingLoad != nullptr; }

	/** Called by AWotCharacter::PossessedBy for the local player. */
	void OnPlayerReady(AWotCharacter* Player);

	/** Called by the game mode when the level is left through a portal. */
	void OnLevelTransition(UWorld* World);

	/** Captures the current state into a save object (without writing it). */
	UWotGameSave* CaptureState(bool bAutosave) const;

	/** Applies everything except the level change from a save object to the live pawn. */
	void ApplyState(const UWotGameSave* Save, AWotCharacter* Player);

	FString DescribeSlots() const;

protected:
	UPROPERTY()
	TObjectPtr<UWotGameSave> PendingLoad;

	/** Inventory / health carried across a portal within one session. */
	UPROPERTY()
	TObjectPtr<UWotGameSave> CarryOver;

	UPROPERTY()
	TArray<FName> StoryFlags;

	float PlaytimeSeconds = 0.0f;
	double LastTickTime = 0.0;
	FTSTicker::FDelegateHandle TickHandle;

	bool TickPlaytime(float DeltaTime);
	AWotCharacter* GetLocalPlayer() const;
	void CapturePlayer(UWotGameSave* Save, const AWotCharacter* Player) const;
	void ApplyInventory(const UWotGameSave* Save, AWotCharacter* Player) const;
};
