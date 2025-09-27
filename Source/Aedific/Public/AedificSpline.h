// Copyright (c) 2025 Pixane.

#pragma once

#include <GameFramework/Actor.h>

#include "AedificSpline.generated.h"

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

private:
#if WITH_EDITORONLY_DATA
	/** Sprite to show the Actor's sprite in Editor. */
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif // WITH_EDITORONLY_DATA
};
