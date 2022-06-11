#include "AI/WotAICharacter.h"
#include "AI/WotAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "WotAttributeComponent.h"
#include "WotDeathEffectComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "UI/WotUWHealthBar.h"
#include "UI/WotUWPopupNumber.h"

AWotAICharacter::AWotAICharacter()
{
  PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");
  AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

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
  AAIController* AIC = Cast<AAIController>(GetController());
  if (AIC) {
    UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
    BBComp->SetValueAsObject("TargetActor", Pawn);
  }
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

void AWotAICharacter::OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta)
{
	ShowHealthBarWidget(NewHealth, Delta, 1.0f);
	ShowPopupWidgetNumber(Delta, 1.0f);
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
		// TODO: get primitive component
		// enable physics collision
		// attached->SetCollisionEnabled();
		// TODO: set simulate physics
		// attached->SetSimulatePhysics(true);
	}
	// Play the death component animation
	DeathEffectComp->Play();
	// hide the mesh so only the death animation plays
	GetMesh()->SetVisibility(false, false);
	// Then destroy after a delay
	GetWorldTimerManager().SetTimer(TimerHandle_Destroy, this, &AWotAICharacter::Destroy_TimeElapsed, KilledDestroyDelay);
}

void AWotAICharacter::Destroy_TimeElapsed()
{
	Destroy();
}
