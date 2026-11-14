// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RevolverCylinderComponent.generated.h"

class ARevolverProjectile;

DECLARE_MULTICAST_DELEGATE(FOnLocalCockRequested);

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
	float ChamberLoadTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	float ChamberAdvanceTime = 0.3f;

	FOnLocalCockRequested OnLocalCockRequested;

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	const TArray<EBulletType>& GetChambers() const { return Chambers; }

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	int32 GetCurrentIndex() const { return CurrentIndex; }

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	float GetCurrentChamberCockTime() const;

	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void BeginSpin();

	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void EndSpin(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	bool IsSpinning() const { return bIsSpinning; }

	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void CancelReload();

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	bool IsReloading() const { return bIsReloading; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	TArray<EBulletType> DefaultLoadout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	TMap<EBulletType, float> CockTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	float DefaultCockTime = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	float CockTimeTolerance = 0.1f;

	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void StartCock();

	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void ReleaseTrigger();

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	bool IsCocking() const { return bIsCocking; }

	// Advances to the next chamber without firing. The skipped bullet stays in its chamber.
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Cycle();

	//cycles back
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void CycleBack();

	// Empties the current chamber. Does not advance.
	UFUNCTION(BlueprintCallable, Category = "Cylinder")
	void Discard();

	// The projectile class to spawn. TSubclassOf restricts the choice to ARevolverProjectile and its subclasses.
	// A Blueprint subclass can be selected here later to change the projectile's look.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	TSubclassOf<ARevolverProjectile> ProjectileClass;

	// Distance in centimeters in front of the player's eyes where the projectile appears.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	float MuzzleOffset = 100.0f;

	UFUNCTION(BlueprintPure, Category = "Cylinder")
	FString GetDebugString() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	TArray<EBulletType> Chambers;

	UPROPERTY(Replicated)
	int32 CurrentIndex = 0;

	UPROPERTY(Replicated)
	bool bIsCocking = false;

	UFUNCTION(Server, Reliable)
	void ServerStartCock();

	UFUNCTION(Server, Reliable)
	void ServerReleaseTrigger();

	UFUNCTION(Server, Reliable)
	void ServerCycle();

	UFUNCTION(Server, Reliable)
	void ServerCycleBack();

	UFUNCTION(Server, Reliable)
	void ServerDiscard();

	UPROPERTY(Replicated)
	bool bIsReloading = false;

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerCancelReload();

	UPROPERTY(Replicated)
	bool bIsSpinning = false;

	UFUNCTION(Server, Reliable)
	void ServerBeginSpin();

	UFUNCTION(Server, Reliable)
	void ServerEndSpin(int32 NewIndex);

private:
	// Moves CurrentIndex forward by one and wraps from the last chamber back to the first.
	void Advance();

	float CockStartTime = 0.0f;

	float CockRequiredTime = 0.0f;

	float GetCockTime(EBulletType BulletType) const;

	void FireCurrentChamber();

	FTimerHandle ReloadTimerHandle;

	int32 ReloadChambersRemaining = 0;

	void StartChamberLoad();

	void FinishChamberLoad();

	void AdvanceReload();

	void StopReload();

	void SpawnProjectile(EBulletType BulletType);
};
