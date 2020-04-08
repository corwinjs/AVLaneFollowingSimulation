using UnrealBuildTool;
using System;

public class VehicleVision : ModuleRules
{
	public VehicleVision(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"PhysXVehicles",
            "Sockets",
			"Networking",
			"ImageWriteQueue",
			"MediaIOCore",
            "RHI",
            "RenderCore",
            "Slate",
            "SlateCore",
		});
	}
}
