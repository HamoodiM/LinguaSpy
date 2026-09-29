// LinguaSpy - language / city pack

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LanguagePackAsset.generated.h"

class UConversationAsset;

/** Describes one playable city and the language learned there (e.g. Paris / French). */
UCLASS(BlueprintType)
class ULanguagePackAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FText LanguageName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FText CityName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TSoftObjectPtr<UWorld> CityMap;

	/** Conversations that belong to this city (for tooling / future mission framework). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TArray<TSoftObjectPtr<UConversationAsset>> Conversations;
};
