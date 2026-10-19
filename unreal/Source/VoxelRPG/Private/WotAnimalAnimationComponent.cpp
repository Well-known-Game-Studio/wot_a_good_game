#include "WotAnimalAnimationComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

UWotAnimalAnimationComponent::UWotAnimalAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

USkeletalMeshComponent* UWotAnimalAnimationComponent::GetMesh() const
{
	if (ACharacter* Char = Cast<ACharacter>(GetOwner())) {
		return Char->GetMesh();
	}
	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

void UWotAnimalAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	USkeletalMeshComponent* Mesh = GetMesh();
	if (!Mesh || (!IdleAnim && !WalkAnim && !RunAnim)) {
		SetComponentTickEnabled(false);
		return;
	}
	if (!bOverrideAnimBlueprint && Mesh->GetAnimationMode() == EAnimationMode::AnimationBlueprint && Mesh->GetAnimInstance()) {
		SetComponentTickEnabled(false);
		return;
	}
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Play(IdleAnim ? IdleAnim : WalkAnim, 1.0f);
}

void UWotAnimalAnimationComponent::Play(UAnimSequenceBase* Anim, float Rate)
{
	USkeletalMeshComponent* Mesh = GetMesh();
	if (!Mesh || !Anim) {
		return;
	}
	if (Anim != CurrentAnim) {
		CurrentAnim = Anim;
		Mesh->PlayAnimation(Anim, true);
	}
	Mesh->SetPlayRate(Rate);
}

void UWotAnimalAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}
	const float Speed = Owner->GetVelocity().Size2D();
	if (Speed >= RunSpeed && RunAnim) {
		Play(RunAnim, bScalePlayRateWithSpeed ? FMath::Clamp(Speed / RunReferenceSpeed, 0.6f, 2.0f) : 1.0f);
	} else if (Speed >= WalkSpeed && WalkAnim) {
		Play(WalkAnim, bScalePlayRateWithSpeed ? FMath::Clamp(Speed / WalkReferenceSpeed, 0.5f, 2.5f) : 1.0f);
	} else if (IdleAnim) {
		Play(IdleAnim, 1.0f);
	}
}
