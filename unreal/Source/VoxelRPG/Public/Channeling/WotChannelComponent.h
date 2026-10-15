#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WotChannelComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AWotCharacter;
class AWotTiedWeave;

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
 * saidin (threads of light gather on the hands; saidin adds the taint's dark
 * flicker), release to loose the selected weave. Charge builds while holding.
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

	/** The taint: a dark flicker that only saidin shows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TSoftObjectPtr<UNiagaraSystem> TaintSystem;

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	static FLinearColor ElementColor(EWotWeave Weave, EWotPowerHalf InHalf);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	static FText WeaveName(EWotWeave Weave);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
		bool bFromEnvironment = false;
	};
	TArray<FWotThread> Threads;

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

	AWotCharacter* GetCharacter() const;
	UNiagaraComponent* AttachEffect(const TSoftObjectPtr<UNiagaraSystem>& System, FName Socket, bool bLoop);
	void StopEffects();
	void SpawnReleaseEffect(EWotWeave Weave, const FVector& Location);

	void WeaveFire(float Charge);
	void WeaveAir(float Charge);
	void WeaveWater(float Charge);
	void WeaveEarth(float Charge);
	void EndEarthShield();
};
