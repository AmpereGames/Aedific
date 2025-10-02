// Copyright (c) 2025 Ampere Games.

#pragma once

#include <Components/SplineMeshComponent.h>

#include "AedificSplineTypes.generated.h"

USTRUCT()
struct FAedificMeshSegment
{
	GENERATED_BODY()

	/**  */
	FString SegmentName;

	/**  */
	FVector StartLocation;

	/**  */
	FVector EndLocation; 

	/**  */
	FVector StartTangent;

	/**  */
	FVector EndTangent; 

	/**  */
	FVector UpVector;

	/**  */
	float StartRollDegrees;

	/**  */
	float EndRollDegrees;

	/**  */
	FVector2D StartScale;

	/**  */
	FVector2D EndScale;

	FAedificMeshSegment()
	{
		SegmentName =		TEXT("");
		StartLocation =		FVector::ZeroVector;
		EndLocation =		FVector::ZeroVector;
		StartTangent =		FVector::ZeroVector;
		EndTangent =		FVector::ZeroVector;
		UpVector =			FVector::UpVector;
		StartRollDegrees =	0.f;
		EndRollDegrees =	0.f;
		StartScale =		FVector2D::UnitVector;
		EndScale =			FVector2D::UnitVector;
	}
};
