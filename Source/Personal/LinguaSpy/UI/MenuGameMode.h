// LinguaSpy - game mode for the title / city-select map

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

class UMainMenuWidget;

/** No pawn; shows the main menu with a UI-only input mode. */
UCLASS()
class AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AMenuGameMode();

protected:

	UPROPERTY(EditAnywhere, Category="Menu")
	TSubclassOf<UMainMenuWidget> MenuWidgetClass;

	virtual void BeginPlay() override;
};
