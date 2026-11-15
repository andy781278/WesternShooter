#include "RoundTextWidget.h"
#include "Components/TextBlock.h"
#include "WesternShooterGameState.h"

void URoundTextWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!RoundText || !GetWorld())
	{
		return;
	}

	// The GameState exists on every machine, so clients read the replicated round data from it.
	const AWesternShooterGameState* State = GetWorld()->GetGameState<AWesternShooterGameState>();
	if (!State)
	{
		return;
	}

	FString Message;

	switch (State->RoundPhase)
	{
	case ERoundPhase::WaitingForPlayers:
		Message = TEXT("Waiting for players");
		break;

	case ERoundPhase::Playing:
		break;

	case ERoundPhase::RoundOver:
	{
		const FString Result = State->WinnerName.IsEmpty() ? TEXT("Draw") : State->WinnerName + TEXT(" wins");

		// CeilToInt rounds up, so the countdown shows 1 until it reaches zero, not 0 for the last second.
		const int32 Seconds = FMath::CeilToInt(State->GetSecondsUntilNextRound());

		Message = FString::Printf(TEXT("%s\nNext round in %d"), *Result, Seconds);
		break;
	}
	}

	if (Message != CachedText)
	{
		CachedText = Message;
		RoundText->SetText(FText::FromString(Message));
	}
}