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
	bCanOpenMenu = true;
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
	bCanOpenMenu = true;
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

void AWotCharacter::SetMenuActive(bool Active)
{
	bMenuActive = Active;
}

FVector AWotCharacter::GetPawnViewLocation() const
{
	// for now we'll keep using the parent's version, which will return actor
	// location + eye height offset
	return Super::GetPawnViewLocation();
}

void AWotCharacter::PrimaryAttack()
{
	// TODO: probably a better way of doing this?
	bCanOpenMenu = false;
	// TODO: for now we determine whether to use weapon or action based on if we
	// have weapon equipped; there's gotta be a better way..
	UWotItemWeapon* EquippedWeapon = EquipmentComp->GetEquippedWeapon();
	if (EquippedWeapon) {
		UE_LOG(LogTemp, Log, TEXT("Got Equipped Weapon %s"), *GetNameSafe(EquippedWeapon));
		EquippedWeapon->PrimaryAttackStart();
	} else {
		UE_LOG(LogTemp, Log, TEXT("No weapon equipped starting action 'PrimaryAttack'"));
		ActionStart("PrimaryAttack");
	}
}

void AWotCharacter::PrimaryAttackStop()
{
	// TODO: probably a better way of doing this?
	bCanOpenMenu = true;
	UWotItemWeapon* EquippedWeapon = EquipmentComp->GetEquippedWeapon();
	if (EquippedWeapon) {
		EquippedWeapon->PrimaryAttackStop();
	} else {
	}
}

void AWotCharacter::PrimaryInteract()
{
	if (InteractionComp)
	{
		InteractionComp->PrimaryInteract();
	}
}

bool AWotCharacter::IsClimbing() const
{
	return bIsOnLadder;
}

void AWotCharacter::HandleMovementInput()
{
	// -- Movement Control -- //

	// if the player is on a ladder, we want to move the player up/down the
	// ladder, so we don't want to do anything else
	if (bIsOnLadder) {
		// use the player's input to move up/down
		auto up_value = GetInputAxisValue("MoveForward");
		// tell the pawn controller to actual move accordingly
		AddMovementInput({0, 0, 1.0f}, up_value);
		// we're done here
		return;
	}

	// we move the character based on the inputs received in the top-down
	// camera's coordinate system, so get the camera spring arm transform
	// (rotation) and determine its 2d vector
	auto t = SpringArmComp->GetRelativeTransform();
	auto r = t.Rotator();
	auto right = UKismetMathLibrary::CreateVectorFromYawPitch(r.Yaw, r.Pitch, 1.0f);
	right.Z = 0;
	auto up = right.RotateAngleAxis(90.0f, {0, 0, 1.0});
	up.Z = 0;

	// now get the user's input turn commands
	auto look_up_value = GetInputAxisValue("LookUp");
	auto look_right_value = GetInputAxisValue("LookRight");
	auto look_vector = up * look_right_value + right * look_up_value;

	// use these as turn inputs if they are large enough (meaning player is
	// actually providing input)
	if (look_vector.Size() > 0.25f) {
		SetActorRotation(look_vector.ToOrientationRotator());
	}

	// now get the user's input movement commands
	auto up_value = GetInputAxisValue("MoveForward");
	auto right_value = GetInputAxisValue("MoveRight");
	auto vector_value = FVector2D(up_value, right_value);
	// limit the speed of the player to max speed (e.g. vector length should be
	// <= 1.0)
	if (vector_value.Size() > 1.0f) {
		vector_value = vector_value.GetSafeNormal();
	}

	// tell the pawn controller to actual move accordingly
	AddMovementInput(right, vector_value.X);
	AddMovementInput(up, vector_value.Y);
}

void AWotCharacter::ActionStart(FName ActionName)
{
	// TODO: probably a better way of doing this?
	bCanOpenMenu = false;
	ActionComp->StartActionByName(this, ActionName);
}

void AWotCharacter::ActionStop(FName ActionName)
{
	// TODO: probably a better way of doing this?
	bCanOpenMenu = true;
	ActionComp->StopActionByName(this, ActionName);
}

// Called every frame
void AWotCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleMovementInput();
}

// Called to bind functionality to input
void AWotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// We're interested in knowing the axis value, but don't need a delegate for
	// it (we read it in the tick event)
	PlayerInputComponent->BindAxis("MoveForward");
	PlayerInputComponent->BindAxis("MoveRight");
	PlayerInputComponent->BindAxis("LookUp");
	PlayerInputComponent->BindAxis("LookRight");

	PlayerInputComponent->BindAction<FActionDelegate>("Sprint", IE_Pressed, this, &AWotCharacter::ActionStart, FName("Sprint"));
	PlayerInputComponent->BindAction<FActionDelegate>("Sprint", IE_Released, this, &AWotCharacter::ActionStop, FName("Sprint"));
	PlayerInputComponent->BindAction<FActionDelegate>("Dash", IE_Pressed, this, &AWotCharacter::ActionStart, FName("Dash"));
	PlayerInputComponent->BindAction<FActionDelegate>("Dash", IE_Released, this, &AWotCharacter::ActionStop, FName("Dash"));
	PlayerInputComponent->BindAction<FActionDelegate>("Jump", IE_Pressed, this, &AWotCharacter::ActionStart, FName("Jump"));
	PlayerInputComponent->BindAction<FActionDelegate>("Jump", IE_Released, this, &AWotCharacter::ActionStop, FName("Jump"));

	PlayerInputComponent->BindAction("PrimaryAttack", IE_Pressed, this, &AWotCharacter::PrimaryAttack);
	PlayerInputComponent->BindAction("PrimaryAttack", IE_Released, this, &AWotCharacter::PrimaryAttackStop);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AWotCharacter::PrimaryInteract);

	PlayerInputComponent->BindAction("ToggleMenu", IE_Pressed, this, &AWotCharacter::ShowInventoryWidget);

	PlayerInputComponent->BindAction("ChangeCamera", IE_Pressed, this, &AWotCharacter::RotateCamera);
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
	bCanOpenMenu = false;
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
	// Unequip all items, so they can be dropped
	EquipmentComp->UnequipAll();
	// Drop all items the character is carrying
	InventoryComp->DropAll();
	// Then destroy after a delay
	GetWorldTimerManager().SetTimer(TimerHandle_Destroy, this, &AWotCharacter::Destroy_TimeElapsed, KilledDestroyDelay);
}

void AWotCharacter::ShowInventoryWidget()
{
	// Now actually try to open the menu
	if (bCanOpenMenu && InventoryWidgetClass) {
		UWotUWInventoryPanel* InventoryWidget = CreateWidget<UWotUWInventoryPanel>(GetWorld(), InventoryWidgetClass);
		InventoryWidget->SetInventory(InventoryComp, FText::FromString("Your Items"));
		InventoryWidget->AddToViewport();
	}
}

void AWotCharacter::PlaySoundGet()
{
    EffectAudioComp->SetSound(GetSound);
    EffectAudioComp->Play(0);
}

void AWotCharacter::RotateCamera()
{
	FRotator Rotation = SpringArmComp->GetRelativeRotation();
	Rotation.Yaw += 90;
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
	if (bMenuActive) {
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
			IWotGameplayInterface::Execute_Highlight(ClosestInteractionComp, HitResult, 1, InteractionCheckPeriod*1.1f);
		}
	} else {
		if (ClosestInteractable->Implements<UWotGameplayInterface>()) {
			IWotGameplayInterface::Execute_Highlight(ClosestInteractable, HitResult, 1, InteractionCheckPeriod*1.1f);
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
