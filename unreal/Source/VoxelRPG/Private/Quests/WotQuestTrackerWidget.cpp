#include "Quests/WotQuestTrackerWidget.h"
#include "Quests/WotQuestSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UWotQuestTrackerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) {
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TrackerRoot"));
		WidgetTree->RootWidget = Canvas;
		Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TrackerBox"));
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Box)) {
			CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
			CanvasSlot->SetPosition(FVector2D(-24.0f, 24.0f));
			CanvasSlot->SetAutoSize(true);
		}
	}
	return Super::RebuildWidget();
}

void UWotQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(this)) {
		Quests->OnQuestStarted.AddUniqueDynamic(this, &UWotQuestTrackerWidget::HandleQuestEvent);
		Quests->OnQuestCompleted.AddUniqueDynamic(this, &UWotQuestTrackerWidget::HandleQuestEvent);
		Quests->OnObjectiveUpdated.AddUniqueDynamic(this, &UWotQuestTrackerWidget::HandleObjectiveEvent);
	}
	Refresh();
}

void UWotQuestTrackerWidget::NativeDestruct()
{
	if (UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(this)) {
		Quests->OnQuestStarted.RemoveDynamic(this, &UWotQuestTrackerWidget::HandleQuestEvent);
		Quests->OnQuestCompleted.RemoveDynamic(this, &UWotQuestTrackerWidget::HandleQuestEvent);
		Quests->OnObjectiveUpdated.RemoveDynamic(this, &UWotQuestTrackerWidget::HandleObjectiveEvent);
	}
	Super::NativeDestruct();
}

void UWotQuestTrackerWidget::HandleQuestEvent(FName QuestId)
{
	Refresh();
}

void UWotQuestTrackerWidget::HandleObjectiveEvent(FName QuestId, FName ObjectiveId, int32 Count, int32 Required)
{
	Refresh();
}

UTextBlock* UWotQuestTrackerWidget::AddLine(const FText& Text, int32 FontSize, const FLinearColor& Color, float LeftPad)
{
	if (!Box || !WidgetTree) {
		return nullptr;
	}
	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(Text);
	Line->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font = Line->GetFont();
	Font.Size = FontSize;
	Line->SetFont(Font);
	Line->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Line->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	Line->SetJustification(ETextJustify::Right);
	if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Line)) {
		BoxSlot->SetPadding(FMargin(LeftPad, 1.0f, 0.0f, 1.0f));
		BoxSlot->SetHorizontalAlignment(HAlign_Right);
	}
	return Line;
}

void UWotQuestTrackerWidget::Refresh()
{
	if (!Box) {
		return;
	}
	Box->ClearChildren();
	UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(this);
	if (!Quests) {
		return;
	}
	for (FName QuestId : Quests->GetActiveQuestIds()) {
		const UWotQuestDefinition* Def = Quests->GetQuestDefinition(QuestId);
		FWotQuestProgress P;
		if (!Def || !Quests->GetQuestProgress(QuestId, P)) {
			continue;
		}
		AddLine(Def->Title, TitleFontSize, TitleColor, 0.0f);
		TArray<FWotQuestObjective> Current = Quests->GetCurrentObjectives(QuestId);
		for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
			const FWotQuestObjective& Obj = Def->Objectives[i];
			const int32 Count = P.ObjectiveCounts.IsValidIndex(i) ? P.ObjectiveCounts[i] : 0;
			const bool bDone = Count >= Obj.RequiredCount;
			const bool bCurrent = Current.ContainsByPredicate([&Obj](const FWotQuestObjective& O) { return O.ObjectiveId == Obj.ObjectiveId; });
			if (!bDone && !bCurrent) {
				continue; // not yet revealed
			}
			FString Line = bDone ? TEXT("[x] ") : TEXT("[ ] ");
			Line += Obj.Description.ToString();
			if (Obj.RequiredCount > 1) {
				Line += FString::Printf(TEXT(" (%d/%d)"), Count, Obj.RequiredCount);
			}
			AddLine(FText::FromString(Line), ObjectiveFontSize, bDone ? DoneColor : ObjectiveColor, 12.0f);
		}
	}
}
