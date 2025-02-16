// Copyright Epic Games, Inc. All Rights Reserved.

#include "RopeSimulationGameMode.h"
#include "RopeSimulationCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARopeSimulationGameMode::ARopeSimulationGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
