// Copyright 2014 afuzzyllama. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class DataAccess : ModuleRules
	{
		public DataAccess(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"DataAccess/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
                    "CoreUObject"
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
				
			if (!UnrealBuildTool.BuildingRocket())
			{
				var SqlitePath = Path.Combine(UnrealBuildTool.GetUProjectPath(), "Plugins", "DataAccess", "Source", "DataAccess", "ThirdParty", "Sqlite", "3.8.5");
				if (Directory.Exists(SqlitePath))
				{
					Definitions.Add("WITH_SQLITE=1");
					PrivateIncludePaths.Add(Path.Combine("DataAccess", "ThirdParty", "Sqlite", "3.8.5"));
				}
			}
		}
	}
}