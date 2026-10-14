#include "Quests/WotQuestTrackerWidget.h"
#include "Quests/WotQuestSubsystem.h"
#include "UI/WotMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UWotQuestTrackerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) {
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TrackerRoot"));
		WidgetTree->RootWidget = Canvas;
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TrackerSize"));
		Size->SetWidthOverride(PanelWidth);
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Size)) {
			CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
			CanvasSlot->SetPosition(FVector2D(-24.0f, 24.0f));
			CanvasSlot->SetAutoSize(true);
		}
		Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TrackerPanel"));
		Panel->SetBrush(WotUI::RoundedBrush(WotUI::Panel * FLinearColor(1, 1, 1, 0.85f), 8.0f, WotUI::PanelLine, 1.5f));
		Panel->SetPadding(FMargin(14.0f, 10.0f, 14.0f, 12.0f));
		Size->AddChild(Panel);
		Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TrackerBox"));
		Panel->SetContent(Box);
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

void UWotQuestTrackerWidget::SetTrackerHidden(bool bHidden)
{
	bHiddenByUser = bHidden;
	Refresh();
}

void UWotQuestTrackerWidget::AddHeader()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Title = UWotMenuWidget::MakeText(WidgetTree, NSLOCTEXT("Wot", "TrackerHeader", "QUESTS"), 11, WotUI::Title, true);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Title)) {
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}
	UTextBlock* Hint = UWotMenuWidget::MakeText(WidgetTree, NSLOCTEXT("Wot", "TrackerHint", "J  journal"), 10, WotUI::Muted);
	Hint->SetJustification(ETextJustify::Right);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Hint)) {
		S->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row)) {
		S->SetPadding(FMargin(0, 0, 0, 4.0f));
	}
	// the thin gold rule under the header
	UBorder* Rule = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Rule->SetBrushColor(WotUI::PanelLine * FLinearColor(1, 1, 1, 0.7f));
	Rule->SetPadding(FMargin(0));
	USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RuleSize->SetHeightOverride(1.0f);
	RuleSize->AddChild(Rule);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(RuleSize)) {
		S->SetPadding(FMargin(0, 0, 0, 6.0f));
		S->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UWotQuestTrackerWidget::AddQuestTitle(const FText& Title)
{
	UTextBlock* Block = UWotMenuWidget::MakeText(WidgetTree, Title, TitleFontSize, WotUI::Text, true);
	Block->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Block)) {
		S->SetPadding(FMargin(0, 6.0f, 0, 3.0f));
	}
}

void UWotQuestTrackerWidget::AddObjectiveRow(const FText& Description, int32 Count, int32 Required, bool bDone, bool bOptional)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// check box: an outlined square, filled gold with a tick when done
	USizeBox* BoxSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BoxSize->SetWidthOverride(14.0f);
	BoxSize->SetHeightOverride(14.0f);
	UBorder* Check = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Check->SetBrush(WotUI::RoundedBrush(bDone ? WotUI::Title : WotUI::ButtonNormal, 3.0f, bDone ? WotUI::Title : WotUI::PanelLine, 1.0f));
	Check->SetPadding(FMargin(0));
	Check->SetHorizontalAlignment(HAlign_Center);
	Check->SetVerticalAlignment(VAlign_Center);
	if (bDone) {
		UTextBlock* Tick = UWotMenuWidget::MakeText(WidgetTree, FText::FromString(TEXT("✓")), 10, WotUI::Panel, true);
		Tick->SetShadowOffset(FVector2D::ZeroVector);
		Check->SetContent(Tick);
	}
	BoxSize->AddChild(Check);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(BoxSize)) {
		S->SetPadding(FMargin(4.0f, 3.0f, 8.0f, 0));
		S->SetVerticalAlignment(VAlign_Top);
	}
	// text column: description (+ progress bar for counted objectives)
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	FText Label = Description;
	if (bOptional) {
		Label = FText::Format(NSLOCTEXT("Wot", "OptionalObjective", "{0} (optional)"), Description);
	}
	UTextBlock* Text = UWotMenuWidget::MakeText(WidgetTree, Label, ObjectiveFontSize, bDone ? WotUI::Muted : WotUI::Text);
	Text->SetAutoWrapText(true);
	if (bDone) {
		FSlateFontInfo Font = Text->GetFont();
		Font.TypefaceFontName = FName("Italic");
		Text->SetFont(Font);
	}
	Column->AddChildToVerticalBox(Text);
	if (Required > 1) {
		UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UProgressBar* Progress = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		Progress->SetPercent(FMath::Clamp(float(Count) / float(Required), 0.0f, 1.0f));
		Progress->SetFillColorAndOpacity(bDone ? WotUI::Title : WotUI::PanelLine);
		FProgressBarStyle Style = Progress->GetWidgetStyle();
		Style.BackgroundImage = WotUI::RoundedBrush(WotUI::ButtonNormal, 2.0f, WotUI::PanelLine * FLinearColor(1, 1, 1, 0.4f), 1.0f);
		Style.FillImage = WotUI::RoundedBrush(FLinearColor::White, 2.0f, FLinearColor::Transparent, 0.0f);
		Progress->SetWidgetStyle(Style);
		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarSize->SetHeightOverride(5.0f);
		BarSize->AddChild(Progress);
		if (UHorizontalBoxSlot* S = Bar->AddChildToHorizontalBox(BarSize)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetVerticalAlignment(VAlign_Center);
		}
		UTextBlock* CountText = UWotMenuWidget::MakeText(WidgetTree, FText::FromString(FString::Printf(TEXT("%d / %d"), Count, Required)), 10, WotUI::Muted);
		if (UHorizontalBoxSlot* S = Bar->AddChildToHorizontalBox(CountText)) {
			S->SetPadding(FMargin(8.0f, 0, 0, 0));
			S->SetVerticalAlignment(VAlign_Center);
		}
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Bar)) {
			S->SetPadding(FMargin(0, 3.0f, 0, 0));
		}
	}
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Column)) {
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Row)) {
		S->SetPadding(FMargin(0, 2.0f, 0, 2.0f));
	}
}

void UWotQuestTrackerWidget::Refresh()
{
	if (!Box || !WidgetTree) {
		return;
	}
	Box->ClearChildren();
	UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(this);
	TArray<FName> Active = Quests ? Quests->GetActiveQuestIds() : TArray<FName>();
	if (!Quests || Active.Num() == 0 || bHiddenByUser) {
		if (Panel) {
			Panel->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	if (Panel) {
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	AddHeader();
	for (FName QuestId : Active) {
		const UWotQuestDefinition* Def = Quests->GetQuestDefinition(QuestId);
		FWotQuestProgress P;
		if (!Def || !Quests->GetQuestProgress(QuestId, P)) {
			continue;
		}
		AddQuestTitle(Def->Title);
		TArray<FWotQuestObjective> Current = Quests->GetCurrentObjectives(QuestId);
		for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
			const FWotQuestObjective& Obj = Def->Objectives[i];
			const int32 Count = P.ObjectiveCounts.IsValidIndex(i) ? P.ObjectiveCounts[i] : 0;
			const bool bDone = Count >= Obj.RequiredCount;
			const bool bCurrent = Current.ContainsByPredicate([&Obj](const FWotQuestObjective& O) { return O.ObjectiveId == Obj.ObjectiveId; });
			if (!bDone && !bCurrent) {
				continue; // not yet revealed
			}
			AddObjectiveRow(Obj.Description, Count, Obj.RequiredCount, bDone, Obj.bOptional);
		}
	}
}
