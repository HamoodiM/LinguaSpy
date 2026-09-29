// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersonalGameMode.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "LanguagePackAsset.h"
#include "LinguaSpyAudio.h"

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

		if (const ULanguagePackAsset* Pack = Missions->GetCurrentPack())
		{
			const ULinguaSpyAudioSettings* Audio = GetDefault<ULinguaSpyAudioSettings>();
			LinguaSpyAudio::PlayLoop(this, Pack->AmbientSound, Audio->AmbientVolume);
			LinguaSpyAudio::PlayLoop(this, Pack->MusicSound, Audio->MusicVolume);
		}
	}
	Super::StartPlay();
}
