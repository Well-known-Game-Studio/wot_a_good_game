// wot.Anim.Build: generates the humanoid clips, locomotion blend space and clip set for every humanoid skeleton (editor only).
//
// The voxel humanoids (Rand medium-flat, Tam, Lan, Fade, Trolloc, the main cast) share a six-bone rig:
// root (hips, pointing up) with head, arm_r, arm_l, leg_l, leg_r hanging off it, each with an _end leaf. Clips are
// authored here as pose keys in MESH space (forward = +Y, right = -X, up = +Z: the mesh component is yawed -90 in the
// character Blueprints) and converted to bone-local keys against each skeleton's reference pose, so one table builds a
// native clip for every rig. Clips are built PER SKELETON on purpose: the rigs are not interchangeable at the data level
// (Rand's import put the 100x unit scale on an extra top bone above root, the cast rigs put it on root itself, and the
// engine maps bone 0 to bone 0 regardless of names), so a clip played through compatible-skeleton remapping shrinks or
// sinks the other rig. The hand-made Rand clips (idle, walk, light attack, jump, holding bow) are rebased onto the other
// skeletons the same way: rotations copied per bone, translation and scale from the target reference pose.
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
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

	void RefPoseCS(const FReferenceSkeleton& RS, TArray<FTransform>& CS)
	{
		const int32 N = RS.GetRawBoneNum(); CS.SetNum(N);
		for (int32 i = 0; i < N; ++i) { const int32 P = RS.GetParentIndex(i); CS[i] = P >= 0 ? RS.GetRefBonePose()[i] * CS[P] : RS.GetRefBonePose()[i]; }
	}

	/** Writes per-bone key arrays (one entry per bone of Skel, Frames keys each) into the sequence asset Folder/Name. */
	UAnimSequence* WriteSequence(USkeleton* Skel, const FString& Folder, const FString& Name, FFrameRate Rate, int32 Frames,
		const TArray<TArray<FVector>>& Pos, const TArray<TArray<FQuat>>& Rot, const TArray<TArray<FVector>>& Scl, bool bLoop, EAnimInterpolationType Interp)
	{
		const FReferenceSkeleton& RS = Skel->GetReferenceSkeleton(); const int32 N = RS.GetRawBoneNum();
		UAnimSequence* Seq = GetOrCreate<UAnimSequence>(Folder, Name);
		Seq->SetSkeleton(Skel);
		IAnimationDataController& Ctl = Seq->GetController();
		Ctl.OpenBracket(FText::FromString(TEXT("wot.Anim.Build")), false);
		Ctl.InitializeModel();
		Ctl.RemoveAllBoneTracks(false);
		Ctl.SetFrameRate(Rate, false);
		Ctl.SetNumberOfFrames(FFrameNumber(Frames - 1), false);
		for (int32 i = 0; i < N; ++i) {
			const FName BoneName = RS.GetBoneName(i);
			Ctl.AddBoneCurve(BoneName, false);
			Ctl.SetBoneTrackKeys(BoneName, Pos[i], Rot[i], Scl[i], false);
		}
		Ctl.NotifyPopulated();
		Ctl.CloseBracket(false);
		Seq->bLoop = bLoop;
		Seq->Interpolation = Interp;
		SaveObj(Seq);
		return Seq;
	}

	/** Copies a hand-made clip onto another skeleton with the same bone names: rotations per bone (re-expressed when the
	 *  parents' frames differ, as the engine's remapping does), translation and scale from the target reference pose, the
	 *  root's travel carried over in component space. The result plays natively, with no cross-rig remapping. */
	UAnimSequence* RebaseClip(UAnimSequence* Src, USkeleton* Skel, const FString& Folder, const FString& Name)
	{
		if (!Src || !Src->GetSkeleton()) { AB_LOG("rebase %s: no source", *Name); return nullptr; }
		const IAnimationDataModel* Model = Src->GetDataModel();
		if (!Model) { AB_LOG("rebase %s: source has no data model", *Name); return nullptr; }
		const FReferenceSkeleton& TR = Skel->GetReferenceSkeleton(); const int32 N = TR.GetRawBoneNum();
		const FReferenceSkeleton& SR = Src->GetSkeleton()->GetReferenceSkeleton();
		TArray<FTransform> TCS, SCS; RefPoseCS(TR, TCS); RefPoseCS(SR, SCS);
		const int32 Keys = FMath::Max(2, Model->GetNumberOfKeys());
		TArray<FName> Tracks; Model->GetBoneTrackNames(Tracks);
		const int32 TRoot = TR.FindBoneIndex(TEXT("root")), SRoot = SR.FindBoneIndex(TEXT("root"));
		TArray<TArray<FVector>> Pos; TArray<TArray<FQuat>> Rot; TArray<TArray<FVector>> Scl; Pos.SetNum(N); Rot.SetNum(N); Scl.SetNum(N);
		int32 Copied = 0;
		for (int32 i = 0; i < N; ++i) {
			const FName BoneName = TR.GetBoneName(i); const int32 s = SR.FindBoneIndex(BoneName);
			TArray<FTransform> SrcKeys;
			if (s >= 0 && Tracks.Contains(BoneName)) { Model->GetBoneTrackTransforms(BoneName, SrcKeys); ++Copied; }
			const FTransform& TRef = TR.GetRefBonePose()[i]; const int32 TP = TR.GetParentIndex(i);
			const int32 SP = s >= 0 ? SR.GetParentIndex(s) : INDEX_NONE;
			const FTransform SParentCS = SP >= 0 ? SCS[SP] : FTransform::Identity, TParentCS = TP >= 0 ? TCS[TP] : FTransform::Identity;
			// Lt = Q0 * Ls * Q1 (FSkeletonRemapping): the same delta rotation in both parents' component-space frames
			const FQuat PS = SParentCS.GetRotation(), PT = TParentCS.GetRotation();
			const FQuat RS = s >= 0 ? SR.GetRefBonePose()[s].GetRotation() : FQuat::Identity, RT = TRef.GetRotation();
			const FQuat Q0 = PT.Inverse() * PS, Q1 = RS.Inverse() * PS.Inverse() * PT * RT;
			for (int32 k = 0; k < Keys; ++k) {
				FTransform L = TRef;
				if (SrcKeys.IsValidIndex(k)) {
					L.SetRotation((Q0 * SrcKeys[k].GetRotation() * Q1).GetNormalized());
					if (i == TRoot && s == SRoot) {
						const FVector OffCS = SParentCS.TransformVector(SrcKeys[k].GetLocation() - SR.GetRefBonePose()[s].GetLocation());
						L.SetLocation(TRef.GetLocation() + TParentCS.InverseTransformVector(OffCS));
					}
				}
				Pos[i].Add(L.GetLocation()); Rot[i].Add(L.GetRotation()); Scl[i].Add(L.GetScale3D());
			}
		}
		UAnimSequence* Seq = WriteSequence(Skel, Folder, Name, Model->GetFrameRate(), Keys, Pos, Rot, Scl, Src->bLoop, Src->Interpolation);
		AB_LOG("rebased %s -> %s/%s: %d keys, %d/%d bone tracks copied", *Src->GetName(), *Folder, *Name, Keys, Copied, N);
		return Seq;
	}

	UAnimSequence* BuildClip(USkeleton* Skel, const FString& Folder, const FClip& C)
	{
		const FReferenceSkeleton& RS = Skel->GetReferenceSkeleton(); const int32 N = RS.GetRawBoneNum();
		// reference pose in component space
		TArray<FTransform> CS; RefPoseCS(RS, CS);
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
		UAnimSequence* Seq = WriteSequence(Skel, Folder, C.Name, FFrameRate(24, 1), Frames, Pos, Rot, Scl, C.bLoop, EAnimInterpolationType::Linear);
		AB_LOG("clip %s/%s: %d frames, %.2fs, loop=%d", *Folder, *C.Name, Frames, C.Length, C.bLoop ? 1 : 0);
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

namespace WotAnimBuild
{
	// the humanoid skeletons; the first is the reference rig whose hand-made clips (idle, walk, light attack) are rebased
	// onto the others, and whose set lives at the folder root (the Anim Blueprint's defaults point there)
	const TCHAR* DefaultSkeletons[] = {
		TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_sk"),
		TEXT("/Game/Wot/Characters/Main/Moiraine/moiraine_medium_flat_Skeleton"),
		TEXT("/Game/Wot/Characters/Lan/MediumFlat/lan_medium_flat_Skeleton"),
		TEXT("/Game/Wot/Characters/Tam/medium_flat/tam_medium_flat_skeleton"),
		TEXT("/Game/Wot/Enemies/fade/medium_flat/fade_medium_flat_Skeleton"),
		TEXT("/Game/Wot/Enemies/trolloc_boar/MediumFlat/trolloc_boar_Skeleton") };
	const TCHAR* RandIdle = TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_idle_animseq.rand_medium_flat_idle_animseq");
	const TCHAR* RandWalk = TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_walking_animseq.rand_medium_flat_walking_animseq");
	const TCHAR* RandLight = TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_light_attack_animseq.rand_medium_flat_light_attack_animseq");
	const TCHAR* RandJump = TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_jump_animseq.rand_medium_flat_jump_animseq");
	const TCHAR* RandBow = TEXT("/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_holding_bow_animseq.rand_medium_flat_holding_bow_animseq");

	USkeleton* LoadSkeleton(const FString& Path)
	{
		return Cast<USkeleton>(FSoftObjectPath(Path.Contains(TEXT(".")) ? Path : Path + TEXT(".") + FPackageName::GetShortName(Path)).TryLoad());
	}

	/** Builds the whole set for one skeleton into Folder: generated clips, (rebased) idle / walk / light attack, blend space, DA. */
	void BuildSet(USkeleton* Skel, const FString& Folder, bool bReference)
	{
		TMap<FString, UAnimSequence*> Built;
		for (const FClip& C : Clips()) { Built.Add(C.Name, BuildClip(Skel, Folder, C)); }
		UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, RandIdle);
		UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, RandWalk);
		UAnimSequence* Light = LoadObject<UAnimSequence>(nullptr, RandLight);
		UAnimSequence* Jump = LoadObject<UAnimSequence>(nullptr, RandJump);
		UAnimSequence* Bow = LoadObject<UAnimSequence>(nullptr, RandBow);
		if (!bReference) {
			Idle = RebaseClip(Idle, Skel, Folder, TEXT("A_Humanoid_Idle"));
			Walk = RebaseClip(Walk, Skel, Folder, TEXT("A_Humanoid_Walk"));
			Light = RebaseClip(Light, Skel, Folder, TEXT("A_Humanoid_LightAttack"));
			Jump = RebaseClip(Jump, Skel, Folder, TEXT("A_Humanoid_Jump"));
			Bow = RebaseClip(Bow, Skel, Folder, TEXT("A_Humanoid_HoldingBow"));
		}
		UBlendSpace1D* BS = BuildLocomotion(Skel, Folder, Idle, Walk, Built[TEXT("A_Humanoid_Run")]);
		UWotAnimSet* Set = GetOrCreate<UWotAnimSet>(Folder, TEXT("DA_HumanoidAnims"));
		Set->Locomotion = BS; Set->LightAttack = Light; Set->Jump = Jump; Set->HoldingBow = Bow; Set->HeavyAttack = Built[TEXT("A_Humanoid_HeavyAttack")]; Set->Cast = Built[TEXT("A_Humanoid_Cast")]; Set->Hit = Built[TEXT("A_Humanoid_Hit")];
		Set->Death = Built[TEXT("A_Humanoid_Death")]; Set->Channel = Built[TEXT("A_Humanoid_Channel")]; Set->RideIdle = Built[TEXT("A_Humanoid_RideIdle")];
		Set->RideMove = Built[TEXT("A_Humanoid_RideMove")]; Set->Climb = Built[TEXT("A_Humanoid_Climb")];
		SaveObj(Set);
		AB_LOG("set for %s: %d generated clips + idle/walk/light attack/jump/bow, blend space and DA_HumanoidAnims under %s", *Skel->GetName(), Built.Num(), *Folder);
	}
}

static FAutoConsoleCommand GWotAnimBuild(
	TEXT("wot.Anim.Build"), TEXT("Build the humanoid clip sets: wot.Anim.Build [Folder=/Game/Wot/Characters/Anims] [SkeletonPath ...] (default: every humanoid skeleton; the first is the reference rig at the folder root, the others go to Folder/<SkeletonName>)"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotAnimBuild;
		const FString Folder = Args.Num() > 0 ? Args[0] : TEXT("/Game/Wot/Characters/Anims");
		TArray<FString> Skeletons;
		for (int32 i = 1; i < Args.Num(); ++i) { Skeletons.Add(Args[i]); }
		if (Skeletons.Num() == 0) { for (const TCHAR* S : DefaultSkeletons) { Skeletons.Add(S); } }
		for (int32 i = 0; i < Skeletons.Num(); ++i) {
			USkeleton* Skel = LoadSkeleton(Skeletons[i]);
			if (!Skel) { AB_LOG("skeleton not found: %s", *Skeletons[i]); continue; }
			const bool bReference = Skel->GetName() == FPackageName::GetShortName(DefaultSkeletons[0]);
			BuildSet(Skel, bReference ? Folder : Folder / Skel->GetName(), bReference);
		}
		AB_LOG("done: %d skeletons", Skeletons.Num());
	}));

// wot.Anim.MakeTemplateABP <AnimBlueprintPath> [1|0]: make the shared humanoid Anim Blueprint a template (no target
// skeleton). USkeletalMeshComponent only spawns an anim instance when the mesh's bones, root included, exist in the
// Anim Blueprint's target skeleton; the voxel rigs name their top bone after the character (tam_medium_flat,
// fade_medium_flat, armature_medium_flat ...), so a Rand-skeleton Anim Blueprint silently refuses those meshes. With no
// target skeleton the engine validates each mesh against its own skeleton, and the per-rig clip sets supply the clips.
static FAutoConsoleCommand GWotAnimMakeTemplateABP(
	TEXT("wot.Anim.MakeTemplateABP"), TEXT("Make an Anim Blueprint a template (no target skeleton) and recompile it: wot.Anim.MakeTemplateABP /Game/Path/ABP [1|0]"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		using namespace WotAnimBuild;
		if (Args.Num() < 1) { AB_LOG("usage: wot.Anim.MakeTemplateABP /Game/Path/ABP [1|0]"); return; }
		const FString Path = Args[0].Contains(TEXT(".")) ? Args[0] : Args[0] + TEXT(".") + FPackageName::GetShortName(Args[0]);
		UAnimBlueprint* BP = Cast<UAnimBlueprint>(FSoftObjectPath(Path).TryLoad());
		if (!BP) { AB_LOG("anim blueprint not found: %s", *Path); return; }
		const bool bTemplate = Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0;
		AB_LOG("%s: template %d -> %d, skeleton %s", *BP->GetName(), BP->bIsTemplate ? 1 : 0, bTemplate ? 1 : 0, *GetNameSafe(BP->TargetSkeleton));
		BP->Modify();
		BP->bIsTemplate = bTemplate;
		if (bTemplate) { BP->TargetSkeleton = nullptr; }
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
		for (const TSharedRef<FTokenizedMessage>& M : Results.Messages) {
			if (M->GetSeverity() <= EMessageSeverity::Warning) { AB_LOG("  compile: %s", *M->ToText().ToString()); }
		}
		AB_LOG("%s: compiled, errors=%d warnings=%d status=%d", *BP->GetName(), Results.NumErrors, Results.NumWarnings, (int32)BP->Status);
		if (Results.NumErrors == 0) { SaveObj(BP); }
	}));
#endif
