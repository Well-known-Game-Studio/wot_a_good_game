#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WotPlayTestSubsystem.generated.h"

class AWotAICharacter;

/**
 * In-game walkability / spawn test harness driven by console commands.
 * Everything it finds is written to the LogWotTest category with a "[PT]"
 * prefix so it can be collected from the log.
 *
 *   wot.PlayTest.Spawn                   - where the player spawned, ground under it, NPC ground clearance
 *   wot.PlayTest.Walk [N] [Seconds] [1]  - walk the player to the N nearest NPCs (default 6, 120 s), optionally interacting
 *   wot.PlayTest.Goto X Y [Seconds]      - walk the player to a world location
 *   wot.PlayTest.Stop                    - abort the current walk
 *   wot.PlayTest.Status                  - one line of where the player is now
 *
 * While a Game/PIE world runs, the subsystem also polls
 * <ProjectSavedDir>/PlayTest/commands.txt every half second; each line of
 * that file is executed as a console command and the file is deleted. This
 * lets external tooling (the editor MCP server has no console-exec tool)
 * drive play sessions.
 */
UCLASS()
class VOXELRPG_API UWotPlayTestSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UWotPlayTestSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }

	/** Log navmesh state (bounds, tiles, whether it is building, nearest nav point to the player). */
	void LogNavState(const TCHAR* Tag);
	/** Rebuild navigation for this world (editor or PIE). */
	void RebuildNav();

	void RunSpawnCheck();
	void StartWalkToNPCs(int32 MaxTargets, float TimeLimit, bool bInteract);
	void StartWalkToLocation(const FVector& Location, float TimeLimit);
	/** Walk to the nearest rideable animal, mount it, ride for RideSeconds, take a screenshot, dismount. */
	void StartMountTest(float RideSeconds);
	void Screenshot(const FString& Name, bool bWithUI = false);
	/** Save a frame every 1/Fps seconds for Seconds (Saved/Screenshots/<platform>/rec_<Name>_NNNNN.png) to cut into a video. */
	void StartRecording(const FString& Name, float Fps, float Seconds, bool bWithUI);
	void StopRecording();
	void StopWalk(const TCHAR* Reason);
	void LogStatus();

protected:
	struct FTarget
	{
		TWeakObjectPtr<AActor> Actor;
		FVector Location = FVector::ZeroVector;
		FString Name;
		TArray<FVector> Path;
		int32 PathIndex = 0;
		bool bPartialPath = false;
		bool bHasPath = false;
	};

	bool bWalking = false;
	bool bMountTest = false;
	bool bRiding = false;
	float RideTime = 0.0f;
	float RideSeconds = 6.0f;
	float SinceRideLog = 0.0f;
	bool bRideShotTaken = false;
	FString ScreenshotPrefix;
	bool bRecording = false;
	bool bRecordUI = false;
	float RecordInterval = 0.2f;
	float RecordLeft = 0.0f;
	float SinceFrame = 0.0f;
	int32 RecordFrame = 0;
	FString RecordName;
	bool bWaitingForNav = false;
	float NavWait = 0.0f;
	void StartWalkNow();
	float SinceCommandPoll = 0.0f;
	void PollCommandFile();
	FString CommandFilePath() const;
	bool bInteractWithTargets = false;
	TArray<FTarget> Targets;
	int32 TargetIndex = -1;
	float Elapsed = 0.0f;
	float TimeLimit = 120.0f;
	float TargetElapsed = 0.0f;
	float SinceProgress = 0.0f;
	float SinceLog = 0.0f;
	float FallingTime = 0.0f;
	float BestDistance = 0.0f;
	float StartZ = 0.0f;
	float MinZ = 0.0f;
	float MaxZ = 0.0f;
	float MaxStepUp = 0.0f;
	float LastZ = 0.0f;
	float SinceStepSample = 0.0f;
	float InteractDelay = 0.0f;
	int32 DialogPresses = 0;
	float SincePath = 0.0f;
	float StuckTime = 0.0f;
	float SinceStuckSample = 0.0f;
	float SinceJump = 0.0f;
	bool bRepathedWhileStuck = false;
	FVector LastSamplePos = FVector::ZeroVector;
	int32 NoPath = 0;
	int32 PartialPaths = 0;
	void ComputePath(FTarget& Target, const FVector& From);
	bool IsDialogueOpen(APawn* Pawn) const;
	int32 Reached = 0;
	int32 Stuck = 0;
	int32 Jumps = 0;
	TArray<FString> StuckAt;
	TArray<FString> Interactions;
	bool bTargetInteracting = false;

	APawn* GetPlayerPawn() const;
	void BeginTarget(int32 Index);
	void FinishTarget(bool bReachedTarget);
	void Summary(const TCHAR* Reason);
	bool TraceGround(const FVector& From, float Up, float Down, FHitResult& OutHit, const AActor* Ignore) const;
	FString DescribeActor(const AActor* Actor) const;
	bool CheckInteractable(const FTarget& Target);
	int32 InteractRetries = 0;
};
