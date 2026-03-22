// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class URRL : ModuleRules
{
	public URRL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "RenderCore", "RHI", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "ProceduralMeshComponent" });
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string JDKPath = "C:/Program Files/Java/jdk-17"; // point to your JDK
			PublicIncludePaths.Add(Path.Combine(JDKPath, "include"));
			PublicIncludePaths.Add(Path.Combine(JDKPath, "include", "win32"));
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
