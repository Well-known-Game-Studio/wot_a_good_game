#pragma once

#include "CoreMinimal.h"
#include "UI/WotMenuWidget.h"
#include "Save/WotSaveTypes.h"
#include "WotGameMenus.generated.h"

class UButton;

/** Esc / IA_Menu: Resume, Save, Load, Main Menu, Quit. */
UCLASS()
class VOXELRPG_API UWotPauseMenu : public UWotMenuWidget
{
	GENERATED_BODY()

public:
	UWotPauseMenu(const FObjectInitializer& ObjectInitializer);

	/** Opens the pause menu for the local player, or closes it when it is already open. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "WorldContextObject"))
	static void Toggle(UObject* WorldContextObject);

protected:
	virtual void BuildContent() override;

	UFUNCTION() void OnResume();
	UFUNCTION() void OnJournal();
	UFUNCTION() void OnSave();
	UFUNCTION() void OnLoad();
	UFUNCTION() void OnMainMenu();
	UFUNCTION() void OnQuit();
};

/** Row of the save/load list; the button that loads or overwrites the slot. */
UCLASS()
class VOXELRPG_API UWotSaveSlotRow : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() FString SlotName;
	UPROPERTY() TObjectPtr<class UWotSaveLoadMenu> Menu;
	UFUNCTION() void OnPrimary();
	UFUNCTION() void OnDelete();
};

/** Save or load list with timestamps, level and current quest per slot. */
UCLASS()
class VOXELRPG_API UWotSaveLoadMenu : public UWotMenuWidget
{
	GENERATED_BODY()

public:
	UWotSaveLoadMenu(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadWrite, Category = "Menu")
	bool bSaveMode = false;

	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "WorldContextObject"))
	static UWotSaveLoadMenu* OpenSaveLoad(UObject* WorldContextObject, bool bSave);

	void SlotPrimary(const FString& SlotName);
	void SlotDelete(const FString& SlotName);
	void Rebuild();

protected:
	virtual void BuildContent() override;

	UFUNCTION() void OnNewSave();
	UFUNCTION() void OnBack();

	UPROPERTY()
	TArray<TObjectPtr<UWotSaveSlotRow>> Rows;

	UPROPERTY()
	TObjectPtr<class UTextBlock> StatusBlock;
};

/** Title screen: New Game, Continue, Load, Quit. */
UCLASS()
class VOXELRPG_API UWotMainMenu : public UWotMenuWidget
{
	GENERATED_BODY()

public:
	UWotMainMenu(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildContent() override;

	UFUNCTION() void OnNewGame();
	UFUNCTION() void OnContinue();
	UFUNCTION() void OnLoad();
	UFUNCTION() void OnQuit();
};
