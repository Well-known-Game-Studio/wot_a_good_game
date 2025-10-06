// Fill out your copyright notice in the Description page of Project Settings.
#include "WotCharacter.h"
#include "WotAttributeComponent.h"
#include "WotEquipmentComponent.h"
#include "WotInventoryComponent.h"
#include "WotDeathEffectComponent.h"
#include "WotInteractionComponent.h"
#include "WotActionComponent.h"
#include "WotGameplayInterface.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Color.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/EngineTypes.h"
#include "Blueprint/UserWidget.h"
#include "UI/WotUWInventoryPanel.h"
#include "UI/WotUWHealthBar.h"
#include "UI/WotUWPopupNumber.h"
#include "Items/WotItem.h"
#include "Items/WotItemWeapon.h"
#include "Components/AudioComponent.h"

// Sets default values
AWotCharacter::AWotCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraLensSettings.MinFocalLength = 4.0f; // mm
	CameraLensSettings.MaxFocalLength = 1000.0f; // mm
	CameraLensSettings.MinFStop = 1.2f;
	CameraLensSettings.MaxFStop = 22.0;
	CameraLensSettings.DiaphragmBladeCount = 7;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);

	CineCameraComp = CreateDefaultSubobject<UCineCameraComponent>("CineCameraComp");
	CineCameraComp->SetupAttachment(SpringArmComp);

	InteractionComp = CreateDefaultSubobject<UWotInteractionComponent>("InteractionComp");

	AttributeComp = CreateDefaultSubobject<UWotAttributeComponent>("AttributeComp");

	EquipmentComp = CreateDefaultSubobject<UWotEquipmentComponent>("EquipmentComp");

	InventoryComp = CreateDefaultSubobject<UWotInventoryComponent>("InventoryComp");

	DeathEffectComp = CreateDefaultSubobject<UWotDeathEffectComponent>("DeathEffectComp");

	ActionComp = CreateDefaultSubobject<UWotActionComponent>("ActionComp");

	GetCharacterMovement()->bOrientRotationToMovement = true;

	EffectAudioComp = CreateDefaultSubobject<UAudioComponent>("EffectAudioComp");
	EffectAudioComp->SetupAttachment(RootComponent);

	bUseControllerRotationYaw = false;
}

void AWotCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AttributeComp->OnHealthChanged.AddDynamic(this, &AWotCharacter::OnHealthChanged);
	AttributeComp->OnKilled.AddDynamic(this, &AWotCharacter::OnKilled);
}

// Called when the game starts or when spawned
void AWotCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetupSpringArm();
	SetupCineCamera();
	// start the interaction check timer
	GetWorldTimerManager().SetTimer(TimerHandle_InteractionCheck, this, &AWotCharacter::InteractionCheck_TimeElapsed, InteractionCheckPeriod, true);
}

void AWotCharacter::SetupSpringArm()
{
	// SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->TargetArmLength = CameraDistance; // mm

	SpringArmComp->bDoCollisionTest = false;

	SpringArmComp->bInheritPitch = false;
	SpringArmComp->bInheritYaw = false;
	SpringArmComp->bInheritRoll = false;

	auto Rotation = FRotator(-50.0f, -45.0f, 0.0f); // PYR
	SpringArmComp->SetRelativeRotation(Rotation, false, nullptr, ETeleportType::None);
}

void AWotCharacter::SetupCineCamera()
{
	if (bUseSquareAspectRatio) {
		FCameraFilmbackSettings FilmbackSettings;
		FilmbackSettings.SensorHeight = 500.0f; // mm
		FilmbackSettings.SensorWidth = 500.0f; // mm
		CineCameraComp->Filmback = FilmbackSettings;
	}

	CineCameraComp->LensSettings = CameraLensSettings;

	FCameraFocusSettings FocusSettings;
	// FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
	FocusSettings.ManualFocusDistance = CameraDistance; // mm
	CineCameraComp->FocusSettings = FocusSettings;

	CineCameraComp->CurrentFocalLength = CurrentFocalLength;
	CineCameraComp->CurrentAperture = CurrentAperture;
}

FVector AWotCharacter::GetPawnViewLocation() const
{
	// for now we'll keep using the parent's version, which will return actor
	// location + eye height offset
	return Super::GetPawnViewLocation();
}

// _Implementation from it being marked as BlueprintNativeEvent
void AWotCharacter::PrimaryInteract_Implementation()
{
	if (!InputEnabled()) {
		return;
	}
	if (InteractionComp)
	{
		InteractionComp->PrimaryInteract();
	}
}

bool AWotCharacter::IsClimbing() const
{
	return CurrentLadderActor != nullptr;
}

void AWotCharacter::HandleMovementInput_Implementation(const FVector &MoveDirection, const FVector &LookDirection)
{
	Move(MoveDirection);
	Look(LookDirection);
}

void AWotCharacter::GetCharacterMovementAxes_Implementation(FVector& OutForward, FVector& OutRight) const {
	// we move the character based on the inputs received in the top-down
	// camera's coordinate system, so get the camera spring arm transform
	// (rotation) and determine its 2d vector
	auto t = SpringArmComp->GetRelativeTransform();
	auto r = t.Rotator();
	auto forward = UKismetMathLibrary::CreateVectorFromYawPitch(r.Yaw, r.Pitch, 1.0f);
	forward.Z = 0;
	auto right = forward.RotateAngleAxis(90.0f, {0, 0, 1.0});
	right.Z = 0;
	// set the output parameters
	OutForward = forward;
	OutRight = right;
}

bool AWotCharacter::Move_Implementation(const FVector &MoveVector)
{
	if (!InputEnabled()) {
		return false;
	}
	if (MoveVector.Size() < 0.25f) {
		// no movement input, so we don't do anything
		return false;
	}

	// if the player is on a ladder, we want to move the player up/down the
	// ladder, so we don't want to do anything else
	if (IsClimbing()) {
		// We have the LadderActor, so get its extents and forward vector. The
		// forward vector for the ladder points normal to the ladder rungs (i.e.
		// out from the ladder). We use this vector to determine if the ladder
		// is still in front of the player (if the user is pressing forward) or
		// if the ground is still below the player (if the user is pressing
		// backward)

		// determine forward/backward direction w.r.t. the ladder
		FVector LadderForward = CurrentLadderActor->GetActorForwardVector();
		FVector LadderUp = CurrentLadderActor->GetActorUpVector();
		// this is the direction from the player to the ladder
		FVector ToLadder = LadderForward * -1.0f;

		// rotate the character to face the ladder
		FRotator TargetRot = ToLadder.ToOrientationRotator();
		SetActorRotation(TargetRot);

		// ensure we negate any existing non-ladder aligned velocity by
		// dot-product with the ladder up vector
		auto vel = GetCharacterMovement()->Velocity;
		GetCharacterMovement()->Velocity = vel.ProjectOnTo(LadderUp);

		// tell the pawn controller to actual move accordingly. we only move in
		// the z axis, so if the user is pressing forward or backward, we move
		// up or down the ladder if the user is pressing left or right, we don't
		// move at all
		float forward_amount = MoveVector.Y;
		FVector LadderMoveVector = LadderUp;
		AddMovementInput(LadderMoveVector, forward_amount);

		// do a line trace in front of the character to determine if the ladder
		// is still there. If not, then we shold move forward off the ladder (if
		// the forward_amount is positive)
		FVector Start = GetActorLocation();
		FVector ForwardEnd = Start + ToLadder * 100.0f;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		FHitResult HitForward;
		bool bHitLadder = GetWorld()->LineTraceSingleByChannel(HitForward, Start, ForwardEnd, ECC_Visibility, QueryParams);

		// do a line trace below the character to determine if the ground is
		// there. If not, then we should move backward off the ladder (if the
		// forward_amount is negative)
		FVector DownEnd = Start - LadderUp * 100.0f;
		FHitResult HitDown;
		bool bHitGround = GetWorld()->LineTraceSingleByChannel(HitDown, Start, DownEnd, ECC_Visibility, QueryParams);

		float move_off_amount = 0.0f;

		if (forward_amount > 0.0f) {
			// user is pressing forward, so only stay on the ladder if it's
			// still in front of the player
			if (!bHitLadder) {
				// ladder is no longer in front of the player, so move off the
				// ladder and stop climbing
				move_off_amount = 100.0f;
			}
		} else if (forward_amount < 0.0f) {
			// user is pressing backward, so only stay on the ladder if the
			// ground is still below the player
			if (!bHitGround) {
				// ground is no longer below the player, so move off the ladder
				// and stop climbing
				move_off_amount = -100.0f;
			}
		}

		if (move_off_amount != 0.0f) {
			AddMovementInput(ToLadder, move_off_amount);
		}

		// we're done here
		return true;
	}

	// now get the user's input movement commands
	auto move_value = MoveVector;
	// limit the speed of the player to max speed (e.g. vector length should be
	// <= 1.0)
	if (move_value.Size() > 1.0f) {
		move_value = move_value.GetSafeNormal();
	}

	FVector right, up;
	GetCharacterMovementAxes(up, right);

	// tell the pawn controller to actual move accordingly
	AddMovementInput(right, move_value.X);
	AddMovementInput(up, move_value.Y);

	return true;
}

bool AWotCharacter::Look_Implementation(const FVector &LookDirection)
{
	if (!InputEnabled()) {
		return false;
	}
	if (LookDirection.Size() < 0.25f) {
		// no look input, so we don't do anything
		return false;
	}

	if (IsClimbing()) {
		// if we're on a ladder, we don't want to rotate the character
		return false;
	}

	FVector right, up;
	GetCharacterMovementAxes(up, right);

	// now get the user's input turn commands
	auto look_vector = up * LookDirection.Y + right * LookDirection.X;
	SetActorRotation(look_vector.GetSafeNormal().ToOrientationRotator());
	return true;
}

void AWotCharacter::ActionStart(FName ActionName)
{
	if (IsInventoryWidgetOpen()) {
		return;
	}
	if (!InputEnabled()) {
		return;
	}
	ActionComp->StartActionByName(ActionName, this);
}

void AWotCharacter::ActionStop(FName ActionName)
{
	ActionComp->StopActionByName(ActionName, this);
}

// Called every frame
void AWotCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// // Get the input vector axes for move and look
	// FVector MoveVector = GetInputVectorAxisValue("IA_Move");
	// FVector LookVector = GetInputVectorAxisValue("IA_Look");

	// HandleMovementInput(MoveVector, LookVector);
}

// Called to bind functionality to input
void AWotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// // We're interested in knowing the axis value, but don't need a delegate for
	// // it (we read it in the tick event)
	// PlayerInputComponent->BindVectorAxis("IA_Move");
	// PlayerInputComponent->BindVectorAxis("IA_Look");

	// PlayerInputComponent->BindAction<FActionDelegate>("IA_Sprint", IE_Pressed, this, &AWotCharacter::ActionStart, FName("Sprint"));
	// PlayerInputComponent->BindAction<FActionDelegate>("IA_Sprint", IE_Released, this, &AWotCharacter::ActionStop, FName("Sprint"));
	// PlayerInputComponent->BindAction<FActionDelegate>("IA_Dash", IE_Pressed, this, &AWotCharacter::ActionStart, FName("Dash"));
	// PlayerInputComponent->BindAction<FActionDelegate>("IA_Dash", IE_Released, this, &AWotCharacter::ActionStop, FName("Dash"));
	// PlayerInputComponent->BindAction<FActionDelegate>("IA_Jump", IE_Pressed, this, &AWotCharacter::ActionStart, FName("Jump"));
	// PlayerInputComponent->BindAction<FActionDelegate>("IA_Jump", IE_Released, this, &AWotCharacter::ActionStop, FName("Jump"));

	// PlayerInputComponent->BindAction("IA_Attack", IE_Pressed, this, &AWotCharacter::PrimaryAttack);
	// PlayerInputComponent->BindAction("IA_Attack", IE_Released, this, &AWotCharacter::PrimaryAttackStop);

	// PlayerInputComponent->BindAction("IA_Interact", IE_Pressed, this, &AWotCharacter::PrimaryInteract);

	// PlayerInputComponent->BindAction("IA_Camera", IE_Pressed, this, &AWotCharacter::RotateCamera);
}

void AWotCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	// spawn a particle effect when we land (if it has been set)
	if (LandingEffect) {
		// spawn it at the impact point
		auto LandingSystemComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,
			LandingEffect, Hit.ImpactPoint, FRotator::ZeroRotator);
	}
}

void AWotCharacter::HitFlash()
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

void AWotCharacter::HealSelf(float Amount /* = 100 */)
{
	AttributeComp->ApplyHealthChangeInstigator(this, Amount);
}

void AWotCharacter::OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta)
{
	ShowHealthBarWidget(NewHealth, Delta, 1.0f);
	ShowPopupWidgetNumber(Delta, 1.0f);
	if (Delta < 0.0f) {
		HitFlash();
	}
	if (NewHealth <= 0.0f) {
		auto PC = Cast<APlayerController>(GetController());
		DisableInput(PC);
	}
}

void AWotCharacter::OnKilled(AActor* InstigatorActor, UWotAttributeComponent* OwningComp)
{
	// turn off collision & physics
	TurnOff(); // freezes the pawn state
	GetCapsuleComponent()->SetSimulatePhysics(false);
	GetCapsuleComponent()->SetCollisionProfileName("NoCollision");
	SetActorEnableCollision(false);
	// ragdoll the mesh
	GetMesh()->SetCollisionProfileName("Ragdoll", true);
	GetMesh()->SetSimulatePhysics(true);
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
	// Unequip all items, so they can be dropped
	EquipmentComp->UnequipAll();
	// Drop all items the character is carrying
	InventoryComp->DropAll();
	// Then destroy after a delay
	GetWorldTimerManager().SetTimer(TimerHandle_Destroy, this, &AWotCharacter::Destroy_TimeElapsed, KilledDestroyDelay);
}

bool AWotCharacter::ShowInventoryWidget_Implementation(UWotUWInventoryPanel*& OutInventoryWidget)
{
	// Now actually try to open the menu
	if (CanOpenInventory()) {
		InventoryWidget = CreateWidget<UWotUWInventoryPanel>(GetWorld(), InventoryWidgetClass);
		InventoryWidget->SetInventory(InventoryComp, FText::FromString("Your Items"));
		InventoryWidget->AddToViewport();
		OutInventoryWidget = InventoryWidget;
		return true;
	}
	return false;
}

void AWotCharacter::SetInventoryWidget(UWotUWInventoryPanel* NewInventoryWidget)
{
	// Set the inventory widget reference
	InventoryWidget = NewInventoryWidget;
}

void AWotCharacter::CloseInventoryWidget_Implementation()
{
	InventoryWidget = nullptr;
}

bool AWotCharacter::IsInventoryWidgetOpen() const
{
	// Check if the inventory widget is set and is currently visible
	return InventoryWidget != nullptr;
}

bool AWotCharacter::CanOpenInventory() const {
	return !IsInventoryWidgetOpen() && !ActionComp->IsAnyActionRunning();
}

void AWotCharacter::PlaySoundGet()
{
    EffectAudioComp->SetSound(GetSound);
    EffectAudioComp->Play(0);
}

void AWotCharacter::RotateCamera_Implementation(float YawDelta, float PitchDelta)
{
	if (IsInventoryWidgetOpen()) {
		return;
	}
	if (!InputEnabled()) {
		return;
	}
	// rotate the camera spring arm by the given deltas
	auto Rotation = SpringArmComp->GetRelativeRotation();
	Rotation.Yaw += YawDelta;
	Rotation.Pitch += PitchDelta;
	SpringArmComp->SetRelativeRotation(Rotation, false, nullptr, ETeleportType::None);
}

void AWotCharacter::ShowHealthBarWidget(float NewHealth, float Delta, float Duration)
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

void AWotCharacter::ShowPopupWidgetNumber(int Number, float Duration, bool Animated)
{
	if (PopupWidgetClass) {
		UWotUWPopupNumber* PopupWidget = CreateWidget<UWotUWPopupNumber>(GetWorld(), PopupWidgetClass);
		PopupWidget->SetDuration(Duration);
		PopupWidget->SetNumber(Number);
		PopupWidget->SetAttachTo(this);
		if (Animated) {
			PopupWidget->PlayPopupAnimation();
		}
		PopupWidget->AddToViewport();
	}
}

void AWotCharacter::ShowPopupWidget(const FText& Text, float Duration, bool Animated)
{
	if (PopupWidgetClass) {
		UWotUWPopup* PopupWidget = CreateWidget<UWotUWPopup>(GetWorld(), PopupWidgetClass);
		PopupWidget->SetDuration(Duration);
		PopupWidget->SetText(Text);
		PopupWidget->SetAttachTo(this);
		if (Animated) {
			PopupWidget->PlayPopupAnimation();
		}
		PopupWidget->AddToViewport();
	}
}

void AWotCharacter::ShowPopupWidgetAttachedTo(const FText& Text, float Duration, AActor* Actor, const FVector& Offset, bool Animated)
{
	if (PopupWidgetClass) {
		UWotUWPopup* PopupWidget = CreateWidget<UWotUWPopup>(GetWorld(), PopupWidgetClass);
		PopupWidget->SetDuration(Duration);
		PopupWidget->SetText(Text);
		PopupWidget->SetOffset(Offset);
		PopupWidget->SetAttachTo(Actor);
		if (Animated) {
			PopupWidget->PlayPopupAnimation();
		}
		PopupWidget->AddToViewport();
	}
}

void AWotCharacter::ShowInteractionWidget(const FText& Text, float Duration, bool Animated)
{
	if (InteractionWidgetClass) {
		UWotUWPopup* InteractionWidget = CreateWidget<UWotUWPopup>(GetWorld(), InteractionWidgetClass);
		InteractionWidget->SetDuration(Duration);
		InteractionWidget->SetText(Text);
		InteractionWidget->SetAttachTo(this);
		if (Animated) {
			InteractionWidget->PlayPopupAnimation();
		}
		InteractionWidget->AddToViewport();
	}
}

void AWotCharacter::ShowInteractionWidgetAttachedTo(const FText& Text, float Duration, AActor* Actor, const FVector& Offset, bool Animated)
{
	if (InteractionWidgetClass) {
		UWotUWPopup* InteractionWidget = CreateWidget<UWotUWPopup>(GetWorld(), InteractionWidgetClass);
		InteractionWidget->SetDuration(Duration);
		InteractionWidget->SetText(Text);
		InteractionWidget->SetOffset(Offset);
		InteractionWidget->SetAttachTo(Actor);
		if (Animated) {
			InteractionWidget->PlayPopupAnimation();
		}
		InteractionWidget->AddToViewport();
	}
}

void AWotCharacter::InteractionCheck_TimeElapsed()
{
	if (IsInventoryWidgetOpen()) {
		return;
	}
	if (!InputEnabled()) {
		return;
	}
	if (!InteractionComp) {
		return;
	}
	// use the interaction component to get the closest interactable
	AActor *ClosestInteractable = nullptr;
	UActorComponent *ClosestInteractionComp = nullptr;
	FHitResult HitResult;
	bool got_interactable = InteractionComp->GetInteractableInRange(ClosestInteractable, ClosestInteractionComp, HitResult);

	// check if the interactible / interaction comp is the same as what we had,
	// and if not, then unhighlight them
	if (InteractionTargetComponent != nullptr) {
		if (InteractionTargetComponent->Implements<UWotGameplayInterface>()) {
			IWotGameplayInterface::Execute_Unhighlight(InteractionTargetComponent, InteractionTargetHitResult);
		}
		InteractionTargetComponent = nullptr;
		InteractionTargetHitResult = FHitResult();
	}
	if (InteractionTargetActor != nullptr) {
		if (InteractionTargetActor->Implements<UWotGameplayInterface>()) {
			IWotGameplayInterface::Execute_Unhighlight(InteractionTargetActor, InteractionTargetHitResult);
		}
		InteractionTargetActor = nullptr;
		InteractionTargetHitResult = FHitResult();
	}

	// if we didn't get an interactable, then we don't need to do anything else
	if (!got_interactable) {
		return;
	}
	FText InteractionText;
	FVector Offset(0, 0, 0);

	// if we got one, use the WotInteractableInterface to call GetInteractionText
	if (ClosestInteractionComp) {
		// we got a component, so use the component
		IWotInteractableInterface::Execute_GetInteractionText(ClosestInteractionComp, this, HitResult, InteractionText);
		// Save the offset of the hit location from the actor
		Offset = HitResult.Location - ClosestInteractable->GetActorLocation();
	} else {
		// we only got an actor, so use the actor
		IWotInteractableInterface::Execute_GetInteractionText(ClosestInteractable, this, HitResult, InteractionText);
	}

	// if the text is empty, don't show the widget
	if (InteractionText.IsEmpty()) {
		return;
	}
	// show the action text widget
	ShowInteractionWidgetAttachedTo(FText::FromString(InteractionText.ToString()),
									InteractionCheckPeriod*1.1f,
									ClosestInteractable,
									Offset,
									false);

	// Use the highlight interface if it can be used
	if (ClosestInteractionComp) {
		if (ClosestInteractionComp->Implements<UWotGameplayInterface>()) {
			IWotGameplayInterface::Execute_Highlight(ClosestInteractionComp, HitResult, 1, 0.0f);
			InteractionTargetComponent = ClosestInteractionComp;
			InteractionTargetHitResult = HitResult;
		}
	} else {
		if (ClosestInteractable->Implements<UWotGameplayInterface>()) {
			IWotGameplayInterface::Execute_Highlight(ClosestInteractable, HitResult, 1, 0.0f);
			InteractionTargetActor = ClosestInteractable;
			InteractionTargetHitResult = HitResult;
		}
	}
}

void AWotCharacter::Destroy_TimeElapsed()
{
	// Store the controller reference
	// AController* Controller = GetController();
	// Destroy the current player
	Destroy();
}
