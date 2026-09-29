// LinguaSpy - always-on HUD: cover meter and hints

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpyHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UCoverComponent;

/** Shows the spy's cover meter and a context hint. Builds a default layout in C++. */
UCLASS()
class USpyHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Binds the cover meter to the given component. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void BindCover(UCoverComponent* Cover);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHint(const FText& Hint);

protected:

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> CoverBar;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CoverLabel;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:

	void BuildDefaultLayout();

	UFUNCTION()
	void HandleCoverChanged(float NewCover, float MaxCover);

	TWeakObjectPtr<UCoverComponent> BoundCover;
};
