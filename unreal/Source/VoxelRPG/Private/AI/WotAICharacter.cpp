#include "AI/WotAICharacter.h"
#include "AI/WotAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"
#include "WotDeathEffectComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "UI/WotUWHealthBar.h"
#include "UI/WotUWPopupNumber.h"
#include "BrainComponent.h"
#include "Items/WotItem.h"

AWotAICharacter::AWotAICharacter()
{
  PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");
  AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	InventoryComp = CreateDefaultSubobject<UWotInventoryComponent>("InventoryComp");

  AttributeComp = CreateDefaultSubobject<UWotAttributeComponent>("AttributeComp");

	DeathEffectComp = CreateDefaultSubobject<UWotDeathEffectComponent>("DeathEffectComp");
}

void AWotAICharacter::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  PawnSensingComp->OnSeePawn.AddDynamic(this, &AWotAICharacter::OnPawnSeen);
	AttributeComp->OnHealthChanged.AddDynamic(this, &AWotAICharacter::OnHealthChanged);
	AttributeComp->OnKilled.AddDynamic(this, &AWotAICharacter::OnKilled);
}

void AWotAICharacter::OnPawnSeen(APawn* Pawn)
{
  SetBlackboardActor("TargetActor", Pawn);
}

void AWotAICharacter::HitFlash()
{
	auto Mesh = GetMesh();
	// register that we were hit now
	Mesh->SetScalarParameterValueOnMaterials("TimeToHit", GetWorld()->GetTimeSeconds());
	// what color should we flash (emissive) - use the health to make it
	// transition from yellow to red
	auto DangerColor = FLinearColor(1.0f, 0.0f, 0.460229f, 1.0f);
	auto WarningColor = FLinearColor(0.815215f, 1.0f, 0.0f, 1.0f);
	auto Progress = AttributeComp->GetHealth() / AttributeComp->GetHealthMax();
	auto LinearColor = FLinearColor::LerpUsingHSV(DangerColor, WarningColor, Progress);
	auto HitColor = FVector4(LinearColor);
	Mesh->SetVectorParameterValueOnMaterials("HitColor", HitColor);
	// how quickly the flash should fade (1.0 = 1 second, 2.0 = 0.5 seconds)
	Mesh->SetScalarParameterValueOnMaterials("FlashTimeFactor", 2.0f);
}

void AWotAICharacter::ShowHealthBarWidget(float NewHealth, float Delta, float Duration)
{
	if (HealthBarWidgetClass) {
		UWotUWHealthBar* HealthBarWidget = CreateWidget<UWotUWHealthBar>(GetWorld(), HealthBarWidgetClass);
		HealthBarWidget->SetDuration(Duration);
		float HealthMax = AttributeComp->GetHealthMax();
		float HealthStart = NewHealth - Delta;
		float HealthEnd = NewHealth;
		HealthBarWidget->SetHealth(HealthStart, HealthEnd, HealthMax);
		HealthBarWidget->SetAttachTo(this);
		HealthBarWidget->PlayTextUpdateAnimation();
		HealthBarWidget->AddToViewport();
	}
}

void AWotAICharacter::ShowPopupWidgetNumber(int Number, float Duration)
{
	if (PopupWidgetClass) {
		UWotUWPopupNumber* PopupWidget = CreateWidget<UWotUWPopupNumber>(GetWorld(), PopupWidgetClass);
		PopupWidget->SetDuration(Duration);
		PopupWidget->SetNumber(Number);
		PopupWidget->SetAttachTo(this);
		PopupWidget->PlayPopupAnimation();
		PopupWidget->AddToViewport();
	}
}

void AWotAICharacter::ShowPopupWidget(const FText& Text, float Duration)
{
	if (PopupWidgetClass) {
		UWotUWPopup* PopupWidget = CreateWidget<UWotUWPopup>(GetWorld(), PopupWidgetClass);
		PopupWidget->SetDuration(Duration);
		PopupWidget->SetText(Text);
		PopupWidget->SetAttachTo(this);
		PopupWidget->PlayPopupAnimation();
		PopupWidget->AddToViewport();
	}
}

void AWotAICharacter::SetBlackboardActor(const FString BlackboardKeyName, AActor* Actor)
{
  // set the instigator as the damage actor
  AAIController* AIC = Cast<AAIController>(GetController());
  if (AIC) {
    UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
    BBComp->SetValueAsObject(FName(*BlackboardKeyName), Actor);
  }
}

void AWotAICharacter::OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta)
{
  // set the instigator as the damage actor
  SetBlackboardActor("DamageActor", InstigatorActor);
  SetBlackboardActor("TargetActor", InstigatorActor);
  // Then forget damage actor after a delay
  GetWorldTimerManager().SetTimer(TimerHandle_ForgetDamageActor,
                                  this,
                                  &AWotAICharacter::ForgetDamageActor_TimeElapsed,
                                  DamageActorForgetDelay);
  // and show the health widgets
	ShowHealthBarWidget(NewHealth, Delta, 1.0f);
	ShowPopupWidgetNumber(Delta, 1.0f);
  // and flash that we were hit
  if (Delta < 0.0f) {
		HitFlash();
    // TODO: how do we want to apply stun effect?
  }
}

void AWotAICharacter::OnKilled(AActor* InstigatorActor, UWotAttributeComponent* OwningComp)
{
	// turn off collision & physics
	TurnOff(); // freezes the pawn state
	GetCapsuleComponent()->SetSimulatePhysics(false);
	GetCapsuleComponent()->SetCollisionProfileName("NoCollision");
	SetActorEnableCollision(false);
	// ragdoll the mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("Ragdoll", true);
	// detatch any attached actors and enable physics on them
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, false, true);
	for (auto& attached : AttachedActors) {
		// detach actor
		attached->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    // get primitive components of this actor
    TArray<UPrimitiveComponent*> PrimitiveComps;
    attached->GetComponents(PrimitiveComps, false);
    for (auto& PrimitiveComp : PrimitiveComps) {
      // TODO: this is a hack to make arrows and such simulate again; probably
      // should only do this for specific actor types and use helper functions?
      PrimitiveComp->SetCollisionProfileName("Item");
      PrimitiveComp->SetSimulatePhysics(true);
    }
	}
  // Stop the behavior tree
	AAIController* AIC = Cast<AAIController>(GetController());
  if (AIC) {
    AIC->GetBrainComponent()->StopLogic("Killed");
  }
  // If we wanted to ragdoll we could call
	// GetMesh()->SetAllBodiesSimulatePhysics(true);
	// GetMesh()->SetCollisionProfileName("Ragdoll");
	// Play the death component animation
	DeathEffectComp->Play();
	// hide the mesh so only the death animation plays
	GetMesh()->SetVisibility(false, false);
  // Drop all items the character is carrying
  InventoryComp->DropAll();
	// Then destroy after a delay (could also use SetLifeSpan(...) instead of timer)
	GetWorldTimerManager().SetTimer(TimerHandle_Destroy, this, &AWotAICharacter::Destroy_TimeElapsed, KilledDestroyDelay);
}

void AWotAICharacter::Destroy_TimeElapsed()
{
	Destroy();
}

void AWotAICharacter::ForgetDamageActor_TimeElapsed()
{
  SetBlackboardActor("DamageActor", nullptr);
}
