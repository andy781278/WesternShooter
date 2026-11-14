// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RevolverCylinderComponent.generated.h"

UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Empty,
	Standard,
	BigPush,
	Bomb,
	Recursive
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WESTERNSHOOTER_API URevolverCylinderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URevolverCylinderComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static constexpr int32 ChamberCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	TArray<EBulletType> DefaultLoadout;

	// Uses the current chamber, empties it, and advances to the next chamber.
	// An empty chamber is a dry fire: nothing is used, and the cylinder still advances.
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Fire();

	// Advances to the next chamber without firing. The skipped bullet stays in its chamber.
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Cycle();

	// Jumps to a random chamber.
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Spin();

	// Empties the current chamber. Does not advance.
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Discard();

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	FString GetDebugString() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	TArray<EBulletType> Chambers;

	UPROPERTY(Replicated)
	int32 CurrentIndex = 0;

	UFUNCTION(Server, Reliable)
	void ServerFire();

	UFUNCTION(Server, Reliable)
	void ServerCycle();

	UFUNCTION(Server, Reliable)
	void ServerSpin();

	UFUNCTION(Server, Reliable)
	void ServerDiscard();

private:
	// Moves CurrentIndex forward by one and wraps from the last chamber back to the first.
	void Advance();
};
