// Copyright (c) 2025 Pixane.

#include "AedificSpline.h"

#include <Components/SplineComponent.h>
#include <Components/SplineMeshComponent.h>

#if WITH_EDITORONLY_DATA
#include <Components/BillboardComponent.h>
#include <ImageUtils.h>
#endif // WITH_EDITORONLY_DATA

#include UE_INLINE_GENERATED_CPP_BY_NAME(AedificSpline)

AAedificSpline::AAedificSpline()
{
	// Set default values for AActor interface members.
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;

	// Set default values for AAedificSpline members.
	SplineMeshComponents.Empty();
	StaticMesh = nullptr;
	DefaultStaticMesh = nullptr;
	OverrideMaterial = nullptr;
	bAbsoluteUpDirection = false;
	AbsoluteUpDirection = FVector::UpVector;
	MeshSizeTreshold = 0.3f;
	TangentScalingFactor = 1.f;

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
		// Assign the mesh to the static mesh reference.
		DefaultStaticMesh = floorMeshFile.Object;
		StaticMesh = DefaultStaticMesh;
	}

	// Fetch default material file.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> floorMaterialFile(TEXT("/Aedific/Materials/MI_Blockout_Basic.MI_Blockout_Basic"));
	if (floorMaterialFile.Object)
	{
		// Assign the material to the material reference.
		OverrideMaterial = floorMaterialFile.Object;
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
		EditorSprite->SpriteInfo.DisplayName = INVTEXT("AedificSpline");
		EditorSprite->SetUsingAbsoluteScale(true);
		EditorSprite->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void AAedificSpline::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == FName(TEXT("StaticMesh")))
	{
		if (!StaticMesh)
		{
			// Assign the mesh to the static mesh reference.
			StaticMesh = DefaultStaticMesh;
		}
		GenerateMeshes();
	}
	else if(PropertyChangedEvent.GetPropertyName() == FName(TEXT("Material")))
	{
		UpdateMaterials();
	}
}
#endif // WITH_EDITOR

void AAedificSpline::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Rebuild();
}

void AAedificSpline::Rebuild()
{
	GenerateMeshes();
}

void AAedificSpline::CalculateTangents()
{
	const int32 PointsAmount = SplineComponent->GetNumberOfSplinePoints();
	const bool bClosed = SplineComponent->IsClosedLoop();

	if (PointsAmount > 1)
	{
		for (int32 i = 0; i < PointsAmount; i++)
		{
			const FVector CurrentPoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);

			FVector Incoming = FVector::ZeroVector;
			FVector Outgoing = FVector::ZeroVector;

			if (bClosed || (i > 0 && i < PointsAmount - 1))
			{
				// Pick neighbors, wrapping if closed.
				const int32 PreviousIndex = (i == 0) ? (PointsAmount - 1) : (i - 1);
				const int32 NextIndex = (i == PointsAmount - 1) ? 0 : (i + 1);

				const FVector PreviousPoint = SplineComponent->GetLocationAtSplinePoint(PreviousIndex, ESplineCoordinateSpace::Local);
				const FVector NextPoint = SplineComponent->GetLocationAtSplinePoint(NextIndex, ESplineCoordinateSpace::Local);
				const FVector Direction = (NextPoint - PreviousPoint).GetSafeNormal();

				Incoming = Direction * (CurrentPoint - PreviousPoint).Length() / TangentScalingFactor;
				Outgoing = Direction * (CurrentPoint - NextPoint).Length() / TangentScalingFactor;
			}
			else if (i == 0) // Open spline first point.
			{
				const FVector FirstPoint = SplineComponent->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::Local);
				const FVector Direction = (FirstPoint - CurrentPoint).GetSafeNormal();
				Outgoing = Direction * (FirstPoint - CurrentPoint).Length() / TangentScalingFactor;
			}
			else if (i == PointsAmount - 1) // Open spline last point.
			{
				const FVector LastPoint = SplineComponent->GetLocationAtSplinePoint(PointsAmount - 2, ESplineCoordinateSpace::Local);
				const FVector Direction = (CurrentPoint - LastPoint).GetSafeNormal();
				Incoming = Direction * (CurrentPoint - LastPoint).Length() / TangentScalingFactor;
			}
			SplineComponent->SetTangentsAtSplinePoint(i, Incoming, Outgoing, ESplineCoordinateSpace::Local, false);
		}
		SplineComponent->UpdateSpline();
		GenerateMeshes();
	}
}

void AAedificSpline::Reset()
{
	SplineComponent->ResetToDefault();

	GenerateMeshes();

	MarkComponentsRenderStateDirty();
}

void AAedificSpline::EmptyMeshes()
{
	if (SplineMeshComponents.Num() > 0)
	{
		for (USplineMeshComponent* spline : SplineMeshComponents)
		{
			spline->DestroyComponent();
		}

		SplineMeshComponents.Reset();
	}
}

void AAedificSpline::GenerateMeshes()
{
	if (StaticMesh)
	{
		EmptyMeshes();

		StaticMesh->CalculateExtendedBounds();

		// Precompute
		const float MeshLength = StaticMesh ? StaticMesh->GetBoundingBox().GetSize().X : 0.f;
		const float SplineLength = SplineComponent->GetSplineLength();

		if (MeshLength <= KINDA_SMALL_NUMBER || SplineLength <= KINDA_SMALL_NUMBER)
		{
			// safe-guard
			return;
		}

		// Start with the minimal number of meshes needed to cover the spline
		int32 LoopSize = FMath::CeilToInt(SplineLength / MeshLength);
		LoopSize = FMath::Max(1, LoopSize);

		// Reduce LoopSize while constraints are violated (but never below 1):
		//  - spacing >= MeshSizeTreshold * MeshLength
		//  - overhang fraction <= MeshSizeTreshold
		while (LoopSize > 1)
		{
			const float Spacing = SplineLength / (float)LoopSize;
			const bool bSpacingTooSmall = Spacing < (MeshSizeTreshold * MeshLength);

			// Overhang fraction if each mesh covers MeshLength but starts spaced by Spacing:
			// last mesh end = (LoopSize - 1) * Spacing + MeshLength
			// overhang = lastMeshEnd - SplineLength
			// overhangFraction = overhang / MeshLength = 1 - (SplineLength / (LoopSize * MeshLength))
			const float OverhangFraction = 1.0f - (SplineLength / ((float)LoopSize * MeshLength));
			const bool bOverhangTooLarge = OverhangFraction > MeshSizeTreshold;

			if (!bSpacingTooSmall && !bOverhangTooLarge)
			{
				break; // constraints satisfied
			}

			// otherwise reduce number of meshes (this increases spacing and reduces overhang)
			LoopSize--;
		}

		// Final sanity clamp
		LoopSize = FMath::Max(1, LoopSize);

		// prepare container
		SplineMeshComponents.Reset(LoopSize);

		// spacing between mesh starts (equal spacing)
		const float Spacing = SplineLength / (float)LoopSize;

		for (int32 i = 0; i < LoopSize; ++i)
		{
			const float StartDistance = i * Spacing;
			const float EndDistanceUnclamped = StartDistance + MeshLength;
			const bool bIsOverhang = EndDistanceUnclamped > SplineLength;
			const float EndDistanceClamped = bIsOverhang ? SplineLength : EndDistanceUnclamped;
			const float OverhangAmount = bIsOverhang ? (EndDistanceUnclamped - SplineLength) : 0.f;

			// Locations
			const FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);

			FVector EndLocation;
			if (!bIsOverhang)
			{
				EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistanceClamped, ESplineCoordinateSpace::Local);
			}
			else
			{
				// Extrapolate past the spline end along the final tangent for a natural overhang
				const FVector SplineEndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local);
				const FVector SplineEndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local).GetSafeNormal();
				EndLocation = SplineEndLocation + SplineEndTangent * OverhangAmount;
			}

			// Tangents (clamped to MeshLength similar to your original code)
			const FVector StartTangent = SplineComponent->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local).GetClampedToMaxSize(MeshLength);
			FVector EndTangent;
			if (!bIsOverhang)
			{
				EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(EndDistanceClamped, ESplineCoordinateSpace::Local).GetClampedToMaxSize(MeshLength);
			}
			else
			{
				// use final spline tangent for overhang tangent
				const FVector SplineEndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local);
				EndTangent = SplineEndTangent.GetClampedToMaxSize(MeshLength);
			}

			const FVector StartScale = SplineComponent->GetScaleAtDistanceAlongSpline(StartDistance);
			const FVector EndScale = SplineComponent->GetScaleAtDistanceAlongSpline(EndDistanceClamped);

			const float StartRoll = SplineComponent->GetRollAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
			const float EndRoll = SplineComponent->GetRollAtDistanceAlongSpline(EndDistanceClamped, ESplineCoordinateSpace::Local);

			// Up vector sampling: use midpoint along the actual part of the mesh that lies on-spline
			float MidSampleDistance = StartDistance + FMath::Min(MeshLength * 0.5f, EndDistanceClamped - StartDistance);
			MidSampleDistance = FMath::Clamp(MidSampleDistance, 0.f, SplineLength);

			FVector UpVector = FVector::UpVector;
			if (bAbsoluteUpDirection)
			{
				UpVector = AbsoluteUpDirection;
			}
			else
			{
				UpVector = SplineComponent->GetUpVectorAtDistanceAlongSpline(MidSampleDistance, ESplineCoordinateSpace::Local);
			}

			// Create & configure spline mesh component (your original code with names preserved)
			const FString SegmentName = FString::Printf(TEXT("SplineMesh%d"), i);
			USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), *SegmentName, EObjectFlags::RF_Transactional);
			NewSplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			NewSplineMeshComponent->AttachToComponent(SplineComponent, FAttachmentTransformRules::FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));
			NewSplineMeshComponent->RegisterComponent();

			NewSplineMeshComponent->SetMobility(EComponentMobility::Static);
			NewSplineMeshComponent->SetComponentTickEnabled(false);
			NewSplineMeshComponent->SetGenerateOverlapEvents(false);
			NewSplineMeshComponent->bComputeFastLocalBounds = true;
			NewSplineMeshComponent->bComputeBoundsOnceForGame = true;

			NewSplineMeshComponent->SetForwardAxis(ESplineMeshAxis::X, false);
			NewSplineMeshComponent->SetSplineUpDir(UpVector, false);
			NewSplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, false);
			NewSplineMeshComponent->SetStartRoll(StartRoll, false);
			NewSplineMeshComponent->SetEndRoll(EndRoll, false);
			NewSplineMeshComponent->SetStartScale(FVector2D(StartScale.Y, StartScale.Z), false);
			NewSplineMeshComponent->SetEndScale(FVector2D(EndScale.Y, EndScale.Z), false);
			NewSplineMeshComponent->UpdateMesh();

			NewSplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndProbe);

			if (StaticMesh)
			{
				NewSplineMeshComponent->SetStaticMesh(StaticMesh);
			}

			if (OverrideMaterial)
			{
				NewSplineMeshComponent->SetMaterial(0, OverrideMaterial);
			}

			SplineMeshComponents.AddUnique(NewSplineMeshComponent);
		}
	}
}

void AAedificSpline::UpdateMaterials()
{
	for (USplineMeshComponent* spline : SplineMeshComponents)
	{
		if (OverrideMaterial)
		{
			spline->SetMaterial(0, OverrideMaterial);
		}
		else if (StaticMesh)
		{
			spline->SetMaterial(0, StaticMesh->GetMaterial(0));
		}
	}
}
