// Copyright (c) 2025 Pixane.

#pragma once

#include <GameFramework/Actor.h>

#include "AedificSpline.generated.h"

class USplineComponent;
class USplineMeshComponent;

/**
 * Use this to create continuous spline based meshes.
 */
UCLASS(ClassGroup = Aedific, HideCategories = (Actor, Collision, DataLayers, Input, LOD, HLOD, Networking, Physics, RayTracing, WorldPartition), MinimalAPI, NotBlueprintable)
class AAedificSpline : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Sets default values for this actor's properties. */
	AAedificSpline();

protected:
	/** The Actor's root. */
	TObjectPtr<USceneComponent> SceneComponent;

	/** For user in-editor manipulation of Spline points. */
	UPROPERTY(VisibleInstanceOnly, Category = Aedific)
	TObjectPtr<USplineComponent> SplineComponent;

	/** Asset that will be used to build the spline. */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Override material of the Static Mesh asset. */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	TObjectPtr<UMaterialInterface> OverrideMaterial;

	/** Wheter to use an absolute normal vector for each mesh instead of the auto computed ones. */
	UPROPERTY(EditInstanceOnly, Category = Aedific, Meta = (InlineEditConditionToggle))
	uint8 bAbsoluteUpDirection : 1;

	/** Fixed direction for each mesh up vector instead of the auto computed one. */
	UPROPERTY(EditInstanceOnly, Category = Aedific, Meta = (EditCondition = "bAbsoluteUpDirection", ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f))
	FVector AbsoluteUpDirection;

	/**
	 * Allows up to this percentage of the StaticMesh size to hang past spline end
	 * and do not accept spacing smaller than this percentage of the StaticMesh size.
	 */
	UPROPERTY(EditInstanceOnly, Category = Aedific, Meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f))
	float MeshSizeTreshold;

	/** Factor to scale each tangent for smoothing. */
	UPROPERTY(EditInstanceOnly, Category = Aedific, Meta = (ClampMin = 0.f, ClampMax = 10.f, UIMin = 0.f, UIMax = 10.f))
	float TangentScalingFactor;

	//~ Begin of UObject implementation.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	//~ End of UObject implementation.

	//~ Begin of AActor implementation.
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End of AActor implementation.

	/** Clear meshes if any, and generate them again. */
	void GenerateMeshes();

	/** Assing the override material to the generated meshes. */
	void UpdateMaterials();

	/** Clear the generated meshes. */
	void EmptyMeshes();

	/** Clear the generated meshes and regenerate them. */
	UFUNCTION(CallInEditor, Category = Aedific)
	void Rebuild();

	/** Resets the spline and leave as the default two-point one. */
	UFUNCTION(CallInEditor, Category = Aedific)
	void Reset();

	/** Auto-compute the tangets and assign them to the spline component. */
	UFUNCTION(CallInEditor, Category = Aedific)
	void CalculateTangents();

private:
#if WITH_EDITORONLY_DATA
	/** Sprite to show the Actor's sprite in Editor. */
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif // WITH_EDITORONLY_DATA

	/** Container for the generated meshes. */
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshComponents;

	/** Default StaticMesh when none is assigned. */
	TObjectPtr<UStaticMesh> DefaultStaticMesh;
};
