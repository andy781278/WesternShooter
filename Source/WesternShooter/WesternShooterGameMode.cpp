// Copyright Epic Games, Inc. All Rights Reserved.

#include "WesternShooterGameMode.h"
#include "WesternShooterCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

AWesternShooterGameMode::AWesternShooterGameMode()
{
	GameStateClass = AWesternShooterGameState::StaticClass();
}

AWesternShooterGameState* AWesternShooterGameMode::GetRoundState() const
{
	return GetGameState<AWesternShooterGameState>();
}

void AWesternShooterGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// The second player to connect starts the first round.
	// Players already have pawns, because the engine spawns them on connect.
	AWesternShooterGameState* State = GetRoundState();
	if (State && State->RoundPhase == ERoundPhase::WaitingForPlayers && GetNumPlayers() >= MinPlayersToPlay)
	{
		State->RoundPhase = ERoundPhase::Playing;
	}
}

void AWesternShooterGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// The leaving player is still in the controller list during Logout.
	// Checking on the next frame counts the players who remain.
	GetWorldTimerManager().SetTimerForNextTick(this, &AWesternShooterGameMode::CheckRoundEnd);
}

void AWesternShooterGameMode::OnPlayerDied(AController* DeadController)
{
	CheckRoundEnd();
}

int32 AWesternShooterGameMode::CountAlivePlayers(AController*& OutLastAlive) const
{
	int32 Alive = 0;
	OutLastAlive = nullptr;

	// Iterates over every connected player's controller.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		const AWesternShooterCharacter* Character = Cast<AWesternShooterCharacter>(PlayerController->GetPawn());
		if (Character && !Character->IsDead())
		{
			++Alive;
			OutLastAlive = PlayerController;
		}
	}

	return Alive;
}

void AWesternShooterGameMode::CheckRoundEnd()
{
	AWesternShooterGameState* State = GetRoundState();
	if (!State || State->RoundPhase != ERoundPhase::Playing)
	{
		return;
	}

	AController* LastAlive = nullptr;
	const int32 Alive = CountAlivePlayers(LastAlive);

	// One left is a win. Zero left is a draw, for example when two players die in the same frame.
	if (Alive <= 1)
	{
		EndRound(Alive == 1 ? LastAlive : nullptr);
	}
}

void AWesternShooterGameMode::EndRound(AController* Winner)
{
	AWesternShooterGameState* State = GetRoundState();
	if (!State)
	{
		return;
	}

	State->RoundPhase = ERoundPhase::RoundOver;
	State->WinnerName = (Winner && Winner->PlayerState) ? Winner->PlayerState->GetPlayerName() : FString();
	State->NextRoundServerTime = State->GetServerWorldTimeSeconds() + RoundOverDelay;

	GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &AWesternShooterGameMode::StartRound, RoundOverDelay, false);
}

void AWesternShooterGameMode::StartRound()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		// RestartPlayer reuses an existing pawn instead of spawning a new one,
		// so the old body, alive or dead, is destroyed first. Destroying it also unpossesses it.
		if (APawn* OldPawn = PlayerController->GetPawn())
		{
			OldPawn->Destroy();
		}

		// Spawns a new pawn at a PlayerStart and possesses it.
		RestartPlayer(PlayerController);
	}

	AWesternShooterGameState* State = GetRoundState();
	if (State)
	{
		State->WinnerName.Empty();
		State->RoundPhase = GetNumPlayers() >= MinPlayersToPlay ? ERoundPhase::Playing : ERoundPhase::WaitingForPlayers;
	}
}