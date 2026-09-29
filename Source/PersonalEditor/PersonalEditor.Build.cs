using UnrealBuildTool;

public class PersonalEditor : ModuleRules
{
	public PersonalEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Personal",
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"ToolMenus",
			"PropertyEditor",
			"AssetRegistry",
			"AssetTools",
			"WorkspaceMenuStructure"
		});
	}
}
