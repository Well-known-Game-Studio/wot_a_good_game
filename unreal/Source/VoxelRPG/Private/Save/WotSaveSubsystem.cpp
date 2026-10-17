#include "Save/WotSaveSubsystem.h"
#include "WotCharacter.h"
#include "WotGameInstance.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"
#include "Items/WotItem.h"
#include "Quests/WotQuestSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotSave, Log, All);

UWotSaveSubsystem* UWotSaveSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) {
		return nullptr;
	}
	UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UWotSaveSubsystem>() : nullptr;
}

void UWotSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LastTickTime = FPlatformTime::Seconds();
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UWotSaveSubsystem::TickPlaytime), 1.0f);
}

void UWotSaveSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

bool UWotSaveSubsystem::TickPlaytime(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && World->HasBegunPlay() && !World->IsPaused()) {
		PlaytimeSeconds += float(Now - LastTickTime);
	}
	LastTickTime = Now;
	return true;
}

AWotCharacter* UWotSaveSubsystem::GetLocalPlayer() const
{
	UGameInstance* GI = GetGameInstance();
	APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr;
	return PC ? Cast<AWotCharacter>(PC->GetPawn()) : nullptr;
}

// ---------------------------------------------------------------- capture / apply

void UWotSaveSubsystem::CapturePlayer(UWotGameSave* Save, const AWotCharacter* Player) const
{
	if (!Save || !Player) {
		return;
	}
	Save->bHasPlayerTransform = true;
	Save->PlayerLocation = Player->GetActorLocation();
	Save->PlayerRotation = Player->GetActorRotation();
	if (UWotAttributeComponent* Attr = UWotAttributeComponent::GetAttributes(const_cast<AWotCharacter*>(Player))) {
		Save->Health = Attr->GetHealth();
	}
	Save->Inventory.Reset();
	if (UWotInventoryComponent* Inv = UWotInventoryComponent::GetInventory(const_cast<AWotCharacter*>(Player))) {
		for (UWotItem* Item : Inv->Items) {
			if (!Item || Item->Count <= 0) {
				continue;
			}
			FWotSavedItem Saved;
			Saved.ItemClass = FSoftClassPath(Item->GetClass());
			Saved.Count = Item->Count;
			Save->Inventory.Add(Saved);
		}
	}
}

UWotGameSave* UWotSaveSubsystem::CaptureState(bool bAutosave) const
{
	UWotGameSave* Save = Cast<UWotGameSave>(UGameplayStatics::CreateSaveGameObject(UWotGameSave::StaticClass()));
	if (!Save) {
		return nullptr;
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	Save->Header.SavedAt = FDateTime::Now();
	Save->Header.PlaytimeSeconds = PlaytimeSeconds;
	Save->Header.bAutosave = bAutosave;
	if (World) {
		Save->Header.LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		Save->Header.LevelPath = World->GetOutermost()->GetName();
		// PIE worlds carry a UEDPIE_ prefix on the package; strip it so the path reopens in any session
		Save->Header.LevelPath = Save->Header.LevelPath.Replace(TEXT("UEDPIE_0_"), TEXT(""));
	}
	if (UWotGameInstance* GI = Cast<UWotGameInstance>(GetGameInstance())) {
		Save->LastPortalName = GI->LastPortalName;
		GI->SaveTimeOfDay();
		Save->TimeOfDaySeconds = GI->CurrentTimeOfDayInSeconds;
	}
	if (UWotQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UWotQuestSubsystem>()) {
		Quests->GetProgressSnapshot(Save->Quests, Save->bQuestAutoStartDone);
		for (FName Id : Quests->GetActiveQuestIds()) {
			if (const UWotQuestDefinition* Def = Quests->GetQuestDefinition(Id)) {
				Save->Header.CurrentQuest = Def->Title;
				break;
			}
		}
	}
	Save->StoryFlags = StoryFlags;
	CapturePlayer(Save, GetLocalPlayer());
	return Save;
}

void UWotSaveSubsystem::ApplyInventory(const UWotGameSave* Save, AWotCharacter* Player) const
{
	UWotInventoryComponent* Inv = Player ? UWotInventoryComponent::GetInventory(Player) : nullptr;
	if (!Inv) {
		return;
	}
	// diff against what the pawn already holds so equipped item objects survive a load
	TSet<UClass*> Wanted;
	for (const FWotSavedItem& Saved : Save->Inventory) {
		UClass* ItemClass = Saved.ItemClass.TryLoadClass<UWotItem>();
		if (!ItemClass || Saved.Count <= 0) {
			UE_LOG(LogWotSave, Warning, TEXT("Could not restore item %s"), *Saved.ItemClass.ToString());
			continue;
		}
		Wanted.Add(ItemClass);
		if (UWotItem* Existing = Inv->FindItem(ItemClass)) {
			Existing->Count = Saved.Count;
		} else {
			UWotItem* Item = NewObject<UWotItem>(Inv, ItemClass);
			Item->Count = Saved.Count;
			Inv->AddItem(Item);
		}
	}
	TArray<UWotItem*> Extra;
	for (UWotItem* Item : Inv->Items) {
		if (Item && !Wanted.Contains(Item->GetClass())) {
			Extra.Add(Item);
		}
	}
	for (UWotItem* Item : Extra) {
		Inv->DeleteItem(Item);
	}
	Inv->OnInventoryUpdated.Broadcast();
}

void UWotSaveSubsystem::ApplyState(const UWotGameSave* Save, AWotCharacter* Player)
{
	if (!Save) {
		return;
	}
	if (Player && Save->bHasPlayerTransform) {
		Player->SetActorLocation(Save->PlayerLocation + FVector(0, 0, 5.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetActorRotation(Save->PlayerRotation);
	}
	ApplyInventory(Save, Player);
	if (Player && Save->Health >= 0.0f) {
		if (UWotAttributeComponent* Attr = UWotAttributeComponent::GetAttributes(Player)) {
			Attr->ApplyHealthChange(Save->Health - Attr->GetHealth());
		}
	}
	if (UWotGameInstance* GI = Cast<UWotGameInstance>(GetGameInstance())) {
		GI->LastPortalName = Save->LastPortalName;
		GI->CurrentTimeOfDayInSeconds = Save->TimeOfDaySeconds;
		GI->LoadTimeOfDay();
	}
	if (UWotQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UWotQuestSubsystem>()) {
		Quests->SetProgressSnapshot(Save->Quests, Save->bQuestAutoStartDone);
	}
	StoryFlags = Save->StoryFlags;
	PlaytimeSeconds = Save->Header.PlaytimeSeconds;
}

// ---------------------------------------------------------------- slots

bool UWotSaveSubsystem::SaveToSlot(const FString& SlotName, bool bAutosave)
{
	UWotGameSave* Save = CaptureState(bAutosave);
	if (!Save) {
		return false;
	}
	Save->Header.SlotName = SlotName;
	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	UE_LOG(LogWotSave, Log, TEXT("Save '%s' %s (level %s, %d items, %d quests)"), *SlotName, bOk ? TEXT("written") : TEXT("FAILED"), *Save->Header.LevelName, Save->Inventory.Num(), Save->Quests.Num());
	if (bOk) {
		OnGameSaved.Broadcast(SlotName);
		if (AWotCharacter* Player = GetLocalPlayer()) {
			Player->ShowPopupWidget(bAutosave ? NSLOCTEXT("Wot", "Autosaved", "Autosaved") : NSLOCTEXT("Wot", "GameSaved", "Game saved"), 2.0f, true);
		}
	}
	return bOk;
}

bool UWotSaveSubsystem::LoadFromSlot(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0)) {
		UE_LOG(LogWotSave, Warning, TEXT("Load: slot '%s' does not exist"), *SlotName);
		return false;
	}
	UWotGameSave* Save = Cast<UWotGameSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save) {
		return false;
	}
	PendingLoad = Save;
	CarryOver = nullptr;
	UE_LOG(LogWotSave, Log, TEXT("Load '%s': opening %s"), *SlotName, *Save->Header.LevelPath);
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World) {
		UGameplayStatics::SetGamePaused(World, false);
		UGameplayStatics::OpenLevel(World, FName(*Save->Header.LevelPath));
	}
	return true;
}

bool UWotSaveSubsystem::DeleteSlot(const FString& SlotName)
{
	return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

bool UWotSaveSubsystem::SlotExists(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

TArray<FWotSaveHeader> UWotSaveSubsystem::ListSlots() const
{
	TArray<FWotSaveHeader> Out;
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.sav")), true, false);
	for (const FString& File : Files) {
		const FString Slot = FPaths::GetBaseFilename(File);
		if (Slot == TEXT("WotQuests")) {
			continue;
		}
		if (UWotGameSave* Save = Cast<UWotGameSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0))) {
			FWotSaveHeader H = Save->Header;
			H.SlotName = Slot;
			Out.Add(H);
		}
	}
	Out.Sort([](const FWotSaveHeader& A, const FWotSaveHeader& B) { return A.SavedAt > B.SavedAt; });
	return Out;
}

bool UWotSaveSubsystem::GetLatestSlot(FWotSaveHeader& OutHeader) const
{
	TArray<FWotSaveHeader> Slots = ListSlots();
	if (Slots.Num() == 0) {
		return false;
	}
	OutHeader = Slots[0];
	return true;
}

void UWotSaveSubsystem::NewGame()
{
	PendingLoad = nullptr;
	CarryOver = nullptr;
	StoryFlags.Reset();
	PlaytimeSeconds = 0.0f;
	if (UWotQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UWotQuestSubsystem>()) {
		Quests->ResetAllQuests();
	}
	if (UWotGameInstance* GI = Cast<UWotGameInstance>(GetGameInstance())) {
		GI->LastPortalName = NAME_None;
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World) {
		UGameplayStatics::SetGamePaused(World, false);
		UGameplayStatics::OpenLevel(World, FName(*NewGameLevel));
	}
}

void UWotSaveSubsystem::QuitToMainMenu()
{
	PendingLoad = nullptr;
	CarryOver = nullptr;
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World) {
		UGameplayStatics::SetGamePaused(World, false);
		UGameplayStatics::OpenLevel(World, FName(TEXT("/Game/Wot/Maps/MainMenu/MainMenu")));
	}
}

void UWotSaveSubsystem::SetStoryFlag(FName Flag, bool bValue)
{
	if (bValue) {
		StoryFlags.AddUnique(Flag);
	} else {
		StoryFlags.Remove(Flag);
	}
}

// ---------------------------------------------------------------- hooks

void UWotSaveSubsystem::OnPlayerReady(AWotCharacter* Player)
{
	if (PendingLoad) {
		UWotGameSave* Save = PendingLoad;
		PendingLoad = nullptr;
		ApplyState(Save, Player);
		UE_LOG(LogWotSave, Log, TEXT("Load applied in %s at %s"), *UGameplayStatics::GetCurrentLevelName(Player, true), *Player->GetActorLocation().ToString());
		OnGameLoaded.Broadcast(Save->Header.SlotName);
		Player->ShowPopupWidget(NSLOCTEXT("Wot", "GameLoaded", "Game loaded"), 2.0f, true);
		return;
	}
	if (CarryOver) {
		// arrived through a portal: the Blueprint already hands the inventory over, so only carry health
		UWotGameSave* Save = CarryOver;
		CarryOver = nullptr;
		if (Save->Health >= 0.0f) {
			if (UWotAttributeComponent* Attr = UWotAttributeComponent::GetAttributes(Player)) {
				Attr->ApplyHealthChange(Save->Health - Attr->GetHealth());
			}
		}
	}
}

void UWotSaveSubsystem::OnLevelTransition(UWorld* World)
{
	if (PendingLoad) {
		return; // a load is in flight; nothing to carry over
	}
	UWotGameSave* Save = CaptureState(true);
	if (!Save) {
		return;
	}
	CarryOver = Save;
	if (bAutosaveOnLevelTransition) {
		Save->Header.SlotName = AutosaveSlot;
		UGameplayStatics::SaveGameToSlot(Save, AutosaveSlot, 0);
		UE_LOG(LogWotSave, Log, TEXT("Autosaved on leaving %s"), *Save->Header.LevelName);
	}
}

FString UWotSaveSubsystem::DescribeSlots() const
{
	FString Out;
	for (const FWotSaveHeader& H : ListSlots()) {
		Out += FString::Printf(TEXT("%s: %s %s playtime=%.0fs quest=\"%s\"%s\n"), *H.SlotName, *H.LevelName, *H.SavedAt.ToString(), H.PlaytimeSeconds, *H.CurrentQuest.ToString(), H.bAutosave ? TEXT(" (auto)") : TEXT(""));
	}
	return Out.IsEmpty() ? TEXT("(no saves)\n") : Out;
}

// ---------------------------------------------------------------- console

static FAutoConsoleCommandWithWorldAndArgs GWotSaveCmd(
	TEXT("wot.Save"), TEXT("Save the game: wot.Save [slot=quicksave]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotSaveSubsystem* S = UWotSaveSubsystem::Get(World)) {
			S->SaveToSlot(Args.Num() > 0 ? Args[0] : S->QuickSlot);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotLoadCmd(
	TEXT("wot.Load"), TEXT("Load a save: wot.Load [slot=quicksave]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotSaveSubsystem* S = UWotSaveSubsystem::Get(World)) {
			S->LoadFromSlot(Args.Num() > 0 ? Args[0] : S->QuickSlot);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotSavesCmd(
	TEXT("wot.Saves"), TEXT("List save slots"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotSaveSubsystem* S = UWotSaveSubsystem::Get(World)) {
			UE_LOG(LogWotSave, Display, TEXT("Saves:\n%s"), *S->DescribeSlots());
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotNewGameCmd(
	TEXT("wot.NewGame"), TEXT("Start a new game"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotSaveSubsystem* S = UWotSaveSubsystem::Get(World)) {
			S->NewGame();
		}
	}));
