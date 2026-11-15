#include "WesternShooterGameState.h"
#include "Net/UnrealNetwork.h"

void AWesternShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWesternShooterGameState, RoundPhase);
	DOREPLIFETIME(AWesternShooterGameState, WinnerName);
	DOREPLIFETIME(AWesternShooterGameState, NextRoundServerTime);
}

float AWesternShooterGameState::GetSecondsUntilNextRound() const
{
	return FMath::Max(0.0f, NextRoundServerTime - GetServerWorldTimeSeconds());
}