// LinguaSpy - tracks the active city, mission, objective and story flags

#include "MissionSubsystem.h"
#include "LanguagePackAsset.h"
#include "LinguaSpySaveSubsystem.h"
#include "FluencyScoring.h"
#include "ConversationSubsystem.h"
#include "LearningTelemetrySubsystem.h"
#include "SrsScheduler.h"
#include "Engine/World.h"
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
	MissionTally = FMissionGrade();
	MissionItems.Reset();
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
	MissionTally = FMissionGrade();
	MissionItems.Reset();
	Progress->ActiveMission = Mission->MissionId;
	Progress->CurrentObjective = Mission->StartObjective;
	Progress->MissionFlags.Reset();
	SaveProgress();
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("mission_start"))
		.Id(TEXT("mission"), Mission->MissionId).Id(TEXT("city"), CurrentPack ? CurrentPack->CityId : NAME_None));
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

		// Fluency grade for this run; keep the best one in the profile
		LastGrade = MissionTally;
		LastGrade.Grade = UFluencyScoring::ComputeGrade(MissionTally.Challenges, MissionTally.Mistakes, MissionTally.Peeks);
		if (ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr)
		{
			FMissionGrade* Best = Save->BestGrades.Find(Completed->MissionId);
			if (!Best || LastGrade.Grade < Best->Grade)
			{
				Save->BestGrades.Add(Completed->MissionId, LastGrade);
			}
		}
		UE_LOG(LogPersonal, Log, TEXT("Mission '%s' graded %s (%d challenges, %d mistakes, %d peeks)"), *Completed->MissionId.ToString(),
			*UFluencyScoring::GradeLetter(LastGrade.Grade).ToString(), LastGrade.Challenges, LastGrade.Mistakes, LastGrade.Peeks);

		// Fade (or restore) the handler's support based on how independently the player spoke
		HandlerBefore = HandlerAfter = SaveSubsystem ? SaveSubsystem->GetHandlerMode(CurrentPack->CityId) : EHandlerMode::Shadowing;
		if (SaveSubsystem)
		{
			HandlerAfter = AdaptHandlerMode(HandlerBefore, LastGrade.Grade);
			if (HandlerAfter != HandlerBefore)
			{
				SaveSubsystem->SetHandlerMode(CurrentPack->CityId, HandlerAfter);
			}
		}
		ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("mission_end"))
			.Id(TEXT("mission"), Completed->MissionId).Str(TEXT("result"), TEXT("complete")).Int(TEXT("grade"), LastGrade.Grade)
			.Int(TEXT("challenges"), LastGrade.Challenges).Int(TEXT("mistakes"), LastGrade.Mistakes).Int(TEXT("peeks"), LastGrade.Peeks)
			.Int(TEXT("handler_before"), static_cast<int32>(HandlerBefore)).Int(TEXT("handler_after"), static_cast<int32>(HandlerAfter)));

		const TArray<FString> Debrief = SelectDebriefItems(MissionItems, Completed->DebriefItems);
		OnMissionCompleted.Broadcast(Completed);

		// Debrief: retrieval of this mission's phrases right away (the first spaced exposure), before the next briefing
		if (!Debrief.IsEmpty())
		{
			UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
			if (UConversationSubsystem* Conversations = World ? World->GetSubsystem<UConversationSubsystem>() : nullptr)
			{
				TArray<FSrsItemKey> Keys;
				for (const FString& KeyString : Debrief)
				{
					FSrsItemKey Key;
					if (FSrsItemKey::Parse(KeyString, Key))
					{
						Keys.Add(Key);
					}
				}
				Conversations->StartDebrief(Keys);
			}
		}
		StartNextMission();
	}
	else if (Next == UMissionAsset::MissionFailed)
	{
		UMissionAsset* Failed = ActiveMission;
		ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("mission_end"))
			.Id(TEXT("mission"), Failed->MissionId).Str(TEXT("result"), TEXT("failed")));
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

// ---- Fluency ----------------------------------------------------------------------------------------------------

void UMissionSubsystem::AddFluencyTally(int32 Challenges, int32 Mistakes, int32 Peeks)
{
	if (ActiveMission)
	{
		MissionTally.Challenges += Challenges;
		MissionTally.Mistakes += Mistakes;
		MissionTally.Peeks += Peeks;
	}
}

bool UMissionSubsystem::GetBestGrade(FName MissionId, FMissionGrade& OutGrade) const
{
	const ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	const FMissionGrade* Best = Save ? Save->BestGrades.Find(MissionId) : nullptr;
	if (Best)
	{
		OutGrade = *Best;
	}
	return Best != nullptr;
}

// ---- Learning loop ----------------------------------------------------------------------------------------------

void UMissionSubsystem::RecordMissionItem(const FSrsItemKey& Key, bool bPassed, bool bAssisted)
{
	if (ActiveMission && Key.IsValid())
	{
		MissionItems.Add({ Key.ToString(), bPassed, bAssisted });
	}
}

TArray<FString> UMissionSubsystem::SelectDebriefItems(const TArray<FMissionItemRecord>& Records, int32 Max)
{
	// Worst result per item: 0 = failed, 1 = assisted, 2 = clean pass
	TArray<FString> Order;
	TMap<FString, int32> Worst;
	for (const FMissionItemRecord& Record : Records)
	{
		const int32 Score = !Record.bPassed ? 0 : Record.bAssisted ? 1 : 2;
		if (int32* Existing = Worst.Find(Record.Key))
		{
			*Existing = FMath::Min(*Existing, Score);
		}
		else
		{
			Worst.Add(Record.Key, Score);
			Order.Add(Record.Key);
		}
	}
	TArray<FString> Out;
	for (int32 Score = 0; Score <= 2; ++Score)
	{
		for (const FString& Key : Order)
		{
			if (Out.Num() < Max && Worst[Key] == Score)
			{
				Out.Add(Key);
			}
		}
	}
	return Out;
}

EHandlerMode UMissionSubsystem::AdaptHandlerMode(EHandlerMode Current, int32 Grade)
{
	const int32 Level = static_cast<int32>(Current);
	if (Grade <= 1)
	{
		return static_cast<EHandlerMode>(FMath::Min(Level + 1, static_cast<int32>(EHandlerMode::RadioSilence)));
	}
	if (Grade >= 3)
	{
		return static_cast<EHandlerMode>(FMath::Max(Level - 1, static_cast<int32>(EHandlerMode::Shadowing)));
	}
	return Current;
}

bool UMissionSubsystem::NeedsBriefing(const UMissionAsset* Mission) const
{
	const ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	return Mission && !Mission->BriefingChunks.IsEmpty() && (!Save || !Save->BriefingsCompleted.Contains(Mission->MissionId));
}

void UMissionSubsystem::MarkBriefingCompleted(const UMissionAsset* Mission, int32 Correct, int32 Total)
{
	if (!Mission)
	{
		return;
	}
	if (ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr)
	{
		Save->BriefingsCompleted.AddUnique(Mission->MissionId);
		SaveProgress();
	}
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("briefing_end"))
		.Id(TEXT("mission"), Mission->MissionId).Int(TEXT("correct"), Correct).Int(TEXT("total"), Total));
}

TArray<FText> UMissionSubsystem::GetEarnedCanDos(const ULanguagePackAsset* Pack) const
{
	TArray<FText> Out;
	const ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	const FCityProgress* Progress = (Save && Pack) ? Save->Cities.Find(Pack->CityId) : nullptr;
	if (!Progress)
	{
		return Out;
	}
	for (const TSoftObjectPtr<UMissionAsset>& MissionPtr : Pack->Missions)
	{
		const UMissionAsset* Mission = MissionPtr.LoadSynchronous();
		if (Mission && Progress->CompletedMissions.Contains(Mission->MissionId))
		{
			for (const FText& CanDo : Mission->CanDoStatements)
			{
				if (!Out.ContainsByPredicate([&CanDo](const FText& Existing) { return Existing.EqualTo(CanDo); }))
				{
					Out.Add(CanDo);
				}
			}
		}
	}
	return Out;
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
