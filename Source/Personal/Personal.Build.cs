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
			"SlateCore",
			"DeveloperSettings",
			"Json"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Personal",
			"Personal/LinguaSpy/Data",
			"Personal/LinguaSpy/Language",
			"Personal/LinguaSpy/Dialogue",
			"Personal/LinguaSpy/Cover",
			"Personal/LinguaSpy/Interaction",
			"Personal/LinguaSpy/UI",
			"Personal/LinguaSpy/Mission",
			"Personal/LinguaSpy/Save",
			"Personal/LinguaSpy/Camera",
			"Personal/LinguaSpy/Audio",
			"Personal/LinguaSpy/Telemetry",
			"Personal/LinguaSpy/Assessment",
			"Personal/LinguaSpy/Look"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
