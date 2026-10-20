#include "Weather/WotWeatherFeet.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UnrealType.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ObjectKey.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

void WotWeatherFeet::TagGround(UWorld* World)
{
	static TSet<FObjectKey> Done;
	if (!World || Done.Contains(FObjectKey(World))) { return; }
	Done.Add(FObjectKey(World));
	UPhysicalMaterial* PM = LoadObject<UPhysicalMaterial>(nullptr, TEXT("/Game/Wot/Materials/PM_Ground.PM_Ground"));
	if (!PM) { return; }
	int32 Comps = 0, Bodies = 0, Seen = 0;
	for (TObjectIterator<UInstancedStaticMeshComponent> It; It; ++It) {
		UInstancedStaticMeshComponent* ISM = *It;
		if (ISM->GetWorld() != World || !ISM->IsRegistered()) { continue; }
		++Seen;
		UMaterialInterface* M = ISM->GetMaterial(0);
		UMaterial* Base = M ? M->GetMaterial() : nullptr;   // the ground instances all sit on voxel_ground_base
		if (!Base || Base->GetName() != TEXT("voxel_ground_base")) { continue; }
		ISM->BodyInstance.SetPhysMaterialOverride(PM);
		for (FBodyInstance* B : ISM->InstanceBodies) { if (B) { B->SetPhysMaterialOverride(PM); ++Bodies; } }
		if (ISM->InstanceBodies.Num() == 0) { ISM->RecreatePhysicsState(); }   // 5.8 keeps instance collision inside the physics state: rebuild it from the template body
		++Comps;
		UE_LOG(LogTemp, Display, TEXT("[FEET]   %s.%s mat0=%s -> %s (%d instances)"), *ISM->GetOwner()->GetName(), *ISM->GetName(), *M->GetName(), *PM->GetName(), ISM->GetInstanceCount());
	}
	UE_LOG(LogTemp, Display, TEXT("[FEET] ground tagged with %s: %d of %d instanced mesh component(s), %d bodies"), *PM->GetName(), Comps, Seen, Bodies);
}

void WotWeatherFeet::Attach(ACharacter* C, float FootSize)
{
	if (!C || !C->GetWorld() || !C->GetWorld()->IsGameWorld()) { return; }
	TagGround(C->GetWorld());
	static TWeakObjectPtr<UClass> Cached;
	UClass* Cls = Cached.Get();
	if (!Cls) {
		Cls = LoadClass<USceneComponent>(nullptr, TEXT("/Game/UltraDynamicSky/Blueprints/Weather_Effects/DLWE_Interaction.DLWE_Interaction_C"));
		Cached = Cls;
	}
	if (!Cls) { return; }
	USkeletalMeshComponent* Mesh = C->GetMesh();
	TArray<FName> Bones;
	if (Mesh && Mesh->GetSkinnedAsset()) {
		for (int32 i = 0; i < Mesh->GetNumBones(); ++i) {
			const FName B = Mesh->GetBoneName(i);
			const FString N = B.ToString();
			// humanoid rigs name them Foot_L / Foot_R; the voxel rigs end each leg chain with leg_l_end / leg_r_end
			if (N.Contains(TEXT("foot"), ESearchCase::IgnoreCase) || (N.Contains(TEXT("leg"), ESearchCase::IgnoreCase) && N.EndsWith(TEXT("_end")))) { Bones.Add(B); }
		}
	}
	auto Make = [&](USceneComponent* Parent, FName Socket, const FString& Name, const FVector& Offset) {
		USceneComponent* Comp = NewObject<USceneComponent>(C, Cls, *Name);
		if (!Comp) { return; }
		if (FProperty* SizeProp = Cls->FindPropertyByName(TEXT("Size"))) {
			if (FDoubleProperty* D = CastField<FDoubleProperty>(SizeProp)) { D->SetPropertyValue_InContainer(Comp, FootSize); }
			else if (FFloatProperty* F = CastField<FFloatProperty>(SizeProp)) { F->SetPropertyValue_InContainer(Comp, FootSize); }
		}
		Comp->SetupAttachment(Parent, Socket);
		Comp->SetRelativeLocation(Offset);
		Comp->RegisterComponent();
	};
	if (Bones.Num() > 0) {
		for (const FName& B : Bones) { Make(Mesh, B, FString::Printf(TEXT("WeatherFoot_%s"), *B.ToString()), FVector::ZeroVector); }
	}
	else if (UCapsuleComponent* Cap = C->GetCapsuleComponent()) {
		Make(Cap, NAME_None, TEXT("WeatherFoot"), FVector(0, 0, -Cap->GetScaledCapsuleHalfHeight()));
	}
	UE_LOG(LogTemp, Display, TEXT("[FEET] %s: %d DLWE interaction component(s) (%s)"), *C->GetName(), FMath::Max(Bones.Num(), 1), *FString::JoinBy(Bones, TEXT(","), [](const FName& N) { return N.ToString(); }));
}

// wot.Weather.Feet : list the player's weather foot components, trace down from each and say what physical material is under it
static FAutoConsoleCommand GWotWeatherFeet(
	TEXT("wot.Weather.Feet"), TEXT("Trace down from the player's DLWE interaction components and log the surface's physical material"),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World) {
		if (!World) { return; }
		APawn* P = UGameplayStatics::GetPlayerPawn(World, 0);
		if (!P) { UE_LOG(LogTemp, Display, TEXT("[FEET] no player pawn")); return; }
		int32 N = 0;
		for (UActorComponent* AC : P->GetComponents()) {
			USceneComponent* SC = Cast<USceneComponent>(AC);
			if (!SC || !SC->GetName().StartsWith(TEXT("WeatherFoot"))) { continue; }
			++N;
			const FVector L = SC->GetComponentLocation();
			FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(WotFeet), false, P); Q.bReturnPhysicalMaterial = true;
			const bool bHit = World->LineTraceSingleByChannel(Hit, L + FVector(0, 0, 30), L - FVector(0, 0, 200), ECC_Visibility, Q);
			UE_LOG(LogTemp, Display, TEXT("[FEET] %s class=%s at (%.0f %.0f %.0f) parent=%s socket=%s hit=%d actor=%s comp=%s phys=%s mat=%s"), *SC->GetName(), *SC->GetClass()->GetName(), L.X, L.Y, L.Z,
				SC->GetAttachParent() ? *SC->GetAttachParent()->GetName() : TEXT("none"), *SC->GetAttachSocketName().ToString(), bHit ? 1 : 0,
				bHit && Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("-"), bHit && Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("-"),
				Hit.PhysMaterial.IsValid() ? *Hit.PhysMaterial->GetName() : TEXT("none"),
				bHit && Hit.GetComponent() && Cast<UPrimitiveComponent>(Hit.GetComponent()) && Cast<UPrimitiveComponent>(Hit.GetComponent())->GetMaterial(0) ? *Cast<UPrimitiveComponent>(Hit.GetComponent())->GetMaterial(0)->GetName() : TEXT("-"));
		}
		UE_LOG(LogTemp, Display, TEXT("[FEET] %d foot component(s) on %s"), N, *P->GetName());
		if (ACharacter* C = Cast<ACharacter>(P)) {
			USkeletalMeshComponent* Mesh = C->GetMesh();
			TArray<FString> Names;
			if (Mesh) { for (int32 i = 0; i < Mesh->GetNumBones(); ++i) { Names.Add(Mesh->GetBoneName(i).ToString()); } }
			UE_LOG(LogTemp, Display, TEXT("[FEET] mesh=%s bones=%d: %s"), Mesh && Mesh->GetSkinnedAsset() ? *Mesh->GetSkinnedAsset()->GetName() : TEXT("none"), Names.Num(), *FString::Join(Names, TEXT(" ")));
		}
	}));

// wot.Weather.MPC [Name Value] : print (or set) the scalars of Ultra Dynamic Weather's material parameter collection in
// this world: the values the snow / wet / dust material functions read (Snowy, Wet, Dusty, DLWE_* ...)
static FAutoConsoleCommandWithWorldAndArgs GWotWeatherMPC(
	TEXT("wot.Weather.MPC"), TEXT("Print or set a scalar of UDW's material parameter collection: wot.Weather.MPC [Snowy 1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/UltraDynamicSky/Materials/Weather/UltraDynamicWeather_Parameters.UltraDynamicWeather_Parameters"));
		if (!World || !MPC) { UE_LOG(LogTemp, Display, TEXT("[MPC] world or collection missing")); return; }
		UMaterialParameterCollectionInstance* Inst = World->GetParameterCollectionInstance(MPC);
		if (!Inst) { UE_LOG(LogTemp, Display, TEXT("[MPC] no instance in this world")); return; }
		if (Args.Num() >= 2) { const bool bOk = Inst->SetScalarParameterValue(*Args[0], FCString::Atof(*Args[1])); UE_LOG(LogTemp, Display, TEXT("[MPC] set %s = %s ok=%d"), *Args[0], *Args[1], bOk ? 1 : 0); }
		TArray<FString> Out;
		for (const FCollectionScalarParameter& P : MPC->ScalarParameters) { float V = 0; Inst->GetScalarParameterValue(P.ParameterName, V); Out.Add(FString::Printf(TEXT("%s=%.2f"), *P.ParameterName.ToString(), V)); }
		for (const FCollectionVectorParameter& P : MPC->VectorParameters) { FLinearColor V; Inst->GetVectorParameterValue(P.ParameterName, V); Out.Add(FString::Printf(TEXT("%s=(%.3g %.3g %.3g %.3g)"), *P.ParameterName.ToString(), V.R, V.G, V.B, V.A)); }
		UE_LOG(LogTemp, Display, TEXT("[MPC] %s"), *FString::Join(Out, TEXT(" ")));
	}));

// wot.Perf.Primitives [N=25] : the heaviest primitive components in this world (instanced meshes by instance count and
// triangles, then static mesh components), to see where a level's draw and triangle budget goes
static FAutoConsoleCommandWithWorldAndArgs GWotPerfPrimitives(
	TEXT("wot.Perf.Primitives"), TEXT("List the heaviest instanced / static mesh components in the world: wot.Perf.Primitives [N]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (!World) { return; }
		const int32 N = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 25;
		struct FRow { FString Name; int32 Instances; int64 Tris; bool bCollision; bool bNanite; };
		TArray<FRow> Rows; int64 TotalTris = 0; int32 TotalInst = 0, NumComps = 0;
		for (TObjectIterator<UStaticMeshComponent> It; It; ++It) {
			UStaticMeshComponent* C = *It;
			if (C->GetWorld() != World || !C->IsRegistered() || !C->GetStaticMesh()) { continue; }
			const UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(C);
			const int32 Inst = ISM ? ISM->GetInstanceCount() : 1;
			if (Inst == 0) { continue; }
			const int64 MeshTris = C->GetStaticMesh()->GetNumTriangles(0);
			FRow R; R.Name = FString::Printf(TEXT("%s.%s [%s]"), *GetNameSafe(C->GetOwner()), *C->GetName(), *C->GetStaticMesh()->GetName());
			R.Instances = Inst; R.Tris = MeshTris * Inst; R.bCollision = C->IsCollisionEnabled(); R.bNanite = C->GetStaticMesh()->IsNaniteEnabled();
			Rows.Add(R); TotalTris += R.Tris; TotalInst += Inst; ++NumComps;
		}
		Rows.Sort([](const FRow& A, const FRow& B) { return A.Tris > B.Tris; });
		UE_LOG(LogTemp, Display, TEXT("[PERF] %d mesh components, %d instances, %lld triangles (LOD0 x instances)"), NumComps, TotalInst, TotalTris);
		for (int32 i = 0; i < FMath::Min(N, Rows.Num()); ++i) { const FRow& R = Rows[i]; UE_LOG(LogTemp, Display, TEXT("[PERF]   %10lld tris %6d inst coll=%d nanite=%d  %s"), R.Tris, R.Instances, R.bCollision ? 1 : 0, R.bNanite ? 1 : 0, *R.Name); }
	}));
