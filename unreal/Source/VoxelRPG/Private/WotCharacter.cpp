// Fill out your copyright notice in the Description page of Project Settings.


#include "WotCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

// For Debug:
#include "DrawDebugHelpers.h"

// Sets default values
AWotCharacter::AWotCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);
}

// Called when the game starts or when spawned
void AWotCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWotCharacter::MoveForward(float value)
{
	// AddMovementInput(GetActorForwardVector(), value);
}

void AWotCharacter::MoveRight(float value)
{
	// AddMovementInput(GetActorRightVector(), value);
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
	// ensure that we normalize them (so that the speed is always constant)
	auto vector_value = FVector2D(up_value, right_value).GetSafeNormal();

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

	PlayerInputComponent->BindAxis("MoveForward", this, &AWotCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AWotCharacter::MoveRight);
}
