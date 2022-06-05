#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WotAICharacter.generated.h"

class UPawnSensingComponent;

UCLASS()
class VOXELRPG_API AWotAICharacter : public ACharacter
{
  GENERATED_BODY()

public:
  // Sets default values for this character's properties
  AWotAICharacter();

protected:

  virtual void PostInitializeComponents() override;

  UPROPERTY(VisibleAnywhere, Category = "Components")
  UPawnSensingComponent* PawnSensingComp;

  UFUNCTION()
  void OnPawnSeen(APawn* Pawn);
};
