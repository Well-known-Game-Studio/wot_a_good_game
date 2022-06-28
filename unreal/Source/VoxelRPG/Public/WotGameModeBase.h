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

  AWotGameModeBase();

  virtual void StartPlay() override;

protected:

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

  UFUNCTION()
  void SpawnBotTimerElapsed();

  UFUNCTION(Exec)
  void KillAll();
};
