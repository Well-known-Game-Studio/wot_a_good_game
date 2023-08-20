#include "WotGameModeBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "GameFramework/PlayerStart.h"
#include <Kismet/GameplayStatics.h>
#include "AI/WotAICharacter.h"
#include "WotCharacter.h"
#include "WotAttributeComponent.h"
#include "WotGameInstance.h"
#include "EngineUtils.h"

static TAutoConsoleVariable<bool> CVarSpawnBots(TEXT("wot.SpawnBots"), true, TEXT("Enable spawning of bots via timer"), ECVF_Cheat);

AWotGameModeBase::AWotGameModeBase()
{
  SpawnTimerInterval = 2.0f;
}

void AWotGameModeBase::StartPlay()
{
  Super::StartPlay();

  // Continuous timer to spawn more bots. Actual amount of bots and whether it
  // is allowed to spawn determined by logic later in the chain...
  GetWorldTimerManager().SetTimer(TimerHandle_SpawnBots, this, &AWotGameModeBase::SpawnBotTimerElapsed, SpawnTimerInterval, true, SpawnTimerInitialDelay);

  LoadTime();
}

void AWotGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  switch (EndPlayReason) {
    case EEndPlayReason::LevelTransition:
      SaveTime();
      break;
    case EEndPlayReason::Destroyed:
    case EEndPlayReason::EndPlayInEditor:
    case EEndPlayReason::RemovedFromWorld:
    case EEndPlayReason::Quit:
    default:
      break;
  }
}

void AWotGameModeBase::SaveTime_Implementation()
{
  UWotGameInstance* GameInstance = Cast<UWotGameInstance>(GetGameInstance());
  if (GameInstance) {
    GameInstance->SaveTimeOfDay();
  }
}

void AWotGameModeBase::LoadTime_Implementation()
{
  UWotGameInstance* GameInstance = Cast<UWotGameInstance>(GetGameInstance());
  if (GameInstance) {
    GameInstance->LoadTimeOfDay();
  }
}

AActor* AWotGameModeBase::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
  // get the game instance
  UWotGameInstance* GameInstance = Cast<UWotGameInstance>(GetGameInstance());
  if (!GameInstance) {
    return Super::FindPlayerStart_Implementation(Player, IncomingName);
  }
  // get the last portal name and use it to find the player start
  FName PortalName = GameInstance->LastPortalName;
  // get all player starts in the level
  TArray<AActor*> PlayerStarts;
  UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
  // find the player start with the matching name
  AActor* PlayerStart = nullptr;
  for (AActor* Actor : PlayerStarts) {
    APlayerStart* PS = Cast<APlayerStart>(Actor);
    if (PS && PS->PlayerStartTag == PortalName) {
      return PS;
    }
  }
  // if we didn't find a player start with the matching name, just return the first one
  return PlayerStarts[0];
}

void AWotGameModeBase::OnActorKilled(AActor* VictimActor, AActor* Killer)
{
  AWotCharacter* Player = Cast<AWotCharacter>(VictimActor);
  if (Player) {
    FTimerHandle TimerHandle_RespawnDelay;
    FTimerDelegate Delegate;
    Delegate.BindUFunction(this, "RespawnPlayerTimerElapsed", Player->GetController());
    float RespawnDelay = 2.0f;
    GetWorldTimerManager().SetTimer(TimerHandle_RespawnDelay, Delegate, RespawnDelay, false);
  }
  UE_LOG(LogTemp, Log, TEXT("OnActorKilled: Victim: %s, Killer: %s"), *GetNameSafe(VictimActor), *GetNameSafe(Killer));
}

void AWotGameModeBase::RespawnPlayerTimerElapsed(AController* Controller)
{
  if (Controller) {
    // Detatch controller so that we will get a new copy of the character
    Controller->UnPossess();
    RestartPlayer(Controller);
  }
}

void AWotGameModeBase::StartSpawningEnemies()
{
  bShouldSpawnEnemies = true;
  SpawnStartTime = GetWorld()->GetTimeSeconds();
}

void AWotGameModeBase::PauseSpawningEnemies()
{
  bShouldSpawnEnemies = false;
}

void AWotGameModeBase::ResumeSpawningEnemies()
{
  bShouldSpawnEnemies = true;
  if (SpawnStartTime < 0) {
    // we have an invalid start time, set it to now since we're resuming
    SpawnStartTime = GetWorld()->GetTimeSeconds();
  }
}

void AWotGameModeBase::StopSpawningEnemies()
{
  bShouldSpawnEnemies = true;
  SpawnStartTime = -1;
}

void AWotGameModeBase::SpawnBotTimerElapsed()
{
  if (!CVarSpawnBots.GetValueOnGameThread()) {
    UE_LOG(LogTemp, Warning, TEXT("Bot spawning disabled via cvar 'CVarSpawnBots'."));
    return;
  }

  if (!bShouldSpawnEnemies) {
    UE_LOG(LogTemp, Log, TEXT("Bot spawning disabled via property bShouldSpawnEnemies."));
    return;
  }

  int32 NumberBotsAlive = 0;
  for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
    AWotAICharacter* Bot = *It;
    UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(Bot);
    if (AttributeComp && AttributeComp->IsAlive()) {
      NumberBotsAlive++;
    }
  }

  int32 MaxBotCount = 10;

  if (DifficultyCurve) {
    MaxBotCount = DifficultyCurve->GetFloatValue(GetWorld()->TimeSeconds - SpawnStartTime);
  }

  if (NumberBotsAlive >= MaxBotCount) {
    return;
  }

  UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, SpawnBotQuery, this, EEnvQueryRunMode::RandomBest5Pct, nullptr);
  if (ensure(QueryInstance) && ensure(MinionClass)) {
    QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &AWotGameModeBase::OnQueryCompleted);
  }
}

void AWotGameModeBase::OnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
  if (QueryStatus != EEnvQueryStatus::Success) {
    UE_LOG(LogTemp, Warning, TEXT("Spawn bot EQS query failed!"));
    return;
  }

  TArray<FVector> Locations = QueryInstance->GetResultsAsLocations();
  if (Locations.Num() <= 0) {
    return;
  }
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  GetWorld()->SpawnActor<AActor>(MinionClass, Locations[0] + FVector(0,0,10), FRotator::ZeroRotator, SpawnParams);
}

void AWotGameModeBase::KillAll()
{
  int32 NumberBotsAlive = 0;
  for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
    AWotAICharacter* Bot = *It;
    UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(Bot);
    if (AttributeComp && AttributeComp->IsAlive()) {
      AttributeComp->Kill(this); // @fixme: pass in player for kill credit?
    }
  }
}
