// Copyright (c) 2025 Pixane.

#include "AedificEditorStyle.h"

#include <Interfaces/IPluginManager.h>
#include <Styling/SlateStyle.h>
#include <Styling/SlateStyleRegistry.h>

TSharedPtr<FSlateStyleSet> FAedificEditorStyle::StyleSet = nullptr;

void FAedificEditorStyle::Startup()
{
	// Create the new Style.
	StyleSet = MakeShareable(new FSlateStyleSet("AedificEditorStyle"));

	// Assign the content root of this Style.
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Aedific"))->GetBaseDir());

	// Register icon for Aedific actor placement class.
	FSlateImageBrush* aedificIconBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/AedificIcon"), TEXT(".png")), FVector2D(16.0f));
	StyleSet->Set("Aedific.Icon", aedificIconBrush);

	// Register icon for the AedificSplineContinuum class.
	FSlateImageBrush* splineIconBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/SplineIcon"), TEXT(".png")), FVector2D(16.0f));
	StyleSet->Set("ClassIcon.AedificSplineContinuum", splineIconBrush);

	// Register thumbnail for the AedificSplineContinuum class.
	FSlateImageBrush* splineClassBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/SplineThumbnail"), TEXT(".png")), FVector2D(256.0f));
	StyleSet->Set("ClassThumbnail.AedificSplineContinuum", splineClassBrush);

	// Register the Aedific Editor Styling.
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FAedificEditorStyle::Shutdown()
{
	// Unregister the Editor Style.
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());

	// Reset the pointer.
	StyleSet.Reset();
}
