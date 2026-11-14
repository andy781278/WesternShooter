#include "RevolverCylinderComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Sets default values for this component's properties
URevolverCylinderComponent::URevolverCylinderComponent() {
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	DefaultLoadout.Init(EBulletType::Standard, ChamberCount);

	CockTimes.Add(EBulletType::Standard, 0.3f);
	CockTimes.Add(EBulletType::BigPush, 0.6f);
	CockTimes.Add(EBulletType::Bomb, 1.0f);
	CockTimes.Add(EBulletType::Recursive, 0.5f);
}

void URevolverCylinderComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(URevolverCylinderComponent, Chambers, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URevolverCylinderComponent, CurrentIndex, COND_OwnerOnly);
	DOREPLIFETIME(URevolverCylinderComponent, bIsReloading);
	DOREPLIFETIME(URevolverCylinderComponent, bIsCocking);
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

void URevolverCylinderComponent::Cycle()
{
	ServerCycle();
}

void URevolverCylinderComponent::CycleBack()
{
	ServerCycleBack();
}

void URevolverCylinderComponent::Spin()
{
	ServerSpin();
}

void URevolverCylinderComponent::Discard()
{
	ServerDiscard();
}

void URevolverCylinderComponent::Reload()
{
	ServerReload();
}

void URevolverCylinderComponent::CancelReload()
{
	ServerCancelReload();
}

void URevolverCylinderComponent::StartCock()
{
	ServerStartCock();
}

void URevolverCylinderComponent::ReleaseTrigger()
{
	ServerReleaseTrigger();
}

void URevolverCylinderComponent::ServerStartCock_Implementation()
{
	if (bIsCocking || !Chambers.IsValidIndex(CurrentIndex))
	{
		return;
	}

	const EBulletType BulletType = Chambers[CurrentIndex];

	// shooting cancels a reload only if the active chamber holds a bullet.
	// With cocking, the cancel happens when the hammer starts moving.
	if (bIsReloading)
	{
		if (BulletType == EBulletType::Empty)
		{
			return;
		}

		StopReload();
	}

	bIsCocking = true;

	// GetTimeSeconds returns the time since the level started, on this machine. This runs on the server.
	CockStartTime = GetWorld()->GetTimeSeconds();

	// The required time is fixed at the start, from the bullet in the chamber at that moment.
	CockRequiredTime = GetCockTime(BulletType);
}

void URevolverCylinderComponent::ServerReleaseTrigger_Implementation()
{
	// A release with no cocking in progress is ignored.
	// This happens when the press was rejected, for example during a reload on an empty chamber.
	if (!bIsCocking)
	{
		return;
	}

	bIsCocking = false;

	const float HeldTime = GetWorld()->GetTimeSeconds() - CockStartTime;

	// Released too early: the hammer is lowered and the bullet stays in the chamber.
	if (HeldTime + CockTimeTolerance < CockRequiredTime)
	{
		UE_LOG(LogTemp, Log, TEXT("Server: %s released early (%.2f of %.2f s)"), *GetNameSafe(GetOwner()), HeldTime, CockRequiredTime);
		return;
	}

	FireCurrentChamber();
}

void URevolverCylinderComponent::FireCurrentChamber()
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

float URevolverCylinderComponent::GetCockTime(EBulletType BulletType) const
{
	// TMap::Find returns a pointer to the value, or nullptr if the key is not in the map.
	// Declaring the variable inside the if condition limits it to this block.
	if (const float* FoundTime = CockTimes.Find(BulletType))
	{
		return *FoundTime;
	}

	return DefaultCockTime;
}

void URevolverCylinderComponent::ServerCycle_Implementation()
{
	if (bIsReloading || bIsCocking)
	{
		return;
	}
	Advance();
}

void URevolverCylinderComponent::ServerCycleBack_Implementation()
{
	// Blocked during a reload, the same as Cycle, Spin, and Discard.
	if (bIsReloading || bIsCocking)
	{
		return;
	}

	if (Chambers.Num() == 0)
	{
		return;
	}

	// Moving back from index 0 must wrap to the last chamber.
	// (0 - 1) % 6 is -1 in C++, which is not a valid index.
	// Adding Chambers.Num() before the % keeps the left side positive: (0 - 1 + 6) % 6 = 5.
	CurrentIndex = (CurrentIndex - 1 + Chambers.Num()) % Chambers.Num();
}

void URevolverCylinderComponent::ServerSpin_Implementation()
{
	if (bIsReloading || bIsCocking)
	{
		return;
	}
	if (Chambers.Num() == 0)
	{
		return;
	}

	// NEW. The server picks the random number, so a client cannot choose the result.
	CurrentIndex = FMath::RandRange(0, Chambers.Num() - 1);
}

void URevolverCylinderComponent::ServerDiscard_Implementation()
{
	if (bIsReloading || bIsCocking)
	{
		return;
	}
	if (!Chambers.IsValidIndex(CurrentIndex))
	{
		return;
	}

	Chambers[CurrentIndex] = EBulletType::Empty;
}

void URevolverCylinderComponent::ServerReload_Implementation()
{
	// Ignore a request while a reload is already running, or if there are no chambers.
	if (bIsReloading || bIsCocking || Chambers.Num() == 0)
	{
		return;
	}

	bIsReloading = true;

	// The reload visits every chamber once, starting at the active one.
	ReloadChambersRemaining = Chambers.Num();

	UE_LOG(LogTemp, Log, TEXT("Server: %s started reloading at chamber %d"), *GetNameSafe(GetOwner()), CurrentIndex);

	StartChamberLoad();
}

void URevolverCylinderComponent::ServerCancelReload_Implementation()
{
	if (!bIsReloading)
	{
		return;
	}

	StopReload();
}

void URevolverCylinderComponent::StartChamberLoad()
{
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&URevolverCylinderComponent::FinishChamberLoad,
		FMath::Max(ChamberLoadTime, 0.01f),
		false);
}

void URevolverCylinderComponent::FinishChamberLoad()
{
	// Place the loadout's bullet for this chamber, but only if the chamber is empty.
	// A chamber that already holds a bullet keeps it.
	// DefaultLoadout is checked with IsValidIndex because it can be shorter than Chambers.
	if (Chambers.IsValidIndex(CurrentIndex)
		&& Chambers[CurrentIndex] == EBulletType::Empty
		&& DefaultLoadout.IsValidIndex(CurrentIndex))
	{
		Chambers[CurrentIndex] = DefaultLoadout[CurrentIndex];
	}

	// Start the advance phase. Calling SetTimer with the same handle replaces the finished timer.
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&URevolverCylinderComponent::AdvanceReload,
		FMath::Max(ChamberAdvanceTime, 0.01f),
		false);
}

void URevolverCylinderComponent::AdvanceReload() {
	Advance();
	--ReloadChambersRemaining;

	// After the last chamber, the index has moved through all six and is back where the reload started.
	if (ReloadChambersRemaining <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Server: %s finished reloading"), *GetNameSafe(GetOwner()));
		StopReload();
		return;
	}

	StartChamberLoad();
}

void URevolverCylinderComponent::StopReload()
{
	// ClearTimer stops the pending timer, so no further phase runs.
	// Without this, a cancelled reload would continue when the timer fired.
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);

	bIsReloading = false;
	ReloadChambersRemaining = 0;
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

	if (bIsReloading)
	{
		Result += TEXT(" RELOADING");
	}

	if (bIsCocking)
	{
		Result += TEXT(" COCKING");
	}

	return Result;
}

