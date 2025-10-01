// Copyright (c) 2025 Pixane.

#include "AedificSplineContinuum.h"
#include "AedificSplineTypes.h"

#include <Components/SplineComponent.h>
#include <Components/SplineMeshComponent.h>

#if WITH_EDITORONLY_DATA
#include <Components/BillboardComponent.h>
#include <DrawDebugHelpers.h>
#include <ImageUtils.h>
#endif // WITH_EDITORONLY_DATA

#include UE_INLINE_GENERATED_CPP_BY_NAME(AedificSplineContinuum)

AAedificSplineContinuum::AAedificSplineContinuum()
{
	// Set default values for AActor interface members.
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;

	// Set default values for this class members.
	StaticMesh = nullptr;
	MaterialOverride = nullptr;
	bAutoComputeSpline = true;
	bComputeTangents = true;
	TangentsScale = 1.f;
	bComputeUpVectors = true;
	bAutoRebuildMesh = true;
	bUseParallelTransport = false;
	bRebuildRequested = false;
	SplineMeshComponents.Empty();

	// Create scene component.
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;
	SceneComponent->SetMobility(EComponentMobility::Static);
	SceneComponent->SetComponentTickEnabled(false);
	SceneComponent->bComputeFastLocalBounds = true;
	SceneComponent->bComputeBoundsOnceForGame = true;

	// Create spline component.
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);
	SplineComponent->SetMobility(EComponentMobility::Static);
	SplineComponent->SetComponentTickEnabled(false);
	SplineComponent->SetGenerateOverlapEvents(false);
	SplineComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplineComponent->bComputeFastLocalBounds = true;
	SplineComponent->bComputeBoundsOnceForGame = true;

	// Fetch default mesh file.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> floorMeshFile(TEXT("/Aedific/Meshes/SM_Floor_Decimated.SM_Floor_Decimated"));
	if (floorMeshFile.Succeeded())
	{
		StaticMesh = floorMeshFile.Object;
	}

#if WITH_EDITORONLY_DATA
	// Create editor sprite.
	EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("SplineSpriteComponent"));
	if (!IsRunningCommandlet() && EditorSprite)
	{
		EditorSprite->SetupAttachment(RootComponent);
		EditorSprite->SetRelativeScale3D(FVector(0.5f));
		UTexture2D* splineSpriteTexture = FImageUtils::ImportFileAsTexture2D(FPaths::ProjectPluginsDir() + "Aedific/Resources/SplineThumbnail.png");
		if (splineSpriteTexture)
		{
			splineSpriteTexture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
			splineSpriteTexture->bUseLegacyGamma = true;
			splineSpriteTexture->LODGroup = TextureGroup::TEXTUREGROUP_World;
			splineSpriteTexture->UpdateResource();
			EditorSprite->Sprite = splineSpriteTexture;
		}
		EditorSprite->SetMobility(EComponentMobility::Static);
		EditorSprite->SpriteInfo.Category = TEXT("Aedific");
		EditorSprite->SpriteInfo.DisplayName = INVTEXT("AedificSplineContinuum");
		EditorSprite->SetUsingAbsoluteScale(true);
		EditorSprite->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void AAedificSplineContinuum::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == FName(TEXT("StaticMesh")))
	{
		if (StaticMesh)
		{
			RebuildMesh();
		}
		else
		{
			EmptyMesh();
		}
	}
	else if (PropertyChangedEvent.GetPropertyName() == FName(TEXT("MaterialOverride")))
	{
		UpdateMaterial();
	}
}
#endif // WITH_EDITOR

void AAedificSplineContinuum::OnConstruction(const FTransform& Transform)
{
	if (bAutoComputeSpline)
	{
		ComputeSpline();
	}

	if (bAutoRebuildMesh)
	{
		RebuildMesh();
	}
}

void AAedificSplineContinuum::ComputeSpline()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsInitialized() || !SplineComponent)
	{
		return;
	}

	const int32 AmountOfPoints = SplineComponent->GetNumberOfSplinePoints();
	if (AmountOfPoints < 2)
	{
		return;
	}

	const bool bClosed = SplineComponent->IsClosedLoop();

	if (bComputeTangents)
	{
		ComputeTangents(AmountOfPoints, bClosed);
	}
	
	if (bComputeUpVectors)
	{
		ComputeUpVectors(AmountOfPoints);
	}

	SplineComponent->UpdateSpline();
}

void AAedificSplineContinuum::ComputeTangents(const int32 SplinePointsNum, const bool bClosed)
{
	if (SplinePointsNum < 2)
		return;

	// --- Step 1: Cache all point locations to avoid redundant lookups. ---
	TArray<FVector> PointLocations;
	PointLocations.Reserve(SplinePointsNum);
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		PointLocations.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
	}

	// --- Step 2: Iterate through cached points to compute tangents. ---
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		const FVector& CurrentPoint = PointLocations[i];
		FVector PreviousPoint, NextPoint;

		// --- Determine neighboring points based on loop type and position. ---
		if (bClosed)
		{
			PreviousPoint = PointLocations[(i == 0) ? SplinePointsNum - 1 : i - 1];
			NextPoint = PointLocations[(i == SplinePointsNum - 1) ? 0 : i + 1];
		}
		else
		{
			PreviousPoint = (i > 0) ? PointLocations[i - 1] : CurrentPoint;
			NextPoint = (i < SplinePointsNum - 1) ? PointLocations[i + 1] : CurrentPoint;
		}

		FVector Incoming = FVector::ZeroVector;
		FVector Outgoing = FVector::ZeroVector;

		// --- Calculate tangent vectors. ---
		if ((i > 0 && i < SplinePointsNum - 1) || bClosed)
		{
			// Interior point or closed spline → unified direction (Catmull-Rom style)
			const FVector UnifiedDir = (NextPoint - PreviousPoint).GetSafeNormal();

			Incoming = UnifiedDir * (CurrentPoint - PreviousPoint).Size() * TangentsScale;
			Outgoing = UnifiedDir * (NextPoint - CurrentPoint).Size() * TangentsScale;
		}
		else
		{
			// Endpoints of an open spline → one-sided tangents
			if (i > 0)
			{
				const FVector IncomingDir = (CurrentPoint - PreviousPoint).GetSafeNormal();
				Incoming = IncomingDir * (CurrentPoint - PreviousPoint).Size() * TangentsScale;
			}
			if (i < SplinePointsNum - 1)
			{
				const FVector OutgoingDir = (NextPoint - CurrentPoint).GetSafeNormal();
				Outgoing = OutgoingDir * (NextPoint - CurrentPoint).Size() * TangentsScale;
			}
		}

		SplineComponent->SetTangentsAtSplinePoint(i, Incoming, Outgoing, ESplineCoordinateSpace::Local, false);
	}
}

void AAedificSplineContinuum::ComputeUpVectors(const int32 SplinePointsNum)
{
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		// 1. Get the roll value you set in the editor (in degrees).
		const FQuat Rotation = SplineComponent->GetRotationAtSplinePoint(i, ESplineCoordinateSpace::World).Quaternion();

		// 5. Apply the roll to the existing up vector.
		const FVector NewUp = Rotation.GetAxisZ();

		// 6. Set the final up vector.
		SplineComponent->SetUpVectorAtSplinePoint(i, NewUp, ESplineCoordinateSpace::World, false);
	}
}

void AAedificSplineContinuum::RebuildMesh()
{
	if (!StaticMesh || bRebuildRequested)
	{
		return;
	}

	bRebuildRequested = true;

	EmptyMesh();

	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		// Mesh and spline dimensions.
		StaticMesh->CalculateExtendedBounds();

		const float MeshLength = StaticMesh->GetBoundingBox().GetExtent().X * 2.f;
		const float SplineLength = SplineComponent->GetSplineLength();

		if (MeshLength <= KINDA_SMALL_NUMBER || SplineLength <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		// Minimal number of meshes needed to cover the spline.
		int32 LoopSize = FMath::CeilToInt(SplineLength / MeshLength);
		LoopSize = FMath::Max(1, LoopSize);

		if (bUseParallelTransport)
		{
			GenerateMeshParallelTransport(MeshLength, SplineLength, LoopSize);
		}
		else
		{
			GenerateMesh(MeshLength, SplineLength, LoopSize);
		}

		bRebuildRequested = false;
	});
}

static float GetRelativeRoll(USplineComponent* Component, const FRotator& Rotation, const float Distance)
{
	const FVector ForwardVector = Rotation.UnrotateVector(Component->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local)).GetSafeNormal();
	const FVector RightVector = Rotation.UnrotateVector(Component->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local)).GetSafeNormal();
	const FVector UpVector = Rotation.UnrotateVector(Component->GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local)).GetSafeNormal();

	FMatrix Matrix(ForwardVector, RightVector, UpVector, FVector::ZeroVector);

	return Matrix.Rotator().Roll;

}

void AAedificSplineContinuum::GenerateMesh(const float MeshLength, const float SplineLength, const int32 LoopSize)
{
	if (MeshLength <= KINDA_SMALL_NUMBER || SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Prepare container
	SplineMeshComponents.Reserve(LoopSize);

	for (int32 i = 0; i < LoopSize; i++)
	{
		const FString SegmentName = FString::Printf(TEXT("SplineMesh%d"), i);

		const float CurrentDistance = i * MeshLength;
		const float NextDistance = FMath::Min((i + 1) * MeshLength, SplineLength);
		const float MidPointDistance = CurrentDistance + NextDistance / 2.f;
		const float CurrentLenght = NextDistance - CurrentDistance;

		const FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);
		const FVector EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::Local);

		const FVector UpVector = SplineComponent->GetRotationAtDistanceAlongSpline(MidPointDistance, ESplineCoordinateSpace::Local).Quaternion().GetAxisZ();

		const FVector SplineStartTangent = SplineComponent->GetTangentAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);
		const FVector StartTangent = SplineStartTangent.GetSafeNormal() * FMath::Min(SplineStartTangent.Size(), CurrentLenght);
		const FVector SplineEndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::Local);
		const FVector EndTangent = SplineEndTangent.GetSafeNormal() * FMath::Min(SplineEndTangent.Size(), CurrentLenght);;

		const float StartRollDegrees = GetRelativeRoll(SplineComponent, SplineComponent->GetRotationAtDistanceAlongSpline(MidPointDistance, ESplineCoordinateSpace::Local), CurrentDistance);
		const float EndRollDegrees = GetRelativeRoll(SplineComponent, SplineComponent->GetRotationAtDistanceAlongSpline(MidPointDistance, ESplineCoordinateSpace::Local), NextDistance);

		const FVector SplineStartScale = SplineComponent->GetScaleAtDistanceAlongSpline(CurrentDistance);
		const FVector2D StartScale = FVector2D(SplineStartScale.Y, SplineStartScale.Z);
		const FVector SplineEndScale = SplineComponent->GetScaleAtDistanceAlongSpline(NextDistance);
		const FVector2D EndScale = FVector2D(SplineEndScale.Y, SplineEndScale.Z);

		FAedificMeshSegment Segment;
		Segment.SegmentName			= SegmentName;
		Segment.StartLocation		= StartLocation;
		Segment.EndLocation			= EndLocation;
		Segment.StartTangent		= StartTangent;
		Segment.EndTangent			= EndTangent;
		Segment.UpVector			= UpVector;
		Segment.StartRollDegrees	= StartRollDegrees;
		Segment.EndRollDegrees		= EndRollDegrees;
		Segment.StartScale			= StartScale;
		Segment.EndScale			= EndScale;

		CreateSegment(Segment);
	}
}

static FVector OrthoNormalFromUp(const FVector& Tangent, const FVector& Up)
{
	// Remove tangent component from Up then normalize
	const float Dot = FVector::DotProduct(Up, Tangent);
	FVector N = (Up - Tangent * Dot);
	if (N.SizeSquared() <= KINDA_SMALL_NUMBER)
	{
		// Up was nearly parallel to tangent — pick any perpendicular vector
		FVector Axis1, Axis2;
		Tangent.FindBestAxisVectors(Axis1, Axis2);
		N = Axis1; // just pick one of them
	}
	return N.GetSafeNormal();
}

void AAedificSplineContinuum::GenerateMeshParallelTransport(const float MeshLength, const float SplineLength, const int32 LoopSize)
{
	const int32 NumFrames = LoopSize + 1; // we need start frame for each mesh plus final frame

	// spacing between mesh starts (equal spacing)
	const float Spacing = SplineLength / (float)LoopSize;

	// 1) Sample positions and tangents at evenly spaced distances
	TArray<FVector> Positions; Positions.SetNumUninitialized(NumFrames);
	TArray<FVector> Tangents;  Tangents.SetNumUninitialized(NumFrames);

	for (int32 k = 0; k < NumFrames; ++k)
	{
		const float Dist = FMath::Clamp(k * Spacing, 0.f, SplineLength);
		Positions[k] = SplineComponent->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local);
		Tangents[k] = SplineComponent->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local).GetSafeNormal();
	}

	FVector InitialUp = FVector::UpVector;

	// 2) Build normals using parallel transport
	TArray<FVector> Normals; Normals.SetNumUninitialized(NumFrames);
	TArray<FQuat>  FrameRot; FrameRot.SetNumUninitialized(NumFrames); // rotation aligning X->tangent and Z->normal (optional)

	// Initial normal (orthonormalize provided InitialUp)
	Normals[0] = OrthoNormalFromUp(Tangents[0], InitialUp);

	// store a rotation for completeness (useful for debugging)
	{
		const FMatrix M0 = FRotationMatrix::MakeFromXZ(Tangents[0], Normals[0]);
		FrameRot[0] = FQuat(M0);
	}

	for (int32 k = 1; k < NumFrames; ++k)
	{
		const FVector PrevT = Tangents[k - 1];
		const FVector CurT = Tangents[k];

		// clamp dot for safety
		const float Dot = FMath::Clamp(FVector::DotProduct(PrevT, CurT), -1.0f, 1.0f);

		if (FMath::Abs(Dot - 1.0f) < 1e-6f)
		{
			// tangents nearly identical -> no rotation
			Normals[k] = Normals[k - 1];
		}
		else if (FMath::Abs(Dot + 1.0f) < 1e-6f)
		{
			// 180-degree flip: choose a rotation axis perpendicular to PrevT

			FVector Axis1, Axis2;
			PrevT.FindBestAxisVectors(Axis1, Axis2);
			const FVector AnyPerp = Axis1;
			const FQuat Q = FQuat(AnyPerp.GetSafeNormal(), PI);
			Normals[k] = Q.RotateVector(Normals[k - 1]);
		}
		else
		{
			const FVector Axis = FVector::CrossProduct(PrevT, CurT);
			const float AxisLen = Axis.Size();

			if (AxisLen <= KINDA_SMALL_NUMBER) // numerically unstable; treat as identity
			{
				Normals[k] = Normals[k - 1];
			}
			else
			{
				const FVector AxisNorm = Axis / AxisLen;
				const float Angle = FMath::Acos(Dot);
				const FQuat Q(AxisNorm, Angle);
				Normals[k] = Q.RotateVector(Normals[k - 1]);
			}
		}

		// re-orthonormalize to avoid drift: make Normal perpendicular to CurT
		{
			const float d = FVector::DotProduct(Normals[k], CurT);
			Normals[k] = (Normals[k] - CurT * d).GetSafeNormal();
		}

		const FMatrix Mk = FRotationMatrix::MakeFromXZ(CurT, Normals[k]);
		FrameRot[k] = FQuat(Mk);
	}

	// 3) If closed loop, correct accumulated drift by distributing a rotation
	if (SplineComponent->IsClosedLoop())
	{
		// Last frame tangent should match first, but normal may drift.
		// Compute quaternion that rotates last normal to first normal, about the last tangent.
		const FVector LastNormal = Normals.Last();
		const FVector FirstNormal = Normals[0];

		// If they are already equal, no correction needed.
		if (!LastNormal.Equals(FirstNormal, 1e-4f))
		{
			// compute correction quaternion that brings LastNormal -> FirstNormal
			// axis can be tangent direction (preferably), but general rotation between vectors works:
			const float DotNN = FMath::Clamp(FVector::DotProduct(LastNormal, FirstNormal), -1.f, 1.f);
			if (FMath::Abs(DotNN - 1.f) > 1e-6f)
			{
				// rotation that maps LastNormal to FirstNormal
				const FVector CorrAxis = FVector::CrossProduct(LastNormal, FirstNormal);
				const float CorrAxisLen = CorrAxis.Size();

				FQuat CorrQuat = FQuat::Identity;
				if (CorrAxisLen > KINDA_SMALL_NUMBER)
				{
					const FVector CorrAxisNorm = CorrAxis / CorrAxisLen;
					const float CorrAngle = FMath::Acos(DotNN);
					CorrQuat = FQuat(CorrAxisNorm, CorrAngle);
				}
				else
				{
					// vectors anti-parallel or parallel - choose small correction around tangent
					CorrQuat = FQuat(Tangents.Last(), 0.f);
				}

				// Distribute correction: for frame j, apply slerp(Identity, CorrQuat, t) where t = j/(N-1)
				for (int32 j = 0; j < NumFrames; ++j)
				{
					const float t = (float)j / (float)(NumFrames - 1);
					const FQuat Step = FQuat::Slerp(FQuat::Identity, CorrQuat, t);
					Normals[j] = Step.RotateVector(Normals[j]);

					// re-orthonormalize to be safe
					const FVector Tj = Tangents[j];
					const float d = FVector::DotProduct(Normals[j], Tj);
					Normals[j] = (Normals[j] - Tj * d).GetSafeNormal();
					FrameRot[j] = FQuat(FRotationMatrix::MakeFromXZ(Tj, Normals[j]));
				}
			}
		}
	}

	// 4) Spawn SplineMeshComponents using frames. For each segment i: start frame = i, end frame = i+1
	SplineMeshComponents.Reset(LoopSize);

	for (int32 i = 0; i < LoopSize; ++i)
	{
		const float StartDistance = i * Spacing;
		const float EndDistanceUnclamped = StartDistance + MeshLength;
		const bool bIsOverhang = EndDistanceUnclamped > SplineLength;
		const float EndDistanceClamped = bIsOverhang ? SplineLength : EndDistanceUnclamped;
		const float Overhang = bIsOverhang ? (EndDistanceUnclamped - SplineLength) : 0.f;

		const FVector StartLocation = Positions[i];
		FVector EndLocation;
		if (!bIsOverhang)
		{
			EndLocation = Positions[i + 1];
		}
		else
		{
			// extrapolate along final tangent by Overhang
			const FVector SplineEndPos = SplineComponent->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local);
			const FVector SplineEndTan = SplineComponent->GetTangentAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local).GetSafeNormal();
			EndLocation = SplineEndPos + SplineEndTan * Overhang;
		}

		const FVector StartTangent = Tangents[i] * MeshLength; // restore magnitude
		FVector EndTangent;
		if (!bIsOverhang)
		{
			EndTangent = Tangents[i + 1] * MeshLength;
		}
		else
		{
			EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local).GetClampedToMaxSize(MeshLength);
		}

		// Average normals for the segment and pass to SplineMeshComponent -> SetSplineUpDir
		const FVector AvgNormal = (Normals[i] + Normals[i + 1]).GetSafeNormal();

		// Create & configure SplineMeshComponent
		const FString SegmentName = FString::Printf(TEXT("SplineMesh%d"), i);
		
		FAedificMeshSegment Segment;
		Segment.SegmentName			= SegmentName;
		Segment.StartLocation		= StartLocation;
		Segment.EndLocation			= EndLocation;
		Segment.StartTangent		= StartTangent;
		Segment.EndTangent			= EndTangent;
		Segment.UpVector			= AvgNormal;
		Segment.StartRollDegrees	= 0.f;
		Segment.EndRollDegrees		= 0.f;
		Segment.StartScale			= FVector2D(1.f);
		Segment.EndScale			= FVector2D(1.f);

		CreateSegment(Segment);
	}
}

void AAedificSplineContinuum::CreateSegment(const FAedificMeshSegment& Segment)
{
	// Create & configure spline mesh component.
	USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(),
		*Segment.SegmentName, EObjectFlags::RF_Transactional);
	NewSplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	NewSplineMeshComponent->RegisterComponent();
	NewSplineMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));

	NewSplineMeshComponent->SetMobility(EComponentMobility::Static);
	NewSplineMeshComponent->SetComponentTickEnabled(false);
	NewSplineMeshComponent->SetGenerateOverlapEvents(false);
	NewSplineMeshComponent->bComputeFastLocalBounds = true;
	NewSplineMeshComponent->bComputeBoundsOnceForGame = true;

	if (StaticMesh)
	{
		NewSplineMeshComponent->SetStaticMesh(StaticMesh);
	}

	NewSplineMeshComponent->SetStartAndEnd(Segment.StartLocation, Segment.StartTangent, Segment.EndLocation, Segment.EndTangent, false);
	NewSplineMeshComponent->SetSplineUpDir(Segment.UpVector, false);
	NewSplineMeshComponent->SetStartRollDegrees(Segment.StartRollDegrees, false);
	NewSplineMeshComponent->SetEndRollDegrees(Segment.EndRollDegrees, false);
	NewSplineMeshComponent->SetStartScale(Segment.StartScale, false);
	NewSplineMeshComponent->SetEndScale(Segment.EndScale, false);

	if (MaterialOverride)
	{
		NewSplineMeshComponent->SetMaterial(0, MaterialOverride);
	}

	NewSplineMeshComponent->UpdateMesh();

	SplineMeshComponents.AddUnique(NewSplineMeshComponent);
}

void AAedificSplineContinuum::EmptyMesh()
{
	if (SplineMeshComponents.Num() > 0)
	{
		for (USplineMeshComponent* Mesh : SplineMeshComponents)
		{
			Mesh->DestroyComponent();
		}

		SplineMeshComponents.Reset();
		MarkComponentsRenderStateDirty();
	}
}

void AAedificSplineContinuum::UpdateMaterial()
{
	if (SplineMeshComponents.Num() > 0)
	{
		for (USplineMeshComponent* Mesh : SplineMeshComponents)
		{
			if (MaterialOverride)
			{
				Mesh->SetMaterial(0, MaterialOverride);
			}
			else
			{
				Mesh->SetMaterial(0, StaticMesh->GetMaterial(0));
			}
		}
	}
}
