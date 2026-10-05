// LinguaSpy - pre/post test forms for measuring learning (docs/Pilot_Study_Protocol.md)

#include "AssessmentAsset.h"
#include "ScaffoldPolicy.h"
#include "SentenceGrader.h"

#define LOCTEXT_NAMESPACE "LinguaSpyAssessment"

bool UAssessmentAsset::ScoreItem(const FAssessmentItem& Item, int32 ChoiceIndex, const TArray<FString>& Tokens)
{
	switch (Item.Type)
	{
	case EAssessmentItemType::ListenChoose:
	case EAssessmentItemType::MeaningChoose:
		return ChoiceIndex != INDEX_NONE && ChoiceIndex == Item.CorrectIndex;
	case EAssessmentItemType::GapFill:
		return USentenceGrader::Grade(Item.Challenge, UScaffoldPolicy::FillGaps(Item.Challenge, Tokens)).bPassed;
	case EAssessmentItemType::Assemble:
		return USentenceGrader::Grade(Item.Challenge, Tokens).bPassed;
	}
	return false;
}

TArray<FText> UAssessmentAsset::Validate() const
{
	TArray<FText> Out;
	if (FormId.IsNone())
	{
		Out.Add(LOCTEXT("NoForm", "Form has no FormId"));
	}
	if (!Language)
	{
		Out.Add(LOCTEXT("NoLang", "Form has no Language"));
	}
	TSet<FName> Ids;
	for (const FAssessmentItem& Item : Items)
	{
		const FText Id = FText::FromName(Item.ItemId);
		if (Item.ItemId.IsNone() || Ids.Contains(Item.ItemId))
		{
			Out.Add(FText::Format(LOCTEXT("BadId", "Item '{0}': missing or duplicate ItemId"), Id));
		}
		Ids.Add(Item.ItemId);
		if (Item.IsChoice())
		{
			if (Item.Options.Num() < 2 || !Item.Options.IsValidIndex(Item.CorrectIndex))
			{
				Out.Add(FText::Format(LOCTEXT("BadOptions", "Item '{0}': needs 2+ options and a valid CorrectIndex"), Id));
			}
			if (Item.TargetText.IsEmpty())
			{
				Out.Add(FText::Format(LOCTEXT("NoTarget", "Item '{0}': no TargetText"), Id));
			}
		}
		else if (Item.Challenge.AcceptedAnswers.IsEmpty())
		{
			Out.Add(FText::Format(LOCTEXT("NoAnswer", "Item '{0}': no accepted answers"), Id));
		}
	}
	return Out;
}

#undef LOCTEXT_NAMESPACE
