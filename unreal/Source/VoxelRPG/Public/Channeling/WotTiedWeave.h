#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Channeling/WotChannelComponent.h"
#include "WotTiedWeave.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;
class UBoxComponent;
class UPointLightComponent;

/**
 * A weave that has been tied off: the braided threads hold on their own at the spot where the channeler
 * tied them and keep working until the knot unravels (Duration) or the actor is destroyed.
 *   Fire  - a burning ward: enemies inside take damage over time.
 *   Air   - a standing wall of air: enemies inside are pushed out.
 *   Water - a healing pool for the channeler and other players inside.
 *   Earth - a solid wall of earth across the channeler's facing that blocks movement and projectiles.
 * The visual is a knot of NS_WeaveThread ribbons anchored to the ground around the centre.
 */
UCLASS()
class VOXELRPG_API AWotTiedWeave : public AActor
{
	GENERATED_BODY()

public:
	AWotTiedWeave();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weave")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weave")
	TObjectPtr<USphereComponent> Reach;

	/** Only used by Earth: the wall itself. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weave")
	TObjectPtr<UBoxComponent> Wall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weave")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave")
	EWotWeave Weave = EWotWeave::Fire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave")
	EWotPowerHalf Half = EWotPowerHalf::Saidar;

	/** Strength x charge at the moment of tying. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave")
	float Power = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave")
	FLinearColor Color = FLinearColor::White;

	/** Seconds before the knot unravels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave")
	float Duration = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave")
	float Radius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave|Effects")
	float FireDamagePerSecond = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave|Effects")
	float AirPush = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave|Effects")
	float WaterHealPerSecond = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave|Effects")
	TSoftObjectPtr<UNiagaraSystem> ThreadSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weave|Effects")
	int32 KnotThreads = 9;

	/** Call right after spawning (before BeginPlay runs) to configure the knot. */
	void Configure(EWotWeave InWeave, EWotPowerHalf InHalf, float InPower, const FLinearColor& InColor, float InDuration);

	UFUNCTION(BlueprintCallable, Category = "Weave")
	float TimeLeft() const { return FMath::Max(0.0f, Duration - Age); }

	/** Unravels the weave now (fades the threads and removes the actor). */
	UFUNCTION(BlueprintCallable, Category = "Weave")
	void Unravel();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	void BuildKnot();
	UNiagaraComponent* SpawnThread(const FVector& From, const FVector& To, float Phase, float Amplitude, float Width);

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> Threads;

	float Age = 0.0f;
	float PulseAccumulator = 0.0f;
	bool bUnravelling = false;
};
