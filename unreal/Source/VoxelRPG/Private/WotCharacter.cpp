// Fill out your copyright notice in the Description page of Project Settings.


#include "WotCharacter.h"
#include "WotAttributeComponent.h"
#include "WotInteractionComponent.h"
#include "CineCameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Color.h"

// For Debug:
#include "DrawDebugHelpers.h"

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

	GetCharacterMovement()->bOrientRotationToMovement = true;

	bUseControllerRotationYaw = false;
}

void AWotCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AttributeComp->OnHealthChanged.AddDynamic(this, &AWotCharacter::OnHealthChanged);
}

// Called when the game starts or when spawned
void AWotCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWotCharacter::SetupSpringArm()
{
	// SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->TargetArmLength = 1400.0f; // mm

	SpringArmComp->bDoCollisionTest = false;

	SpringArmComp->bInheritPitch = false;
	SpringArmComp->bInheritYaw = false;
	SpringArmComp->bInheritRoll = false;

	auto Rotation = FRotator(-50.0f, -45.0f, 0.0f); // PYR
	SpringArmComp->SetRelativeRotation(Rotation, false, nullptr, ETeleportType::None);
}

void AWotCharacter::SetupCineCamera()
{
	FCameraFilmbackSettings FilmbackSettings;
	FilmbackSettings.SensorHeight = 500.0f; // mm
	FilmbackSettings.SensorWidth = 500.0f; // mm
	CineCameraComp->Filmback = FilmbackSettings;

	FCameraLensSettings LensSettings;
	LensSettings.MinFocalLength = 4.0f; // mm
	LensSettings.MaxFocalLength = 1000.0f; // mm
	LensSettings.MinFStop = 1.2f;
	LensSettings.MaxFStop = 22.0;
	LensSettings.DiaphragmBladeCount = 7;
	CineCameraComp->LensSettings = LensSettings;

	FCameraFocusSettings FocusSettings;
	// FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
	FocusSettings.ManualFocusDistance = 1450.0f; // mm
	CineCameraComp->FocusSettings = FocusSettings;

	CineCameraComp->CurrentFocalLength = 500.0f; // mm
	CineCameraComp->CurrentAperture = 1.2;
}

void AWotCharacter::MoveForward(float value)
{
	auto control_rot = GetControlRotation();
	control_rot.Pitch = 0.0f;
	control_rot.Roll = 0.0f;
	AddMovementInput(control_rot.Vector(), value);
}

void AWotCharacter::MoveRight(float value)
{
	auto control_rot = GetControlRotation();
	control_rot.Pitch = 0.0f;
	control_rot.Roll = 0.0f;

	// using the kismet (old name for blueprint) math library:
	// auto right_vector = UKismetMathLibrary::GetRightVector(control_rot);

	// is the same as this:
	auto right_vector = FRotationMatrix(control_rot).GetScaledAxis(EAxis::Y);
	AddMovementInput(right_vector, value);
}

void AWotCharacter::PrimaryAttack()
{
	PlayAnimMontage(AttackAnim);

	GetWorldTimerManager().SetTimer(TimerHandle_PrimaryAttack, this, &AWotCharacter::PrimaryAttack_TimeElapsed, 0.2f);
}

void AWotCharacter::PrimaryAttack_TimeElapsed()
{
	if (ensure(ProjectileClass)) {
		auto HandLocation = GetMesh()->GetSocketLocation("Hand_R");

		auto SpawnTM = FTransform(GetActorRotation(), HandLocation);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Instigator = this;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnTM, SpawnParams);
	}
}

void AWotCharacter::LightAttack()
{
}

void AWotCharacter::HeavyAttack()
{
}

void AWotCharacter::PrimaryInteract()
{
	if (InteractionComp)
	{
		InteractionComp->PrimaryInteract();
	}
}

void AWotCharacter::Drop()
{
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
	auto up = right.RotateAngleAxis(90.0f, {0, 0, 1.0});

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

// Called every frame
void AWotCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleMovementInput();

	// -- Rotation Visualization -- //
	const float DrawScale = 100.0f;
	const float Thickness = 5.0f;

	FVector LineStart = GetActorLocation();
	// Offset to the right of pawn
	LineStart += GetActorRightVector() * 100.0f;
	// Set line end in direction of the actor's forward
	FVector ActorDirection_LineEnd = LineStart + (GetActorForwardVector() * 100.0f);
	// Draw Actor's Direction
	DrawDebugDirectionalArrow(GetWorld(), LineStart, ActorDirection_LineEnd, DrawScale, FColor::Yellow, false, 0.0f, 0, Thickness);

	FVector ControllerDirection_LineEnd = LineStart + (GetControlRotation().Vector() * 100.0f);
	// Draw 'Controller' Rotation ('PlayerController' that 'possessed' this character)
	DrawDebugDirectionalArrow(GetWorld(), LineStart, ControllerDirection_LineEnd, DrawScale, FColor::Green, false, 0.0f, 0, Thickness);
}

// Called to bind functionality to input
void AWotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// PlayerInputComponent->BindAxis("MoveForward", this, &AWotCharacter::MoveForward);
	// PlayerInputComponent->BindAxis("MoveRight", this, &AWotCharacter::MoveRight);

	PlayerInputComponent->BindAction("PrimaryAttack", IE_Pressed, this, &AWotCharacter::PrimaryAttack);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AWotCharacter::PrimaryInteract);
	PlayerInputComponent->BindAction("Drop", IE_Pressed, this, &AWotCharacter::Drop);

	// Jump is an action that is already in the base class of ACharacter, so we
	// don't have to implement it
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AWotCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AWotCharacter::StopJumping);
}

void AWotCharacter::HitFlash()
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

void AWotCharacter::OnHealthChanged(AActor* InstigatorActor, UWotAttributeComponent* OwningComp, float NewHealth, float Delta)
{
	if (NewHealth <= 0.0f && Delta < 0.0f) {
		auto PC = Cast<APlayerController>(GetController());
		DisableInput(PC);
	} else if (NewHealth > 0.0f && Delta < 0.0f) {
		HitFlash();
	}
}
