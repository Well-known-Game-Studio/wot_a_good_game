#include "UI/WotGameMenus.h"
#include "UI/WotQuestJournal.h"
#include "Save/WotSaveSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "WotCharacter.h"
#include "UI/WotUWInventoryPanel.h"

// ---------------------------------------------------------------- pause menu

UWotPauseMenu::UWotPauseMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TitleText = NSLOCTEXT("Wot", "Paused", "Paused");
	PanelWidth = 420.0f;
}

void UWotPauseMenu::BuildContent()
{
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "Resume", "Resume"))) B->OnClicked.AddDynamic(this, &UWotPauseMenu::OnResume);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "JournalButton", "Journal"))) B->OnClicked.AddDynamic(this, &UWotPauseMenu::OnJournal);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "SaveGame", "Save Game"))) B->OnClicked.AddDynamic(this, &UWotPauseMenu::OnSave);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "LoadGame", "Load Game"))) B->OnClicked.AddDynamic(this, &UWotPauseMenu::OnLoad);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "MainMenu", "Main Menu"))) B->OnClicked.AddDynamic(this, &UWotPauseMenu::OnMainMenu);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "QuitDesktop", "Quit to Desktop"), WotUI::Danger)) B->OnClicked.AddDynamic(this, &UWotPauseMenu::OnQuit);
	if (UWotSaveSubsystem* S = UWotSaveSubsystem::Get(this)) {
		const int32 Secs = int32(S->GetPlaytimeSeconds());
		AddLabel(FText::Format(NSLOCTEXT("Wot", "Playtime", "Played {0}h {1}m"), Secs / 3600, (Secs / 60) % 60), 13, WotUI::Muted);
	}
}

void UWotPauseMenu::OnJournal()
{
	UObject* Ctx = GetWorld();
	Close();
	UWotQuestJournal::Toggle(Ctx);
}

void UWotPauseMenu::Toggle(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World) {
		return;
	}
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UWotMenuWidget::StaticClass(), false);
	if (Found.Num() > 0) {
		CloseAllMenus(WorldContextObject);
		return;
	}
	OpenMenu(WorldContextObject, UWotPauseMenu::StaticClass());
}

void UWotPauseMenu::OnResume() { Close(); }

void UWotPauseMenu::OnSave()
{
	UObject* Ctx = GetOwningPlayer();
	Close();
	UWotSaveLoadMenu::OpenSaveLoad(Ctx, true);
}

void UWotPauseMenu::OnLoad()
{
	UObject* Ctx = GetOwningPlayer();
	Close();
	UWotSaveLoadMenu::OpenSaveLoad(Ctx, false);
}

void UWotPauseMenu::OnMainMenu()
{
	UObject* Ctx = GetOwningPlayer();
	Close();
	OpenMenu(Ctx, UWotMainMenu::StaticClass());
}

void UWotPauseMenu::OnQuit()
{
	if (UWotSaveSubsystem* S = UWotSaveSubsystem::Get(this)) {
		S->SaveToSlot(S->AutosaveSlot, true);
	}
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

// ---------------------------------------------------------------- save / load

void UWotSaveSlotRow::OnPrimary() { if (Menu) Menu->SlotPrimary(SlotName); }
void UWotSaveSlotRow::OnDelete() { if (Menu) Menu->SlotDelete(SlotName); }

UWotSaveLoadMenu::UWotSaveLoadMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TitleText = NSLOCTEXT("Wot", "LoadGame", "Load Game");
	PanelWidth = 720.0f;
}

UWotSaveLoadMenu* UWotSaveLoadMenu::OpenSaveLoad(UObject* WorldContextObject, bool bSave)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC) {
		return nullptr;
	}
	UWotSaveLoadMenu* Menu = CreateWidget<UWotSaveLoadMenu>(PC, UWotSaveLoadMenu::StaticClass());
	if (Menu) {
		Menu->bSaveMode = bSave;
		Menu->TitleText = bSave ? NSLOCTEXT("Wot", "SaveGame", "Save Game") : NSLOCTEXT("Wot", "LoadGame", "Load Game");
		Menu->Open();
	}
	return Menu;
}

void UWotSaveLoadMenu::BuildContent()
{
	Rebuild();
}

void UWotSaveLoadMenu::Rebuild()
{
	if (!Content || !WidgetTree) {
		return;
	}
	Content->ClearChildren();
	Rows.Reset();
	FirstButton = nullptr;
	if (TitleBlock) {
		TitleBlock->SetText(TitleText);
	}
	UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this);
	TArray<FWotSaveHeader> Slots = Saves ? Saves->ListSlots() : TArray<FWotSaveHeader>();
	if (bSaveMode) {
		if (UButton* B = AddButton(NSLOCTEXT("Wot", "NewSave", "+ New save"), WotUI::Selected)) B->OnClicked.AddDynamic(this, &UWotSaveLoadMenu::OnNewSave);
	}
	if (Slots.Num() == 0) {
		AddLabel(NSLOCTEXT("Wot", "NoSaves", "No saved games yet."), 16, WotUI::Muted);
	}
	for (const FWotSaveHeader& H : Slots) {
		UWotSaveSlotRow* Row = NewObject<UWotSaveSlotRow>(this);
		Row->SlotName = H.SlotName;
		Row->Menu = this;
		Rows.Add(Row);
		UHorizontalBox* Line = AddRow();
		if (!Line) {
			continue;
		}
		// primary button: slot name + details
		UButton* Primary = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		StyleButton(Primary, H.bAutosave ? WotUI::ButtonNormal * FLinearColor(0.8f, 0.8f, 0.9f, 1.0f) : WotUI::ButtonNormal);
		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		const int32 Secs = int32(H.PlaytimeSeconds);
		const FText Line1 = FText::Format(NSLOCTEXT("Wot", "SlotLine1", "{0}   -   {1}"), FText::FromString(H.SlotName), FText::FromString(H.LevelName));
		const FText Line2 = FText::Format(NSLOCTEXT("Wot", "SlotLine2", "{0}   |   {1}h {2}m   |   {3}"), FText::FromString(H.SavedAt.ToString(TEXT("%Y-%m-%d %H:%M"))), Secs / 3600, (Secs / 60) % 60, H.CurrentQuest.IsEmpty() ? NSLOCTEXT("Wot", "NoQuest", "no active quest") : H.CurrentQuest);
		Lines->AddChildToVerticalBox(MakeText(WidgetTree, Line1, 17, WotUI::Title));
		Lines->AddChildToVerticalBox(MakeText(WidgetTree, Line2, 13, WotUI::Muted));
		Primary->AddChild(Lines);
		if (UButtonSlot* BS = Cast<UButtonSlot>(Lines->Slot)) {
			BS->SetHorizontalAlignment(HAlign_Left);
		}
		Primary->OnClicked.AddDynamic(Row, &UWotSaveSlotRow::OnPrimary);
		if (UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Primary)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetPadding(FMargin(0, 0, 6.0f, 0));
		}
		if (!FirstButton) {
			FirstButton = Primary;
		}
		// delete
		UButton* Del = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		StyleButton(Del, WotUI::Danger);
		UTextBlock* DelText = MakeText(WidgetTree, NSLOCTEXT("Wot", "Delete", "Delete"), 14, WotUI::Text);
		Del->AddChild(DelText);
		Del->OnClicked.AddDynamic(Row, &UWotSaveSlotRow::OnDelete);
		Line->AddChildToHorizontalBox(Del);
	}
	StatusBlock = AddLabel(FText::GetEmpty(), 13, WotUI::Muted);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "Back", "Back"))) B->OnClicked.AddDynamic(this, &UWotSaveLoadMenu::OnBack);
}

void UWotSaveLoadMenu::SlotPrimary(const FString& SlotName)
{
	UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this);
	if (!Saves) {
		return;
	}
	if (bSaveMode) {
		Saves->SaveToSlot(SlotName);
		Rebuild();
	} else {
		Close();
		Saves->LoadFromSlot(SlotName);
	}
}

void UWotSaveLoadMenu::SlotDelete(const FString& SlotName)
{
	if (UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this)) {
		Saves->DeleteSlot(SlotName);
	}
	Rebuild();
}

void UWotSaveLoadMenu::OnNewSave()
{
	if (UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this)) {
		const FString Slot = FString::Printf(TEXT("save_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
		Saves->SaveToSlot(Slot);
	}
	Rebuild();
}

void UWotSaveLoadMenu::OnBack()
{
	UObject* Ctx = GetOwningPlayer();
	Close();
	OpenMenu(Ctx, UWotPauseMenu::StaticClass());
}

// ---------------------------------------------------------------- main menu

UWotMainMenu::UWotMainMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TitleText = NSLOCTEXT("Wot", "GameTitle", "The Wheel of Time");
	PanelWidth = 460.0f;
}

void UWotMainMenu::BuildContent()
{
	UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this);
	FWotSaveHeader Latest;
	const bool bHasSave = Saves && Saves->GetLatestSlot(Latest);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "NewGame", "New Game"))) B->OnClicked.AddDynamic(this, &UWotMainMenu::OnNewGame);
	if (bHasSave) {
		if (UButton* B = AddButton(FText::Format(NSLOCTEXT("Wot", "ContinueFmt", "Continue  ({0})"), FText::FromString(Latest.LevelName)))) B->OnClicked.AddDynamic(this, &UWotMainMenu::OnContinue);
	}
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "LoadGame", "Load Game"))) B->OnClicked.AddDynamic(this, &UWotMainMenu::OnLoad);
	if (UButton* B = AddButton(NSLOCTEXT("Wot", "QuitDesktop", "Quit to Desktop"), WotUI::Danger)) B->OnClicked.AddDynamic(this, &UWotMainMenu::OnQuit);
	AddLabel(NSLOCTEXT("Wot", "MainMenuHint", "Esc returns to the game."), 12, WotUI::Muted);
}

void UWotMainMenu::OnNewGame()
{
	UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this);
	Close();
	if (Saves) {
		Saves->NewGame();
	}
}

void UWotMainMenu::OnContinue()
{
	UWotSaveSubsystem* Saves = UWotSaveSubsystem::Get(this);
	FWotSaveHeader Latest;
	if (Saves && Saves->GetLatestSlot(Latest)) {
		Close();
		Saves->LoadFromSlot(Latest.SlotName);
	}
}

void UWotMainMenu::OnLoad()
{
	UObject* Ctx = GetOwningPlayer();
	Close();
	UWotSaveLoadMenu::OpenSaveLoad(Ctx, false);
}

void UWotMainMenu::OnQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

// ---------------------------------------------------------------- console

static FAutoConsoleCommandWithWorldAndArgs GWotMenuPause(
	TEXT("wot.Menu.Pause"), TEXT("Toggle the pause menu"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) { UWotPauseMenu::Toggle(World); }));

static FAutoConsoleCommandWithWorldAndArgs GWotMenuMain(
	TEXT("wot.Menu.Main"), TEXT("Open the main menu"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) { UWotMenuWidget::OpenMenu(World, UWotMainMenu::StaticClass()); }));

static FAutoConsoleCommandWithWorldAndArgs GWotMenuSave(
	TEXT("wot.Menu.Save"), TEXT("Open the save menu"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) { UWotSaveLoadMenu::OpenSaveLoad(World, true); }));

static FAutoConsoleCommandWithWorldAndArgs GWotMenuLoad(
	TEXT("wot.Menu.Load"), TEXT("Open the load menu"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) { UWotSaveLoadMenu::OpenSaveLoad(World, false); }));

static FAutoConsoleCommandWithWorldAndArgs GWotMenuJournal(
	TEXT("wot.Menu.Journal"), TEXT("Toggle the quest journal"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UWotQuestJournal::Toggle(World);
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotMenuInventory(
	TEXT("wot.Menu.Inventory"), TEXT("Open the player's inventory screen"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (AWotCharacter* Player = PC ? Cast<AWotCharacter>(PC->GetPawn()) : nullptr) {
			UWotUWInventoryPanel* Panel = nullptr;
			Player->ShowInventoryWidget(Panel);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotMenuCloseAll(
	TEXT("wot.Menu.CloseAll"), TEXT("Close every open menu and the inventory"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UWotMenuWidget::CloseAllMenus(World);
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (AWotCharacter* Player = PC ? Cast<AWotCharacter>(PC->GetPawn()) : nullptr) {
			if (Player->IsInventoryWidgetOpen()) {
				Player->CloseInventoryWidget();
			}
		}
	}));
