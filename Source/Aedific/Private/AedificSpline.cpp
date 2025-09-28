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
	Material = nullptr;
	DefaultMaterial = nullptr;
	bInterpRoll = false;
	bAbsoluteUpDirection = false;
	UpDirection = FVector::UpVector;
	TangentScalingFactor = 1.f;

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
		DefaultMaterial = floorMaterialFile.Object;
		Material = DefaultMaterial;
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
		if (!Material)
		{
			Material = DefaultMaterial;
		}
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
	const int32 PointsNum = SplineComponent->GetNumberOfSplinePoints();
	if (PointsNum > 1)
	{
		for (int32 i = 0; i < PointsNum; i++)
		{
			const FVector P = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);

			FVector Incoming = FVector::ZeroVector;
			FVector Outgoing = FVector::ZeroVector;

			if (i == 0) // first point
			{
				const FVector P1 = SplineComponent->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::Local);
				const FVector Dir = (P1 - P).GetSafeNormal();

				// Incoming stays zero
				Outgoing = Dir * (P1 - P).Length() / TangentScalingFactor;
			}
			else if (i == PointsNum - 1) // last point
			{
				const FVector Pn1 = SplineComponent->GetLocationAtSplinePoint(PointsNum - 2, ESplineCoordinateSpace::Local);
				const FVector Dir = (P - Pn1).GetSafeNormal();

				// Outgoing stays zero
				Incoming = Dir * (P - Pn1).Length() / TangentScalingFactor;
			}
			else // interior point
			{
				const FVector Prev = SplineComponent->GetLocationAtSplinePoint(i - 1, ESplineCoordinateSpace::Local);
				const FVector Next = SplineComponent->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);

				const FVector Dir = (Next - Prev).GetSafeNormal();

				Incoming = Dir * (P - Prev).Length() / TangentScalingFactor;
				Outgoing = Dir * (P - Next).Length() / TangentScalingFactor;
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
		
		const float MeshLength = StaticMesh->GetBoundingBox().GetSize().X;
		const float SplineLeght = SplineComponent->GetSplineLength();
		const int32 LoopSize = FMath::CeilToInt(SplineLeght / MeshLength);

		FVector StartLocation = FVector::ZeroVector;
		FVector EndLocation = FVector::ZeroVector;
		FVector StartTangent = FVector::ZeroVector;
		FVector EndTangent = FVector::ZeroVector;
		FVector UpVector = FVector::UpVector;
		float StartDistance = 0.f;
		float MidDistance = 0.f;
		float EndDistance = 0.f;

		SplineMeshComponents.Reset(LoopSize);

		for (int32 i = 0; i < LoopSize; i++)
		{
			if (SplineLeght < MeshLength)
			{
				StartDistance = 0.f;
				EndDistance = SplineLeght;
			}
			else
			{
				StartDistance = i * MeshLength;
				EndDistance = StartDistance + MeshLength;
			}

			if (EndDistance >= SplineLeght)
			{
				EndDistance = SplineLeght;
			}

			MidDistance = StartDistance + (EndDistance - StartDistance) / 2.f;

			StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
			EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);

			StartTangent = SplineComponent->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local).GetClampedToMaxSize(MeshLength);
			EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local).GetClampedToMaxSize(MeshLength);

			if (bAbsoluteUpDirection)
			{
				UpVector = UpDirection;
			}
			else
			{
				UpVector = SplineComponent->GetUpVectorAtDistanceAlongSpline(MidDistance, ESplineCoordinateSpace::Local);
			}

			const FString SegmentName = "SplineMesh" + FString::SanitizeFloat(i);
			USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), *SegmentName, EObjectFlags::RF_Transactional);
			NewSplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			NewSplineMeshComponent->RegisterComponent();
			NewSplineMeshComponent->AttachToComponent(SplineComponent, FAttachmentTransformRules::FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));
			NewSplineMeshComponent->SetMobility(EComponentMobility::Static);
			
			NewSplineMeshComponent->SetComponentTickEnabled(false);
			NewSplineMeshComponent->bSmoothInterpRollScale = bInterpRoll;

			NewSplineMeshComponent->SetForwardAxis(ESplineMeshAxis::X);
			NewSplineMeshComponent->SetSplineUpDir(UpVector);
			NewSplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);

			NewSplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndProbe);

			if (StaticMesh)
			{
				NewSplineMeshComponent->SetStaticMesh(StaticMesh);
			}

			if (Material)
			{
				NewSplineMeshComponent->SetMaterial(0, Material);
			}
		}
	}
}

void AAedificSpline::UpdateMaterials()
{
	if (Material)
	{
		for (USplineMeshComponent* spline : SplineMeshComponents)
		{
			spline->SetMaterial(0, Material);
		}
	}
}
