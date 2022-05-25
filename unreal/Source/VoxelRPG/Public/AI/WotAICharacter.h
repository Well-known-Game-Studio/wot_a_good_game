#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotAICharacter.generated.h"

UCLASS()
class VOXELRPG_API AWotAICharacter : public ACharacter
{
  GENERATED_BODY()

public:
  // Sets default values for this character's properties
  AWotAICharacter();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
