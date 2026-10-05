// LinguaSpy - UI sound settings and helpers

#include "LinguaSpyAudio.h"
#include "LinguaSpySaveSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Personal.h"

void ULinguaSpyAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = Collection.InitializeDependency<ULinguaSpySaveSubsystem>();
	if (SaveSubsystem)
	{
		SaveSubsystem->OnSettingsChanged.AddUniqueDynamic(this, &ULinguaSpyAudioSubsystem::ApplyVolumes);
	}
}

void ULinguaSpyAudioSubsystem::Deinitialize()
{
	if (SaveSubsystem)
	{
		SaveSubsystem->OnSettingsChanged.RemoveAll(this);
	}
	Loops.Reset();
	VoiceComponent.Reset();
	Super::Deinitialize();
}

float ULinguaSpyAudioSubsystem::GetChannelVolume(ESpyAudioChannel Channel) const
{
	const ULinguaSpyAudioSettings* Mix = GetDefault<ULinguaSpyAudioSettings>();
	const FLinguaSpySettings* Player = SaveSubsystem && SaveSubsystem->GetSave() ? &SaveSubsystem->GetSettings() : nullptr;
	const float Master = Player ? Player->MasterVolume : 1.f;
	switch (Channel)
	{
	case ESpyAudioChannel::Music:    return Mix->MusicVolume * Master * (Player ? Player->MusicVolume : 1.f);
	case ESpyAudioChannel::Ambience: return Mix->AmbientVolume * Master * (Player ? Player->AmbienceVolume : 1.f);
	case ESpyAudioChannel::Voice:    return Mix->VoiceVolume * Master * (Player ? Player->VoiceVolume : 1.f);
	}
	return 0.f;
}

float ULinguaSpyAudioSubsystem::GetUIVolume() const
{
	const ULinguaSpyAudioSettings* Mix = GetDefault<ULinguaSpyAudioSettings>();
	const FLinguaSpySettings* Player = SaveSubsystem && SaveSubsystem->GetSave() ? &SaveSubsystem->GetSettings() : nullptr;
	return Mix->UIVolume * (Player ? Player->MasterVolume * Player->UIVolume : 1.f);
}

void ULinguaSpyAudioSubsystem::PlayUI(const UObject* WorldContext, ESpyUISound Sound) const
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
	const float Volume = GetUIVolume();
	if (!WorldContext || !Entry || Entry->IsNull() || Volume <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	if (USoundBase* Loaded = Entry->LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(WorldContext, Loaded, Volume);
	}
}

UAudioComponent* ULinguaSpyAudioSubsystem::PlayLoop(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, ESpyAudioChannel Channel)
{
	if (!WorldContext || Sound.IsNull())
	{
		return nullptr;
	}
	USoundBase* Loaded = Sound.LoadSynchronous();
	if (!Loaded)
	{
		return nullptr;
	}
	// Spawn at volume 1 and fade to the target; the multiplier is what ApplyVolumes adjusts later
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(WorldContext, Loaded, 1.f, 1.f, 0.f, nullptr, false, true);
	if (!Component)
	{
		return nullptr;
	}
	Component->FadeIn(GetDefault<ULinguaSpyAudioSettings>()->LoopFadeInSeconds, GetChannelVolume(Channel));

	Loops.RemoveAll([](const FLiveLoop& Loop) { return !Loop.Component.IsValid(); });
	Loops.Add({ Component, Channel });
	UE_LOG(LogPersonal, Log, TEXT("Playing loop %s at %.2f"), *Loaded->GetName(), GetChannelVolume(Channel));
	return Component;
}

void ULinguaSpyAudioSubsystem::ApplyVolumes()
{
	Loops.RemoveAll([](const FLiveLoop& Loop) { return !Loop.Component.IsValid(); });
	const float Duck = bDucked ? GetDefault<ULinguaSpyAudioSettings>()->MusicDuckFactor : 1.f;
	for (const FLiveLoop& Loop : Loops)
	{
		const float Volume = GetChannelVolume(Loop.Channel) * (Loop.Channel == ESpyAudioChannel::Music ? Duck : 1.f);
		Loop.Component->AdjustVolume(0.25f, Volume);
	}
	if (UAudioComponent* Voice = VoiceComponent.Get())
	{
		Voice->SetVolumeMultiplier(GetChannelVolume(ESpyAudioChannel::Voice));
	}
}

bool ULinguaSpyAudioSubsystem::PlayVoice(const UObject* WorldContext, USoundBase* Sound, float Pitch)
{
	StopVoice();
	const float Volume = GetChannelVolume(ESpyAudioChannel::Voice);
	if (!WorldContext || !Sound || Volume <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(WorldContext, Sound, Volume, Pitch, 0.f, nullptr, false, true);
	if (!Component)
	{
		return false;
	}
	Component->bIsUISound = true; // keeps playing while the game is paused (briefing, assessment)
	Component->OnAudioFinished.AddUniqueDynamic(this, &ULinguaSpyAudioSubsystem::HandleVoiceFinished);
	Component->Play();
	VoiceComponent = Component;
	bDucked = true;
	ApplyVolumes();
	return true;
}

void ULinguaSpyAudioSubsystem::StopVoice()
{
	if (UAudioComponent* Voice = VoiceComponent.Get())
	{
		Voice->OnAudioFinished.RemoveAll(this);
		Voice->Stop();
	}
	VoiceComponent.Reset();
	if (bDucked)
	{
		bDucked = false;
		ApplyVolumes();
	}
}

bool ULinguaSpyAudioSubsystem::IsVoicePlaying() const
{
	const UAudioComponent* Voice = VoiceComponent.Get();
	return Voice && Voice->IsPlaying();
}

void ULinguaSpyAudioSubsystem::HandleVoiceFinished()
{
	VoiceComponent.Reset();
	bDucked = false;
	ApplyVolumes();
}

namespace LinguaSpyAudio
{
	static ULinguaSpyAudioSubsystem* GetAudio(const UObject* WorldContext)
	{
		const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<ULinguaSpyAudioSubsystem>() : nullptr;
	}

	void PlayUI(const UObject* WorldContext, ESpyUISound Sound)
	{
		if (const ULinguaSpyAudioSubsystem* Audio = GetAudio(WorldContext))
		{
			Audio->PlayUI(WorldContext, Sound);
		}
	}

	void PlayLoop(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, ESpyAudioChannel Channel)
	{
		if (ULinguaSpyAudioSubsystem* Audio = GetAudio(WorldContext))
		{
			Audio->PlayLoop(WorldContext, Sound, Channel);
		}
	}

	bool PlayVoice(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Pitch)
	{
		ULinguaSpyAudioSubsystem* Audio = GetAudio(WorldContext);
		USoundBase* Loaded = Sound.IsNull() ? nullptr : Sound.LoadSynchronous();
		return Audio && Loaded && Audio->PlayVoice(WorldContext, Loaded, Pitch);
	}

	void StopVoice(const UObject* WorldContext)
	{
		if (ULinguaSpyAudioSubsystem* Audio = GetAudio(WorldContext))
		{
			Audio->StopVoice();
		}
	}
}
