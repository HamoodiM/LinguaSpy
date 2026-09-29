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
class ULinguaSpySaveSubsystem : public UGameInstanceSubsystem
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
	void SetHelperEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Save")
	void SetShowReadings(bool bShow);

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Save")
	FOnLinguaSpySettingsChanged OnSettingsChanged;

	static const FString SlotName;

private:

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveGame> Save;
};
