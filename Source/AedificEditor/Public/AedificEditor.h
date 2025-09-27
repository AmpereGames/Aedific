// Copyright (c) 2025 Pixane.

#pragma once

#include <Modules/ModuleInterface.h>

class FAedificEditorModule : public IModuleInterface
{
public:
	//~ Begin of IModuleInterface implementation.
	void StartupModule() override;
	void ShutdownModule() override;
	//~ End of IModuleInterface implementation.
};
