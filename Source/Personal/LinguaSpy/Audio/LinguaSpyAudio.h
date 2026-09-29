// LinguaSpy - UI sound settings and helpers

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LinguaSpyAudio.generated.h"

class USoundBase;

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

	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float UIVolume = 0.8f;

	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float AmbientVolume = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category="Mix", meta=(ClampMin="0", ClampMax="2"))
	float MusicVolume = 0.35f;
};

namespace LinguaSpyAudio
{
	/** Plays a 2D UI sound if one is configured. Safe to call from widgets and actors. */
	PERSONAL_API void PlayUI(const UObject* WorldContext, ESpyUISound Sound);

	/** Spawns a looping 2D sound (ambience/music) that stops when the level unloads. */
	PERSONAL_API void PlayLoop(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& Sound, float Volume);
}
