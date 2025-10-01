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

	// Create spline component.
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);
	SplineComponent->SetMobility(EComponentMobility::Static);
	SplineComponent->SetComponentTickEnabled(false);
	SplineComponent->SetGenerateOverlapEvents(false);
	SplineComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

void AAedificSplineContinuum::BeginDestroy()
{
	EmptyMesh();

	Super::BeginDestroy();
}

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
	// Cache all point locations to avoid redundant lookups.
	TArray<FVector> PointLocations;
	PointLocations.Reserve(SplinePointsNum);
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		PointLocations.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
	}

	// Iterate through cached points to compute tangents.
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		const FVector& CurrentPoint = PointLocations[i];
		FVector PreviousPoint, NextPoint;

		// Determine neighboring points based on loop type and position.
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

		// Calculate tangent vectors.
		if ((i > 0 && i < SplinePointsNum - 1) || bClosed)
		{
			// Interior point or closed spline become unified direction (Catmull-Rom style).
			const FVector UnifiedDir = (NextPoint - PreviousPoint).GetSafeNormal();

			Incoming = UnifiedDir * (CurrentPoint - PreviousPoint).Size() * TangentsScale;
			Outgoing = UnifiedDir * (NextPoint - CurrentPoint).Size() * TangentsScale;
		}
		else
		{
			// Endpoints of an open spline become one-sided tangents.
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
		// Get the roll value you set in the editor (in degrees).
		const FQuat Rotation = SplineComponent->GetRotationAtSplinePoint(i, ESplineCoordinateSpace::World).Quaternion();

		// Apply the roll to the existing up vector.
		const FVector NewUp = Rotation.GetAxisZ();

		// Set the final up vector.
		SplineComponent->SetUpVectorAtSplinePoint(i, NewUp, ESplineCoordinateSpace::World, false);
	}
}

void AAedificSplineContinuum::RebuildMesh()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsInitialized() || !SplineComponent || !StaticMesh || bRebuildRequested)
	{
		return;
	}

	bRebuildRequested = true;

	EmptyMesh();

	// Avoid cleaning and re-generating the meshes on the same frame.
	World->GetTimerManager().SetTimerForNextTick([this]()
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
		const int32 LoopSize = FMath::Max(1, FMath::CeilToInt(SplineLength / MeshLength));

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
	// Prepare container.
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

		// @TODO: Implement proper scaling?
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

static float CalculateRollInDegrees (const FVector& Tangent, const FVector& Normal, const FVector& ReferenceUpVector)
{
	// Project the ReferenceUpVector onto the plane perpendicular to the tangent.
	// This gives us the spline mesh's default, non-rolled "up" direction.
	const FVector DefaultUp = (ReferenceUpVector - Tangent * FVector::DotProduct(ReferenceUpVector, Tangent)).GetSafeNormal();

	// Create an orthonormal basis on that plane with a "right" vector (binormal).
	const FVector Binormal = FVector::CrossProduct(Tangent, DefaultUp);

	// Calculate the angle between the DefaultUp and our desired Normal on that plane.
	// We get the cosine of the angle from the dot product.
	const float CosAngle = FVector::DotProduct(DefaultUp, Normal);
	// We get the sine of the angle by projecting the Normal onto the Binormal.
	const float SinAngle = FVector::DotProduct(Binormal, Normal);

	// Use Atan2 to find the angle in radians and convert to degrees.
	return -FMath::RadiansToDegrees(FMath::Atan2(SinAngle, CosAngle));
};

void AAedificSplineContinuum::GenerateMeshParallelTransport(const float MeshLength, const float SplineLength, const int32 LoopSize)
{
	// Determine the number of points (frames) to generate. For N segments, we need N+1 points.
	const int32 NumFrames = LoopSize + 1;

	// Calculate the distance between each frame along the spline.
	const float Spacing = SplineLength / (float)LoopSize;

	// Sample positions and tangents at evenly spaced distances along the spline.
	// These will form the "spine" for our generated meshes.
	TArray<FVector> Positions; Positions.SetNumUninitialized(NumFrames);
	TArray<FVector> Tangents;  Tangents.SetNumUninitialized(NumFrames);

	for (int32 k = 0; k < NumFrames; ++k)
	{
		const float Dist = k * Spacing;
		// We don't need to clamp here as k * Spacing will not exceed SplineLength.
		Positions[k] = SplineComponent->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local);
		Tangents[k] = SplineComponent->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local).GetSafeNormal();
	}

	// Build normals using Parallel Transport to create smooth, twist-free orientation frames.
	TArray<FVector> Normals; Normals.SetNumUninitialized(NumFrames);
	FVector InitialUp = FVector::UpVector; // Define an initial "up" direction.

	// The first normal is calculated by making the InitialUp vector orthogonal to the first tangent.
	Normals[0] = (InitialUp - Tangents[0] * FVector::DotProduct(InitialUp, Tangents[0])).GetSafeNormal();

	for (int32 k = 1; k < NumFrames; ++k)
	{
		const FVector PrevTangent = Tangents[k - 1];
		const FVector CurrentTangent = Tangents[k];

		// Calculate the rotation that transforms the previous tangent to the current one.
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(PrevTangent, CurrentTangent);

		// Apply this rotation to the previous normal to get the new normal.
		Normals[k] = DeltaRotation.RotateVector(Normals[k - 1]);

		// Re-orthonormalize to prevent floating-point drift from accumulating.
		Normals[k] = (Normals[k] - CurrentTangent * FVector::DotProduct(Normals[k], CurrentTangent)).GetSafeNormal();
	}

	// If the spline is a closed loop, distribute the accumulated rotational error.
	if (SplineComponent->IsClosedLoop())
	{
		// The start and end tangents are identical, but floating point errors can cause the normals to drift.
		const FVector LastNormal = Normals.Last();
		const FVector FirstNormal = Normals[0];

		// Calculate the total correction rotation needed to align the last normal with the first.
		const FQuat TotalCorrection = FQuat::FindBetweenNormals(LastNormal, FirstNormal);

		// Apply the correction incrementally along the spline using Slerp.
		for (int32 j = 0; j < NumFrames; ++j)
		{
			const float Alpha = (float)j / (float)(NumFrames - 1);
			const FQuat StepCorrection = FQuat::Slerp(FQuat::Identity, TotalCorrection, Alpha);

			Normals[j] = StepCorrection.RotateVector(Normals[j]);

			// Re-orthonormalize one last time.
			const FVector Tj = Tangents[j];
			Normals[j] = (Normals[j] - Tj * FVector::DotProduct(Normals[j], Tj)).GetSafeNormal();
		}
	}

	// Spawn SplineMeshComponents using the generated frames.
	// Each segment 'i' uses frame 'i' for its start and frame 'i+1' for its end.
	SplineMeshComponents.Reset(LoopSize);

	for (int32 i = 0; i < LoopSize; ++i)
	{
		const int32 StartIndex = i;
		const int32 EndIndex = i + 1;

		const FVector StartTangentVec = Tangents[StartIndex];
		const FVector EndTangentVec = Tangents[EndIndex];
		const FVector StartNormalVec = Normals[StartIndex];
		const FVector EndNormalVec = Normals[EndIndex];

		// The single "UpVector" for SetSplineUpDir acts as a reference frame.
		// Averaging the start and end normals is a reasonable choice for this reference.
		const FVector ReferenceUp = (StartNormalVec + EndNormalVec).GetSafeNormal();

		// The magnitude of the tangent for a spline mesh controls its curvature.
		// A good default is the distance between the points.
		const float TangentMagnitude = (Positions[EndIndex] - Positions[StartIndex]).Size();

		// Calculate the roll needed at the start and end of the segment.
		// @TODO: Combine with user inputed Spline rotation?
		const float StartRoll = CalculateRollInDegrees(StartTangentVec, StartNormalVec, ReferenceUp);
		const float EndRoll = CalculateRollInDegrees(EndTangentVec, EndNormalVec, ReferenceUp);

		FAedificMeshSegment Segment;
		Segment.SegmentName			= FString::Printf(TEXT("SplineMesh%d"), i);
		Segment.UpVector			= ReferenceUp; // For SetSplineUpDir()
		Segment.StartLocation		= Positions[StartIndex];
		Segment.StartTangent		= StartTangentVec * TangentMagnitude;
		Segment.EndLocation			= Positions[EndIndex];
		Segment.EndTangent			= EndTangentVec * TangentMagnitude;
		Segment.StartRollDegrees	= StartRoll;
		Segment.EndRollDegrees		= EndRoll;

		// @TODO: Implement proper scaling?
		Segment.StartScale	= FVector2D::UnitVector;
		Segment.EndScale	= FVector2D::UnitVector;

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
			if (Mesh->IsValidLowLevelFast())
			{
				Mesh->DestroyComponent();
			}
		}

		SplineMeshComponents.Reset();
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
