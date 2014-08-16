// Copyright 2014 afuzzyllama. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class DataAccess : ModuleRules
	{
		public DataAccess(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"DataAccess/Private"
				}
            );

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
                    "CoreUObject"
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