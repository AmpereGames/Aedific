// Copyright (c) 2025 Pixane.

#pragma once

#include "AedificTypes.h"

#include <GameFramework/Actor.h>

#include "AedificSplineContinuum.generated.h"

class USplineComponent;
class USplineMeshComponent;
enum class EAedificTangentsComputeMethod : uint8;

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

protected:

	/** The Actor's root component. */
	TObjectPtr<USceneComponent> SceneComponent;

	/** For user in-editor manipulation of Spline points. */
	UPROPERTY(VisibleInstanceOnly, Category = "Aedific")
	TObjectPtr<USplineComponent> SplineComponent;

	/** Asset that will be used to build the spline. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Mesh")
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Override material of the Static Mesh asset. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Mesh")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** .  */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Mesh")
	uint8 bAutoRebuildMesh : 1;
	
	/**
	 * Linear tangent scale factor.
	 * 0.0 = Constant, 1.0 = Smooth.
	 */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Tangents", Meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f, Delta = 0.01f))
	float LinearScaleFactor;

	/** If true, the Spline values will be computed and overriden.  */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Tangents")
	uint8 bAutoComputeSpline : 1;

	/** . */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Tangents")
	uint8 bComputeTangents : 1;

	/** . */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Tangents")
	uint8 bComputeParallelTransport : 1;

	/** . */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Tangents")
	uint8 bComputeManualRoll : 1;

#if WITH_EDITORONLY_DATA
	/** Scale of the drawn UpVector arrows. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Debug", Meta = (ForceUnits = "cm"))
	float DebugUpVectorScale;

	/** Size of the drawn UpVector arrows. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Debug", Meta = (ForceUnits = "cm"))
	float DebugUpVectorSize;

	/** Thickness of the drawn UpVector arrows. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Debug", Meta = (ForceUnits = "cm"))
	float DebugUpVectorThickness;

	/** For how long the UpVector arrows will be drawn for. */
	UPROPERTY(EditInstanceOnly, Category = "Aedific|Debug", Meta = (ForceUnits = "seconds"))
	float DebugUpVectorDuration;

	/** Debug function to visualize up vectors at each spline point. */
	UFUNCTION(CallInEditor, Category = "Aedific")
	void DrawDebugUpVectors();
#endif // WITH_EDITORONLY_DATA

	/** Override manual spline values with computed ones. */
	UFUNCTION(CallInEditor, Category = "Aedific")
	void RebuildMesh();

	/** Override manual spline values with computed ones. */
	UFUNCTION(CallInEditor, Category = "Aedific")
	void ComputeSpline();

private:

#if WITH_EDITORONLY_DATA
	/** Sprite to show the Actor's sprite in Editor. */
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif // WITH_EDITORONLY_DATA

	/** Tangent computation function. */
	void ComputeSplineTangents(const int32 SplinePointsNum, const bool bClosed);

	/** Applies parallel transport to ensure smooth rotation along closed loops. */
	void ComputeParallelTransport(const int32 SplinePointsNum);
	
	/** Applies manual roll adjustments at each spline point. */
	void ComputeManualRoll(const int32 SplinePointsNum);
};
