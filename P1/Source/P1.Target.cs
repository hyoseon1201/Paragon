// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class P1Target : TargetRules
{
	public P1Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("P1");

		// Push Model 리플리케이션 최적화용 — bWithPushModel은 TargetType.Editor일 때만 기본 true라
		// Game/Server 타겟에서는 명시적으로 켜줘야 MARK_PROPERTY_DIRTY_* 매크로가 실제로 컴파일된다.
		bWithPushModel = true;
	}
}
