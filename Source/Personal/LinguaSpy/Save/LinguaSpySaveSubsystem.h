// LinguaSpy - loads/saves the player profile

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LinguaSpySaveGame.h"
#include "LinguaSpySaveSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLinguaSpySettingsChanged);

/**
 *  Owns the single save profile. Other subsystems read/write it via GetSave() and call RequestSave().
 */
UCLASS()
class PERSONAL_API ULinguaSpySaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Always valid after Initialize. */
	ULinguaSpySaveGame* GetSave() const { return Save; }

	/** Writes the profile to disk. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void RequestSave();

	/** Wipes all progress (keeps settings). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void ResetProgress();

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Save")
	const FLinguaSpySettings& GetSettings() const { return Save->Settings; }

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetAlwaysShowTranslation(bool bAlways);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetShowReadings(bool bShow);

	/** Channel volumes in 0..1. Broadcasts OnSettingsChanged; saved when the settings screen closes (SaveSettings). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetVolumes(float Master, float Music, float Ambience, float UI);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetVoiceVolume(float Voice);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetTelemetryEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Save")
	EHandlerMode GetHandlerMode(FName CityId) const { return Save->GetHandlerMode(CityId); }

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetHandlerMode(FName CityId, EHandlerMode Mode);

	/** Pilot participant code: letters, digits and '-', at most 8 characters (never a name). Returns false if rejected. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	bool SetParticipantCode(const FString& Code);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Save")
	FString GetParticipantCode() const { return Save->ParticipantCode; }

	static bool IsValidParticipantCode(const FString& Code);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SaveSettings() { RequestSave(); }

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Save")
	FOnLinguaSpySettingsChanged OnSettingsChanged;

	static const FString SlotName;

private:

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveGame> Save;
};
