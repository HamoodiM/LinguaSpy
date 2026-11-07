// LinguaSpy - fluency grade from helper reliance and mistakes

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FluencyScoring.generated.h"

/**
 *  Pure grading of how independently the player spoke.
 *  Reliance = (Mistakes + PeekWeight * Peeks) / Challenges.  S = no mistakes and no peeks, A <= 0.25, B <= 0.6, else C.
 *  Grades are ints so they compare naturally: 0 = S (best) .. 3 = C.
 */
/** How a challenge ended, for the mission tally (the debrief reports these separately). */
UENUM(BlueprintType)
enum class EAnswerOutcome : uint8
{
	Clean,      // right on the first try
	AfterHint,  // wrong first, right after the handler's hint (help, like a peek; not a mistake)
	Missed      // still wrong after the last try (a mistake; the handler recasts the model answer)
};

UCLASS()
class PERSONAL_API UFluencyScoring : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static constexpr float PeekWeight = 0.35f;

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Fluency")
	static int32 ComputeGrade(int32 Challenges, int32 Mistakes, int32 Peeks);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Fluency")
	static float ComputeReliance(int32 Challenges, int32 Mistakes, int32 Peeks);

	/** Final answer -> tally bucket. Attempts = 1 for a first-try answer. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Fluency")
	static EAnswerOutcome ClassifyAnswer(bool bPassed, int32 Attempts);

	/** "S", "A", "B" or "C". */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Fluency")
	static FText GradeLetter(int32 Grade);
};
