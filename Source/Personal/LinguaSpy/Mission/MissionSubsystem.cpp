// LinguaSpy - tracks the active city, mission, objective and story flags

#include "MissionSubsystem.h"
#include "LanguagePackAsset.h"
#include "LinguaSpySaveSubsystem.h"
#include "Engine/AssetManager.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Personal.h"

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = Collection.InitializeDependency<ULinguaSpySaveSubsystem>();
}

// ---- City selection ---------------------------------------------------------------------------------------------

TArray<ULanguagePackAsset*> UMissionSubsystem::GetAllLanguagePacks() const
{
	TArray<FSoftObjectPath> Paths;
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		AssetManager->GetPrimaryAssetPathList(ULanguagePackAsset::PrimaryAssetType, Paths);

		// Fallback when the primary asset type is not configured: ask the asset registry directly
		if (Paths.IsEmpty())
		{
			TArray<FAssetData> Assets;
			AssetManager->GetAssetRegistry().GetAssetsByClass(ULanguagePackAsset::StaticClass()->GetClassPathName(), Assets, true);
			for (const FAssetData& Asset : Assets)
			{
				Paths.Add(Asset.GetSoftObjectPath());
			}
		}
	}

	TArray<ULanguagePackAsset*> Packs;
	for (const FSoftObjectPath& Path : Paths)
	{
		if (ULanguagePackAsset* Pack = Cast<ULanguagePackAsset>(Path.TryLoad()))
		{
			Packs.AddUnique(Pack);
		}
	}
	Packs.Sort([](const ULanguagePackAsset& A, const ULanguagePackAsset& B) { return A.SortOrder < B.SortOrder; });
	return Packs;
}

void UMissionSubsystem::SelectLanguagePack(ULanguagePackAsset* Pack)
{
	SelectedPack = Pack;
}

void UMissionSubsystem::EnterCityForWorld(UWorld* World)
{
	const FString MapName = UGameplayStatics::GetCurrentLevelName(World, true);

	ULanguagePackAsset* Pack = nullptr;
	if (SelectedPack && SelectedPack->CityMap.ToSoftObjectPath().GetAssetName() == MapName)
	{
		Pack = SelectedPack;
	}
	else
	{
		// Launched straight into a city map (e.g. PIE): find the pack that owns it
		for (ULanguagePackAsset* Candidate : GetAllLanguagePacks())
		{
			if (Candidate->CityMap.ToSoftObjectPath().GetAssetName() == MapName)
			{
				Pack = Candidate;
				break;
			}
		}
	}

	CurrentPack = Pack;
	ActiveMission = nullptr;
	if (!CurrentPack)
	{
		UE_LOG(LogPersonal, Warning, TEXT("No language pack uses map '%s'"), *MapName);
		OnObjectiveChanged.Broadcast();
		return;
	}

	// Resume the saved mission, or start the next one
	FCityProgress& Progress = *GetProgress();
	if (!Progress.ActiveMission.IsNone())
	{
		for (const TSoftObjectPtr<UMissionAsset>& MissionPtr : CurrentPack->Missions)
		{
			UMissionAsset* Mission = MissionPtr.LoadSynchronous();
			if (Mission && Mission->MissionId == Progress.ActiveMission)
			{
				ActiveMission = Mission;
				break;
			}
		}
	}

	if (ActiveMission)
	{
		if (!ActiveMission->FindObjective(Progress.CurrentObjective))
		{
			Progress.CurrentObjective = ActiveMission->StartObjective;
		}
		OnMissionStarted.Broadcast(ActiveMission);
		OnObjectiveChanged.Broadcast();
	}
	else
	{
		StartNextMission();
	}
}

void UMissionSubsystem::LeaveCity()
{
	SaveProgress();
	CurrentPack = nullptr;
	ActiveMission = nullptr;
}

FName UMissionSubsystem::GetCurrentCityId() const
{
	return CurrentPack ? CurrentPack->CityId : NAME_None;
}

void UMissionSubsystem::GetCityCompletion(const ULanguagePackAsset* Pack, int32& OutCompleted, int32& OutTotal) const
{
	OutCompleted = 0;
	OutTotal = Pack ? Pack->Missions.Num() : 0;
	const ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	if (Pack && Save)
	{
		if (const FCityProgress* Progress = Save->Cities.Find(Pack->CityId))
		{
			OutCompleted = FMath::Min(Progress->CompletedMissions.Num(), OutTotal);
		}
	}
}

// ---- Mission state ----------------------------------------------------------------------------------------------

FCityProgress* UMissionSubsystem::GetProgress() const
{
	ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	if (!Save || !CurrentPack)
	{
		return nullptr;
	}
	return &Save->Cities.FindOrAdd(CurrentPack->CityId);
}

const FMissionObjective* UMissionSubsystem::GetCurrentObjective() const
{
	const FCityProgress* Progress = GetProgress();
	return (ActiveMission && Progress) ? ActiveMission->FindObjective(Progress->CurrentObjective) : nullptr;
}

FName UMissionSubsystem::GetCurrentTargetId() const
{
	const FMissionObjective* Objective = GetCurrentObjective();
	return Objective ? Objective->TargetId : NAME_None;
}

FText UMissionSubsystem::GetCurrentObjectiveText() const
{
	if (const FMissionObjective* Objective = GetCurrentObjective())
	{
		return Objective->Description;
	}
	if (CurrentPack && !ActiveMission)
	{
		return NSLOCTEXT("LinguaSpy", "CityDone", "All missions here are done. Press P to return to the menu.");
	}
	return FText::GetEmpty();
}

bool UMissionSubsystem::HasFlag(FName Flag) const
{
	const FCityProgress* Progress = GetProgress();
	return Progress && Progress->MissionFlags.Contains(Flag);
}

void UMissionSubsystem::SetFlag(FName Flag)
{
	if (FCityProgress* Progress = GetProgress())
	{
		if (!Flag.IsNone())
		{
			Progress->MissionFlags.Add(Flag);
		}
	}
}

bool UMissionSubsystem::HasAllFlags(const TArray<FName>& Flags) const
{
	return !Flags.ContainsByPredicate([this](FName Flag) { return !HasFlag(Flag); });
}

bool UMissionSubsystem::HasAnyFlag(const TArray<FName>& Flags) const
{
	return Flags.ContainsByPredicate([this](FName Flag) { return HasFlag(Flag); });
}

void UMissionSubsystem::StartNextMission()
{
	FCityProgress* Progress = GetProgress();
	if (!Progress)
	{
		return;
	}

	for (const TSoftObjectPtr<UMissionAsset>& MissionPtr : CurrentPack->Missions)
	{
		UMissionAsset* Mission = MissionPtr.LoadSynchronous();
		if (Mission && !Progress->CompletedMissions.Contains(Mission->MissionId))
		{
			StartMission(Mission);
			return;
		}
	}

	// Everything in this city is done
	ActiveMission = nullptr;
	Progress->ActiveMission = NAME_None;
	Progress->CurrentObjective = NAME_None;
	SaveProgress();
	OnCityCompleted.Broadcast();
	OnObjectiveChanged.Broadcast();
}

void UMissionSubsystem::StartMission(UMissionAsset* Mission)
{
	FCityProgress* Progress = GetProgress();
	if (!Progress || !Mission)
	{
		return;
	}
	ActiveMission = Mission;
	Progress->ActiveMission = Mission->MissionId;
	Progress->CurrentObjective = Mission->StartObjective;
	Progress->MissionFlags.Reset();
	SaveProgress();
	OnMissionStarted.Broadcast(Mission);
	OnObjectiveChanged.Broadcast();
}

void UMissionSubsystem::CompleteObjective(FName Outcome)
{
	const FMissionObjective* Objective = GetCurrentObjective();
	FCityProgress* Progress = GetProgress();
	if (!Objective || !Progress)
	{
		return;
	}

	for (FName Flag : Objective->SetFlags)
	{
		Progress->MissionFlags.Add(Flag);
	}

	const FName Next = UMissionAsset::ResolveTransition(*Objective, Outcome, Progress->MissionFlags);
	UE_LOG(LogPersonal, Log, TEXT("Objective '%s' finished with '%s' -> '%s'"), *Objective->ObjectiveId.ToString(), *Outcome.ToString(), *Next.ToString());

	if (Next.IsNone())
	{
		SaveProgress(); // flags may have changed; objective stays active
		return;
	}

	if (Next == UMissionAsset::MissionComplete)
	{
		UMissionAsset* Completed = ActiveMission;
		Progress->CompletedMissions.AddUnique(Completed->MissionId);
		OnMissionCompleted.Broadcast(Completed);
		StartNextMission();
	}
	else if (Next == UMissionAsset::MissionFailed)
	{
		UMissionAsset* Failed = ActiveMission;
		OnMissionFailed.Broadcast(Failed);
		StartMission(Failed); // restart from the top
	}
	else
	{
		Progress->CurrentObjective = Next;
		SaveProgress();
		OnObjectiveChanged.Broadcast();
	}
}

// ---- Reports from the world -------------------------------------------------------------------------------------

void UMissionSubsystem::ReportConversationFinished(FName NpcTargetId, FName Outcome)
{
	const FMissionObjective* Objective = GetCurrentObjective();
	if (Objective && Objective->Type == EObjectiveType::TalkTo && Objective->TargetId == NpcTargetId)
	{
		CompleteObjective(Outcome);
	}
	else
	{
		SaveProgress(); // keep skill stats / flags from side conversations
	}
}

void UMissionSubsystem::ReportMarkerReached(FName MarkerTargetId)
{
	const FMissionObjective* Objective = GetCurrentObjective();
	if (Objective && Objective->Type == EObjectiveType::ReachLocation && Objective->TargetId == MarkerTargetId)
	{
		CompleteObjective(UMissionAsset::OutcomeReached);
	}
}

void UMissionSubsystem::ReportCollected(FName PickupTargetId)
{
	const FMissionObjective* Objective = GetCurrentObjective();
	if (Objective && Objective->Type == EObjectiveType::Collect && Objective->TargetId == PickupTargetId)
	{
		CompleteObjective(UMissionAsset::OutcomeCollected);
	}
}

void UMissionSubsystem::SaveProgress()
{
	if (SaveSubsystem)
	{
		SaveSubsystem->RequestSave();
	}
}
