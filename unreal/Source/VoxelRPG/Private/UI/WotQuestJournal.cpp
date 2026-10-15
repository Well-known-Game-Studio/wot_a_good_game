#include "UI/WotQuestJournal.h"
#include "Quests/WotQuestSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

UWotQuestJournal::UWotQuestJournal()
{
	TitleText = NSLOCTEXT("Wot", "Journal", "Journal");
	PanelWidth = 640.0f;
	bPausesGame = true;
}

void UWotQuestJournal::Toggle(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World) {
		return;
	}
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UWotQuestJournal::StaticClass(), false);
	if (Found.ContainsByPredicate([](UUserWidget* W) { UWotMenuWidget* M = Cast<UWotMenuWidget>(W); return M && M->IsOpen(); })) {
		CloseAllMenus(WorldContextObject);
		return;
	}
	CloseAllMenus(WorldContextObject);
	OpenMenu(WorldContextObject, UWotQuestJournal::StaticClass());
}

void UWotQuestJournal::AddQuest(FName QuestId, bool bActive)
{
	UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(this);
	const UWotQuestDefinition* Def = Quests ? Quests->GetQuestDefinition(QuestId) : nullptr;
	FWotQuestProgress P;
	if (!Def || !Quests->GetQuestProgress(QuestId, P)) {
		return;
	}
	const FLinearColor TitleColor = bActive ? WotUI::Title : WotUI::Muted;
	const FLinearColor TextColor = bActive ? WotUI::Text : WotUI::Muted;
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrush(WotUI::RoundedBrush(WotUI::ButtonNormal * FLinearColor(1, 1, 1, bActive ? 1.0f : 0.6f), 6.0f, WotUI::PanelLine * FLinearColor(1, 1, 1, bActive ? 0.8f : 0.35f), 1.0f));
	Card->SetPadding(FMargin(14.0f, 10.0f));
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(Column);
	// title row with status
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Title = MakeText(WidgetTree, Def->Title, 18, TitleColor, true);
	Title->SetAutoWrapText(true);
	if (UHorizontalBoxSlot* S = Head->AddChildToHorizontalBox(Title)) {
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UTextBlock* Status = MakeText(WidgetTree, bActive ? NSLOCTEXT("Wot", "QuestActive", "in progress") : NSLOCTEXT("Wot", "QuestDone", "completed"), 11, WotUI::Muted);
	if (UHorizontalBoxSlot* S = Head->AddChildToHorizontalBox(Status)) {
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(8.0f, 0, 0, 0));
	}
	Column->AddChildToVerticalBox(Head);
	if (!Def->Description.IsEmpty()) {
		UTextBlock* Desc = MakeText(WidgetTree, Def->Description, 13, WotUI::Muted);
		Desc->SetAutoWrapText(true);
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Desc)) {
			S->SetPadding(FMargin(0, 4.0f, 0, 6.0f));
		}
	}
	TArray<FWotQuestObjective> Current = Quests->GetCurrentObjectives(QuestId);
	for (int32 i = 0; i < Def->Objectives.Num(); ++i) {
		const FWotQuestObjective& Obj = Def->Objectives[i];
		const int32 Count = P.ObjectiveCounts.IsValidIndex(i) ? P.ObjectiveCounts[i] : 0;
		const bool bDone = Count >= Obj.RequiredCount;
		const bool bCurrent = Current.ContainsByPredicate([&Obj](const FWotQuestObjective& O) { return O.ObjectiveId == Obj.ObjectiveId; });
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const TCHAR* Glyph = bDone ? TEXT("✓") : (bCurrent ? TEXT("●") : TEXT("·"));
		UTextBlock* Mark = MakeText(WidgetTree, FText::FromString(Glyph), 13, bDone ? WotUI::Title : (bCurrent ? WotUI::Text : WotUI::Muted), true);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Mark)) {
			S->SetPadding(FMargin(4.0f, 0, 10.0f, 0));
		}
		FString Line = (bDone || bCurrent) ? Obj.Description.ToString() : TEXT("...");
		if (Obj.RequiredCount > 1 && (bDone || bCurrent)) {
			Line += FString::Printf(TEXT("   %d / %d"), Count, Obj.RequiredCount);
		}
		if (Obj.bOptional && (bDone || bCurrent)) {
			Line += TEXT("  (optional)");
		}
		UTextBlock* Text = MakeText(WidgetTree, FText::FromString(Line), 13, bDone ? WotUI::Muted : TextColor);
		Text->SetAutoWrapText(true);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Text)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Row)) {
			S->SetPadding(FMargin(0, 2.0f));
		}
	}
	if (!bActive && !Def->CompletionText.IsEmpty()) {
		UTextBlock* Done = MakeText(WidgetTree, Def->CompletionText, 12, WotUI::Muted);
		Done->SetAutoWrapText(true);
		FSlateFontInfo Font = Done->GetFont();
		Font.TypefaceFontName = FName("Italic");
		Done->SetFont(Font);
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Done)) {
			S->SetPadding(FMargin(0, 6.0f, 0, 0));
		}
	}
	if (UVerticalBoxSlot* S = Content->AddChildToVerticalBox(Card)) {
		S->SetPadding(FMargin(0, 4.0f));
	}
}

void UWotQuestJournal::BuildContent()
{
	UWotQuestSubsystem* Quests = UWotQuestSubsystem::Get(this);
	if (!Quests) {
		AddLabel(NSLOCTEXT("Wot", "NoQuests", "No quests yet."), 14, WotUI::Muted);
		if (UButton* B = AddButton(NSLOCTEXT("Wot", "Close", "Close"))) B->OnClicked.AddDynamic(this, &UWotQuestJournal::OnClose);
		return;
	}
	TArray<FName> Active = Quests->GetActiveQuestIds();
	TArray<FName> Completed;
	for (FName Id : Quests->GetAllQuestIds()) {
		if (Quests->IsQuestCompleted(Id)) {
			Completed.Add(Id);
		}
	}
	// the list scrolls when it grows past the panel
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JournalScroll"));
	USizeBox* ScrollSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JournalScrollSize"));
	ScrollSize->SetMaxDesiredHeight(560.0f);
	ScrollSize->AddChild(Scroll);
	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JournalList"));
	Scroll->AddChild(List);
	UVerticalBox* Outer = Content;
	Content = List;   // AddQuest/AddLabel append into the list
	if (Active.Num() == 0 && Completed.Num() == 0) {
		AddLabel(NSLOCTEXT("Wot", "NoQuests", "No quests yet."), 14, WotUI::Muted);
	}
	if (Active.Num() > 0) {
		AddLabel(NSLOCTEXT("Wot", "ActiveQuests", "ACTIVE"), 11, WotUI::Title);
		for (FName Id : Active) {
			AddQuest(Id, true);
		}
	}
	if (Completed.Num() > 0) {
		AddLabel(NSLOCTEXT("Wot", "CompletedQuests", "COMPLETED"), 11, WotUI::Title);
		for (FName Id : Completed) {
			AddQuest(Id, false);
		}
	}
	Content = Outer;
	if (UVerticalBoxSlot* S = Content->AddChildToVerticalBox(ScrollSize)) {
		S->SetPadding(FMargin(0, 0, 0, 10.0f));
	}
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "Close", "Close"))) B->OnClicked.AddDynamic(this, &UWotQuestJournal::OnClose);
}

void UWotQuestJournal::OnClose()
{
	Close();
}
