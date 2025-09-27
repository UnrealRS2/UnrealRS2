// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class UnrealRS2 : ModuleRules
{
	public UnrealRS2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		var ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty/client_377");

		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "Include"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Lib/client_377.lib"));
		PublicDelayLoadDLLs.Add("client_377.dll"); // load at runtime
		RuntimeDependencies.Add("$(BinaryOutputDir)/client_377.dll"); // make sure it’s packaged

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
