// Fill out your copyright notice in the Description page of Project Settings.


#include "RevolverCylinderComponent.h"

// Sets default values for this component's properties
URevolverCylinderComponent::URevolverCylinderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
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

