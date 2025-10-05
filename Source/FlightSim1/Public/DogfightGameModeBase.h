// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h" // --- CHANGE: Corrected .hh to .h ---
#include "DogfightGameModeBase.generated.h"

class AAIAircraftPawn;
class UUserWidget;

UCLASS()
class FLIGHTSIM1_API ADogfightGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	void EnemyDestroyed();
	void PlayerDied();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// Function to handle spawning the AI enemies
	void SpawnEnemies();

	// Function to check if the player has won
	void CheckWinCondition();

protected:
	// The type of AI pawn to spawn. We can set this to our BP_AIAircraft in the editor.
	UPROPERTY(EditDefaultsOnly, Category = "AI Spawning")
	TSubclassOf<AAIAircraftPawn> AIPawnClass;

	// How many enemies to spawn at the start of the game
	UPROPERTY(EditDefaultsOnly, Category = "AI Spawning")
	int32 NumberOfEnemiesToSpawn;

	// The radius around the center of the map where enemies will be spawned
	UPROPERTY(EditDefaultsOnly, Category = "AI Spawning")
	float SpawnRadius;

	// A property to hold the Game Over widget
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

private:
	int32 AliveEnemiesCount;
};
