#include "AI/WotAICharacter.h"
#include "AI/WotAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "WotActionComponent.h"
#include "WotAttributeComponent.h"
#include "WotEquipmentComponent.h"
#include "WotInventoryComponent.h"
#include "WotDeathEffectComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "UI/WotUWHealthBar.h"
#include "UI/WotUWPopupNumber.h"
#include "BrainComponent.h"
#include "Items/WotItem.h"
#include "Items/WotItemWeapon.h"

AWotAICharacter::AWotAICharacter()
{
  PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");
  AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	EquipmentComp = CreateDefaultSubobject<UWotEquipmentComponent>("EquipmentComp");

	InventoryComp = CreateDefaultSubobject<UWotInventoryComponent>("InventoryComp");

  AttributeComp = CreateDefaultSubobject<UWotAttributeComponent>("AttributeComp");

  ActionComp = CreateDefaultSubobject<UWotActionComponent>("ActionComp");

	DeathEffectComp = CreateDefaultSubobject<UWotDeathEffectComponent>("DeathEffectComp");
}

void AWotAICharacter::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  PawnSensingComp->OnSeePawn.AddDynamic(this, &AWotAICharacter::OnPawnSeen);
	AttributeComp->OnHealthChanged.AddDynamic(this, &AWotAICharacter::OnHealthChanged);
	AttributeComp->OnKilled.AddDynamic(this, &AWotAICharacter::OnKilled);
  // TODO: hack for projectile / collision - should actually set up proper
  // channels with responses!
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
  GetMesh()->SetGenerateOverlapEvents(true);
}

void AWotAICharacter::Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration=0)
{
  SetHighlightEnabled(HighlightValue, true);
  // if duration is > 0, start a timer to unhighlight the object
  if (Duration > 0) {
    GetWorldTimerManager().SetTimer(HighlightTimerHandle, this, &AWotAICharacter::OnHighlightTimerExpired, Duration, false);
  }
}

void AWotAICharacter::Unhighlight_Implementation(FHitResult Hit)
{
  SetHighlightEnabled(0, false);
}

void AWotAICharacter::OnHighlightTimerExpired()
{
  // dummy hit
  FHitResult Hit;
  IWotGameplayInterface::Execute_Unhighlight(this, Hit);
}

void AWotAICharacter::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  // set the character mesh to render custom depth
  GetMesh()->SetRenderCustomDepth(Enabled);
  // set the custom depth stencil value
  GetMesh()->CustomDepthStencilValue = HighlightValue;
}

void AWotAICharacter::OnPawnSeen(APawn* Pawn)
{
  SetBlackboardActor("TargetActor", Pawn);
}

void AWotAICharacter::PrimaryAttack(AActor* TargetActor)
{
	// TODO: for now we determine whether to use weapon or action based on if we
	// have weapon equipped; there's gotta be a better way..
	UWotItemWeapon* EquippedWeapon = EquipmentComp->GetEquippedWeapon();
	if (EquippedWeapon) {
		UE_LOG(LogTemp, Log, TEXT("Got Equipped Weapon %s"), *GetNameSafe(EquippedWeapon));
		EquippedWeapon->PrimaryAttackStart();
	} else {
		UE_LOG(LogTemp, Log, TEXT("No weapon equipped starting action 'PrimaryAttack'"));
		ActionComp->StartActionByName(this, "PrimaryAttack");
	}
}

void AWotAICharacter::PrimaryAttackStop()
{
	UWotItemWeapon* EquippedWeapon = EquipmentComp->GetEquippedWeapon();
	if (EquippedWeapon) {
		EquippedWeapon->PrimaryAttackStop();
	} else {
	}
}

void AWotAICharacter::HitFlash()
{
	auto _mesh = GetMesh();
	// register that we were hit now
	_mesh->SetScalarParameterValueOnMaterials("TimeToHit", GetWorld()->GetTimeSeconds());
	// what color should we flash (emissive) - use the health to make it
	// transition from yellow to red
	auto DangerColor = FLinearColor(1.0f, 0.0f, 0.460229f, 1.0f);
	auto WarningColor = FLinearColor(0.815215f, 1.0f, 0.0f, 1.0f);
	auto Progress = AttributeComp->GetHealth() / AttributeComp->GetHealthMax();
	auto LinearColor = FLinearColor::LerpUsingHSV(DangerColor, WarningColor, Progress);
	auto HitColor = FVector4(LinearColor);
	_mesh->SetVectorParameterValueOnMaterials("HitColor", HitColor);
	// how quickly the flash should fade (1.0 = 1 second, 2.0 = 0.5 seconds)
	_mesh->SetScalarParameterValueOnMaterials("FlashTimeFactor", 2.0f);
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
  if (!AIC) {
    UE_LOG(LogTemp, Warning, TEXT("Could not get AI Controller for SetBlackboardActor!"));
    return;
  }
  UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
  BBComp->SetValueAsObject(FName(*BlackboardKeyName), Actor);
}

void AWotAICharacter::OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta)
{
  // set the instigator as the damage actor
  SetBlackboardActor("DamageActor", InstigatorActor);
  UE_LOG(LogTemp, Warning, TEXT("Run away from %s"), *GetNameSafe(InstigatorActor));
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
	GetMesh()->SetAllBodiesSimulatePhysics(true);
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
