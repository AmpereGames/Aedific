// Copyright (c) 2025 Ampere Games.

#pragma once

class FSlateStyleSet;

class FAedificEditorStyle
{
public:
	/** Configures the Editor Style and register it. */
	static void Startup();

	/** Resets the Editor Style and unregister it. */
	static void Shutdown();

private:
	/** Static reference for the plugin's Editor Style. */
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
