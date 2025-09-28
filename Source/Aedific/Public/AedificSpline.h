// Copyright (c) 2025 Pixane.

#pragma once

#include <GameFramework/Actor.h>

#include "AedificSpline.generated.h"

class USplineComponent;
class USplineMeshComponent;

/**
 * Actor class to created spline based meshes.
 */
UCLASS(ClassGroup = Aedific, HideCategories = (Actor, Collision, DataLayers, Input, LOD, HLOD, Networking, Physics, WorldPartition), MinimalAPI, NotBlueprintable)
class AAedificSpline : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Sets default values for this actor's properties. */
	AAedificSpline();

protected:
	/** Scene component. */
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneComponent;

	/** Spline component. */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	TObjectPtr<USplineComponent> SplineComponent;

	/** Mesh. */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Material. */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	TObjectPtr<UMaterialInterface> Material;

	/**  */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	uint8 bInterpRoll : 1;

	/**  */
	UPROPERTY(EditInstanceOnly, Category = Aedific, Meta = (InlineEditConditionToggle))
	uint8 bAbsoluteUpDirection : 1;

	/**  */
	UPROPERTY(EditInstanceOnly, Category = Aedific, Meta = (EditCondition = "bAbsoluteUpDirection"))
	FVector UpDirection;

	/**  */
	UPROPERTY(EditInstanceOnly, Category = Aedific)
	float TangentScalingFactor;

	//~ Begin of UObject implementation.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	//~ End of UObject implementation.

	//~ Begin of AActor implementation.
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End of AActor implementation.

	/**  */
	void GenerateMeshes();

	/**  */
	void UpdateMaterials();

	/**  */
	void EmptyMeshes();

	/**  */
	UFUNCTION(CallInEditor, Category = Aedific)
	void Rebuild();

	/**  */
	UFUNCTION(CallInEditor, Category = Aedific)
	void Reset();

	/**  */
	UFUNCTION(CallInEditor, Category = Aedific)
	void CalculateTangents();

private:
#if WITH_EDITORONLY_DATA
	/** Sprite to show the Actor's sprite in Editor. */
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif // WITH_EDITORONLY_DATA

	/**  */
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshComponents;

	/** Mesh. */
	TObjectPtr<UStaticMesh> DefaultStaticMesh;

	/** Material. */
	TObjectPtr<UMaterialInterface> DefaultMaterial;
};
