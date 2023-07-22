// Fill out your copyright notice in the Description page of Project Settings.
#include "WotCharacter.h"
#include "WotAttributeComponent.h"
#include "WotEquipmentComponent.h"
#include "WotInventoryComponent.h"
#include "WotDeathEffectComponent.h"
#include "WotInteractionComponent.h"
#include "WotActionComponent.h"
#include "CineCameraComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Color.h"
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

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);
	SetupSpringArm();

	CineCameraComp = CreateDefaultSubobject<UCineCameraComponent>("CineCameraComp");
	CineCameraComp->SetupAttachment(SpringArmComp);
	SetupCineCamera();

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

	FCameraLensSettings LensSettings;
	LensSettings.MinFocalLength = 4.0f; // mm
	LensSettings.MaxFocalLength = 1000.0f; // mm
	LensSettings.MinFStop = 1.2f;
	LensSettings.MaxFStop = 22.0;
	LensSettings.DiaphragmBladeCount = 7;
	CineCameraComp->LensSettings = LensSettings;

	FCameraFocusSettings FocusSettings;
	// FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
	FocusSettings.ManualFocusDistance = CameraDistance; // mm
	CineCameraComp->FocusSettings = FocusSettings;

	CineCameraComp->CurrentFocalLength = 500.0f; // mm
	CineCameraComp->CurrentAperture = 1.2;
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
		ActionComp->StartActionByName(this, "PrimaryAttack");
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

void AWotCharacter::HandleMovementInput()
{
	// -- Movement Control -- //

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

void AWotCharacter::SprintStart()
{
	// TODO: probably a better way of doing this?
	bCanOpenMenu = false;
	ActionComp->StartActionByName(this, "Sprint");
}

void AWotCharacter::SprintStop()
{
	// TODO: probably a better way of doing this?
	bCanOpenMenu = true;
	ActionComp->StopActionByName(this, "Sprint");
}

void AWotCharacter::JumpStart()
{
	bCanOpenMenu = false;
	ActionComp->StartActionByName(this, "Jump");
}

void AWotCharacter::JumpStop()
{
	bCanOpenMenu = true;
	ActionComp->StopActionByName(this, "Jump");
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

	// Our movement actions (handled by BP actions)
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AWotCharacter::SprintStart);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AWotCharacter::SprintStop);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AWotCharacter::JumpStart);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AWotCharacter::JumpStop);

	PlayerInputComponent->BindAction("PrimaryAttack", IE_Pressed, this, &AWotCharacter::PrimaryAttack);
	PlayerInputComponent->BindAction("PrimaryAttack", IE_Released, this, &AWotCharacter::PrimaryAttackStop);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AWotCharacter::PrimaryInteract);

	PlayerInputComponent->BindAction("ToggleMenu", IE_Pressed, this, &AWotCharacter::ShowInventoryWidget);

	PlayerInputComponent->BindAction("ChangeCamera", IE_Pressed, this, &AWotCharacter::RotateCamera);
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

void AWotCharacter::ShowPopupWidgetNumber(int Number, float Duration)
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

void AWotCharacter::ShowPopupWidget(const FText& Text, float Duration)
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

void AWotCharacter::ShowActionTextWidget(FString Text, float Duration)
{
	if (ActionTextWidgetClass) {
		UUserWidget* ActionTextWidget = CreateWidget<UUserWidget>(GetWorld(), ActionTextWidgetClass);
		ActionTextWidget->AddToViewport();
	}
}

void AWotCharacter::Destroy_TimeElapsed()
{
	// Store the controller reference
	// AController* Controller = GetController();
	// Destroy the current player
	Destroy();
}
