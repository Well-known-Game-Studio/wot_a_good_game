// Copyright Epic Games, Inc. All Rights Reserved.

#include "voxels_of_timeGameMode.h"
#include "voxels_of_timePlayerController.h"
#include "voxels_of_timeCharacter.h"
#include "UObject/ConstructorHelpers.h"

Avoxels_of_timeGameMode::Avoxels_of_timeGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = Avoxels_of_timePlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}