// LinguaSpy - branching spy missions

#include "MissionAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "LinguaSpyMission"

const FName UMissionAsset::MissionComplete(TEXT("MissionComplete"));
const FName UMissionAsset::MissionFailed(TEXT("MissionFailed"));
const FName UMissionAsset::OutcomeReached(TEXT("Reached"));
const FName UMissionAsset::OutcomeCollected(TEXT("Collected"));

const FMissionObjective* UMissionAsset::FindObjective(FName ObjectiveId) const
{
	if (ObjectiveId.IsNone())
	{
		return nullptr;
	}
	return Objectives.FindByPredicate([ObjectiveId](const FMissionObjective& O) { return O.ObjectiveId == ObjectiveId; });
}

FName UMissionAsset::ResolveTransition(const FMissionObjective& Objective, FName Outcome, const TSet<FName>& Flags)
{
	for (const FObjectiveTransition& Transition : Objective.Transitions)
	{
		if (!Transition.RequiredOutcome.IsNone() && Transition.RequiredOutcome != Outcome)
		{
			continue;
		}
		const bool bHasRequired = !Transition.RequiredFlags.ContainsByPredicate([&Flags](FName Flag) { return !Flags.Contains(Flag); });
		const bool bHasBlocked = Transition.BlockedFlags.ContainsByPredicate([&Flags](FName Flag) { return Flags.Contains(Flag); });
		if (bHasRequired && !bHasBlocked)
		{
			return Transition.NextObjective;
		}
	}
	return NAME_None;
}

TArray<FText> UMissionAsset::Validate() const
{
	TArray<FText> Out;
	if (MissionId.IsNone())
	{
		Out.Add(LOCTEXT("NoId", "MissionId is empty"));
	}
	if (!FindObjective(StartObjective))
	{
		Out.Add(FText::Format(LOCTEXT("BadStart", "StartObjective '{0}' does not exist"), FText::FromName(StartObjective)));
	}

	TSet<FName> Ids;
	for (const FMissionObjective& Objective : Objectives)
	{
		if (Ids.Contains(Objective.ObjectiveId))
		{
			Out.Add(FText::Format(LOCTEXT("DupId", "Duplicate objective id '{0}'"), FText::FromName(Objective.ObjectiveId)));
		}
		Ids.Add(Objective.ObjectiveId);

		if (Objective.TargetId.IsNone())
		{
			Out.Add(FText::Format(LOCTEXT("NoTarget", "Objective '{0}' has no TargetId"), FText::FromName(Objective.ObjectiveId)));
		}
		if (Objective.Transitions.IsEmpty())
		{
			Out.Add(FText::Format(LOCTEXT("NoTransitions", "Objective '{0}' has no transitions (it can never finish)"), FText::FromName(Objective.ObjectiveId)));
		}
		for (const FObjectiveTransition& Transition : Objective.Transitions)
		{
			const FName Next = Transition.NextObjective;
			if (Next != MissionComplete && Next != MissionFailed && !FindObjective(Next))
			{
				Out.Add(FText::Format(LOCTEXT("BadNext", "Objective '{0}': NextObjective '{1}' does not exist"),
					FText::FromName(Objective.ObjectiveId), FText::FromName(Next)));
			}
		}
	}
	return Out;
}

#if WITH_EDITOR
EDataValidationResult UMissionAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	for (const FText& Problem : Validate())
	{
		Context.AddWarning(Problem);
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
