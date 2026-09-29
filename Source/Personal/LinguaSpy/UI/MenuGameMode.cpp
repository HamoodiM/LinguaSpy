// LinguaSpy - game mode for the title / city-select map

#include "MenuGameMode.h"
#include "MainMenuWidget.h"
#include "MissionSubsystem.h"
#include "LinguaSpyAudio.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AMenuGameMode::AMenuGameMode()
{
	DefaultPawnClass = nullptr;
	MenuWidgetClass = UMainMenuWidget::StaticClass();
}

void AMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
	{
		Missions->LeaveCity();
	}

	LinguaSpyAudio::PlayLoop(this, GetDefault<ULinguaSpyAudioSettings>()->MenuMusic, ESpyAudioChannel::Music);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !MenuWidgetClass)
	{
		return;
	}
	UMainMenuWidget* Menu = CreateWidget<UMainMenuWidget>(PC, MenuWidgetClass);
	Menu->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Menu->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}
