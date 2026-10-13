#include "WotMountComponent.h"
#include "WotCharacter.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Animation/AnimSequenceBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotMount, Log, All);

UWotMountComponent::UWotMountComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.1f;
	MountDisplayName = FText::FromString(TEXT("horse"));
}

UWotMountComponent* UWotMountComponent::FindMount(AActor* Actor)
{
	if (!Actor) {
		return nullptr;
	}
	return Actor->FindComponentByClass<UWotMountComponent>();
}

bool UWotMountComponent::Mount(AWotCharacter* NewRider)
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !NewRider || Rider || NewRider->IsMounted()) {
		return false;
	}
	UCharacterMovementComponent* MountMove = Owner->GetCharacterMovement();
	UCharacterMovementComponent* RiderMove = NewRider->GetCharacterMovement();
	if (!MountMove || !RiderMove) {
		return false;
	}

	// Suspend the animal's AI while it is ridden.
	if (AAIController* AI = Cast<AAIController>(Owner->GetController())) {
		if (UBrainComponent* Brain = AI->GetBrainComponent()) {
			Brain->StopLogic(TEXT("Mounted"));
		}
		AI->StopMovement();
		AI->UnPossess();
		SuspendedAIController = AI;
	}

	SavedMaxWalkSpeed = MountMove->MaxWalkSpeed;
	bSavedOrientRotationToMovement = MountMove->bOrientRotationToMovement;
	bSavedRunPhysicsWithNoController = MountMove->bRunPhysicsWithNoController;
	bSavedUseControllerRotationYaw = Owner->bUseControllerRotationYaw;
	MountMove->MaxWalkSpeed = MountedMaxWalkSpeed;
	MountMove->bOrientRotationToMovement = true;
	MountMove->bRunPhysicsWithNoController = true;
	Owner->bUseControllerRotationYaw = false;
	MountMove->StopMovementImmediately();

	// The two bodies must ignore each other.
	Owner->MoveIgnoreActorAdd(NewRider);
	NewRider->MoveIgnoreActorAdd(Owner);

	// Park the rider's own movement and collision.
	RiderMove->StopMovementImmediately();
	RiderMove->SetMovementMode(MOVE_None);
	if (UCapsuleComponent* Capsule = NewRider->GetCapsuleComponent()) {
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	USceneComponent* AttachTo = Owner->GetRootComponent();
	FName Socket = NAME_None;
	if (USkeletalMeshComponent* Mesh = Owner->GetMesh()) {
		if (Mesh->DoesSocketExist(SaddleSocketName)) {
			AttachTo = Mesh;
			Socket = SaddleSocketName;
		}
	}
	NewRider->AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	NewRider->SetActorRelativeLocation(RiderOffset);
	NewRider->SetActorRelativeRotation(FRotator::ZeroRotator);

	Rider = NewRider;
	NewRider->SetMount(this);
	CurrentRiderAnim = nullptr;
	PlayRiderAnim(RiderIdleAnim);
	SetComponentTickEnabled(true);
	UE_LOG(LogWotMount, Log, TEXT("%s mounted %s"), *GetNameSafe(NewRider), *GetNameSafe(Owner));
	OnMounted.Broadcast(this, NewRider);
	return true;
}

bool UWotMountComponent::Dismount()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !Rider) {
		return false;
	}
	AWotCharacter* R = Rider;
	Rider = nullptr;
	SetComponentTickEnabled(false);

	// hand the rider's mesh back to its animation Blueprint
	if (CurrentRiderAnim) {
		if (USkeletalMeshComponent* RiderMesh = R->GetMesh()) {
			RiderMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}
		CurrentRiderAnim = nullptr;
	}

	R->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Place the rider beside the mount, on the ground.
	float HalfHeight = 88.0f;
	if (UCapsuleComponent* Capsule = R->GetCapsuleComponent()) {
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}
	FVector Loc = Owner->GetActorLocation() + Owner->GetActorRightVector() * DismountSideOffset;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WotDismount), false);
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(R);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Loc + FVector(0, 0, 300.0f), Loc - FVector(0, 0, 800.0f), ECC_Visibility, Params)) {
		Loc = Hit.Location + FVector(0, 0, HalfHeight + 2.0f);
	}
	R->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
	R->SetActorRotation(FRotator(0.0f, Owner->GetActorRotation().Yaw, 0.0f));
	if (UCapsuleComponent* Capsule = R->GetCapsuleComponent()) {
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (UCharacterMovementComponent* RiderMove = R->GetCharacterMovement()) {
		RiderMove->SetMovementMode(MOVE_Walking);
	}
	Owner->MoveIgnoreActorRemove(R);
	R->MoveIgnoreActorRemove(Owner);

	if (UCharacterMovementComponent* MountMove = Owner->GetCharacterMovement()) {
		MountMove->StopMovementImmediately();
		MountMove->MaxWalkSpeed = SavedMaxWalkSpeed;
		MountMove->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		MountMove->bRunPhysicsWithNoController = bSavedRunPhysicsWithNoController;
	}
	Owner->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;

	if (SuspendedAIController) {
		SuspendedAIController->Possess(Owner);
		if (UBrainComponent* Brain = SuspendedAIController->GetBrainComponent()) {
			Brain->RestartLogic();
		}
		SuspendedAIController = nullptr;
	}

	R->SetMount(nullptr);
	UE_LOG(LogWotMount, Log, TEXT("%s dismounted %s"), *GetNameSafe(R), *GetNameSafe(Owner));
	OnDismounted.Broadcast(this, R);
	return true;
}

void UWotMountComponent::AddRiderMovementInput(FVector WorldDirection, float Scale)
{
	if (APawn* Owner = Cast<APawn>(GetOwner())) {
		Owner->AddMovementInput(WorldDirection, Scale);
	}
}

void UWotMountComponent::SetSprinting(bool bSprinting)
{
	if (!Rider) {
		return;
	}
	if (ACharacter* Owner = Cast<ACharacter>(GetOwner())) {
		if (UCharacterMovementComponent* MountMove = Owner->GetCharacterMovement()) {
			MountMove->MaxWalkSpeed = bSprinting ? MountedSprintSpeed : MountedMaxWalkSpeed;
		}
	}
}

void UWotMountComponent::PlayRiderAnim(UAnimSequenceBase* Anim)
{
	if (!Rider || !Anim || Anim == CurrentRiderAnim) {
		return;
	}
	USkeletalMeshComponent* RiderMesh = Rider->GetMesh();
	if (!RiderMesh) {
		return;
	}
	RiderMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	RiderMesh->PlayAnimation(Anim, true);
	CurrentRiderAnim = Anim;
}

void UWotMountComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Rider) {
		return;
	}
	const float Speed = GetOwner() ? GetOwner()->GetVelocity().Size2D() : 0.0f;
	PlayRiderAnim(Speed > 30.0f && RiderMoveAnim ? RiderMoveAnim : RiderIdleAnim);
}

void UWotMountComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Rider) {
		Dismount();
	}
	Super::EndPlay(EndPlayReason);
}
