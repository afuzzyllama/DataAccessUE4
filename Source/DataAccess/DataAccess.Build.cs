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
				var SqlitePath = Path.Combine("..", "Plugins", "DataAccess", "Source", "DataAccess", "ThirdParty", "Sqlite", "3.8.6");
                if (Directory.Exists(SqlitePath))
				{
					Definitions.Add("WITH_SQLITE=1");

                    var IncludePath = Path.GetFullPath(SqlitePath);
					PrivateIncludePaths.Add(IncludePath);
				}
			}
		}
	}
}