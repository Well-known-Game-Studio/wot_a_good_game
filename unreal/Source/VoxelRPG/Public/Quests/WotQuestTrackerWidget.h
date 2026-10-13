#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WotQuestTrackerWidget.generated.h"

class UVerticalBox;
class UTextBlock;

/**
 * Minimal on-screen quest tracker built entirely in C++ (no UMG asset needed).
 * Shows every active quest with its current objectives in the top-right corner.
 * Subclass in Blueprint to restyle, or replace by assigning a different class
 * in UWotQuestSubsystem::EnsureTracker.
 */
UCLASS()
class VOXELRPG_API UWotQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void Refresh();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FLinearColor TitleColor = FLinearColor(1.0f, 0.85f, 0.4f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FLinearColor ObjectiveColor = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FLinearColor DoneColor = FLinearColor(0.55f, 0.75f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 TitleFontSize = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 ObjectiveFontSize = 14;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Box;

	UTextBlock* AddLine(const FText& Text, int32 FontSize, const FLinearColor& Color, float LeftPad);

	UFUNCTION()
	void HandleQuestEvent(FName QuestId);

	UFUNCTION()
	void HandleObjectiveEvent(FName QuestId, FName ObjectiveId, int32 Count, int32 Required);
};
