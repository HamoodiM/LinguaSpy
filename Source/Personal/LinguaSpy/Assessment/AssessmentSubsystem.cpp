// LinguaSpy - runs an assessment form (no feedback), stores the result and logs it

#include "AssessmentSubsystem.h"
#include "LanguagePackAsset.h"
#include "LinguaSpySaveSubsystem.h"
#include "LearningTelemetrySubsystem.h"
#include "Engine/AssetManager.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/GameInstance.h"
#include "Personal.h"

void UAssessmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = Collection.InitializeDependency<ULinguaSpySaveSubsystem>();
}

TArray<UAssessmentAsset*> UAssessmentSubsystem::GetForms(const ULanguagePackAsset* Language) const
{
	TArray<UAssessmentAsset*> Forms;
	if (!Language)
	{
		return Forms;
	}
	TArray<FAssetData> Assets;
	IAssetRegistry::GetChecked().GetAssetsByClass(UAssessmentAsset::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		UAssessmentAsset* Form = Cast<UAssessmentAsset>(Asset.GetAsset());
		if (Form && Form->Language == Language && !Form->Items.IsEmpty())
		{
			Forms.Add(Form);
		}
	}
	Forms.Sort([](const UAssessmentAsset& A, const UAssessmentAsset& B) { return A.FormId.LexicalLess(B.FormId); });
	return Forms;
}

int32 UAssessmentSubsystem::ChooseFormIndex(int32 NumForms, const FString& ParticipantCode, FName Phase)
{
	if (NumForms <= 1)
	{
		return 0;
	}
	// Stable across sessions and platforms (not GetTypeHash, which may change between engine versions)
	uint32 Hash = 0;
	for (const TCHAR C : ParticipantCode.ToUpper())
	{
		Hash = Hash * 31u + static_cast<uint32>(C);
	}
	const int32 First = static_cast<int32>(Hash % 2u);
	const int32 Second = 1 - First;
	if (Phase == LinguaSpyAssessmentPhase::Post)
	{
		return Second;
	}
	if (Phase == LinguaSpyAssessmentPhase::Placement)
	{
		return 0;
	}
	return First; // Pre and Delayed
}

UAssessmentAsset* UAssessmentSubsystem::ChooseForm(const ULanguagePackAsset* Language, FName Phase) const
{
	const TArray<UAssessmentAsset*> Forms = GetForms(Language);
	if (Forms.IsEmpty())
	{
		return nullptr;
	}
	const FString Code = SaveSubsystem ? SaveSubsystem->GetParticipantCode() : FString();
	return Forms[FMath::Clamp(ChooseFormIndex(Forms.Num(), Code, Phase), 0, Forms.Num() - 1)];
}

bool UAssessmentSubsystem::StartRun(UAssessmentAsset* Form, FName Phase)
{
	if (!Form || Form->Items.IsEmpty() || IsRunning())
	{
		return false;
	}
	ActiveForm = Form;
	ActivePhase = Phase;
	ItemIndex = 0;
	ItemStart = FDateTime::UtcNow();
	Running = FAssessmentResult();
	Running.FormId = Form->FormId;
	Running.Phase = Phase;
	Running.MaxScore = Form->Items.Num();

	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("assessment_start"))
		.Id(TEXT("form"), Form->FormId).Id(TEXT("phase"), Phase).Int(TEXT("items"), Form->Items.Num()));
	return true;
}

void UAssessmentSubsystem::GetProgress(int32& OutIndex, int32& OutCount) const
{
	OutIndex = ItemIndex;
	OutCount = ActiveForm ? ActiveForm->Items.Num() : 0;
}

const FAssessmentItem* UAssessmentSubsystem::GetCurrentItem() const
{
	return ActiveForm && ActiveForm->Items.IsValidIndex(ItemIndex) ? &ActiveForm->Items[ItemIndex] : nullptr;
}

void UAssessmentSubsystem::SubmitChoice(int32 ChoiceIndex)
{
	if (const FAssessmentItem* Item = GetCurrentItem())
	{
		RecordAnswer(UAssessmentAsset::ScoreItem(*Item, ChoiceIndex, {}));
	}
}

void UAssessmentSubsystem::SubmitTokens(const TArray<FString>& Tokens)
{
	if (const FAssessmentItem* Item = GetCurrentItem())
	{
		RecordAnswer(UAssessmentAsset::ScoreItem(*Item, INDEX_NONE, Tokens));
	}
}

void UAssessmentSubsystem::RecordAnswer(bool bCorrect)
{
	const FAssessmentItem* Item = GetCurrentItem();
	if (!Item)
	{
		return;
	}
	const FDateTime Now = FDateTime::UtcNow();
	FAssessmentItemResult& Result = Running.Items.AddDefaulted_GetRef();
	Result.ItemId = Item->ItemId;
	Result.bCorrect = bCorrect;
	Result.Milliseconds = static_cast<int32>((Now - ItemStart).GetTotalMilliseconds());
	Running.Score += bCorrect ? 1 : 0;

	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("assessment_item"))
		.Id(TEXT("form"), ActiveForm->FormId).Id(TEXT("phase"), ActivePhase).Id(TEXT("item"), Item->ItemId)
		.Int(TEXT("type"), static_cast<int32>(Item->Type)).Id(TEXT("skill"), Item->SkillTag)
		.Bool(TEXT("correct"), bCorrect).Int(TEXT("ms"), Result.Milliseconds));

	++ItemIndex;
	ItemStart = Now;
	if (!GetCurrentItem())
	{
		FinishRun();
	}
}

void UAssessmentSubsystem::FinishRun()
{
	Running.UtcTicks = FDateTime::UtcNow().GetTicks();
	LastResult = Running;

	if (ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr)
	{
		Save->AssessmentResults.Add(Running);

		// Placement sets how much the handler helps in this language's city
		if (ActivePhase == LinguaSpyAssessmentPhase::Placement && ActiveForm->Language && Running.MaxScore > 0)
		{
			const float Share = static_cast<float>(Running.Score) / Running.MaxScore;
			SaveSubsystem->SetHandlerMode(ActiveForm->Language->CityId,
				Share >= PlacementOnCallThreshold ? EHandlerMode::OnCall : EHandlerMode::Shadowing);
		}
		SaveSubsystem->RequestSave();
	}

	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("assessment_end"))
		.Id(TEXT("form"), Running.FormId).Id(TEXT("phase"), Running.Phase)
		.Int(TEXT("score"), Running.Score).Int(TEXT("max"), Running.MaxScore));
	UE_LOG(LogPersonal, Log, TEXT("Assessment %s (%s): %d / %d"), *Running.FormId.ToString(), *Running.Phase.ToString(), Running.Score, Running.MaxScore);

	ActiveForm = nullptr;
	ActivePhase = NAME_None;
}

void UAssessmentSubsystem::CancelRun()
{
	if (ActiveForm)
	{
		ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("assessment_cancel"))
			.Id(TEXT("form"), ActiveForm->FormId).Int(TEXT("answered"), ItemIndex));
	}
	ActiveForm = nullptr;
	ActivePhase = NAME_None;
}

bool UAssessmentSubsystem::GetLatestResult(const ULanguagePackAsset* Language, FName Phase, FAssessmentResult& OutResult) const
{
	const ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	if (!Save || !Language)
	{
		return false;
	}
	TSet<FName> LanguageForms;
	for (const UAssessmentAsset* Form : GetForms(Language))
	{
		LanguageForms.Add(Form->FormId);
	}
	for (int32 i = Save->AssessmentResults.Num() - 1; i >= 0; --i)
	{
		const FAssessmentResult& Result = Save->AssessmentResults[i];
		if (Result.Phase == Phase && LanguageForms.Contains(Result.FormId))
		{
			OutResult = Result;
			return true;
		}
	}
	return false;
}
