// LinguaSpy - the spy's cover meter

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoverComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoverChanged, float, NewCover, float, MaxCover);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoverBlown);

/** How convincing the spy's cover is. Drops on language mistakes; at zero the cover is blown. */
UCLASS(ClassGroup=(LinguaSpy), meta=(BlueprintSpawnableComponent))
class PERSONAL_API UCoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCoverComponent();

	UPROPERTY(BlueprintAssignable, Category="Cover")
	FOnCoverChanged OnCoverChanged;

	UPROPERTY(BlueprintAssignable, Category="Cover")
	FOnCoverBlown OnCoverBlown;

	UFUNCTION(BlueprintCallable, Category="Cover")
	void ApplyPenalty(float Amount);

	UFUNCTION(BlueprintCallable, Category="Cover")
	void Recover(float Amount);

	UFUNCTION(BlueprintCallable, Category="Cover")
	void ResetCover();

	UFUNCTION(BlueprintPure, Category="Cover")
	float GetCover() const { return Cover; }

	UFUNCTION(BlueprintPure, Category="Cover")
	float GetMaxCover() const { return MaxCover; }

	UFUNCTION(BlueprintPure, Category="Cover")
	bool IsBlown() const { return Cover <= 0.f; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cover", meta=(ClampMin="1"))
	float MaxCover = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Cover")
	float Cover = 100.f;

	virtual void BeginPlay() override;

private:

	void SetCover(float NewValue);
};
