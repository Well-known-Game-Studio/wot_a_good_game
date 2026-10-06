// Fill out your copyright notice in the Description page of Project Settings.

#include "WotDeathEffectComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Texture.h"
#include "RHIDefinitions.h"
#include "Materials/MaterialLayersFunctions.h"

// Sets default values for this component's properties
UWotDeathEffectComponent::UWotDeathEffectComponent()
{
  SetComponentTickEnabled(false);
}

void UWotDeathEffectComponent::Play()
{
  // Set the texture for the maaterial instance we will create
  ACharacter* Character = Cast<ACharacter>(GetOwner());
  if (!ensure(Character)) {
	  UE_LOG(LogTemp, Warning, TEXT("No Character!"));
	  return;
  }
  USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
  if (!ensure(CharacterMesh)) {
	  UE_LOG(LogTemp, Warning, TEXT("No Mesh!"));
	  return;
  }
  UMaterialInterface* MeshMaterial = CharacterMesh->GetMaterial(0);
  if (!ensure(MeshMaterial)) {
	  UE_LOG(LogTemp, Warning, TEXT("No Material!"));
	  return;
  }
  // GetTextureParameterValue does not write the out value on failure, so
  // initialize it and bail out if the material has no such parameter.
  UTexture* MeshTexture = nullptr;
  if (!MeshMaterial->GetTextureParameterValue(TextureParameterName, MeshTexture) || !MeshTexture) {
	  UE_LOG(LogTemp, Warning, TEXT("Could not get texture parameter '%s' from material %s!"),
	         *TextureParameterName.ToString(), *GetNameSafe(MeshMaterial));
	  return;
  }
  if (!ensure(EffectNiagaraSystem)) {
	  UE_LOG(LogTemp, Warning, TEXT("No System!"));
	  return;
  }
  if (!ensure(EffectMaterialBase)) {
	  UE_LOG(LogTemp, Warning, TEXT("No EffectMaterialBase!"));
	  return;
  }
  // Now actually make the effect
  auto EffectNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(EffectNiagaraSystem,
																		CharacterMesh,
																		NAME_None,
																		FVector(0.f),
																		FRotator(0.f),
																		EAttachLocation::Type::KeepRelativeOffset,
																		true);
  // SpawnSystemAttached can return null (dedicated server, culled, etc.)
  if (EffectNiagaraComp) {
	  // create dynamic material instance for the mesh
	  UMaterialInstanceDynamic* EffectMaterial = UMaterialInstanceDynamic::Create(EffectMaterialBase, this);
	  if (EffectMaterial) {
		  // set the texture for the new material
		  EffectMaterial->SetTextureParameterValue("Color Texture", MeshTexture);
		  // Set the material for the meshes (cubes) in the niagara effect
		  EffectNiagaraComp->SetVariableMaterial("Material", EffectMaterial);
	  }
  }
  if (EffectSound) {
	  UGameplayStatics::PlaySoundAtLocation(this, EffectSound, Character->GetActorLocation(), 1.0f, 1.0f, 0.0f);
  }
}
