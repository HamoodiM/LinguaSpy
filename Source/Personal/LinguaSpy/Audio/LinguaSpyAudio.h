// LinguaSpy - UI sound settings and helpers

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LinguaSpyAudio.generated.h"

class USoundBase;
class UAudioComponent;
class ULinguaSpySaveSubsystem;

UENUM(BlueprintType)
enum class ESpyUISound : uint8
{
	Click,
	TilePlace,
	Pass,
	Fail,
	CoverBlown,
	MissionComplete,
	MenuSelect
};

/** Mixer channel of a looping sound (each has its own volume slider). */
UENUM(BlueprintType)
enum class ESpyAudioChannel : uint8
{
	Music,
	Ambience,
	/** Spoken lines (one-shots via PlayVoice; music ducks under them). */
	Voice
};

/** Project Settings -> Game -> LinguaSpy Audio. Sounds are loaded on first use. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LinguaSpy Audio"))
class PERSONAL_API ULinguaSpyAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> Click;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> TilePlace;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> Pass;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> Fail;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> CoverBlown;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> MissionComplete;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftObjectPtr<USoundBase> MenuSelect;

	/** Played on the main menu. */
	UPROPERTY(Config, EditAnywhere, Category="Music")
	TSoftObjectPtr<USoundBase> MenuMusic;

	/** Base mix levels; the player's volume sliders (save settings) multiply these. */
	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float UIVolume = 0.8f;

	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float AmbientVolume = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float MusicVolume = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float VoiceVolume = 1.f;

	/** Music volume multiplier while a voice line plays. */
	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="1"))
	float MusicDuckFactor = 0.5f;

	/** Fade time for looping music/ambience when a level starts. */
	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0"))
	float LoopFadeInSeconds = 1.5f;
};

/**
 *  Plays UI one-shots and level loops at (project mix level x player slider x master).
 *  Keeps track of the live loops so slider changes apply immediately.
 */
UCLASS()
class PERSONAL_API ULinguaSpyAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void PlayUI(const UObject* WorldContext, ESpyUISound Sound) const;

	/** Spawns a 2D loop that stops when its level unloads. The sound asset itself must be set to loop. */
	UAudioComponent* PlayLoop(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, ESpyAudioChannel Channel);

	/** Plays a spoken line, replacing any line still playing, and ducks the music under it. Returns false if nothing plays. */
	bool PlayVoice(const UObject* WorldContext, USoundBase* Sound, float Pitch = 1.f);

	/** Stops the current spoken line (e.g. the conversation closed). */
	void StopVoice();

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Audio")
	bool IsVoicePlaying() const;

	/** Effective volume for a channel right now. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Audio")
	float GetChannelVolume(ESpyAudioChannel Channel) const;

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Audio")
	float GetUIVolume() const;

	/** Re-applies the save's volume settings to every live loop. */
	UFUNCTION()
	void ApplyVolumes();

private:

	struct FLiveLoop
	{
		TWeakObjectPtr<UAudioComponent> Component;
		ESpyAudioChannel Channel;
	};
	TArray<FLiveLoop> Loops;

	UFUNCTION()
	void HandleVoiceFinished();

	TWeakObjectPtr<UAudioComponent> VoiceComponent;
	bool bDucked = false;

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveSubsystem> SaveSubsystem;
};

namespace LinguaSpyAudio
{
	/** Plays a 2D UI sound if one is configured. Safe to call from widgets and actors. */
	PERSONAL_API void PlayUI(const UObject* WorldContext, ESpyUISound Sound);

	/** Spawns a looping 2D sound (ambience/music) that stops when the level unloads. */
	PERSONAL_API void PlayLoop(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, ESpyAudioChannel Channel);

	/** Loads and plays a spoken line on the Voice channel. Returns false if the line has no audio. */
	PERSONAL_API bool PlayVoice(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Pitch = 1.f);

	PERSONAL_API void StopVoice(const UObject* WorldContext);
}
