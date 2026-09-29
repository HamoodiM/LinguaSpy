// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PersonalGameMode.generated.h"

/**
 *  Simple Game Mode for a top-down perspective game
 *  Sets the default gameplay framework classes
 *  Check the Blueprint derived class for the set values
 */
UCLASS(abstract)
class APersonalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	APersonalGameMode();

	/** Enters the city (mission state) before actors begin play */
	virtual void StartPlay() override;
};



