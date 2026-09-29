// LinguaSpy - spaced-repetition items stored in the save profile

#include "SpacedRepetitionSubsystem.h"
#include "ConversationAsset.h"
#include "LanguagePackAsset.h"
#include "LinguaSpySaveSubsystem.h"
#include "SentenceGrader.h"

void USpacedRepetitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = Collection.InitializeDependency<ULinguaSpySaveSubsystem>();
}

TMap<FString, FSrsItemState>* USpacedRepetitionSubsystem::GetItems() const
{
	ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	return Save ? &Save->SrsItems : nullptr;
}

void USpacedRepetitionSubsystem::RecordResult(const FSrsItemKey& Key, FName CityId, FName SkillTag, bool bPassed, bool bAssisted)
{
	TMap<FString, FSrsItemState>* Items = GetItems();
	if (!Items || !Key.IsValid())
	{
		return;
	}
	FSrsItemState& State = Items->FindOrAdd(Key.ToString());
	State.CityId = CityId;
	State.SkillTag = SkillTag;
	USrsScheduler::Update(State, bPassed, bAssisted, GetNow());
	// Saved with the next mission/conversation checkpoint (or at the end of a review session)
}

int32 USpacedRepetitionSubsystem::GetDueCount(FName CityId) const
{
	const TMap<FString, FSrsItemState>* Items = GetItems();
	return Items ? USrsScheduler::CountDue(*Items, CityId, GetNow()) : 0;
}

int32 USpacedRepetitionSubsystem::GetLearnedCount(FName CityId) const
{
	int32 Count = 0;
	if (const TMap<FString, FSrsItemState>* Items = GetItems())
	{
		for (const TPair<FString, FSrsItemState>& Pair : *Items)
		{
			if (Pair.Value.CityId == CityId && Pair.Value.Passes > 0)
			{
				++Count;
			}
		}
	}
	return Count;
}

TArray<FSrsItemKey> USpacedRepetitionSubsystem::BuildReviewQueue(FName CityId, int32 Max) const
{
	TArray<FSrsItemKey> Out;
	const TMap<FString, FSrsItemState>* Items = GetItems();
	if (!Items)
	{
		return Out;
	}
	// Ask for extra keys so stale ones (renamed nodes / deleted assets) can be skipped
	for (const FString& KeyString : USrsScheduler::SelectReviewItems(*Items, CityId, GetNow(), Max * 2, true))
	{
		FSrsItemKey Key;
		FSrsItemView View;
		if (FSrsItemKey::Parse(KeyString, Key) && ResolveItem(Key, View))
		{
			Out.Add(Key);
			if (Out.Num() >= Max)
			{
				break;
			}
		}
	}
	return Out;
}

TArray<FSrsItemView> USpacedRepetitionSubsystem::GetLearnedItems(FName CityId) const
{
	TArray<FSrsItemView> Out;
	const TMap<FString, FSrsItemState>* Items = GetItems();
	if (!Items)
	{
		return Out;
	}
	const FDateTime Now = GetNow();
	for (const TPair<FString, FSrsItemState>& Pair : *Items)
	{
		FSrsItemKey Key;
		FSrsItemView View;
		if (Pair.Value.CityId == CityId && Pair.Value.Passes > 0 && FSrsItemKey::Parse(Pair.Key, Key) && ResolveItem(Key, View))
		{
			View.State = Pair.Value;
			View.bDue = USrsScheduler::IsDue(Pair.Value, Now);
			Out.Add(View);
		}
	}
	Out.Sort([](const FSrsItemView& A, const FSrsItemView& B)
	{
		if (A.State.SkillTag != B.State.SkillTag)
		{
			return A.State.SkillTag.LexicalLess(B.State.SkillTag);
		}
		return A.Phrase < B.Phrase;
	});
	return Out;
}

TMap<FName, float> USpacedRepetitionSubsystem::GetSkillStrengths(FName CityId) const
{
	TMap<FName, float> Sum;
	TMap<FName, int32> Count;
	if (const TMap<FString, FSrsItemState>* Items = GetItems())
	{
		for (const TPair<FString, FSrsItemState>& Pair : *Items)
		{
			if (Pair.Value.CityId == CityId && !Pair.Value.SkillTag.IsNone())
			{
				Sum.FindOrAdd(Pair.Value.SkillTag) += USrsScheduler::GetStrength(Pair.Value);
				++Count.FindOrAdd(Pair.Value.SkillTag);
			}
		}
	}
	for (TPair<FName, float>& Pair : Sum)
	{
		Pair.Value /= FMath::Max(Count.FindRef(Pair.Key), 1);
	}
	Sum.KeySort([](FName A, FName B) { return A.LexicalLess(B); });
	return Sum;
}

bool USpacedRepetitionSubsystem::ResolveItem(const FSrsItemKey& Key, FSrsItemView& OutView)
{
	const UConversationAsset* Conversation = Cast<UConversationAsset>(Key.Conversation.TryLoad());
	const FWordTileChallenge* Challenge = Conversation ? Conversation->FindChallenge(Key.NodeId, Key.ChoiceIndex) : nullptr;
	if (!Challenge || Challenge->AcceptedAnswers.IsEmpty())
	{
		return false;
	}
	const ULanguagePackAsset* Language = Conversation->Language;
	const bool bSpaces = !Language || Language->bUsesWordSpacing;
	const TArray<FString>& Tokens = Challenge->AcceptedAnswers[0].Tokens;

	OutView.Key = Key;
	OutView.Meaning = Challenge->IntentPrompt;
	OutView.Phrase = USentenceGrader::JoinTokens(Tokens, bSpaces);
	OutView.Reading.Reset();
	if (Language && Language->bHasReadings)
	{
		TArray<FString> Readings;
		for (const FString& Token : Tokens)
		{
			const FString Reading = Challenge->FindReading(Token);
			if (!Reading.IsEmpty())
			{
				Readings.Add(Reading);
			}
		}
		OutView.Reading = FString::Join(Readings, TEXT(" "));
	}
	return true;
}
