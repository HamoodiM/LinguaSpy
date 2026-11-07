// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersonalGameMode.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "LanguagePackAsset.h"
#include "LinguaSpyAudio.h"
#include "CityLook.h"

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
			LinguaSpyAudio::PlayLoop(this, Pack->AmbientSound, ESpyAudioChannel::Ambience);
			LinguaSpyAudio::PlayLoop(this, Pack->MusicSound, ESpyAudioChannel::Music);

			// The city's time of day and grading (data-driven; the map's own lights are the fallback)
			if (const UCityLookAsset* Look = Pack->Look.LoadSynchronous())
			{
				if (UCityLookSubsystem* Looks = GetWorld()->GetSubsystem<UCityLookSubsystem>())
				{
					Looks->ApplyLook(Look->Look);
				}
			}
		}
	}
	Super::StartPlay();
}
