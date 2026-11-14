#include "RevolverCylinderComponent.h"

// Sets default values for this component's properties
URevolverCylinderComponent::URevolverCylinderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	DefaultLoadout.Init(EBulletType::Standard, ChamberCount);
}

// Called when the game starts
void URevolverCylinderComponent::BeginPlay()
{
	Super::BeginPlay();

	Chambers = DefaultLoadout;
	
	Chambers.SetNum(ChamberCount);

	CurrentIndex = 0;
}

void URevolverCylinderComponent::Fire() {
	if (!Chambers.IsValidIndex(CurrentIndex)) {
		return;
	}

	const EBulletType FiredType = Chambers[CurrentIndex];

	if (FiredType != EBulletType::Empty)
	{
		// The bullet is used up.
		Chambers[CurrentIndex] = EBulletType::Empty;

		// UE_LOG writes a line to the Output Log window (Window > Output Log in the editor).
		// LogTemp is a general-purpose log category. Log is the severity level.
		// %d prints an integer. The enum is cast to int32 because %d cannot print an enum class directly.
		UE_LOG(LogTemp, Log, TEXT("Fired bullet type %d"), static_cast<int32>(FiredType));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Dry fire: chamber was empty"));
	}

	Advance();
}

void URevolverCylinderComponent::Cycle() {
	Advance();
}

void URevolverCylinderComponent::Spin() {
	if (Chambers.Num() == 0)
	{
		return;
	}

	// FMath::RandRange(Min, Max) returns a random integer. Both ends are included.
	CurrentIndex = FMath::RandRange(0, Chambers.Num() - 1);
}

void URevolverCylinderComponent::Discard() {
	if (!Chambers.IsValidIndex(CurrentIndex)) {
		return;
	}

	Chambers[CurrentIndex] = EBulletType::Empty;
}

void URevolverCylinderComponent::Advance() {
	// Guard against dividing by zero in the modulo below.
	if (Chambers.Num() == 0)
	{
		return;
	}

	// Modulo wraps the index: with 6 chambers, index 5 advances to (5 + 1) % 6 = 0.
	CurrentIndex = (CurrentIndex + 1) % Chambers.Num();
}

FString URevolverCylinderComponent::GetDebugString() const
{
	FString Result;

	for (int32 Index = 0; Index < Chambers.Num(); ++Index)
	{
		// TCHAR is Unreal's character type (wide characters). TEXT() produces a literal of that type.
		TCHAR Symbol = TEXT('?');
		switch (Chambers[Index])
		{
		case EBulletType::Empty:     Symbol = TEXT('.'); break;
		case EBulletType::Standard:  Symbol = TEXT('S'); break;
		case EBulletType::BigPush:   Symbol = TEXT('P'); break;
		case EBulletType::Bomb:      Symbol = TEXT('B'); break;
		case EBulletType::Recursive: Symbol = TEXT('R'); break;
		}

		// Brackets around the chamber that fires next.
		if (Index == CurrentIndex)
		{
			Result += FString::Printf(TEXT("[%c] "), Symbol);
		}
		else
		{
			Result += FString::Printf(TEXT(" %c  "), Symbol);
		}
	}

	return Result;
}

