#include "RevolverCylinderComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
URevolverCylinderComponent::URevolverCylinderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	DefaultLoadout.Init(EBulletType::Standard, ChamberCount);
}

void URevolverCylinderComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(URevolverCylinderComponent, Chambers, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URevolverCylinderComponent, CurrentIndex, COND_OwnerOnly);
}

// Called when the game starts
void URevolverCylinderComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority()) {
		Chambers = DefaultLoadout;

		Chambers.SetNum(ChamberCount);

		CurrentIndex = 0;
	}
}

void URevolverCylinderComponent::Fire()
{
	ServerFire();
}

void URevolverCylinderComponent::Cycle()
{
	ServerCycle();
}

void URevolverCylinderComponent::Spin()
{
	ServerSpin();
}

void URevolverCylinderComponent::Discard()
{
	ServerDiscard();
}

void URevolverCylinderComponent::ServerFire_Implementation()
{
	if (!Chambers.IsValidIndex(CurrentIndex))
	{
		return;
	}

	const EBulletType FiredType = Chambers[CurrentIndex];

	if (FiredType != EBulletType::Empty)
	{
		Chambers[CurrentIndex] = EBulletType::Empty;

		// NEW. GetNameSafe returns the owner's name. It shows which player's cylinder fired.
		UE_LOG(LogTemp, Log, TEXT("Server: %s fired bullet type %d"), *GetNameSafe(GetOwner()), static_cast<int32>(FiredType));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Server: %s dry fired"), *GetNameSafe(GetOwner()));
	}

	Advance();
}

void URevolverCylinderComponent::ServerCycle_Implementation()
{
	Advance();
}

void URevolverCylinderComponent::ServerSpin_Implementation()
{
	if (Chambers.Num() == 0)
	{
		return;
	}

	// NEW. The server picks the random number, so a client cannot choose the result.
	CurrentIndex = FMath::RandRange(0, Chambers.Num() - 1);
}

void URevolverCylinderComponent::ServerDiscard_Implementation()
{
	if (!Chambers.IsValidIndex(CurrentIndex))
	{
		return;
	}

	Chambers[CurrentIndex] = EBulletType::Empty;
}

void URevolverCylinderComponent::Advance()
{
	if (Chambers.Num() == 0)
	{
		return;
	}

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

