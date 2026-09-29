// LinguaSpy - always-on HUD: cover meter, objective tracker, mission banners

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpyHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UBorder;
class UCoverComponent;
class UMissionAsset;
class UMissionSubsystem;

/** Shows the spy's cover meter, current objective and mission banners. Builds a default layout in C++. */
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

	/** Shows a large centred message for a few seconds. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowBanner(const FText& Title, const FText& Subtitle, float Duration = 4.f);

protected:

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> CoverBar;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CoverLabel;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MissionTitleText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UBorder> BannerPanel;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BannerTitle;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BannerSubtitle;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildDefaultLayout();
	UMissionSubsystem* GetMissionSubsystem() const;

	UFUNCTION()
	void HandleCoverChanged(float NewCover, float MaxCover);

	UFUNCTION()
	void HandleObjectiveChanged();

	UFUNCTION()
	void HandleMissionStarted(UMissionAsset* Mission);

	UFUNCTION()
	void HandleMissionCompleted(UMissionAsset* Mission);

	UFUNCTION()
	void HandleMissionFailed(UMissionAsset* Mission);

	UFUNCTION()
	void HandleCityCompleted();

	TWeakObjectPtr<UCoverComponent> BoundCover;
	float BannerTimeLeft = 0.f;
	float BannerPulseTime = -1.f;
};
