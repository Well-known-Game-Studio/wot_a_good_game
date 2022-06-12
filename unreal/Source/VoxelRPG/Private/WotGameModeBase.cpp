#include "WotGameModeBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "AI/WotAICharacter.h"
#include "WotAttributeComponent.h"
#include "EngineUtils.h"

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
}

void AWotGameModeBase::SpawnBotTimerElapsed()
{
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

  int32 NumberBotsAlive = 0;
  for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
    AWotAICharacter* Bot = *It;
    UWotAttributeComponent* AttributeComp = Cast<UWotAttributeComponent>(Bot->GetComponentByClass(UWotAttributeComponent::StaticClass()));
    if (AttributeComp && AttributeComp->IsAlive()) {
      NumberBotsAlive++;
    }
  }

  int32 MaxBotCount = 10;

  if (DifficultyCurve) {
    MaxBotCount = DifficultyCurve->GetFloatValue(GetWorld()->TimeSeconds);
  }

  if (NumberBotsAlive >= MaxBotCount) {
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
