// Copyright (c) 2025 Pixane.

#pragma once

#include <GameFramework/Actor.h>

#include "AedificSplineContinuum.generated.h"

class USplineComponent;
class USplineMeshComponent;
struct FAedificMeshSegment;

/**
 * A spline-based construction tool designed for continuous distribution of meshes along
 * a spline path. Useful for creating roads, pipes, fences, rails, or any geometry that
 * needs to follow a defined curve seamlessly.
 *
 * Automatically handles mesh tiling, orientation, and alignment along the spline, while
 * exposing controls for scaling, rotation, spacing, and mesh selection.
 */
UCLASS(ClassGroup = Aedific, HideCategories = (Actor, Collision, DataLayers, Input, LOD, HLOD, Networking, Physics, RayTracing, TextureStreaming, WorldPartition), MinimalAPI, NotBlueprintable)
class AAedificSplineContinuum : public AActor
{
	GENERATED_BODY()
	
public:

	/** Sets default values for this actor's properties. */
	AAedificSplineContinuum();

	//~ Begin of UObject implementation.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	//~ End of UObject implementation.

	//~ Begin of AActor implementation.
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End of AActor implementation.

	/** Override manual spline values with computed ones. */
	void ComputeSpline();

	/**  */
	void RebuildMesh();

protected:

	/** The Actor's root component. */
	TObjectPtr<USceneComponent> SceneComponent;

	/** For user in-editor manipulation of Spline points. */
	UPROPERTY(VisibleInstanceOnly, Category = "Aedific|Spline")
	TObjectPtr<USplineComponent> SplineComponent;

	/** Asset that will be used to build the spline. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific")
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Override material of the Static Mesh asset. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** If true, the Spline's properies (tangents, up-vectors and rotations) will automatically be computed.  */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Spline")
	uint8 bAutoComputeSpline : 1;

	/** . */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Spline", Meta = (EditCondition = bAutoComputeSpline))
	uint8 bComputeTangents : 1;

	/**
	 * Linear tangent scale factor.
	 * 0.0 = Constant, 1.0 = Smooth.
	 */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Spline", Meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f, Delta = 0.01f, EditCondition = "bAutoComputeSpline && bComputeTangents"))
	float TangentsScale;

	/** . */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Spline", Meta = (EditCondition = "bAutoComputeSpline && !bUseParallelTransport"))
	uint8 bComputeUpVectors : 1;

	/** .  */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Mesh")
	uint8 bAutoRebuildMesh : 1;

	/** . */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Mesh", Meta = (EditCondition = bAutoRebuildMesh))
	uint8 bUseParallelTransport : 1;

	/** Tangent computation function. */
	void ComputeTangents(const int32 SplinePointsNum, const bool bClosed);

	/**  */
	void ComputeUpVectors(const int32 SplinePointsNum);

	/**  */
	void GenerateMesh(const float MeshLength, const float SplineLength, const int32 LoopSize);

	/**  Applies Frenet-like parallel transport frame builder to ensure smooth rotation along loops. */
	void GenerateMeshParallelTransport(const float MeshLength, const float SplineLength, const int32 LoopSize);

	/**  */
	void CreateSegment(const FAedificMeshSegment& Segment);

	/**  */
	void EmptyMesh();

	/**  */
	void UpdateMaterial();

private:

#if WITH_EDITORONLY_DATA
	/** Sprite to show the Actor's sprite in Editor. */
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif // WITH_EDITORONLY_DATA

	/** .  */
	uint8 bRebuildRequested : 1;

	/** Container for the generated meshes. */
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshComponents;
};
