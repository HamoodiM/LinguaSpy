// LinguaSpy - pre/post test forms for measuring learning (docs/Pilot_Study_Protocol.md)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConversationTypes.h"
#include "AssessmentAsset.generated.h"

class ULanguagePackAsset;
class USoundBase;

UENUM(BlueprintType)
enum class EAssessmentItemType : uint8
{
	/** Hear a phrase (audio only), pick its meaning. */
	ListenChoose,
	/** Read a phrase, pick its meaning. */
	MeaningChoose,
	/** A sentence with blanks; type the missing words (Challenge + its gap indices). */
	GapFill,
	/** Build the sentence from tiles (Challenge). */
	Assemble
};

USTRUCT(BlueprintType)
struct PERSONAL_API FAssessmentItem
{
	GENERATED_BODY()

	/** Stable id for analysis, e.g. "A01". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	EAssessmentItemType Type = EAssessmentItemType::MeaningChoose;

	/** Instruction / question in the player's language. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(MultiLine=true))
	FText Prompt;

	/** Target-language phrase (shown for MeaningChoose; spoken for ListenChoose, shown only if there is no audio). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FString TargetText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TSoftObjectPtr<USoundBase> Audio;

	/** Choice items: options in the player's language. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TArray<FText> Options;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	int32 CorrectIndex = 0;

	/** GapFill / Assemble items. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FWordTileChallenge Challenge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FName SkillTag;

	bool IsChoice() const { return Type == EAssessmentItemType::ListenChoose || Type == EAssessmentItemType::MeaningChoose; }
};

/** One test form. Parallel forms (A/B) cover the same skills with different items; see the pilot protocol. */
UCLASS(BlueprintType)
class PERSONAL_API UAssessmentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** "FR_A", "FR_B"... Forms of one language are counterbalanced in FormId order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assessment")
	FName FormId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assessment")
	TObjectPtr<ULanguagePackAsset> Language;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assessment")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assessment", meta=(TitleProperty="ItemId"))
	TArray<FAssessmentItem> Items;

	/** Pure scoring. Choice items use ChoiceIndex; GapFill uses Tokens as the typed gap words; Assemble uses Tokens. */
	static bool ScoreItem(const FAssessmentItem& Item, int32 ChoiceIndex, const TArray<FString>& Tokens);

	UFUNCTION(BlueprintCallable, Category="Assessment")
	TArray<FText> Validate() const;
};
