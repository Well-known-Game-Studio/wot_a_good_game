#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WotChannelComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AWotCharacter;

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
	/** Threads gathering on the hands while channeling (both halves). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channeling|Effects")
	TSoftObjectPtr<UNiagaraSystem> ThreadsSystem;

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Channeling")
	static FText WeaveName(EWotWeave Weave);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	bool bChanneling = false;
	double ChannelStartTime = 0.0;
	FTimerHandle ShieldTimer;

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> ActiveEffects;

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
