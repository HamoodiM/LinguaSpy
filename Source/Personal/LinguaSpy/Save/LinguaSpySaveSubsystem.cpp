// LinguaSpy - loads/saves the player profile

#include "LinguaSpySaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Personal.h"

const FString ULinguaSpySaveSubsystem::SlotName(TEXT("LinguaSpyProfile"));

void ULinguaSpySaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		Save = Cast<ULinguaSpySaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}
	if (!Save)
	{
		Save = Cast<ULinguaSpySaveGame>(UGameplayStatics::CreateSaveGameObject(ULinguaSpySaveGame::StaticClass()));
	}
	UE_LOG(LogPersonal, Log, TEXT("LinguaSpy profile loaded (%d cities, %d skills)"), Save->Cities.Num(), Save->SkillStats.Num());
}

void ULinguaSpySaveSubsystem::RequestSave()
{
	if (Save)
	{
		Save->Version = ULinguaSpySaveGame::CurrentVersion;
		UGameplayStatics::AsyncSaveGameToSlot(Save, SlotName, 0);
	}
}

void ULinguaSpySaveSubsystem::ResetProgress()
{
	const FLinguaSpySettings Settings = Save->Settings;
	Save = Cast<ULinguaSpySaveGame>(UGameplayStatics::CreateSaveGameObject(ULinguaSpySaveGame::StaticClass()));
	Save->Settings = Settings;
	RequestSave();
}

void ULinguaSpySaveSubsystem::SetHelperEnabled(bool bEnabled)
{
	Save->Settings.bHelperEnabled = bEnabled;
	OnSettingsChanged.Broadcast();
	RequestSave();
}

void ULinguaSpySaveSubsystem::SetShowReadings(bool bShow)
{
	Save->Settings.bShowReadings = bShow;
	OnSettingsChanged.Broadcast();
	RequestSave();
}
