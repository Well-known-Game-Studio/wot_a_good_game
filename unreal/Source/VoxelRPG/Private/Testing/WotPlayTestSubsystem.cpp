#include "Testing/WotPlayTestSubsystem.h"
#include "AI/WotAICharacter.h"
#include "WotCharacter.h"
#include "WotInteractionComponent.h"
#include "WotInteractableInterface.h"
#include "Quests/WotQuestSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/Engine.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/RecastNavMesh.h"
#include "Engine/OverlapResult.h"
#include "Engine/HitResult.h"
#include "WotMountComponent.h"
#include "Animation/Skeleton.h"
#include "UObject/SoftObjectPath.h"
#include "UnrealClient.h"
#include "Misc/DateTime.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimClassInterface.h"
#include "ReferenceSkeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "TimerManager.h"
#include "WotCharacterAnimInstance.h"
#include "WotActionComponent.h"
#if WITH_EDITOR
#include "Animation/AnimData/IAnimationDataModel.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogWotTest, Log, All);

#define PT_LOG(Fmt, ...) UE_LOG(LogWotTest, Display, TEXT("[PT] ") Fmt, ##__VA_ARGS__)

bool UWotPlayTestSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) {
		return false;
	}
	UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Editor);
}

void UWotPlayTestSubsystem::LogNavState(const TCHAR* Tag)
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!Nav) {
		PT_LOG("NAV %s no navigation system", Tag);
		return;
	}
	ANavigationData* Data = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	ARecastNavMesh* Recast = Cast<ARecastNavMesh>(Data);
	const int32 Tiles = Recast ? Recast->GetNavMeshTilesCount() : 0;
	const int32 Bounds = Nav->GetNavigationBounds().Num();
	const bool bBuilding = Nav->IsNavigationBeingBuilt(World);
	FString Nearest = TEXT("n/a");
	if (APawn* Pawn = GetPlayerPawn()) {
		FNavLocation Out;
		const FVector Loc = Pawn->GetActorLocation();
		if (Nav->ProjectPointToNavigation(Loc, Out, FVector(300.0f, 300.0f, 5000.0f))) {
			Nearest = FString::Printf(TEXT("(%.0f %.0f %.0f) dz=%.0f"), Out.Location.X, Out.Location.Y, Out.Location.Z, Out.Location.Z - Loc.Z);
		} else {
			Nearest = TEXT("NONE within 300x300x5000");
		}
	}
	PT_LOG("NAV %s navdata=%s bounds=%d tiles=%d building=%d runtime=%s nearest=%s", Tag, *GetNameSafe(Data), Bounds, Tiles, bBuilding ? 1 : 0,
		Recast ? *UEnum::GetValueAsString(Recast->GetRuntimeGenerationMode()) : TEXT("n/a"), *Nearest);
}

void UWotPlayTestSubsystem::RebuildNav()
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!Nav) {
		PT_LOG("NAVBUILD no navigation system");
		return;
	}
	Nav->Build();
	PT_LOG("NAVBUILD started (world=%s)", *GetNameSafe(World));
	LogNavState(TEXT("after-build-request"));
}

APawn* UWotPlayTestSubsystem::GetPlayerPawn() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

bool UWotPlayTestSubsystem::TraceGround(const FVector& From, float Up, float Down, FHitResult& OutHit, const AActor* Ignore) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WotPlayTest), false);
	if (Ignore) {
		Params.AddIgnoredActor(Ignore);
	}
	return GetWorld()->LineTraceSingleByChannel(OutHit, From + FVector(0, 0, Up), From - FVector(0, 0, Down), ECC_Visibility, Params);
}

FString UWotPlayTestSubsystem::DescribeActor(const AActor* Actor) const
{
	if (!Actor) {
		return TEXT("none");
	}
	TArray<FName> Names = UWotQuestSubsystem::GetActorQuestNames(Actor);
	FString Label = Names.Num() > 0 ? Names[0].ToString() : Actor->GetName();
	return FString::Printf(TEXT("%s<%s>"), *Label, *Actor->GetClass()->GetName());
}

void UWotPlayTestSubsystem::RunSpawnCheck()
{
	UWorld* World = GetWorld();
	APawn* Pawn = GetPlayerPawn();
	PT_LOG("SPAWNCHECK level=%s", *UGameplayStatics::GetCurrentLevelName(World, true));
	if (!Pawn) {
		PT_LOG("SPAWN no player pawn");
		return;
	}
	ACharacter* Char = Cast<ACharacter>(Pawn);
	const FVector Loc = Pawn->GetActorLocation();
	float HalfHeight = 0.0f;
	if (Char && Char->GetCapsuleComponent()) {
		HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FHitResult Hit;
	const bool bHit = TraceGround(Loc, 0.0f, 10000.0f, Hit, Pawn);
	const float Clearance = bHit ? (Loc.Z - HalfHeight) - Hit.Location.Z : -1.0f;
	FString MoveMode = TEXT("n/a");
	bool bFalling = false;
	if (Char && Char->GetCharacterMovement()) {
		MoveMode = Char->GetCharacterMovement()->GetMovementName();
		bFalling = Char->GetCharacterMovement()->IsFalling();
	}
	PT_LOG("SPAWN pawn=%s loc=(%.0f %.0f %.0f) halfheight=%.0f ground=%s groundZ=%.0f clearance=%.0f hit=%s/%s mode=%s falling=%d",
		*Pawn->GetClass()->GetName(), Loc.X, Loc.Y, Loc.Z, HalfHeight, bHit ? TEXT("yes") : TEXT("NO"),
		bHit ? Hit.Location.Z : 0.0f, Clearance, bHit ? *GetNameSafe(Hit.GetActor()) : TEXT(""), bHit ? *GetNameSafe(Hit.GetComponent()) : TEXT(""),
		*MoveMode, bFalling ? 1 : 0);

	// Player starts and portals
	for (TActorIterator<APlayerStart> It(World); It; ++It) {
		const FVector L = It->GetActorLocation();
		FHitResult H;
		const bool bH = TraceGround(L, 0.0f, 10000.0f, H, *It);
		PT_LOG("PLAYERSTART %s tag=%s loc=(%.0f %.0f %.0f) clearance=%.0f", *It->GetName(), *It->PlayerStartTag.ToString(), L.X, L.Y, L.Z, bH ? L.Z - H.Location.Z : -1.0f);
	}
	for (TActorIterator<AActor> It(World); It; ++It) {
		if (It->GetClass()->GetName().StartsWith(TEXT("Portal"))) {
			const FVector L = It->GetActorLocation();
			FHitResult H;
			const bool bH = TraceGround(L, 50.0f, 10000.0f, H, *It);
			PT_LOG("PORTAL %s loc=(%.0f %.0f %.0f) clearance=%.0f", *It->GetName(), L.X, L.Y, L.Z, bH ? L.Z - H.Location.Z : -1.0f);
		}
	}

	// NPCs
	int32 Total = 0, Floating = 0, Buried = 0, NoGround = 0;
	for (TActorIterator<AWotAICharacter> It(World); It; ++It) {
		AWotAICharacter* NPC = *It;
		++Total;
		const FVector L = NPC->GetActorLocation();
		float HH = NPC->GetCapsuleComponent() ? NPC->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;
		FHitResult H;
		const bool bH = TraceGround(L, 0.0f, 10000.0f, H, NPC);
		const float C = bH ? (L.Z - HH) - H.Location.Z : -1.0f;
		const TCHAR* State = TEXT("ok");
		if (!bH) { State = TEXT("NOGROUND"); ++NoGround; }
		else if (C > 60.0f) { State = TEXT("FLOATING"); ++Floating; }
		else if (C < -40.0f) { State = TEXT("BURIED"); ++Buried; }
		const bool bFall = NPC->GetCharacterMovement() && NPC->GetCharacterMovement()->IsFalling();
		PT_LOG("NPC %s loc=(%.0f %.0f %.0f) clearance=%.0f falling=%d %s", *DescribeActor(NPC), L.X, L.Y, L.Z, C, bFall ? 1 : 0, State);
	}
	PT_LOG("NPCSUMMARY total=%d floating=%d buried=%d noground=%d", Total, Floating, Buried, NoGround);
	LogNavState(TEXT("spawn"));
}

void UWotPlayTestSubsystem::StartWalkToNPCs(int32 MaxTargets, float InTimeLimit, bool bInteract)
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn) {
		PT_LOG("WALK no player pawn");
		return;
	}
	Targets.Reset();
	const FVector Loc = Pawn->GetActorLocation();
	for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
		FTarget T;
		T.Actor = *It;
		T.Location = It->GetActorLocation();
		T.Name = DescribeActor(*It);
		Targets.Add(T);
	}
	Targets.Sort([Loc](const FTarget& A, const FTarget& B) { return FVector::DistSquared2D(A.Location, Loc) < FVector::DistSquared2D(B.Location, Loc); });
	if (MaxTargets > 0 && Targets.Num() > MaxTargets) {
		Targets.SetNum(MaxTargets);
	}
	if (Targets.Num() == 0) {
		PT_LOG("WALK no NPCs in level");
		return;
	}
	bInteractWithTargets = bInteract;
	TimeLimit = InTimeLimit;
	Elapsed = 0.0f; Reached = 0; Stuck = 0; Jumps = 0; FallingTime = 0.0f; MaxStepUp = 0.0f; NoPath = 0; PartialPaths = 0;
	StuckAt.Reset(); Interactions.Reset();
	StartZ = Loc.Z; MinZ = Loc.Z; MaxZ = Loc.Z; LastZ = Loc.Z; SinceStepSample = 0.0f;
	PT_LOG("WALK start targets=%d limit=%.0fs from=(%.0f %.0f %.0f)", Targets.Num(), TimeLimit, Loc.X, Loc.Y, Loc.Z);
	StartWalkNow();
}

void UWotPlayTestSubsystem::StartWalkNow()
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (Nav && Nav->IsNavigationBeingBuilt(GetWorld())) {
		if (!bWaitingForNav) {
			bWaitingForNav = true;
			NavWait = 0.0f;
			PT_LOG("NAVWAIT navmesh still building; waiting before walking");
		}
		return;
	}
	bWaitingForNav = false;
	LogNavState(TEXT("walk"));
	bWalking = true;
	BeginTarget(0);
}

void UWotPlayTestSubsystem::StartWalkToLocation(const FVector& Location, float InTimeLimit)
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn) {
		PT_LOG("GOTO no player pawn");
		return;
	}
	Targets.Reset();
	FTarget T;
	T.Location = Location;
	T.Name = FString::Printf(TEXT("point(%.0f %.0f)"), Location.X, Location.Y);
	Targets.Add(T);
	bInteractWithTargets = false;
	TimeLimit = InTimeLimit;
	const FVector Loc = Pawn->GetActorLocation();
	Elapsed = 0.0f; Reached = 0; Stuck = 0; Jumps = 0; FallingTime = 0.0f; MaxStepUp = 0.0f; NoPath = 0; PartialPaths = 0;
	StuckAt.Reset(); Interactions.Reset();
	StartZ = Loc.Z; MinZ = Loc.Z; MaxZ = Loc.Z; LastZ = Loc.Z; SinceStepSample = 0.0f;
	PT_LOG("GOTO start to=(%.0f %.0f) from=(%.0f %.0f %.0f)", Location.X, Location.Y, Loc.X, Loc.Y, Loc.Z);
	StartWalkNow();
}

void UWotPlayTestSubsystem::BeginTarget(int32 Index)
{
	TargetIndex = Index;
	TargetElapsed = 0.0f;
	SinceProgress = 0.0f;
	SinceLog = 0.0f;
	InteractDelay = 0.0f;
	bTargetInteracting = false;
	APawn* Pawn = GetPlayerPawn();
	if (!Targets.IsValidIndex(Index) || !Pawn) {
		Summary(TEXT("done"));
		return;
	}
	FTarget& T = Targets[Index];
	if (T.Actor.IsValid()) {
		T.Location = T.Actor->GetActorLocation();
	}
	BestDistance = FVector::Dist2D(Pawn->GetActorLocation(), T.Location);
	PT_LOG("TARGET %d/%d %s at=(%.0f %.0f %.0f) dist=%.0f", Index + 1, Targets.Num(), *T.Name, T.Location.X, T.Location.Y, T.Location.Z, BestDistance);
	StuckTime = 0.0f; SinceStuckSample = 0.0f; SinceJump = 0.0f; SincePath = 0.0f; bRepathedWhileStuck = false; DialogPresses = 0;
	LastSamplePos = Pawn->GetActorLocation();
	ComputePath(T, Pawn->GetActorLocation());
}

void UWotPlayTestSubsystem::ComputePath(FTarget& T, const FVector& From)
{
	T.Path.Reset();
	T.PathIndex = 0;
	T.bHasPath = false;
	T.bPartialPath = false;
	APawn* Pawn = GetPlayerPawn();
	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), From, T.Location, Pawn);
	if (Path && Path->IsValid() && Path->PathPoints.Num() > 1) {
		T.Path = Path->PathPoints;
		T.PathIndex = 1;
		T.bHasPath = true;
		T.bPartialPath = Path->IsPartial();
		if (T.bPartialPath) {
			++PartialPaths;
		}
		PT_LOG("PATH to %s points=%d partial=%d length=%.0f", *T.Name, T.Path.Num(), T.bPartialPath ? 1 : 0, Path->GetPathLength());
	} else {
		++NoPath;
		PT_LOG("NOPATH to %s (navmesh has no route; walking straight)", *T.Name);
	}
}

bool UWotPlayTestSubsystem::IsDialogueOpen(APawn* Pawn) const
{
	ACharacter* Char = Cast<ACharacter>(Pawn);
	if (Char && Char->GetCharacterMovement() && !Char->GetCharacterMovement()->IsComponentTickEnabled()) {
		return true; // PlayDialogForActor disables the movement tick while talking
	}
	if (Pawn) {
		if (FBoolProperty* Prop = CastField<FBoolProperty>(Pawn->GetClass()->FindPropertyByName(TEXT("InteractionLocked")))) {
			return Prop->GetPropertyValue_InContainer(Pawn);
		}
	}
	return false;
}

void UWotPlayTestSubsystem::FinishTarget(bool bReachedTarget)
{
	APawn* Pawn = GetPlayerPawn();
	const FVector Loc = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	FTarget& T = Targets[TargetIndex];
	if (bReachedTarget) {
		++Reached;
		PT_LOG("REACHED %s in %.1fs at=(%.0f %.0f %.0f)", *T.Name, TargetElapsed, Loc.X, Loc.Y, Loc.Z);
	} else {
		++Stuck;
		StuckAt.Add(FString::Printf(TEXT("%s@(%.0f %.0f %.0f)"), *T.Name, Loc.X, Loc.Y, Loc.Z));
		PT_LOG("STUCK heading to %s after %.1fs at=(%.0f %.0f %.0f) remaining=%.0f", *T.Name, TargetElapsed, Loc.X, Loc.Y, Loc.Z, FVector::Dist2D(Loc, T.Location));
		// what is around the capsule?
		if (ACharacter* Char = Cast<ACharacter>(Pawn)) {
			if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent()) {
				TArray<FOverlapResult> Overlaps;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(WotStuck), false);
				Params.AddIgnoredActor(Pawn);
				GetWorld()->OverlapMultiByChannel(Overlaps, Loc, FQuat::Identity, ECC_Pawn,
					FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius() + 20.0f, Capsule->GetScaledCapsuleHalfHeight()), Params);
				TSet<FString> Names;
				for (const FOverlapResult& O : Overlaps) {
					Names.Add(FString::Printf(TEXT("%s/%s"), *GetNameSafe(O.GetActor()), *GetNameSafe(O.GetComponent())));
				}
				TArray<FString> List = Names.Array();
				PT_LOG("STUCKNEAR %s", List.Num() ? *FString::Join(List, TEXT(" | ")) : TEXT("nothing within 20cm"));
				FVector Fwd = T.Location - Loc; Fwd.Z = 0; Fwd.Normalize();
				FHitResult Hit;
				if (GetWorld()->SweepSingleByChannel(Hit, Loc, Loc + Fwd * 120.0f, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params)) {
					PT_LOG("STUCKAHEAD %s/%s at dist=%.0f normal=(%.2f %.2f %.2f)", *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), Hit.Distance, Hit.ImpactNormal.X, Hit.ImpactNormal.Y, Hit.ImpactNormal.Z);
				} else {
					PT_LOG("STUCKAHEAD clear for 120cm");
				}
			}
		}
	}
	BeginTarget(TargetIndex + 1);
}

bool UWotPlayTestSubsystem::CheckInteractable(const FTarget& Target)
{
	AWotCharacter* Player = Cast<AWotCharacter>(GetPlayerPawn());
	if (!Player || !Target.Actor.IsValid()) {
		return false;
	}
	// face the target first so the forward sweep can find it
	FVector Dir = Target.Actor->GetActorLocation() - Player->GetActorLocation();
	Dir.Z = 0.0f;
	if (!Dir.IsNearlyZero()) {
		Player->SetActorRotation(Dir.GetSafeNormal().ToOrientationRotator());
	}
	UWotInteractionComponent* Comp = Player->GetInteractionComponent();
	AActor* Found = nullptr;
	UActorComponent* FoundComp = nullptr;
	FHitResult Hit;
	const bool bFound = Comp && Comp->GetInteractableInRange(Found, FoundComp, Hit);
	FText Text;
	if (bFound && Found) {
		if (FoundComp) {
			IWotInteractableInterface::Execute_GetInteractionText(FoundComp, Player, Hit, Text);
		} else if (Found->Implements<UWotInteractableInterface>()) {
			IWotInteractableInterface::Execute_GetInteractionText(Found, Player, Hit, Text);
		}
	}
	const bool bSame = bFound && Found == Target.Actor.Get();
	if (!bSame && InteractRetries < 20) {
		++InteractRetries;
		return false;
	}
	InteractRetries = 0;
	PT_LOG("INTERACTABLE target=%s found=%s same=%d text=\"%s\"", *Target.Name, *DescribeActor(Found), bSame ? 1 : 0, *Text.ToString());
	Interactions.Add(FString::Printf(TEXT("%s:%s"), *Target.Name, bSame ? TEXT("ok") : TEXT("MISSING")));
	if (bInteractWithTargets && bSame) {
		Player->TryInteract();
		bTargetInteracting = true;
		InteractDelay = 0.8f;
		DialogPresses = 0;
		PT_LOG("INTERACT started with %s", *Target.Name);
	}
	return true;
}

FString UWotPlayTestSubsystem::CommandFilePath() const
{
	const bool bEditor = GetWorld() && GetWorld()->WorldType == EWorldType::Editor;
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PlayTest"), bEditor ? TEXT("commands_editor.txt") : TEXT("commands.txt"));
}

void UWotPlayTestSubsystem::PollCommandFile()
{
	const FString Path = CommandFilePath();
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.FileExists(*Path)) {
		return;
	}
	// move the file aside first: a script appending between our read and delete would otherwise lose its lines
	const FString Taken = Path + TEXT(".taken");
	PF.DeleteFile(*Taken);
	if (!PF.MoveFile(*Taken, *Path)) {
		return;
	}
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *Taken)) {
		PF.DeleteFile(*Taken);
		return;
	}
	PF.DeleteFile(*Taken);
	for (FString& Line : Lines) {
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#"))) {
			continue;
		}
		PT_LOG("EXEC %s", *Line);
		if (GEngine) {
			GEngine->Exec(GetWorld(), *Line);
		}
	}
}

void UWotPlayTestSubsystem::Tick(float DeltaTime)
{
	if (bRecording) {
		SinceFrame += DeltaTime;
		RecordLeft -= DeltaTime;
		if (SinceFrame >= RecordInterval) {
			SinceFrame = 0.0f;
			FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("rec_%s_%05d"), *RecordName, RecordFrame++), bRecordUI, false);
		}
		if (RecordLeft <= 0.0f) {
			StopRecording();
		}
	}
	SinceCommandPoll += DeltaTime;
	if (SinceCommandPoll >= 0.5f) {
		SinceCommandPoll = 0.0f;
		PollCommandFile();
	}
	if (bWaitingForNav) {
		NavWait += DeltaTime;
		if (NavWait > 120.0f) {
			bWaitingForNav = false;
			PT_LOG("NAVWAIT gave up after %.0fs; walking anyway", NavWait);
			LogNavState(TEXT("walk"));
			bWalking = true;
			BeginTarget(0);
		} else if (FMath::Fmod(NavWait, 1.0f) < DeltaTime) {
			StartWalkNow();
		}
		return;
	}
	if (!bWalking) {
		return;
	}
	APawn* Pawn = GetPlayerPawn();
	ACharacter* Char = Cast<ACharacter>(Pawn);
	if (!Pawn || !Targets.IsValidIndex(TargetIndex)) {
		Summary(TEXT("lost pawn"));
		return;
	}
	Elapsed += DeltaTime;
	TargetElapsed += DeltaTime;
	SinceLog += DeltaTime;
	SinceStepSample += DeltaTime;
	const FVector Loc = Pawn->GetActorLocation();
	MinZ = FMath::Min(MinZ, Loc.Z);
	MaxZ = FMath::Max(MaxZ, Loc.Z);

	// riding: steer the mount forward, screenshot, then dismount
	if (bRiding) {
		AWotCharacter* Player = Cast<AWotCharacter>(Pawn);
		UWotMountComponent* Mount = Player ? Player->GetMount() : nullptr;
		RideTime += DeltaTime;
		SinceRideLog += DeltaTime;
		if (Mount) {
			// gentle arc so the ride covers some ground
			const float Yaw = FMath::DegreesToRadians(RideTime * 25.0f);
			Mount->AddRiderMovementInput(FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f), 1.0f);
		}
		if (SinceRideLog >= 1.0f) {
			SinceRideLog = 0.0f;
			AActor* MountActor = Mount ? Mount->GetOwner() : nullptr;
			PT_LOG("RIDE t=%.0fs rider=(%.0f %.0f %.0f) mount=(%.0f %.0f %.0f) speed=%.0f mounted=%d", RideTime, Loc.X, Loc.Y, Loc.Z,
				MountActor ? MountActor->GetActorLocation().X : 0.0f, MountActor ? MountActor->GetActorLocation().Y : 0.0f, MountActor ? MountActor->GetActorLocation().Z : 0.0f,
				MountActor ? MountActor->GetVelocity().Size2D() : 0.0f, Player && Player->IsMounted() ? 1 : 0);
		}
		if (!bRideShotTaken && RideTime > RideSeconds * 0.5f) {
			bRideShotTaken = true;
			Screenshot(ScreenshotPrefix.IsEmpty() ? TEXT("ride") : ScreenshotPrefix + TEXT("_ride"));
		}
		if (RideTime >= RideSeconds || !Mount) {
			bRiding = false;
			bMountTest = false;
			const bool bWas = Player && Player->IsMounted();
			if (Player) {
				Player->Dismount();
			}
			AActor* MountActor = Mount ? Mount->GetOwner() : nullptr;
			PT_LOG("DISMOUNTED wasmounted=%d rider=(%.0f %.0f %.0f) controller=%s", bWas ? 1 : 0, Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, Pawn->GetActorLocation().Z,
				MountActor && Cast<APawn>(MountActor) ? *GetNameSafe(Cast<APawn>(MountActor)->GetController()) : TEXT("none"));
			Interactions.Add(FString::Printf(TEXT("%s:ridden %.0fs"), *Targets[TargetIndex].Name, RideTime));
			FinishTarget(true);
		}
		return;
	}

	// in a conversation: step through the lines the way the interact key does
	if (bTargetInteracting) {
		InteractDelay -= DeltaTime;
		if (InteractDelay <= 0.0f) {
			InteractDelay = 0.8f;
			FTarget& T = Targets[TargetIndex];
			AWotCharacter* Player = Cast<AWotCharacter>(Pawn);
			if (IsDialogueOpen(Pawn) && DialogPresses < 30) {
				++DialogPresses;
				if (UFunction* Fn = Pawn->FindFunction(TEXT("HandleDialogInteraction"))) {
					Pawn->ProcessEvent(Fn, nullptr);
				} else if (Player) {
					Player->TryInteract();
				}
			} else {
				if (DialogPresses >= 30) {
					PT_LOG("DIALOG with %s did not close after %d presses; forcing", *T.Name, DialogPresses);
					if (T.Actor.IsValid() && T.Actor->Implements<UWotInteractableInterface>()) {
						FHitResult Dummy;
						IWotInteractableInterface::Execute_FinishInteraction(T.Actor.Get(), Pawn, Dummy);
					}
					if (UFunction* Fn = Pawn->FindFunction(TEXT("UnlockAndHideDialog"))) {
						Pawn->ProcessEvent(Fn, nullptr);
					}
				}
				if (Player && Player->IsMounted() && bMountTest) {
					bRiding = true;
					RideTime = 0.0f;
					SinceRideLog = 0.0f;
					bTargetInteracting = false;
					AActor* MountActor = Player->GetMount() ? Player->GetMount()->GetOwner() : nullptr;
					PT_LOG("MOUNTED %s rider=(%.0f %.0f %.0f) mount=(%.0f %.0f %.0f) controller=%s", *T.Name, Player->GetActorLocation().X, Player->GetActorLocation().Y, Player->GetActorLocation().Z,
						MountActor ? MountActor->GetActorLocation().X : 0.0f, MountActor ? MountActor->GetActorLocation().Y : 0.0f, MountActor ? MountActor->GetActorLocation().Z : 0.0f,
						MountActor && Cast<APawn>(MountActor) ? *GetNameSafe(Cast<APawn>(MountActor)->GetController()) : TEXT("none"));
					return;
				}
				if (Player) {
					Player->Dismount();
				}
				PT_LOG("INTERACT finished with %s presses=%d dialogue=%s", *T.Name, DialogPresses, DialogPresses > 0 ? TEXT("yes") : TEXT("none"));
				Interactions.Last() += DialogPresses > 0 ? FString::Printf(TEXT("(%d lines)"), DialogPresses) : TEXT("(no dialogue)");
				bTargetInteracting = false;
				FinishTarget(true);
			}
		}
		return;
	}

	if (Elapsed > TimeLimit) {
		Summary(TEXT("time limit"));
		return;
	}
	if (Char && Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsFalling()) {
		FallingTime += DeltaTime;
		if (FallingTime > 4.0f) {
			PT_LOG("FELL falling for %.1fs at=(%.0f %.0f %.0f)", FallingTime, Loc.X, Loc.Y, Loc.Z);
			Summary(TEXT("fell through world"));
			return;
		}
	} else {
		FallingTime = 0.0f;
	}
	if (SinceStepSample >= 0.25f) {
		MaxStepUp = FMath::Max(MaxStepUp, Loc.Z - LastZ);
		LastZ = Loc.Z;
		SinceStepSample = 0.0f;
	}

	FTarget& T = Targets[TargetIndex];
	if (T.Actor.IsValid()) {
		T.Location = T.Actor->GetActorLocation();
	}
	const float Dist = FVector::Dist2D(Loc, T.Location);
	if (Dist < 170.0f) {
		if (T.Actor.IsValid()) {
			if (!CheckInteractable(T) && Dist > 90.0f && TargetElapsed < 44.0f) {
				// not in the interaction sweep yet: keep closing in for a moment
				FVector Close = T.Location - Loc;
				Close.Z = 0.0f;
				Pawn->AddMovementInput(Close.GetSafeNormal(), 0.6f);
				return;
			}
			if (bTargetInteracting) {
				return;
			}
		}
		FinishTarget(true);
		return;
	}
	// follow the navmesh path when there is one, else head straight
	SincePath += DeltaTime;
	if (T.Actor.IsValid() && SincePath > 4.0f) {
		SincePath = 0.0f;
		ComputePath(T, Loc);
	}
	FVector Waypoint = T.Location;
	if (T.bHasPath) {
		while (T.PathIndex < T.Path.Num() - 1 && FVector::Dist2D(Loc, T.Path[T.PathIndex]) < 80.0f) {
			++T.PathIndex;
		}
		Waypoint = T.Path[FMath::Min(T.PathIndex, T.Path.Num() - 1)];
	}
	FVector Dir = Waypoint - Loc;
	Dir.Z = 0.0f;
	Dir.Normalize();
	if (StuckTime >= 2.0f && StuckTime < 3.5f) {
		// wedged against something: back off sideways for a moment, then try again
		const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector) * ((TargetIndex % 2) ? 1.0f : -1.0f);
		Pawn->AddMovementInput((Side - Dir).GetSafeNormal(), 1.0f);
	} else {
		Pawn->AddMovementInput(Dir, 1.0f);
	}

	// stuck detection: displacement over half-second windows
	SinceStuckSample += DeltaTime;
	SinceJump += DeltaTime;
	if (SinceStuckSample >= 0.5f) {
		const float Before = FVector::Dist2D(LastSamplePos, Waypoint);
		const float Now = FVector::Dist2D(Loc, Waypoint);
		LastSamplePos = Loc;
		const bool bProgress = (Before - Now) > 12.0f;
		StuckTime = bProgress ? 0.0f : StuckTime + SinceStuckSample;
		SinceStuckSample = 0.0f;
		if (bProgress) {
			bRepathedWhileStuck = false;
		}
	}
	if (StuckTime >= 1.0f && SinceJump >= 1.2f && Char && Char->CanJump() && Jumps < 300) {
		Char->Jump();
		++Jumps;
		SinceJump = 0.0f;
		PT_LOG("JUMP at=(%.0f %.0f %.0f) toward %s stuck=%.1fs", Loc.X, Loc.Y, Loc.Z, *T.Name, StuckTime);
	}
	if (StuckTime >= 3.0f && !bRepathedWhileStuck) {
		bRepathedWhileStuck = true;
		SincePath = 0.0f;
		ComputePath(T, Loc);
	}
	if (StuckTime >= 7.0f) {
		FinishTarget(false);
		return;
	}
	if (TargetElapsed > 45.0f) {
		++Stuck;
		StuckAt.Add(FString::Printf(TEXT("%s unreachable@(%.0f %.0f %.0f)"), *T.Name, Loc.X, Loc.Y, Loc.Z));
		PT_LOG("UNREACHED %s after %.1fs at=(%.0f %.0f %.0f) remaining=%.0f partialpath=%d haspath=%d", *T.Name, TargetElapsed, Loc.X, Loc.Y, Loc.Z, Dist, T.bPartialPath ? 1 : 0, T.bHasPath ? 1 : 0);
		BeginTarget(TargetIndex + 1);
		return;
	}
	if (SinceLog >= 3.0f) {
		SinceLog = 0.0f;
		PT_LOG("POS (%.0f %.0f %.0f) dist=%.0f to %s speed=%.0f wp=%d/%d", Loc.X, Loc.Y, Loc.Z, Dist, *T.Name, Pawn->GetVelocity().Size2D(), T.PathIndex, T.Path.Num());
	}
}

void UWotPlayTestSubsystem::Screenshot(const FString& Name, bool bWithUI)
{
	const FString File = FString::Printf(TEXT("%s_%s"), *Name, *FDateTime::Now().ToString(TEXT("%H%M%S")));
	FScreenshotRequest::RequestScreenshot(File, bWithUI, false);
	PT_LOG("SCREENSHOT requested %s (Saved/Screenshots)", *File);
}

void UWotPlayTestSubsystem::StartRecording(const FString& Name, float Fps, float Seconds, bool bWithUI)
{
	RecordName = Name;
	RecordInterval = 1.0f / FMath::Clamp(Fps, 1.0f, 30.0f);
	RecordLeft = FMath::Max(1.0f, Seconds);
	bRecordUI = bWithUI;
	SinceFrame = RecordInterval;   // first frame now
	RecordFrame = 0;
	bRecording = true;
	PT_LOG("RECORD start %s fps=%.0f seconds=%.0f ui=%d", *Name, Fps, Seconds, bWithUI ? 1 : 0);
}

void UWotPlayTestSubsystem::StopRecording()
{
	if (bRecording) {
		PT_LOG("RECORD stop %s frames=%d", *RecordName, RecordFrame);
	}
	bRecording = false;
}

void UWotPlayTestSubsystem::StartMountTest(float InRideSeconds)
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn) {
		PT_LOG("MOUNT no player pawn");
		return;
	}
	AActor* Best = nullptr;
	float BestDist = FLT_MAX;
	for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
		if (UWotMountComponent::FindMount(*It)) {
			const float D = FVector::DistSquared2D(It->GetActorLocation(), Pawn->GetActorLocation());
			if (D < BestDist) {
				BestDist = D;
				Best = *It;
			}
		}
	}
	if (!Best) {
		PT_LOG("MOUNT no rideable animal in level");
		return;
	}
	Targets.Reset();
	FTarget T;
	T.Actor = Best;
	T.Location = Best->GetActorLocation();
	T.Name = DescribeActor(Best);
	Targets.Add(T);
	bInteractWithTargets = true;
	bMountTest = true;
	bRiding = false;
	bRideShotTaken = false;
	RideSeconds = InRideSeconds;
	RideTime = 0.0f;
	TimeLimit = 120.0f;
	const FVector Loc = Pawn->GetActorLocation();
	Elapsed = 0.0f; Reached = 0; Stuck = 0; Jumps = 0; FallingTime = 0.0f; MaxStepUp = 0.0f; NoPath = 0; PartialPaths = 0;
	StuckAt.Reset(); Interactions.Reset();
	StartZ = Loc.Z; MinZ = Loc.Z; MaxZ = Loc.Z; LastZ = Loc.Z; SinceStepSample = 0.0f;
	PT_LOG("MOUNT test: heading to %s at=(%.0f %.0f %.0f) dist=%.0f", *T.Name, T.Location.X, T.Location.Y, T.Location.Z, FMath::Sqrt(BestDist));
	StartWalkNow();
}

void UWotPlayTestSubsystem::StopWalk(const TCHAR* Reason)
{
	if (bWalking) {
		Summary(Reason);
	}
}

void UWotPlayTestSubsystem::Summary(const TCHAR* Reason)
{
	bWalking = false;
	APawn* Pawn = GetPlayerPawn();
	const FVector Loc = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	PT_LOG("SUMMARY reason=%s reached=%d/%d stuck=%d jumps=%d nopath=%d partial=%d time=%.0fs zrange=[%.0f %.0f] startZ=%.0f maxStepUp=%.0f endloc=(%.0f %.0f %.0f)",
		Reason, Reached, Targets.Num(), Stuck, Jumps, NoPath, PartialPaths, Elapsed, MinZ, MaxZ, StartZ, MaxStepUp, Loc.X, Loc.Y, Loc.Z);
	if (StuckAt.Num() > 0) {
		PT_LOG("STUCKLIST %s", *FString::Join(StuckAt, TEXT(" | ")));
	}
	if (Interactions.Num() > 0) {
		PT_LOG("INTERACTLIST %s", *FString::Join(Interactions, TEXT(" | ")));
	}
	Targets.Reset();
	TargetIndex = -1;
}

void UWotPlayTestSubsystem::LogStatus()
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn) {
		PT_LOG("STATUS no pawn");
		return;
	}
	const FVector Loc = Pawn->GetActorLocation();
	ACharacter* Char = Cast<ACharacter>(Pawn);
	const bool bFalling = Char && Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsFalling();
	PT_LOG("STATUS loc=(%.0f %.0f %.0f) vel=%.0f falling=%d walking=%d target=%d/%d", Loc.X, Loc.Y, Loc.Z, Pawn->GetVelocity().Size(), bFalling ? 1 : 0, bWalking ? 1 : 0, TargetIndex + 1, Targets.Num());
}

// ---------------------------------------------------------------- console commands

static UWotPlayTestSubsystem* GetPlayTest(UWorld* World)
{
	return World ? World->GetSubsystem<UWotPlayTestSubsystem>() : nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestSpawn(
	TEXT("wot.PlayTest.Spawn"), TEXT("Log the player spawn, ground under it and NPC clearances"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->RunSpawnCheck();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestWalk(
	TEXT("wot.PlayTest.Walk"), TEXT("Walk to the N nearest NPCs: wot.PlayTest.Walk [N=6] [Seconds=120] [Interact=0]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			const int32 N = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 6;
			const float Seconds = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 120.0f;
			const bool bInteract = Args.Num() > 2 && FCString::Atoi(*Args[2]) != 0;
			PT->StartWalkToNPCs(N, Seconds, bInteract);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestGoto(
	TEXT("wot.PlayTest.Goto"), TEXT("Walk to a location: wot.PlayTest.Goto X Y [Seconds=60]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			if (Args.Num() >= 2) {
				PT->StartWalkToLocation(FVector(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), 0.0f), Args.Num() > 2 ? FCString::Atof(*Args[2]) : 60.0f);
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestStop(
	TEXT("wot.PlayTest.Stop"), TEXT("Abort the current play test walk"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->StopWalk(TEXT("stopped"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestMount(
	TEXT("wot.PlayTest.Mount"), TEXT("Walk to the nearest rideable animal, mount, ride, screenshot, dismount: wot.PlayTest.Mount [RideSeconds=6] [ScreenshotPrefix]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->StartMountTest(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 6.0f);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestShot(
	TEXT("wot.PlayTest.Screenshot"), TEXT("Save a screenshot of the game viewport: wot.PlayTest.Screenshot <name>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->Screenshot(Args.Num() > 0 ? Args[0] : TEXT("playtest"), Args.Num() > 1 && FCString::Atoi(*Args[1]) != 0);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotSkeletonRetarget(
	TEXT("wot.Skeleton.SetTranslationRetargeting"), TEXT("Set bone translation retargeting on a skeleton (root keeps Animation): wot.Skeleton.SetTranslationRetargeting <SkeletonPath> [Skeleton|Animation|AnimationScaled]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
#if WITH_EDITOR
		if (Args.Num() < 1) {
			return;
		}
		USkeleton* Skeleton = Cast<USkeleton>(FSoftObjectPath(Args[0]).TryLoad());
		if (!Skeleton) {
			UE_LOG(LogWotTest, Display, TEXT("[PT] RETARGET skeleton not found: %s"), *Args[0]);
			return;
		}
		EBoneTranslationRetargetingMode::Type Mode = EBoneTranslationRetargetingMode::Skeleton;
		if (Args.Num() > 1) {
			if (Args[1] == TEXT("Animation")) Mode = EBoneTranslationRetargetingMode::Animation;
			else if (Args[1] == TEXT("AnimationScaled")) Mode = EBoneTranslationRetargetingMode::AnimationScaled;
		}
		const int32 Num = Skeleton->GetReferenceSkeleton().GetNum();
		for (int32 i = 1; i < Num; ++i) {
			Skeleton->SetBoneTranslationRetargetingMode(i, Mode, false);
		}
		Skeleton->MarkPackageDirty();
		UE_LOG(LogWotTest, Display, TEXT("[PT] RETARGET %s: %d bones set to %d"), *Skeleton->GetName(), Num - 1, (int32)Mode);
#endif
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotNavRebuild(
	TEXT("wot.Nav.Rebuild"), TEXT("Rebuild the navmesh of this world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->RebuildNav();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotNavProbe(
	TEXT("wot.Nav.Probe"), TEXT("Project points onto the navmesh: wot.Nav.Probe X Y [X Y ...]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!Nav) {
			return;
		}
		for (int32 i = 0; i + 1 < Args.Num(); i += 2) {
			const FVector P(FCString::Atof(*Args[i]), FCString::Atof(*Args[i + 1]), 0.0f);
			FNavLocation Out;
			if (Nav->ProjectPointToNavigation(P, Out, FVector(60.0f, 60.0f, 6000.0f))) {
				UE_LOG(LogWotTest, Display, TEXT("[PT] PROBE (%.0f %.0f) -> navmesh at z=%.0f"), P.X, P.Y, Out.Location.Z);
			} else {
				UE_LOG(LogWotTest, Display, TEXT("[PT] PROBE (%.0f %.0f) -> no navmesh within 60cm"), P.X, P.Y);
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotNavStatus(
	TEXT("wot.Nav.Status"), TEXT("Log navmesh state"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->LogNavState(TEXT("status"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestRecord(
	TEXT("wot.PlayTest.Record"), TEXT("Record frames for a video: wot.PlayTest.Record <name> [fps=5] [seconds=30] [ui=0]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->StartRecording(Args.Num() > 0 ? Args[0] : TEXT("clip"), Args.Num() > 1 ? FCString::Atof(*Args[1]) : 5.0f, Args.Num() > 2 ? FCString::Atof(*Args[2]) : 30.0f, Args.Num() > 3 && FCString::Atoi(*Args[3]) != 0);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestRecordStop(
	TEXT("wot.PlayTest.RecordStop"), TEXT("Stop recording frames"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->StopRecording();
		}
	}));


// ---------------------------------------------------------------------------
// wot.Anim.Status [ActorNameSubstring] : log how a character is animated right now and whether its bones move.
static void WotLogAnimStatus(UWorld* World, const FString& Who)
{
	ACharacter* C = nullptr;
	if (Who.IsEmpty()) {
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		C = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	} else {
		for (TActorIterator<ACharacter> It(World); It; ++It) {
			bool bMatch = It->GetName().Contains(Who);
#if WITH_EDITOR
			bMatch = bMatch || It->GetActorLabel().Contains(Who);
#endif
			if (bMatch) { C = *It; break; }
		}
	}
	if (!C) {
		PT_LOG("ANIM no character for '%s'", *Who);
		return;
	}
	USkeletalMeshComponent* M = C->GetMesh();
	if (!M) {
		PT_LOG("ANIM %s has no mesh component", *C->GetName());
		return;
	}
	USkeletalMesh* SkM = M->GetSkeletalMeshAsset();
	USkeleton* Skel = SkM ? SkM->GetSkeleton() : nullptr;
	UAnimInstance* AI = M->GetAnimInstance();
	PT_LOG("ANIM %s mesh=%s skeleton=%s bones=%d mode=%d animClass=%s instance=%s pause=%d noSkelUpdate=%d tickOpt=%d ticking=%d rendered=%d rate=%.2f playing=%d vel=%.0f",
		*C->GetName(), *GetNameSafe(SkM), *GetNameSafe(Skel), M->GetNumBones(), (int32)M->GetAnimationMode(), *GetNameSafe(M->GetAnimClass()), *GetNameSafe(AI ? AI->GetClass() : nullptr),
		M->bPauseAnims ? 1 : 0, M->bNoSkeletonUpdate ? 1 : 0, (int32)M->VisibilityBasedAnimTickOption, M->IsComponentTickEnabled() ? 1 : 0, M->bRecentlyRendered ? 1 : 0, M->GlobalAnimRateScale, M->IsPlaying() ? 1 : 0, C->GetVelocity().Size());
	{
		IAnimClassInterface* ACI = IAnimClassInterface::GetFromClass(M->GetAnimClass());
		USkeleton* ClassSkel = ACI ? ACI->GetTargetSkeleton() : nullptr;
		TArray<FString> MeshBones, SkelBones;
		if (SkM) for (int32 i = 0; i < SkM->GetRefSkeleton().GetRawBoneNum(); ++i) MeshBones.Add(SkM->GetRefSkeleton().GetBoneName(i).ToString());
		if (Skel) for (int32 i = 0; i < Skel->GetReferenceSkeleton().GetRawBoneNum(); ++i) SkelBones.Add(Skel->GetReferenceSkeleton().GetBoneName(i).ToString());
		PT_LOG("ANIM   spawn enable=%d classSkeleton=%s needSpawn=%d compatMesh=%d registered=%d", M->bEnableAnimation ? 1 : 0, *GetNameSafe(ClassSkel), -1,
			(Skel && SkM) ? (Skel->IsCompatibleMesh(SkM, false) ? 1 : 0) : -1, M->IsRegistered() ? 1 : 0);
		PT_LOG("ANIM   meshBones=[%s]", *FString::Join(MeshBones, TEXT(",")));
		if (SkM) {
			const FReferenceSkeleton& RS = SkM->GetRefSkeleton();
			for (int32 i = 0; i < RS.GetRawBoneNum(); ++i) {
				const FTransform& T = RS.GetRefBonePose()[i];
				const FRotator R = T.GetRotation().Rotator();
				PT_LOG("ANIM   refpose %s parent=%s t=(%.1f %.1f %.1f) r=(p%.1f y%.1f r%.1f) s=(%.3f %.3f %.3f)", *RS.GetBoneName(i).ToString(), RS.GetParentIndex(i) >= 0 ? *RS.GetBoneName(RS.GetParentIndex(i)).ToString() : TEXT("-"), T.GetLocation().X, T.GetLocation().Y, T.GetLocation().Z, R.Pitch, R.Yaw, R.Roll, T.GetScale3D().X, T.GetScale3D().Y, T.GetScale3D().Z);
			}
		}
		PT_LOG("ANIM   skelBones=[%s]", *FString::Join(SkelBones, TEXT(",")));
	}
	if (UWotCharacterAnimInstance* W = Cast<UWotCharacterAnimInstance>(AI)) {
		PT_LOG("ANIM   vars speed=%.0f attacking=%d heavy=%d inAir=%d riding=%d channeling=%d climbing=%d montage=%s", W->Speed, W->bIsAttacking ? 1 : 0, W->bIsHeavyAttack ? 1 : 0, W->bIsInAir ? 1 : 0, W->bIsRiding ? 1 : 0, W->bIsChanneling ? 1 : 0, W->bIsClimbing ? 1 : 0, *GetNameSafe(W->GetCurrentActiveMontage()));
	}
	if (AI) {
		for (TFieldIterator<FObjectProperty> It(AI->GetClass()); It; ++It) {
			if (!It->PropertyClass || !It->PropertyClass->IsChildOf(UAnimationAsset::StaticClass())) continue;
			UAnimationAsset* Asset = Cast<UAnimationAsset>(It->GetObjectPropertyValue_InContainer(AI));
			FString Extra;
			if (UAnimSequence* Seq = Cast<UAnimSequence>(Asset)) {
				Extra = FString::Printf(TEXT("len=%.2f keys=%d compressedValid=%d"), Seq->GetPlayLength(), Seq->GetNumberOfSampledKeys(), Seq->IsCompressedDataValid() ? 1 : 0);
			} else if (UBlendSpace* BS = Cast<UBlendSpace>(Asset)) {
				TArray<FString> Names;
				for (const FBlendSample& Smp : BS->GetBlendSamples()) Names.Add(FString::Printf(TEXT("%s@%.0f"), *GetNameSafe(Smp.Animation), Smp.SampleValue.X));
				Extra = FString::Printf(TEXT("samples=[%s]"), *FString::Join(Names, TEXT(",")));
			}
			PT_LOG("ANIM   asset %s = %s skeleton=%s compatible=%d %s", *It->GetName(), *GetNameSafe(Asset), Asset ? *GetNameSafe(Asset->GetSkeleton()) : TEXT(""),
				(Asset && Skel) ? (Skel->IsCompatibleForEditor(Asset->GetSkeleton()) ? 1 : 0) : -1, *Extra);
		}
	}
	// live pose: component scale, root height, bone-space scale range, height of the bone cloud
	auto PoseSummary = [](USkeletalMeshComponent* Mesh) {
		const TArray<FTransform>& CS = Mesh->GetComponentSpaceTransforms();
		const TArray<FTransform>& BS = Mesh->GetBoneSpaceTransforms();
		float MinZ = FLT_MAX, MaxZ = -FLT_MAX, MinS = FLT_MAX, MaxS = -FLT_MAX; FString Scales;
		for (int32 i = 0; i < CS.Num(); ++i) { MinZ = FMath::Min(MinZ, (float)CS[i].GetLocation().Z); MaxZ = FMath::Max(MaxZ, (float)CS[i].GetLocation().Z); }
		for (int32 i = 0; i < BS.Num(); ++i) {
			const FVector Sc = BS[i].GetScale3D(); MinS = FMath::Min(MinS, (float)Sc.GetMin()); MaxS = FMath::Max(MaxS, (float)Sc.GetMax());
			if (!Sc.Equals(FVector::OneVector, 0.01f)) Scales += FString::Printf(TEXT(" %s=(%.2f %.2f %.2f)"), *Mesh->GetBoneName(i).ToString(), Sc.X, Sc.Y, Sc.Z);
		}
		const FVector RootCS = CS.Num() > 0 ? CS[0].GetLocation() : FVector::ZeroVector;
		const FVector RootBS = BS.Num() > 0 ? BS[0].GetLocation() : FVector::ZeroVector;
		return FString::Printf(TEXT("compScale=(%.2f %.2f %.2f) relLoc=(%.0f %.0f %.0f) rootCS=(%.1f %.1f %.1f) rootBS=(%.1f %.1f %.1f) boneZ=[%.1f..%.1f] boneScale=[%.3f..%.3f] nonUnit=%s"),
			Mesh->GetComponentScale().X, Mesh->GetComponentScale().Y, Mesh->GetComponentScale().Z, Mesh->GetRelativeLocation().X, Mesh->GetRelativeLocation().Y, Mesh->GetRelativeLocation().Z,
			RootCS.X, RootCS.Y, RootCS.Z, RootBS.X, RootBS.Y, RootBS.Z, MinZ, MaxZ, MinS, MaxS, Scales.IsEmpty() ? TEXT("none") : *Scales);
	};
	PT_LOG("ANIM   pose %s", *PoseSummary(M));
	// bone motion: compare component-space transforms now and 0.35 s later
	TArray<FTransform> Before = M->GetComponentSpaceTransforms();
	TWeakObjectPtr<USkeletalMeshComponent> WeakM = M;
	FTimerHandle H;
	World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WeakM, Before, PoseSummary]() {
		if (!WeakM.IsValid()) return;
		PT_LOG("ANIM   pose+0.35s %s", *PoseSummary(WeakM.Get()));
		const TArray<FTransform>& After = WeakM->GetComponentSpaceTransforms();
		float Moved = 0.0f; int32 N = FMath::Min(Before.Num(), After.Num()); int32 Movers = 0;
		for (int32 i = 0; i < N; ++i) {
			const float D = FVector::Dist(Before[i].GetLocation(), After[i].GetLocation()) + (float)FMath::RadiansToDegrees(Before[i].GetRotation().AngularDistance(After[i].GetRotation()));
			Moved += D; if (D > 0.5f) ++Movers;
		}
		UWotCharacterAnimInstance* W = Cast<UWotCharacterAnimInstance>(WeakM->GetAnimInstance());
		PT_LOG("ANIM   motion bones=%d moved=%d total=%.1f speed=%.0f attacking=%d", N, Movers, Moved, W ? W->Speed : -1.0f, W ? (W->bIsAttacking ? 1 : 0) : -1);
	}), 0.35f, false);
}

#if WITH_EDITOR
// wot.Anim.Dump <AnimSequencePath> : per-bone translation / scale ranges of the sequence's source keys
static FAutoConsoleCommand GWotAnimDump(
	TEXT("wot.Anim.Dump"), TEXT("Log per-bone translation / scale ranges of a sequence, or the bone hierarchy of a skeleton / skeletal mesh: wot.Anim.Dump /Game/Path/Asset"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		if (Args.Num() < 1) { PT_LOG("ANIMDUMP usage: wot.Anim.Dump /Game/Path/Anim"); return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UObject* Obj = FSoftObjectPath(Path).TryLoad();
		if (!Obj) { PT_LOG("ANIMDUMP not found: %s", *Path); return; }
		// a skeleton or skeletal mesh: the bone hierarchy with reference pose (and the skeleton's translation retargeting modes)
		const FReferenceSkeleton* RefSkel = nullptr; USkeleton* Skeleton = Cast<USkeleton>(Obj);
		if (Skeleton) RefSkel = &Skeleton->GetReferenceSkeleton();
		else if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Obj)) RefSkel = &Mesh->GetRefSkeleton();
		if (RefSkel) {
			PT_LOG("ANIMDUMP %s bones=%d", *Obj->GetName(), RefSkel->GetRawBoneNum());
			for (int32 i = 0; i < RefSkel->GetRawBoneNum(); ++i) {
				const FTransform& T = RefSkel->GetRefBonePose()[i]; const FRotator R = T.GetRotation().Rotator();
				PT_LOG("ANIMDUMP   %d %s parent=%s t=(%.2f %.2f %.2f) r=(p%.1f y%.1f r%.1f) s=(%.2f %.2f %.2f) retarget=%d", i, *RefSkel->GetBoneName(i).ToString(),
					RefSkel->GetParentIndex(i) >= 0 ? *RefSkel->GetBoneName(RefSkel->GetParentIndex(i)).ToString() : TEXT("-"), T.GetLocation().X, T.GetLocation().Y, T.GetLocation().Z,
					R.Pitch, R.Yaw, R.Roll, T.GetScale3D().X, T.GetScale3D().Y, T.GetScale3D().Z, Skeleton ? (int32)Skeleton->GetBoneTranslationRetargetingMode(i) : -1);
			}
			return;
		}
		UAnimSequence* Seq = Cast<UAnimSequence>(Obj);
		if (!Seq) { PT_LOG("ANIMDUMP %s is a %s, not a sequence / skeleton / mesh", *Path, *Obj->GetClass()->GetName()); return; }
		const IAnimationDataModel* Model = Seq->GetDataModel();
		if (!Model) { PT_LOG("ANIMDUMP %s has no data model", *Path); return; }
		PT_LOG("ANIMDUMP %s skeleton=%s len=%.2f keys=%d tracks=%d", *Seq->GetName(), *GetNameSafe(Seq->GetSkeleton()), Seq->GetPlayLength(), Model->GetNumberOfKeys(), Model->GetNumBoneTracks());
		TArray<FName> Names; Model->GetBoneTrackNames(Names);
		for (const FName& Name : Names) {
			TArray<FTransform> Keys; Model->GetBoneTrackTransforms(Name, Keys);
			if (Keys.Num() == 0) continue;
			FVector TMin = Keys[0].GetLocation(), TMax = TMin, SMin = Keys[0].GetScale3D(), SMax = SMin;
			for (const FTransform& K : Keys) { TMin = TMin.ComponentMin(K.GetLocation()); TMax = TMax.ComponentMax(K.GetLocation()); SMin = SMin.ComponentMin(K.GetScale3D()); SMax = SMax.ComponentMax(K.GetScale3D()); }
			PT_LOG("ANIMDUMP   %s keys=%d t=[(%.1f %.1f %.1f)..(%.1f %.1f %.1f)] s=[(%.3f %.3f %.3f)..(%.3f %.3f %.3f)]", *Name.ToString(), Keys.Num(), TMin.X, TMin.Y, TMin.Z, TMax.X, TMax.Y, TMax.Z, SMin.X, SMin.Y, SMin.Z, SMax.X, SMax.Y, SMax.Z);
		}
	}));
#endif

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestAttack(
	TEXT("wot.PlayTest.Attack"), TEXT("Start the player's primary attack (released 0.4 s later): wot.PlayTest.Attack"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AWotCharacter* C = PC ? Cast<AWotCharacter>(PC->GetPawn()) : nullptr;
		if (!C) {
			PT_LOG("ATTACK no player character");
			return;
		}
		UWotActionComponent* AC = C->FindComponentByClass<UWotActionComponent>();
		if (!AC) {
			PT_LOG("ATTACK no action component");
			return;
		}
		const bool bStarted = AC->StartActionByName(FName("PrimaryAttack"), C);
		PT_LOG("ATTACK started=%d", bStarted ? 1 : 0);
		TWeakObjectPtr<UWotActionComponent> Weak = AC; TWeakObjectPtr<AWotCharacter> WeakC = C;
		FTimerHandle H;
		World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([Weak, WeakC]() {
			if (Weak.IsValid()) { Weak->StopActionByName(FName("PrimaryAttack"), WeakC.Get()); PT_LOG("ATTACK stopped"); }
		}), 0.4f, false);
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotAnimStatus(
	TEXT("wot.Anim.Status"), TEXT("Log how a character is animated: wot.Anim.Status [ActorNameSubstring] (default: the player)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		WotLogAnimStatus(World, Args.Num() > 0 ? Args[0] : FString());
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotPlayTestStatus(
	TEXT("wot.PlayTest.Status"), TEXT("Log where the player is"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotPlayTestSubsystem* PT = GetPlayTest(World)) {
			PT->LogStatus();
		}
	}));
