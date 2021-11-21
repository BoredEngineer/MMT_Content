// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MMT_ContentTarget : TargetRules
{
	public MMT_ContentTarget(TargetInfo Target) : base (Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.V2;
		Type = TargetType.Game;
		ExtraModuleNames.Add("MMT_Content");
	}
}
