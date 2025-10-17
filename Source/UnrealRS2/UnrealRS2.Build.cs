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
		
		var ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty/");
		var Rs2Path = Path.Combine(ModuleDirectory, "./RS2");
		PublicIncludePaths.Add(Rs2Path);
		PublicIncludePaths.Add(Path.Combine(Rs2Path, "datastruct"));
		PublicIncludePaths.Add(Path.Combine(Rs2Path, "entry"));
		PublicIncludePaths.Add(Path.Combine(Rs2Path, "platform"));
		PublicIncludePaths.Add(Path.Combine(Rs2Path, "sound"));
		PublicIncludePaths.Add(Path.Combine(Rs2Path, "thirdparty"));
		PublicIncludePaths.Add(Path.Combine(Rs2Path, "wordenc"));
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "SDL2-2.30.9/include"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "SDL2-2.30.9/lib/x64/SDL2.lib"));
		PublicDelayLoadDLLs.Add("SDL2.dll"); // load at runtime
		RuntimeDependencies.Add("$(BinaryOutputDir)/SDL2.dll"); // make sure it’s packaged

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
