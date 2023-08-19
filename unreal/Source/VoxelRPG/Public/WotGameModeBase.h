#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "WotGameModeBase.generated.h"

class UEnvQuery;
class UEnvQueryInstanceBlueprintWrapper;
class UCurveFloat;

UCLASS()
class VOXELRPG_API AWotGameModeBase : public AGameModeBase
{
  GENERATED_BODY()

public:

  virtual void OnActorKilled(AActor* VictimActor, AActor* Killer);

  AWotGameModeBase();

  virtual void StartPlay() override;

  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

  UFUNCTION(Exec)
  void KillAll();

protected:

  UFUNCTION(BlueprintImplementableEvent, Category = "Time")
  void OnTimeOfDayChanged(float TimeOfDay);

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
  void SaveTime();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
  void LoadTime();

  UPROPERTY(EditDefaultsOnly, Category = "AI")
  TSubclassOf<AActor> MinionClass;

  UPROPERTY(EditDefaultsOnly, Category = "AI")
  UEnvQuery* SpawnBotQuery;

  UPROPERTY(EditDefaultsOnly, Category = "AI")
  UCurveFloat* DifficultyCurve;

  UFUNCTION()
  void OnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus);

  FTimerHandle TimerHandle_SpawnBots;

  UPROPERTY(EditDefaultsOnly, Category = "AI")
  float SpawnTimerInterval;

  UPROPERTY(EditDefaultsOnly, Category = "AI")
  float SpawnTimerInitialDelay;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AI")
  bool bShouldSpawnEnemies{false};

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AI")
  float SpawnStartTime{0};

  UFUNCTION(BlueprintCallable, Category = "AI")
  void StartSpawningEnemies();

  UFUNCTION(BlueprintCallable, Category = "AI")
  void PauseSpawningEnemies();

  UFUNCTION(BlueprintCallable, Category = "AI")
  void ResumeSpawningEnemies();

  UFUNCTION(BlueprintCallable, Category = "AI")
  void StopSpawningEnemies();

  UFUNCTION()
  void SpawnBotTimerElapsed();

  UFUNCTION()
  void RespawnPlayerTimerElapsed(AController* Controller);
};
