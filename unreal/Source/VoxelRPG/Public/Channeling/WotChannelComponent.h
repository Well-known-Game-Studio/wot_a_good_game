#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WotChannelComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AWotCharacter;
class AWotTiedWeave;
class UWotChannelCameraShake;
class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Which half of the One Power the channeler touches. */
UENUM(BlueprintType)
enum class EWotPowerHalf : uint8
{
	None,
	Saidin   UMETA(DisplayName = "saidin (male half)"),
	Saidar   UMETA(DisplayName = "saidar (female half)")
};

UENUM(BlueprintType)
enum class EWotWeave : uint8
{
	Fire   UMETA(ToolTip = "A bolt of fire (projectile)"),
	Air    UMETA(ToolTip = "A wall of air: throws nearby enemies back"),
	Water  UMETA(ToolTip = "Healing (Water and Spirit)"),
	Earth  UMETA(ToolTip = "A shield of earth: parries for a few seconds")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWotOnWeave, EWotWeave, Weave, float, Charge);

/**
 * Channeling the One Power. Hold the channel input to embrace saidar / seize
 * saidin (threads of light gather on the hands), release to loose the selected
 * weave. Charge builds while holding.
 *
 * saidar is the clean, steady light of the show. saidin depends on the world:
 * until the SaidinCleansed story flag is set (UWotSaveSubsystem, saved with the
 * game) it carries the Dark One's taint: the threads turn black and bruised
 * purple and flicker, shadow clings to the hands, black ooze drips from them and
 * pools on the ground, and the screen desaturates and darkens at the edges.
 * Cleansed saidin is the raging torrent of fire and ice: white-blue threads that
 * thrash and surge with lightning, and a camera that trembles with the effort of
 * holding it. Both halves of saidin shake the camera; the taint lurches, clean
 * saidin trembles.
 */
UCLASS(ClassGroup = (Wot), meta = (BlueprintSpawnableComponent))
class VOXELRPG_API UWotChannelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWotChannelComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling")
	EWotPowerHalf Half = EWotPowerHalf::Saidin;

	/** 0 = cannot channel. Scales damage, push and healing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling", meta = (ClampMin = 0.0, ClampMax = 3.0))
	float Strength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling")
	EWotWeave SelectedWeave = EWotWeave::Fire;

	/** Seconds of holding for a full charge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling")
	float FullChargeSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling")
	FName LeftHandSocket = TEXT("Hand_L");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling")
	FName RightHandSocket = TEXT("Hand_R");

	// --- effects (soft so the component works without the marketplace pack)
	/** Legacy hand effect (unused when ThreadSystem is set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TSoftObjectPtr<UNiagaraSystem> ThreadsSystem;

	/**
	 * The braided ribbon (NS_WeaveThread) drawn from each environment source to the hands: fire is pulled from
	 * lamps, torches and hearths, water from streams, wells and reeds, earth from the ground and stone, air from
	 * the sky around the channeler. Each thread is one component with User.Source / User.Target / User.Color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TSoftObjectPtr<UNiagaraSystem> ThreadSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	int32 ThreadCount = 5;

	/** How far the channeler reaches for sources in the environment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	float SourceSearchRadius = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	float ThreadWidth = 20.0f;

	/** Actor spawned when a weave is tied off (T / gamepad LB while channeling). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	TSubclassOf<AWotTiedWeave> TiedWeaveClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	float TiedWeaveSeconds = 30.0f;

	/** Oldest tied weave unravels when more than this many are held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	int32 MaxTiedWeaves = 3;

	/** Glow burst on the hands when the Power is first embraced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TSoftObjectPtr<UNiagaraSystem> EmbraceSystemSaidar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TSoftObjectPtr<UNiagaraSystem> EmbraceSystemSaidin;

	/** The taint: a dark flicker on the hands (tainted saidin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UNiagaraSystem> TaintSystem;

	/** Shadow clinging to both hands while tainted saidin is held (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UNiagaraSystem> TaintTrailSystem;

	/** Burst of shadow when a tainted weave is loosed or surges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UNiagaraSystem> TaintBurstSystem;

	/** Lightning crackling on the hands while clean saidin is held (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UNiagaraSystem> SurgeTrailSystem;

	/** Lightning burst at the hands when clean saidin surges or is loosed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UNiagaraSystem> SurgeBurstSystem;

	/** Camera shake while saidin is held (the local player only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSubclassOf<UWotChannelCameraShake> SaidinShakeClass;

	/** Overall camera shake strength; 0 turns it off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin", meta = (ClampMin = 0.0, ClampMax = 3.0))
	float SaidinShakeScale = 1.0f;

	/** Post-process while saidin is held (desaturation and dark vignette when tainted, bloom and fringe when clean); 0 turns it off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float SaidinScreenEffect = 1.0f;

	/** The ooze: dark blobs dripping from the hands while tainted saidin is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UStaticMesh> OozeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	TSoftObjectPtr<UMaterialInterface> OozeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	FLinearColor OozeColor = FLinearColor(0.012f, 0.0f, 0.02f);

	/** Drops per second at full charge; 0 turns the ooze off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin", meta = (ClampMin = 0.0, ClampMax = 30.0))
	float OozeRate = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects|Saidin")
	int32 MaxOozeDrops = 36;

	/** Released-weave burst at the hands / on the ground, per weave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TMap<EWotWeave, TSoftObjectPtr<UNiagaraSystem>> ReleaseSystems;

	/** Projectile actor for Fire (defaults to the MagicProjectile Blueprint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	TSoftClassPtr<AActor> FireProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	float AirRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	float AirKnockback = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	float AirDamage = -5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	float WaterHeal = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	float EarthShieldSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Weaves")
	FGameplayTag EarthShieldTag;

	UPROPERTY(BlueprintAssignable, Category = "Channeling")
	FWotOnWeave OnWeaveReleased;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	bool CanChannel() const { return Half != EWotPowerHalf::None && Strength > 0.0f; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	bool IsChanneling() const { return bChanneling; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	float GetCharge() const;

	UFUNCTION(BlueprintCallable, Category = "Channeling")
	void BeginChannel();

	UFUNCTION(BlueprintCallable, Category = "Channeling")
	void ReleaseWeave();

	UFUNCTION(BlueprintCallable, Category = "Channeling")
	void CancelChannel();

	UFUNCTION(BlueprintCallable, Category = "Channeling")
	void SelectWeave(EWotWeave Weave);

	UFUNCTION(BlueprintCallable, Category = "Channeling")
	void NextWeave();

	UFUNCTION(BlueprintCallable, Category = "Channeling")
	void PrevWeave();

	/** Ties off the weave being channeled so it holds on its own in front of the channeler. */
	UFUNCTION(BlueprintCallable, Category = "Channeling")
	bool TieOffWeave();

	/** Element colour for a half of the Power; saidin is read as tainted (see ElementColorFor). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	static FLinearColor ElementColor(EWotWeave Weave, EWotPowerHalf InHalf);

	/** Element colour: saidar is the clean white light, tainted saidin black and bruised purple, cleansed saidin white-blue fire and ice. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	static FLinearColor ElementColorFor(EWotWeave Weave, EWotPowerHalf InHalf, bool bSaidinCleansed);

	/** True while this channeler holds saidin and saidin has not been cleansed (UWotSaveSubsystem::IsSaidinCleansed). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	bool IsTainted() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	bool IsSaidin() const { return Half == EWotPowerHalf::Saidin; }

	/** 0..~1.5: how wildly saidin thrashes right now (noise plus surges); 0 for saidar. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	float GetChaos() const { return Chaos; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	static FText WeaveName(EWotWeave Weave);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AWotCharacter* GetCharacter() const;

protected:
	bool bChanneling = false;
	double ChannelStartTime = 0.0;
	FTimerHandle ShieldTimer;

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> ActiveEffects;

	/** One environment thread: where it is drawn from and the ribbon component drawing it. */
	struct FWotThread
	{
		TWeakObjectPtr<UNiagaraComponent> Comp;
		TWeakObjectPtr<AActor> SourceActor;   // follows a moving source (a carried lantern, a horse)
		FVector SourceOffset = FVector::ZeroVector;
		FVector Source = FVector::ZeroVector;
		FVector Drift = FVector::ZeroVector;  // free sources (air, mist, embers) wander
		float Bob = 0.0f;
		float Amplitude = 30.0f;               // the ribbon's wave at rest; saidin multiplies it by its chaos
		bool bFromEnvironment = false;
	};
	TArray<FWotThread> Threads;

	// --- saidin: chaos, surges, ooze, camera
	/** A drop of the taint: falls from a hand, splats on the ground, sinks away. */
	struct FWotOozeDrop
	{
		TWeakObjectPtr<UStaticMeshComponent> Comp;
		FVector Velocity = FVector::ZeroVector;
		float GroundZ = 0.0f;
		float Size = 8.0f;
		float Age = 0.0f;       // since the splat (< 0 while falling)
		bool bLanded = false;
	};
	TArray<FWotOozeDrop> Ooze;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OozeMID;

	TWeakObjectPtr<UWotChannelCameraShake> Shake;
	float Chaos = 0.0f;
	float SurgeUntil = 0.0f;
	float NextSurgeTime = 0.0f;
	float OozeAccumulator = 0.0f;
	float ReleaseKickUntil = 0.0f;
	bool bChanneledTainted = false;   // the taint state when channeling began (so the release burst matches)

	void BeginSaidin();
	void UpdateSaidin(float DeltaTime);
	void StopSaidin(bool bLoosed, const FVector& LoosedAt);
	void SpawnOozeDrop(const FVector& From);
	void UpdateOoze(float DeltaTime);
	void DestroyOoze();
	void UpdateCamera(float DeltaTime, bool bHolding);
	class APlayerController* LocalPlayerController() const;

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> ThreadComps;

	UPROPERTY()
	TArray<TWeakObjectPtr<AWotTiedWeave>> TiedWeaves;

	FTimerHandle ThreadFadeTimer;

	void SpawnThreads();
	void GatherSources(EWotWeave Weave, const FVector& Origin, int32 Count, TArray<FWotThread>& Out) const;
	void UpdateThreads(float DeltaTime);
	void ReleaseThreads(const FVector& To);
	void DestroyThreads();
	FVector HandsPoint() const;

	UNiagaraComponent* AttachEffect(const TSoftObjectPtr<UNiagaraSystem>& System, FName Socket, bool bLoop);
	UNiagaraComponent* AttachEffectScaled(const TSoftObjectPtr<UNiagaraSystem>& System, const FVector& Offset, float Scale);
	void StopEffects();
	void SpawnReleaseEffect(EWotWeave Weave, const FVector& Location);

	void WeaveFire(float Charge);
	void WeaveAir(float Charge);
	void WeaveWater(float Charge);
	void WeaveEarth(float Charge);
	void EndEarthShield();
};
