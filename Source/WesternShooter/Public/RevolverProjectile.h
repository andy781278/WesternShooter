#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RevolverCylinderComponent.h"

#include "RevolverProjectile.generated.h"

// Forward declarations. They tell the compiler these classes exist without including their headers.
// A pointer to a class only needs a forward declaration. This keeps compile times lower.
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class WESTERNSHOOTER_API ARevolverProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARevolverProjectile();

	// Sphere used for collision. It is the root component, so the Actor's position is this sphere's position.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	// The visible sphere. It has no collision of its own.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// Engine component that moves the Actor along its velocity each frame and detects what it runs into.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	// Which bullet this projectile came from. Set by the cylinder right after spawning.
	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	EBulletType BulletType = EBulletType::Standard;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
};