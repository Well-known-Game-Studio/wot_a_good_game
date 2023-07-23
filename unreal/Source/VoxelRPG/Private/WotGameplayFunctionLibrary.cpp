#include "WotGameplayFunctionLibrary.h"
#include "WotAttributeComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"

void UWotGameplayFunctionLibrary::DrawHitPointAndBounds(AActor* HitActor, const FHitResult& Hit)
{
  if (!HitActor) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot draw hit point and bounds, invalid actor pointer!"));
    return;
  }
  FVector HitActorLocation;
  FVector HitBoxExtent;
  HitActor->GetActorBounds(false, HitActorLocation, HitBoxExtent, false);
  // draw a box around what was hit
  DrawDebugBox(HitActor->GetWorld(), HitActorLocation, HitBoxExtent,
               HitActor->GetActorRotation().Quaternion(), FColor::Green, false, 2.0f, 0, 2.0f);
  // draw a point for the hit location itself
  DrawDebugPoint(HitActor->GetWorld(), Hit.ImpactPoint, 10, FColor::Red, false, 2.0f, 100);
}

bool UWotGameplayFunctionLibrary::ApplyDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount)
{
  UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(TargetActor);
  if (AttributeComp) {
    return AttributeComp->ApplyHealthChangeInstigator(DamageCauser, DamageAmount);
  } else if (TargetActor) {
    // See if the actor we hit is actually attached to an actor that can be
    // damaged; if so then damage that actor
    AActor* ParentActor = TargetActor->GetAttachParentActor();
    UE_LOG(LogTemp, Log, TEXT("ApplyDamage: got attach parent actor '%s'"), *GetNameSafe(ParentActor));
    return ApplyDamage(DamageCauser, ParentActor, DamageAmount);
  }
  return false;
}

bool UWotGameplayFunctionLibrary::ApplyDirectionalDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount, const FHitResult& HitResult)
{
  if (ApplyDamage(DamageCauser, TargetActor, DamageAmount)) {
    // Ensure the actor we're going to apply an impulse (for explosion) to has
    // collision enabled
    TargetActor->SetActorEnableCollision(true);
    UPrimitiveComponent* HitComp = HitResult.GetComponent();
    if (HitComp && HitComp->IsSimulatingPhysics(HitResult.BoneName)) {
      FVector Direction = HitResult.TraceEnd - HitResult.TraceStart;
      Direction.Normalize();
      HitComp->AddImpulseAtLocation(Direction * 3000.0f, HitResult.ImpactPoint, HitResult.BoneName);
    }
    return true;
  }
  return false;
}

FText UWotGameplayFunctionLibrary::GetFloatAsTextWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero)
{
  FNumberFormattingOptions NumberFormat;
  NumberFormat.MinimumIntegralDigits = (IncludeLeadingZero) ? 1 : 0;
  NumberFormat.MaximumIntegralDigits = 10000;
  NumberFormat.MinimumFractionalDigits = Precision;
  NumberFormat.MaximumFractionalDigits = Precision;
  return FText::AsNumber(TheFloat, &NumberFormat);
}

FString UWotGameplayFunctionLibrary::GetFloatAsStringWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero)
{
  return GetFloatAsTextWithPrecision(TheFloat, Precision, IncludeLeadingZero).ToString();
}

FText UWotGameplayFunctionLibrary::GetIntAsText(int TheNumber)
{
  FNumberFormattingOptions NumberFormat;
  NumberFormat.MinimumIntegralDigits = 1;
  NumberFormat.MaximumIntegralDigits = 10000;
  NumberFormat.MinimumFractionalDigits = 0;
  NumberFormat.MaximumFractionalDigits = 0;
  return FText::AsNumber(TheNumber, &NumberFormat);
}

FString UWotGameplayFunctionLibrary::GetIntAsString(int TheNumber)
{
  return GetIntAsText(TheNumber).ToString();
}

void UWotGameplayFunctionLibrary::GetAllCppSubclasses(UClass* BaseClass, TArray<UClass*>& ClassArray)
{
	FName BaseClassName = BaseClass->GetFName();
	UE_LOG(LogTemp, Log, TEXT("Getting all c++ subclasses of '%s'"), *BaseClassName.ToString());
  bool bRecursive = true;
  GetDerivedClasses(BaseClass, ClassArray, bRecursive);
}

void UWotGameplayFunctionLibrary::GetAllBlueprintSubclasses(UClass* BaseClass, TArray<UClass*>& ClassArray)
{
	FName BaseClassName = BaseClass->GetFName();
	UE_LOG(LogTemp, Log, TEXT("Getting all blueprint subclasses of '%s'"), *BaseClassName.ToString());

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FAssetData> AssetData;
	// The asset registry is populated asynchronously at startup, so there's no guarantee it has finished.
	// This simple approach just runs a synchronous scan on the entire content directory.
	// Better solutions would be to specify only the path to where the relevant blueprints are,
	// or to register a callback with the asset registry to be notified of when it's finished populating.
	TArray< FString > ContentPaths;
	ContentPaths.Add(TEXT("/Game"));
	AssetRegistry.ScanPathsSynchronous(ContentPaths);

	// Use the asset registry to get the set of all class names deriving from Base
	TSet< FName > DerivedNames;
	{
		TArray< FName > BaseNames;
		BaseNames.Add(BaseClassName);

		TSet< FName > Excluded;
        AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
		// AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
	}

	FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	// Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray< FAssetData > AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	// Iterate over retrieved blueprint assets
	for(auto const& Asset : AssetList) {
		// Get the the class this blueprint generates (this is stored as a full path)
        auto GeneratedClassPathPtr = Asset.TagsAndValues.FindTag(TEXT("GeneratedClass")).AsString();
		if(!GeneratedClassPathPtr.IsEmpty()) {
			// Convert path to just the name part
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr);
			const FString ObjectClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

			// Check if this class is in the derived set
			if(!DerivedNames.Contains(*ObjectClassName)) {
				continue;
			}

			UClass* Class = nullptr;
			const UBlueprint* BlueprintAsset = Cast<UBlueprint>(Asset.GetAsset());
			if (BlueprintAsset) {
				Class = BlueprintAsset->GeneratedClass;
			} else {
				UE_LOG(LogTemp, Error, TEXT("Could not cast '%s' to blueprint class"), *ObjectClassName);
			}
			if (Class) {
				UE_LOG(LogTemp, Log, TEXT("Got subclass '%s'"), *ObjectClassName);
				ClassArray.Add(Class);
			} else {
				UE_LOG(LogTemp, Error, TEXT("Invalid BP Class Data!"));
			}
		}
	}
}
