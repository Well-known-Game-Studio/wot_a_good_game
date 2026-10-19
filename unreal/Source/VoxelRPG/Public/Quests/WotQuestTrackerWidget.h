#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WotQuestTrackerWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UBorder;

/**
 * On-screen quest tracker built in C++ in the same parchment-and-gold style as the menus
 * (WotUI palette): a small panel in the top-right corner listing every active quest with
 * its revealed objectives, a check box per objective and a progress bar for counted ones.
 * Press J (gamepad Back) for the full journal (UWotQuestJournal).
 */
UCLASS()
class VOXELRPG_API UWotQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void Refresh();

	/** Width of the tracker panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	float PanelWidth = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 TitleFontSize = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 ObjectiveFontSize = 13;

	/** Hide the tracker entirely (the journal still works). */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SetTrackerHidden(bool bHidden);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY()
	TObjectPtr<UBorder> Panel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Box;

	void AddQuestTitle(const FText& Title);
	void AddObjectiveRow(const FText& Description, int32 Count, int32 Required, bool bDone, bool bOptional);
	void AddHeader();

	UFUNCTION()
	void HandleQuestEvent(FName QuestId);

	UFUNCTION()
	void HandleObjectiveEvent(FName QuestId, FName ObjectiveId, int32 Count, int32 Required);

	bool bHiddenByUser = false;
};
