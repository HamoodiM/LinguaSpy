// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Personal : ModuleRules
{
	public Personal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Personal",
			"Personal/Variant_Strategy",
			"Personal/Variant_Strategy/UI",
			"Personal/Variant_TwinStick",
			"Personal/Variant_TwinStick/AI",
			"Personal/Variant_TwinStick/Gameplay",
			"Personal/Variant_TwinStick/UI",
			"Personal/LinguaSpy/Data",
			"Personal/LinguaSpy/Language",
			"Personal/LinguaSpy/Dialogue",
			"Personal/LinguaSpy/Cover",
			"Personal/LinguaSpy/Interaction",
			"Personal/LinguaSpy/UI",
			"Personal/LinguaSpy/Mission",
			"Personal/LinguaSpy/Save"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
