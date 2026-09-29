// LinguaSpy - UI sound settings and helpers

#include "LinguaSpyAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Personal.h"

namespace LinguaSpyAudio
{
	void PlayUI(const UObject* WorldContext, ESpyUISound Sound)
	{
		const ULinguaSpyAudioSettings* Settings = GetDefault<ULinguaSpyAudioSettings>();
		const TSoftObjectPtr<USoundBase>* Entry = nullptr;
		switch (Sound)
		{
		case ESpyUISound::Click:           Entry = &Settings->Click; break;
		case ESpyUISound::TilePlace:       Entry = &Settings->TilePlace; break;
		case ESpyUISound::Pass:            Entry = &Settings->Pass; break;
		case ESpyUISound::Fail:            Entry = &Settings->Fail; break;
		case ESpyUISound::CoverBlown:      Entry = &Settings->CoverBlown; break;
		case ESpyUISound::MissionComplete: Entry = &Settings->MissionComplete; break;
		case ESpyUISound::MenuSelect:      Entry = &Settings->MenuSelect; break;
		}
		if (!WorldContext || !Entry || Entry->IsNull())
		{
			return;
		}
		if (USoundBase* Loaded = Entry->LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(WorldContext, Loaded, Settings->UIVolume);
		}
	}

	void PlayLoop(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Volume)
	{
		if (!WorldContext || Sound.IsNull())
		{
			return;
		}
		if (USoundBase* Loaded = Sound.LoadSynchronous())
		{
			UGameplayStatics::SpawnSound2D(WorldContext, Loaded, Volume);
			UE_LOG(LogPersonal, Log, TEXT("Playing loop %s"), *Loaded->GetName());
		}
	}
}
