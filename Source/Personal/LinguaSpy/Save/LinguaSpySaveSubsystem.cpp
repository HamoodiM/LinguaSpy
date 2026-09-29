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
		Save->Version = ULinguaSpySaveGame::CurrentVersion;
	}
	else
	{
		const int32 LoadedVersion = Save->Version;
		if (ULinguaSpySaveGame::Migrate(Save))
		{
			UE_LOG(LogPersonal, Log, TEXT("LinguaSpy profile migrated v%d -> v%d"), LoadedVersion, Save->Version);
			RequestSave();
		}
	}
	UE_LOG(LogPersonal, Log, TEXT("LinguaSpy profile loaded (%d cities, %d skills, %d SRS items)"), Save->Cities.Num(), Save->SkillStats.Num(), Save->SrsItems.Num());
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
	Save->Version = ULinguaSpySaveGame::CurrentVersion;
	Save->Settings = Settings;
	RequestSave();
}

void ULinguaSpySaveSubsystem::SetAlwaysShowTranslation(bool bAlways)
{
	Save->Settings.bAlwaysShowTranslation = bAlways;
	OnSettingsChanged.Broadcast();
	RequestSave();
}

void ULinguaSpySaveSubsystem::SetShowReadings(bool bShow)
{
	Save->Settings.bShowReadings = bShow;
	OnSettingsChanged.Broadcast();
	RequestSave();
}

void ULinguaSpySaveSubsystem::SetVolumes(float Master, float Music, float Ambience, float UI)
{
	FLinguaSpySettings& Settings = Save->Settings;
	Settings.MasterVolume = FMath::Clamp(Master, 0.f, 1.f);
	Settings.MusicVolume = FMath::Clamp(Music, 0.f, 1.f);
	Settings.AmbienceVolume = FMath::Clamp(Ambience, 0.f, 1.f);
	Settings.UIVolume = FMath::Clamp(UI, 0.f, 1.f);
	OnSettingsChanged.Broadcast();
}
