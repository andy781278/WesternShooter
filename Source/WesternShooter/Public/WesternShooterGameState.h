#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "WesternShooterGameState.generated.h"

UENUM(BlueprintType)
enum class ERoundPhase : uint8
{
	WaitingForPlayers,
	Playing,
	RoundOver
};

UCLASS()
class WESTERNSHOOTER_API AWesternShooterGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Round")
	ERoundPhase RoundPhase = ERoundPhase::WaitingForPlayers;

	// Empty when the round ended in a draw.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Round")
	FString WinnerName;

	// Server time at which the next round starts.
	// Clients have their own clocks, so the time is stored in the server's clock
	// and compared with GetServerWorldTimeSeconds, which the engine keeps in sync on clients.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Round")
	float NextRoundServerTime = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Round")
	float GetSecondsUntilNextRound() const;
};