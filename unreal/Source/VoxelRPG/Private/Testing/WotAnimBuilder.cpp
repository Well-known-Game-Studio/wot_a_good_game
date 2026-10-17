// wot.Anim.Build: generates the shared humanoid clips, locomotion blend space and clip set (editor only).
//
// The voxel humanoids (Rand medium-flat, Tam, Lan, Fade, Trolloc, the main cast) share a six-bone rig:
// root (hips, pointing up) with head, arm_r, arm_l, leg_l, leg_r hanging off it, each with an _end leaf. Clips are
// authored here as pose keys in MESH space (forward = +Y, right = -X, up = +Z: the mesh component is yawed -90 in the
// character Blueprints) and converted to bone-local keys against the skeleton's reference pose, so the same tables work
// for every rig that shares the bone names. Translation keys stay at the reference pose; set the skeletons' bone
// translation retargeting to "Skeleton" so each mesh keeps its own proportions (wot.Skeleton.SetTranslationRetargeting).
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/Skeleton.h"
#include "Animation/WotAnimSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotAnimBuild, Log, All);
#define AB_LOG(Fmt, ...) UE_LOG(LogWotAnimBuild, Display, TEXT("[AB] ") Fmt, ##__VA_ARGS__)

namespace WotAnimBuild
{
	const FVector Forward(0, 1, 0), Right(-1, 0, 0), Up(0, 0, 1);

	/** One bone's pose in a key: the bone tip moves Fwd degrees toward the mesh forward (negative = back), Out degrees away
	 *  from the body's centre line, and twists about its own axis. */
	struct FBonePose { FName Bone; float Fwd = 0, Out = 0, Twist = 0; };
	struct FKey
	{
		float Time = 0;
		TArray<FBonePose> Bones;
		FVector RootOffset = FVector::ZeroVector;   // cm along (forward, right, up)
	};
	struct FClip { FString Name; bool bLoop = false; float Length = 1.0f; TArray<FKey> Keys; };

	FBonePose B(const TCHAR* Bone, float Fwd, float Out = 0, float Twist = 0) { FBonePose P; P.Bone = Bone; P.Fwd = Fwd; P.Out = Out; P.Twist = Twist; return P; }
	FKey K(float T, std::initializer_list<FBonePose> Bones, FVector Off = FVector::ZeroVector) { FKey Key; Key.Time = T; Key.Bones = Bones; Key.RootOffset = Off; return Key; }

	TArray<FClip> Clips()
	{
		TArray<FClip> Out;
		// ---- run: a bigger, faster stride than the hand-made walk, with a forward lean and a bob at each passing
		{ FClip C; C.Name = TEXT("A_Humanoid_Run"); C.bLoop = true; C.Length = 0.56f;
		  C.Keys = { K(0.00f, {B(TEXT("leg_l"), 48), B(TEXT("leg_r"), -48), B(TEXT("arm_r"), 42), B(TEXT("arm_l"), -42), B(TEXT("root"), 12), B(TEXT("head"), -6)}, FVector(0, 0, 0)),
		             K(0.14f, {B(TEXT("leg_l"), 0), B(TEXT("leg_r"), 0), B(TEXT("arm_r"), 0), B(TEXT("arm_l"), 0), B(TEXT("root"), 12), B(TEXT("head"), -6)}, FVector(0, 0, 6)),
		             K(0.28f, {B(TEXT("leg_l"), -48), B(TEXT("leg_r"), 48), B(TEXT("arm_r"), -42), B(TEXT("arm_l"), 42), B(TEXT("root"), 12), B(TEXT("head"), -6)}, FVector(0, 0, 0)),
		             K(0.42f, {B(TEXT("leg_l"), 0), B(TEXT("leg_r"), 0), B(TEXT("arm_r"), 0), B(TEXT("arm_l"), 0), B(TEXT("root"), 12), B(TEXT("head"), -6)}, FVector(0, 0, 6)) };
		  Out.Add(C); }
		// ---- heavy attack: wind the right arm up over the head, chop down with the torso, recover
		{ FClip C; C.Name = TEXT("A_Humanoid_HeavyAttack"); C.Length = 0.95f;
		  C.Keys = { K(0.00f, {}),
		             K(0.30f, {B(TEXT("arm_r"), 172, 10), B(TEXT("arm_l"), -20, 15), B(TEXT("root"), -10, 0, 18), B(TEXT("head"), -8)}),
		             K(0.44f, {B(TEXT("arm_r"), 35, 0), B(TEXT("arm_l"), -30, 20), B(TEXT("root"), 18, 0, -12), B(TEXT("head"), 10)}, FVector(6, 0, -4)),
		             K(0.62f, {B(TEXT("arm_r"), 35, 0), B(TEXT("arm_l"), -30, 20), B(TEXT("root"), 18, 0, -12), B(TEXT("head"), 10)}, FVector(6, 0, -4)),
		             K(0.95f, {}) };
		  Out.Add(C); }
		// ---- cast: pull the right hand back, then throw it forward (the projectile leaves ~0.2 s in)
		{ FClip C; C.Name = TEXT("A_Humanoid_Cast"); C.Length = 0.6f;
		  C.Keys = { K(0.00f, {}),
		             K(0.10f, {B(TEXT("arm_r"), -45, 25), B(TEXT("arm_l"), 20, 10), B(TEXT("root"), -4, 0, 16)}),
		             K(0.22f, {B(TEXT("arm_r"), 115, 0), B(TEXT("arm_l"), -15, 10), B(TEXT("root"), 8, 0, -10)}, FVector(4, 0, 0)),
		             K(0.38f, {B(TEXT("arm_r"), 100, 0), B(TEXT("arm_l"), -10, 10), B(TEXT("root"), 6, 0, -6)}, FVector(3, 0, 0)),
		             K(0.60f, {}) };
		  Out.Add(C); }
		// ---- channel: both arms raised and open, a slow sway while the weave is held
		{ FClip C; C.Name = TEXT("A_Humanoid_Channel"); C.bLoop = true; C.Length = 2.4f;
		  C.Keys = { K(0.00f, {B(TEXT("arm_r"), 150, 18), B(TEXT("arm_l"), 150, 18), B(TEXT("head"), -12), B(TEXT("root"), -4, 0, 3)}),
		             K(1.20f, {B(TEXT("arm_r"), 162, 28), B(TEXT("arm_l"), 162, 28), B(TEXT("head"), -16), B(TEXT("root"), -6, 0, -3)}, FVector(0, 0, 3)) };
		  Out.Add(C); }
		// ---- riding: legs forward and apart over the saddle, hands forward on the reins
		{ FClip C; C.Name = TEXT("A_Humanoid_RideIdle"); C.bLoop = true; C.Length = 2.0f;
		  C.Keys = { K(0.00f, {B(TEXT("leg_l"), 72, 38), B(TEXT("leg_r"), 72, 38), B(TEXT("arm_r"), 36, 6), B(TEXT("arm_l"), 36, 6), B(TEXT("root"), 5)}),
		             K(1.00f, {B(TEXT("leg_l"), 72, 38), B(TEXT("leg_r"), 72, 38), B(TEXT("arm_r"), 40, 6), B(TEXT("arm_l"), 40, 6), B(TEXT("root"), 4)}, FVector(0, 0, 1.5f)) };
		  Out.Add(C); }
		{ FClip C; C.Name = TEXT("A_Humanoid_RideMove"); C.bLoop = true; C.Length = 0.6f;
		  C.Keys = { K(0.00f, {B(TEXT("leg_l"), 72, 38), B(TEXT("leg_r"), 72, 38), B(TEXT("arm_r"), 34, 6), B(TEXT("arm_l"), 34, 6), B(TEXT("root"), 8)}),
		             K(0.15f, {B(TEXT("leg_l"), 72, 38), B(TEXT("leg_r"), 72, 38), B(TEXT("arm_r"), 40, 6), B(TEXT("arm_l"), 40, 6), B(TEXT("root"), 10)}, FVector(0, 0, 4)),
		             K(0.30f, {B(TEXT("leg_l"), 72, 38), B(TEXT("leg_r"), 72, 38), B(TEXT("arm_r"), 34, 6), B(TEXT("arm_l"), 34, 6), B(TEXT("root"), 8)}),
		             K(0.45f, {B(TEXT("leg_l"), 72, 38), B(TEXT("leg_r"), 72, 38), B(TEXT("arm_r"), 40, 6), B(TEXT("arm_l"), 40, 6), B(TEXT("root"), 10)}, FVector(0, 0, 4)) };
		  Out.Add(C); }
		// ---- climb: alternate reaching arms and stepping legs against the ladder
		{ FClip C; C.Name = TEXT("A_Humanoid_Climb"); C.bLoop = true; C.Length = 1.2f;
		  C.Keys = { K(0.00f, {B(TEXT("arm_r"), 165, 8), B(TEXT("arm_l"), 110, 8), B(TEXT("leg_l"), 55, 6), B(TEXT("leg_r"), 12, 6), B(TEXT("root"), 6)}),
		             K(0.60f, {B(TEXT("arm_r"), 110, 8), B(TEXT("arm_l"), 165, 8), B(TEXT("leg_l"), 12, 6), B(TEXT("leg_r"), 55, 6), B(TEXT("root"), 6)}, FVector(0, 0, 3)) };
		  Out.Add(C); }
		// ---- hit: a flinch back
		{ FClip C; C.Name = TEXT("A_Humanoid_Hit"); C.Length = 0.4f;
		  C.Keys = { K(0.00f, {}),
		             K(0.10f, {B(TEXT("root"), -14), B(TEXT("head"), -22), B(TEXT("arm_r"), 28, 22), B(TEXT("arm_l"), 28, 22)}, FVector(-6, 0, 0)),
		             K(0.40f, {}) };
		  Out.Add(C); }
		// ---- death: fall onto the back and stay there (the last frame is held by the anim instance)
		{ FClip C; C.Name = TEXT("A_Humanoid_Death"); C.Length = 1.3f;
		  C.Keys = { K(0.00f, {}),
		             K(0.25f, {B(TEXT("root"), -30), B(TEXT("head"), -15), B(TEXT("arm_r"), 60, 40), B(TEXT("arm_l"), 60, 40)}, FVector(-4, 0, -6)),
		             K(0.70f, {B(TEXT("root"), -88), B(TEXT("head"), 10), B(TEXT("arm_r"), 95, 65), B(TEXT("arm_l"), 95, 65), B(TEXT("leg_l"), 12, 10), B(TEXT("leg_r"), 12, 10)}, FVector(-12, 0, -54)),
		             K(1.30f, {B(TEXT("root"), -88), B(TEXT("head"), 10), B(TEXT("arm_r"), 95, 65), B(TEXT("arm_l"), 95, 65), B(TEXT("leg_l"), 12, 10), B(TEXT("leg_r"), 12, 10)}, FVector(-12, 0, -54)) };
		  Out.Add(C); }
		return Out;
	}

	float Smooth(float T) { return T * T * (3.0f - 2.0f * T); }

	/** Pose of every authored bone at time T (looping clips blend the last key back into the first). */
	void Sample(const FClip& C, float T, TMap<FName, FBonePose>& Out, FVector& RootOffset)
	{
		const TArray<FKey>& Ks = C.Keys;
		if (Ks.Num() == 0) { return; }
		int32 A = 0;
		for (int32 i = 0; i < Ks.Num(); ++i) { if (Ks[i].Time <= T) { A = i; } }
		int32 Bi = A + 1; float TB = Bi < Ks.Num() ? Ks[Bi].Time : C.Length;
		if (Bi >= Ks.Num()) { Bi = C.bLoop ? 0 : A; }
		const float Span = FMath::Max(TB - Ks[A].Time, 1e-4f); const float F = Smooth(FMath::Clamp((T - Ks[A].Time) / Span, 0.0f, 1.0f));
		auto Get = [](const FKey& Key, FName Bone) { for (const FBonePose& P : Key.Bones) { if (P.Bone == Bone) { return P; } } FBonePose Z; Z.Bone = Bone; return Z; };
		TSet<FName> Names; for (const FKey& Key : Ks) { for (const FBonePose& P : Key.Bones) { Names.Add(P.Bone); } }
		for (FName N : Names) {
			const FBonePose PA = Get(Ks[A], N), PB = Get(Ks[Bi], N); FBonePose P; P.Bone = N;
			P.Fwd = FMath::Lerp(PA.Fwd, PB.Fwd, F); P.Out = FMath::Lerp(PA.Out, PB.Out, F); P.Twist = FMath::Lerp(PA.Twist, PB.Twist, F);
			Out.Add(N, P);
		}
		RootOffset = FMath::Lerp(Ks[A].RootOffset, Ks[Bi].RootOffset, F);
	}

	bool SaveObj(UObject* Obj)
	{
		UPackage* Pkg = Obj->GetOutermost(); Pkg->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
		const bool bOk = UPackage::SavePackage(Pkg, Obj, *File, Args);
		AB_LOG("saved %s -> %d", *Pkg->GetName(), bOk ? 1 : 0);
		return bOk;
	}

	template <typename T> T* GetOrCreate(const FString& Folder, const FString& Name)
	{
		const FString PkgName = Folder / Name;
		if (T* Existing = LoadObject<T>(nullptr, *(PkgName + TEXT(".") + Name))) { return Existing; }
		UPackage* Pkg = CreatePackage(*PkgName);
		T* Obj = NewObject<T>(Pkg, *Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Obj);
		return Obj;
	}

	UAnimSequence* BuildClip(USkeleton* Skel, const FString& Folder, const FClip& C)
	{
		const FReferenceSkeleton& RS = Skel->GetReferenceSkeleton(); const int32 N = RS.GetRawBoneNum();
		// reference pose in component space
		TArray<FTransform> CS; CS.SetNum(N);
		for (int32 i = 0; i < N; ++i) { const int32 P = RS.GetParentIndex(i); CS[i] = P >= 0 ? RS.GetRefBonePose()[i] * CS[P] : RS.GetRefBonePose()[i]; }
		auto Index = [&](FName Name) { return RS.FindBoneIndex(Name); };
		// bone direction (component space): toward the _end child, else up for the root
		auto Dir = [&](int32 i) {
			const FName EndName(*(RS.GetBoneName(i).ToString() + TEXT("_end"))); const int32 E = Index(EndName);
			if (E >= 0) { return (CS[E].GetLocation() - CS[i].GetLocation()).GetSafeNormal(); }
			return Up;
		};
		const int32 Root = Index(TEXT("root"));
		const float Fps = 24.0f; const int32 Frames = FMath::Max(2, FMath::RoundToInt(C.Length * Fps) + 1);
		TArray<TArray<FVector>> Pos; TArray<TArray<FQuat>> Rot; TArray<TArray<FVector>> Scl; Pos.SetNum(N); Rot.SetNum(N); Scl.SetNum(N);
		for (int32 f = 0; f < Frames; ++f) {
			const float T = C.bLoop ? FMath::Fmod(f / Fps, C.Length) : FMath::Min(f / Fps, C.Length);
			TMap<FName, FBonePose> Poses; FVector Off; Sample(C, T, Poses, Off);
			TArray<FTransform> NewCS = CS;
			for (int32 i = 0; i < N; ++i) {
				const FBonePose* P = Poses.Find(RS.GetBoneName(i));
				if (!P) { continue; }
				const FVector D = Dir(i); const float Side = CS[i].GetLocation().X >= 0 ? 1.0f : -1.0f;   // +X is the mesh's left
				const FVector OutAxis = FVector::CrossProduct(D, FVector(Side, 0, 0)).GetSafeNormal();
				const FVector FwdAxis = FVector::CrossProduct(D, Forward).GetSafeNormal();
				FQuat Q = FQuat::Identity;
				if (!FwdAxis.IsNearlyZero()) { Q = FQuat(FwdAxis, FMath::DegreesToRadians(P->Fwd)) * Q; }
				if (!OutAxis.IsNearlyZero()) { Q = FQuat(OutAxis, FMath::DegreesToRadians(P->Out)) * Q; }
				Q = FQuat(D, FMath::DegreesToRadians(P->Twist)) * Q;
				NewCS[i].SetRotation((Q * CS[i].GetRotation()).GetNormalized());
				if (i == Root) { NewCS[i].SetLocation(CS[i].GetLocation() + Forward * Off.X + Right * Off.Y + Up * Off.Z); }
			}
			if (Root >= 0 && !Poses.Contains(TEXT("root"))) { NewCS[Root].SetLocation(CS[Root].GetLocation() + Forward * Off.X + Right * Off.Y + Up * Off.Z); }
			for (int32 i = 0; i < N; ++i) {
				const int32 P = RS.GetParentIndex(i);
				FTransform Local = P >= 0 ? NewCS[i].GetRelativeTransform(NewCS[P]) : NewCS[i];
				const FTransform& Ref = RS.GetRefBonePose()[i];
				if (i != Root) { Local.SetLocation(Ref.GetLocation()); }   // only the root travels; every other bone keeps its reference offset
				Local.SetScale3D(Ref.GetScale3D());
				Pos[i].Add(Local.GetLocation()); Rot[i].Add(Local.GetRotation()); Scl[i].Add(Local.GetScale3D());
			}
		}
		UAnimSequence* Seq = GetOrCreate<UAnimSequence>(Folder, C.Name);
		Seq->SetSkeleton(Skel);
		IAnimationDataController& Ctl = Seq->GetController();
		Ctl.OpenBracket(FText::FromString(TEXT("wot.Anim.Build")), false);
		Ctl.InitializeModel();
		Ctl.RemoveAllBoneTracks(false);
		Ctl.SetFrameRate(FFrameRate(24, 1), false);
		Ctl.SetNumberOfFrames(FFrameNumber(Frames - 1), false);
		for (int32 i = 0; i < N; ++i) {
			const FName Name = RS.GetBoneName(i);
			Ctl.AddBoneCurve(Name, false);
			Ctl.SetBoneTrackKeys(Name, Pos[i], Rot[i], Scl[i], false);
		}
		Ctl.NotifyPopulated();
		Ctl.CloseBracket(false);
		Seq->bLoop = C.bLoop;
		Seq->Interpolation = EAnimInterpolationType::Linear;
		SaveObj(Seq);
		AB_LOG("clip %s: %d frames, %.2fs, loop=%d", *C.Name, Frames, C.Length, C.bLoop ? 1 : 0);
		return Seq;
	}

	UBlendSpace1D* BuildLocomotion(USkeleton* Skel, const FString& Folder, UAnimSequence* Idle, UAnimSequence* Walk, UAnimSequence* Run)
	{
		UBlendSpace1D* BS = GetOrCreate<UBlendSpace1D>(Folder, TEXT("BS_Humanoid_Locomotion"));
		BS->SetSkeleton(Skel);
		if (FProperty* Prop = UBlendSpace::StaticClass()->FindPropertyByName(TEXT("BlendParameters"))) {
			FBlendParameter* Params = Prop->ContainerPtrToValuePtr<FBlendParameter>(BS, 0);
			Params[0].DisplayName = TEXT("Speed"); Params[0].Min = 0.0f; Params[0].Max = 1000.0f; Params[0].GridNum = 10;
		}
		if (FProperty* SP = UBlendSpace::StaticClass()->FindPropertyByName(TEXT("SampleData"))) {
			if (FArrayProperty* AP = CastField<FArrayProperty>(SP)) { FScriptArrayHelper H(AP, AP->ContainerPtrToValuePtr<void>(BS)); H.EmptyValues(); }
		}
		if (Idle) { BS->AddSample(Idle, FVector(0, 0, 0)); }
		if (Walk) { BS->AddSample(Walk, FVector(260, 0, 0)); }
		if (Run) { BS->AddSample(Run, FVector(750, 0, 0)); }
		BS->ValidateSampleData();
		BS->ResampleData();   // rebuilds the blend grid; without it the space evaluates to the reference pose
		SaveObj(BS);
		return BS;
	}
}

static FAutoConsoleCommand GWotAnimBuild(
	TEXT("wot.Anim.Build"), TEXT("Build the shared humanoid clips, locomotion blend space and DA_HumanoidAnims: wot.Anim.Build [Skeleton=/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_sk] [Folder=/Game/Wot/Characters/Anims]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotAnimBuild;
		const FString SkelPath = Args.Num() > 0 ? Args[0] : TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_sk");
		const FString Folder = Args.Num() > 1 ? Args[1] : TEXT("/Game/Wot/Characters/Anims");
		USkeleton* Skel = Cast<USkeleton>(FSoftObjectPath(SkelPath + TEXT(".") + FPackageName::GetShortName(SkelPath)).TryLoad());
		if (!Skel) { AB_LOG("skeleton not found: %s", *SkelPath); return; }
		TMap<FString, UAnimSequence*> Built;
		for (const FClip& C : Clips()) { Built.Add(C.Name, BuildClip(Skel, Folder, C)); }
		UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_idle_animseq.rand_medium_flat_idle_animseq"));
		UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_walking_animseq.rand_medium_flat_walking_animseq"));
		UBlendSpace1D* BS = BuildLocomotion(Skel, Folder, Idle, Walk, Built[TEXT("A_Humanoid_Run")]);
		UWotAnimSet* Set = GetOrCreate<UWotAnimSet>(Folder, TEXT("DA_HumanoidAnims"));
		Set->Locomotion = BS; Set->HeavyAttack = Built[TEXT("A_Humanoid_HeavyAttack")]; Set->Cast = Built[TEXT("A_Humanoid_Cast")]; Set->Hit = Built[TEXT("A_Humanoid_Hit")];
		Set->Death = Built[TEXT("A_Humanoid_Death")]; Set->Channel = Built[TEXT("A_Humanoid_Channel")]; Set->RideIdle = Built[TEXT("A_Humanoid_RideIdle")];
		Set->RideMove = Built[TEXT("A_Humanoid_RideMove")]; Set->Climb = Built[TEXT("A_Humanoid_Climb")];
		SaveObj(Set);
		AB_LOG("done: %d clips, blend space and clip set under %s", Built.Num(), *Folder);
	}));
#endif
