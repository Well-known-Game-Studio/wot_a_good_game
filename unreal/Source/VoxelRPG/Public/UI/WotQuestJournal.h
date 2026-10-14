#pragma once

#include "CoreMinimal.h"
#include "UI/WotMenuWidget.h"
#include "WotQuestJournal.generated.h"

/**
 * The journal: every quest the player has taken on, with its description and all of its
 * objectives (done, current and still hidden ones shown as "..."), active quests first
 * and completed ones below in muted text. Opened with J / gamepad Back or from the pause menu.
 */
UCLASS()
class VOXELRPG_API UWotQuestJournal : public UWotMenuWidget
{
	GENERATED_BODY()

public:
	UWotQuestJournal();

	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "WorldContextObject"))
	static void Toggle(UObject* WorldContextObject);

protected:
	virtual void BuildContent() override;
	void AddQuest(FName QuestId, bool bActive);

	UFUNCTION() void OnClose();
};
