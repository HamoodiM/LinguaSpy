// LinguaSpy - runs an assessment form (no feedback), stores the result and logs it

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AssessmentAsset.h"
#include "LinguaSpySaveGame.h"
#include "AssessmentSubsystem.generated.h"

class ULanguagePackAsset;
class ULinguaSpySaveSubsystem;

namespace LinguaSpyAssessmentPhase
{
	inline const FName Pre(TEXT("Pre"));
	inline const FName Post(TEXT("Post"));
	inline const FName Delayed(TEXT("Delayed"));
	inline const FName Placement(TEXT("Placement"));
}

/**
 *  Assessment runs: StartRun -> (SubmitChoice | SubmitTokens) per item -> result stored in the save + telemetry.
 *  Counterbalancing: forms are sorted by FormId; participants whose code hashes even take form 0 first (pre and delayed)
 *  and form 1 as the post-test; odd codes the reverse.
 */
UCLASS()
class PERSONAL_API UAssessmentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** All forms for a language, sorted by FormId. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Assessment")
	TArray<UAssessmentAsset*> GetForms(const ULanguagePackAsset* Language) const;

	/** The form this participant should take for a phase (see class comment). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Assessment")
	UAssessmentAsset* ChooseForm(const ULanguagePackAsset* Language, FName Phase) const;

	/** Pure counterbalancing rule (exposed for tests): index into the sorted forms. */
	static int32 ChooseFormIndex(int32 NumForms, const FString& ParticipantCode, FName Phase);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Assessment")
	bool StartRun(UAssessmentAsset* Form, FName Phase);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Assessment")
	bool IsRunning() const { return ActiveForm != nullptr; }

	/** Current item index (0-based) and item count. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Assessment")
	void GetProgress(int32& OutIndex, int32& OutCount) const;

	const FAssessmentItem* GetCurrentItem() const;

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Assessment")
	void SubmitChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Assessment")
	void SubmitTokens(const TArray<FString>& Tokens);

	/** Abandons the run (nothing is stored). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Assessment")
	void CancelRun();

	/** Result of the last finished run. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Assessment")
	FAssessmentResult GetLastResult() const { return LastResult; }

	/** Latest stored result for a phase in a language (false if none). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Assessment")
	bool GetLatestResult(const ULanguagePackAsset* Language, FName Phase, FAssessmentResult& OutResult) const;

	/** Placement: at or above this share the handler starts in OnCall instead of Shadowing. */
	static constexpr float PlacementOnCallThreshold = 0.7f;

private:

	void RecordAnswer(bool bCorrect);
	void FinishRun();

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveSubsystem> SaveSubsystem;

	UPROPERTY()
	TObjectPtr<UAssessmentAsset> ActiveForm;

	FName ActivePhase;
	int32 ItemIndex = 0;
	FDateTime ItemStart;
	FAssessmentResult Running;
	FAssessmentResult LastResult;
};
