#include "WotAction_ProjectileAttack.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

UWotAction_ProjectileAttack::UWotAction_ProjectileAttack()
{
  AttackAnimDelay = 0.2f;
  HandSocketName = "Hand_R";
}

void UWotAction_ProjectileAttack::Start_Implementation(AActor* Instigator)
{
  Super::Start_Implementation(Instigator);

  ACharacter* Character = Cast<ACharacter>(Instigator);

  if (!Character) {
    UE_LOG(LogTemp, Warning, TEXT("Provided instigator is not a character!"));
    return;
  }

  // Play the casting animation
  Character->PlayAnimMontage(AttackAnim);

  // We don't require a casting system, so don't ensure it
  if (CastingNiagaraSystem) {
    auto CastingNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,
                                                                             CastingNiagaraSystem,
                                                                             Character->GetActorLocation(),
                                                                             Character->GetActorRotation());
  }

  // Start the timer for launching the projectile
  FTimerHandle TimerHandle_AttackDelay;
  FTimerDelegate Delegate;
  Delegate.BindUFunction(this, "AttackDelay_TimerElapsed", Character);
  GetWorld()->GetTimerManager().SetTimer(TimerHandle_AttackDelay, Delegate, AttackAnimDelay, false);
}

void UWotAction_ProjectileAttack::Stop_Implementation(AActor* Instigator)
{
  Super::Stop_Implementation(Instigator);
}

void UWotAction_ProjectileAttack::AttackDelay_TimerElapsed(ACharacter* InstigatorCharacter)
{
  if (!InstigatorCharacter) {
    UE_LOG(LogTemp, Warning, TEXT("Provided instigatorcharacter is null!"));
    return;
  }

  if (!ensure(ProjectileClass)) {
    UE_LOG(LogTemp, Error, TEXT("No configured projectile class!"));
    return;
  }

  // Now spawn the projectile
  auto HandLocation = InstigatorCharacter->GetMesh()->GetSocketLocation(HandSocketName);
  auto SpawnTM = FTransform(InstigatorCharacter->GetActorRotation(), HandLocation);
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  // Set the instigator so the projectile doesn't interact / damage the owner pawn
  SpawnParams.Instigator = InstigatorCharacter;
  GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnTM, SpawnParams);

  // Now actually stop the action since we're done
  Stop(InstigatorCharacter);
}
