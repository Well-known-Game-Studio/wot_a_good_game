#include "AI/WotAICharacter.h"

AWotAICharacter::AWotAICharacter()
{
  // Set this parameter to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
}

void AWotAICharacter::BeginPlay()
{
  Super::BeginPlay();
}

void AWotAICharacter::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
