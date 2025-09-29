// Copyright (c) 2025 Pixane.

#include "AedificSplineContinuum.h"

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
	bAutoRebuildMesh = true;
	LinearScaleFactor = 1.f;
	bAutoComputeSpline = false;
	bComputeTangents = true;
	bComputeParallelTransport = true;
	bComputeManualRoll = true;
#if WITH_EDITORONLY_DATA
	DebugUpVectorScale = 100.f;
	DebugUpVectorSize = 10.f;
	DebugUpVectorThickness = 5.f;
	DebugUpVectorDuration = 5.f;
#endif // WITH_EDITORONLY_DATA

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

	// Fetch default material file.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> floorMaterialFile(TEXT("/Aedific/Materials/MI_Blockout_Basic.MI_Blockout_Basic"));
	if (floorMaterialFile.Object)
	{
		MaterialOverride = floorMaterialFile.Object;
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
		ComputeSplineTangents(AmountOfPoints, bClosed);
	}

	if (bClosed && bComputeParallelTransport)
	{
		ComputeParallelTransport(AmountOfPoints);
	}
	
	if (bComputeManualRoll)
	{
		ComputeManualRoll(AmountOfPoints);
	}

	SplineComponent->UpdateSpline();
}

void AAedificSplineContinuum::ComputeSplineTangents(const int32 SplinePointsNum, const bool bClosed)
{
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
			// For a closed loop, neighbors always exist and wrap around.
			PreviousPoint = PointLocations[(i == 0) ? SplinePointsNum - 1 : i - 1];
			NextPoint = PointLocations[(i == SplinePointsNum - 1) ? 0 : i + 1];
		}
		else // Open spline logic
		{
			// Use current point as a fallback for ends of the spline.
			PreviousPoint = (i > 0) ? PointLocations[i - 1] : CurrentPoint;
			NextPoint = (i < SplinePointsNum - 1) ? PointLocations[i + 1] : CurrentPoint;
		}

		FVector Incoming = FVector::ZeroVector;
		FVector Outgoing = FVector::ZeroVector;

		// --- Calculate tangent vectors. ---
		// Handle first point of an open spline (no incoming tangent from a previous point).
		if (i > 0 || bClosed)
		{
			const FVector IncomingDir = (CurrentPoint - PreviousPoint).GetSafeNormal();
			const float IncomingLength = (CurrentPoint - PreviousPoint).Size();
			Incoming = IncomingDir * IncomingLength * LinearScaleFactor;
		}

		// Handle last point of an open spline (no outgoing tangent to a next point).
		if (i < SplinePointsNum - 1 || bClosed)
		{
			const FVector OutgoingDir = (NextPoint - CurrentPoint).GetSafeNormal();
			const float OutgoingLength = (NextPoint - CurrentPoint).Size();
			Outgoing = OutgoingDir * OutgoingLength * LinearScaleFactor;
		}

		// For points in the middle of a spline, use a unified direction for smoothness.
		if ((i > 0 && i < SplinePointsNum - 1) || bClosed)
		{
			const FVector UnifiedDir = (NextPoint - PreviousPoint).GetSafeNormal();
			Incoming = UnifiedDir * (CurrentPoint - PreviousPoint).Size() * LinearScaleFactor;
			Outgoing = UnifiedDir * (NextPoint - CurrentPoint).Size() * LinearScaleFactor;
		}

		SplineComponent->SetTangentsAtSplinePoint(i, Incoming, Outgoing, ESplineCoordinateSpace::Local, false);
	}
}

void AAedificSplineContinuum::ComputeParallelTransport(const int32 SplinePointsNum)
{
	// A closed loop needs at least 3 points to define a plane.
	if (SplinePointsNum < 3)
	{
		return;
	}

	// --- Pre-calculate all normalized tangents to avoid redundant work ---
	TArray<FVector> Normals;
	Normals.Reserve(SplinePointsNum);
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		Normals.Add(SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local).GetSafeNormal());
	}

	// --- PASS 1: Calculate the total twist error around the loop ---
	const FVector StartUp = FVector::UpVector;
	FVector CurrentUp = StartUp;

	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		const int32 NextIndex = (i + 1) % SplinePointsNum;
		const FVector& T0 = Normals[i];
		const FVector& T1 = Normals[NextIndex];

		const FQuat SegmentRotation = FQuat::FindBetweenNormals(T0, T1);
		CurrentUp = SegmentRotation.RotateVector(CurrentUp);
	}

	const FQuat TotalErrorQuat = FQuat::FindBetweenNormals(CurrentUp, StartUp);

	// --- PASS 2: Distribute the error correction across all points ---

	// Decompose the total error into an axis and angle.
	FVector ErrorAxis;
	float ErrorAngle;
	TotalErrorQuat.ToAxisAndAngle(ErrorAxis, ErrorAngle);

	// Calculate the small, incremental correction to apply at each segment.
	const float PerSegmentAngle = ErrorAngle / SplinePointsNum;
	const FQuat PerSegmentCorrectionQuat(ErrorAxis, PerSegmentAngle);

	// Set the initial point's up vector.
	SplineComponent->SetUpVectorAtSplinePoint(0, StartUp, ESplineCoordinateSpace::Local, false);
	CurrentUp = StartUp;

	for (int32 i = 0; i < SplinePointsNum - 1; ++i)
	{
		const FVector T0 = Normals[i];
		const FVector T1 = Normals[i + 1];

		// 1. Calculate the standard transport rotation for this segment.
		const FQuat SegmentRotation = FQuat::FindBetweenNormals(T0, T1);

		// 2. Apply the incremental error correction *first*.
		CurrentUp = PerSegmentCorrectionQuat.RotateVector(CurrentUp);

		// 3. Then apply the standard transport rotation.
		const FVector NextUp = SegmentRotation.RotateVector(CurrentUp);

		// 4. Set the final, corrected up vector for the next point.
		SplineComponent->SetUpVectorAtSplinePoint(i + 1, NextUp, ESplineCoordinateSpace::Local, false);
		CurrentUp = NextUp;
	}
}

void AAedificSplineContinuum::ComputeManualRoll(const int32 SplinePointsNum)
{
	for (int32 i = 0; i < SplinePointsNum; ++i)
	{
		// 1. Get the roll value you set in the editor (in degrees).
		const float RollInDegrees = SplineComponent->GetRollAtSplinePoint(i, ESplineCoordinateSpace::Local);

		// 2. Get the up vector that was calculated by ComputeParallelTransport.
		const FVector CurrentUp = SplineComponent->GetUpVectorAtSplinePoint(i, ESplineCoordinateSpace::Local);

		// 3. Get the tangent to use as the axis of rotation.
		const FVector TangentNormal = SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local).GetSafeNormal();

		// 4. Create a quaternion representing the roll rotation.
		const FQuat RollRotation(TangentNormal, FMath::DegreesToRadians(RollInDegrees));

		// 5. Apply the roll to the existing up vector.
		const FVector NewUp = RollRotation.RotateVector(CurrentUp);

		// 6. Set the final up vector.
		SplineComponent->SetUpVectorAtSplinePoint(i, NewUp, ESplineCoordinateSpace::Local, false);
	}
}

#if	WITH_EDITORONLY_DATA
void AAedificSplineContinuum::DrawDebugUpVectors()
{
	const int32 AmountOfPoints = SplineComponent->GetNumberOfSplinePoints();

	for (int32 i = 0; i < AmountOfPoints; i++)
	{
		const FVector Location = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		const FVector UpVector = SplineComponent->GetUpVectorAtSplinePoint(i, ESplineCoordinateSpace::World);

		DrawDebugDirectionalArrow(GetWorld(), Location, Location + UpVector * DebugUpVectorScale, DebugUpVectorSize, FColor::Blue, false, DebugUpVectorDuration, 0, DebugUpVectorThickness);
	}
}
#endif // WITH_EDITORONLY_DATA
