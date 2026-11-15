// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WesternShooterGameState.h"
#include "WesternShooterGameMode.generated.h"

UCLASS(abstract)
class AWesternShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWesternShooterGameMode();


public:
	// Called by a character on the server when it dies.
	void OnPlayerDied(AController* DeadController);

	// Seconds between the end of a round and the next spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	float RoundOverDelay = 4.0f;

	// A round only ends by elimination if at least this many players are connected.
	// Prevents a single player from ending and restarting rounds in a loop.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
	int32 MinPlayersToPlay = 2;

protected:
	// Called on the server after a player connects.
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// Called on the server when a player disconnects.
	virtual void Logout(AController* Exiting) override;

private:
	FTimerHandle RoundTimerHandle;

	void CheckRoundEnd();
	void EndRound(AController* Winner);
	void StartRound();

	// Returns how many players are alive, and the last alive one found.
	int32 CountAlivePlayers(AController*& OutLastAlive) const;

	AWesternShooterGameState* GetRoundState() const;

};