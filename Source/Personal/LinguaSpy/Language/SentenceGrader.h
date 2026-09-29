// LinguaSpy - word-tile answer grading

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConversationTypes.h"
#include "SentenceGrader.generated.h"

/**
 *  Pure grading logic for word-tile challenges.
 *  Comparison ignores case, surrounding whitespace/punctuation and apostrophe style,
 *  but accents are significant ("café" != "cafe").
 */
UCLASS()
class PERSONAL_API USentenceGrader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Grading")
	static FGradeResult Grade(const FWordTileChallenge& Challenge, const TArray<FString>& PlayerTokens);

	/** Normalizes a single token for comparison. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Grading")
	static FString NormalizeToken(const FString& Token);

	/** Joins tokens into a display sentence (e.g. for showing the model answer). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Grading")
	static FString JoinTokens(const TArray<FString>& Tokens, bool bUseSpaces = true);
};
