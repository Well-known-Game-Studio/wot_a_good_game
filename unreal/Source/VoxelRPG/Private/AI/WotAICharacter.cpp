#include "AI/WotAICharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"

AWotAICharacter::AWotAICharacter()
{
  PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");
}

void AWotAICharacter::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  PawnSensingComp->OnSeePawn.AddDynamic(this, &AWotAICharacter::OnPawnSeen);
}

void AWotAICharacter::OnPawnSeen(APawn* Pawn)
{
  AAIController* AIC = Cast<AAIController>(GetController());
  if (AIC) {
    UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
    BBComp->SetValueAsObject("TargetActor", Pawn);
    DrawDebugString(GetWorld(), GetActorLocation(), "PLAYER SPOTTED", nullptr, FColor::White, 4.0f, true);
  }
}
