// Copyright Epic Games, Inc. All Rights Reserved.

#include "voxel_cpp_testGameMode.h"
#include "voxel_cpp_testPlayerController.h"
#include "voxel_cpp_testCharacter.h"
#include "UObject/ConstructorHelpers.h"

Avoxel_cpp_testGameMode::Avoxel_cpp_testGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = Avoxel_cpp_testPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}