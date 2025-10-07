// Copyright (c) 2025 Ampere Games.

#include "AedificEditor.h"
#include "AedificEditorStyle.h"
#include "AedificSplineContinuum.h"

#include <IPlacementModeModule.h>
#include <Modules/ModuleManager.h>

#define LOCTEXT_NAMESPACE "FEmpyreanEditorModule"

void FAedificEditorModule::StartupModule()
{
	// Init the Editor Style.
	FAedificEditorStyle::Startup();

	// Create a new Actor Placement category and register the plugin's actors.
	IPlacementModeModule& placementModeModule = IPlacementModeModule::Get();

	// Create a custom category and register it.
	FPlacementCategoryInfo info(INVTEXT("Aedific"), FSlateIcon("AedificEditorStyle", "Aedific.Icon"), "Aedific", TEXT("PMAedific"), 32);
	placementModeModule.RegisterPlacementCategory(info);

	// Add actor classes to the category.
	placementModeModule.RegisterPlaceableItem(info.UniqueHandle, MakeShareable(new FPlaceableItem(nullptr, FAssetData(AAedificSplineContinuum::StaticClass()))));
}

void FAedificEditorModule::ShutdownModule()
{
	// Unregister the custom Actor Placement category.
	IPlacementModeModule::Get().UnregisterPlacementCategory("Aedific");

	// Clean the module.
	FModuleManager::Get().OnModulesChanged().RemoveAll(this);

	// Clean the Editor Style.
	FAedificEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAedificEditorModule, AedificEditor)
