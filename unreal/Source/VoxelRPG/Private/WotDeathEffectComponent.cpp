// Fill out your copyright notice in the Description page of Project Settings.

#include "WotDeathEffectComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
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
  // Get the parent of material instance dynamic (if it exists), otherwise it
  // will find invalid parameters
  UMaterialInterface* Parent = MeshMaterial;
  UMaterialInstance* MatInst = Cast<UMaterialInstance>(MeshMaterial);
  if (MatInst) {
	  Parent = MatInst;
  }
  UTexture* MeshTexture;
  if (!Parent->GetTextureParameterValue(TextureParameterName, MeshTexture)) {
	  UE_LOG(LogTemp, Warning, TEXT("Could not get texture!"));
  }
  if (!ensure(MeshTexture)) {
	  UE_LOG(LogTemp, Warning, TEXT("No Texture!"));
	  return;
  }
  if (!ensure(EffectNiagaraSystem)) {
	  UE_LOG(LogTemp, Warning, TEXT("No System!"));
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
  // create dynamic material instance for the mesh
  UMaterialInstanceDynamic* EffectMaterial = UMaterialInstanceDynamic::Create(EffectMaterialBase, this);
  // set the texture for the new material
  EffectMaterial->SetTextureParameterValue("Color Texture", MeshTexture);
  // Set the material for the meshes (cubes) in the niagara effect
  EffectNiagaraComp->SetVariableMaterial("Material", EffectMaterial);
  if (EffectSound) {
	  UGameplayStatics::PlaySoundAtLocation(this, EffectSound, Character->GetActorLocation(), 1.0f, 1.0f, 0.0f);
  }
}
