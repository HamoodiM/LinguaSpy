// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersonalGameMode.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"

APersonalGameMode::APersonalGameMode()
{
	// stub
}
void APersonalGameMode::StartPlay()
{
	// Set up the mission first so NPCs, markers and the HUD see the current objective in BeginPlay
	if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
	{
		Missions->EnterCityForWorld(GetWorld());
	}
	Super::StartPlay();
}
