#include "RevolverProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ARevolverProjectile::ARevolverProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// The server's copy of this Actor is sent to clients, and each client creates its own copy.
	bReplicates = true;

	// The server also sends position and velocity, so clients stay in agreement with it.
	SetReplicateMovement(true);

	// CreateDefaultSubobject creates a component that is part of every instance of this class.
	// It can only be called in a constructor. The text is the component's internal name.
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

	// Radius in centimeters.
	CollisionComponent->InitSphereRadius(8.0f);

	// A collision profile is a named set of rules for what an object blocks, overlaps, or ignores.
	// BlockAllDynamic is a built-in profile for moving objects that are stopped by everything.
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	// Attaching makes the mesh follow the collision sphere.
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Loads the engine's built-in sphere mesh so the projectile is visible without any editor setup.
	// FObjectFinder can only be used in a constructor. "static" makes the lookup run once for the class.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereMesh.Object);

		// The built-in sphere is 100 cm wide. This scales it to 16 cm, matching the collision radius of 8.
		MeshComponent->SetRelativeScale3D(FVector(0.16f));
	}

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));

	// The component it moves. Collision is tested against this component's shape.
	MovementComponent->UpdatedComponent = CollisionComponent;

	// Centimeters per second. 3000 is 30 meters per second.
	MovementComponent->InitialSpeed = 3000.0f;
	MovementComponent->MaxSpeed = 3000.0f;

	// 0 means no gravity, so the path is a straight line. 1 is normal gravity.
	MovementComponent->ProjectileGravityScale = 0.0f;

	// The Actor is destroyed automatically after this many seconds, in case it never hits anything.
	InitialLifeSpan = 5.0f;
}

void ARevolverProjectile::BeginPlay()
{
	Super::BeginPlay();

	// The Instigator is the Pawn responsible for this Actor. The cylinder sets it when spawning.
	// This stops the projectile from colliding with the player who fired it.
	if (GetInstigator())
	{
		CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}

	// Only the server decides what a hit means, so only the server listens for hits.
	// AddDynamic subscribes a function to an engine event. OnComponentHit fires when this component is blocked.
	if (HasAuthority())
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ARevolverProjectile::OnHit);
	}
}

void ARevolverProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Log, TEXT("Server: projectile of type %d hit %s"), static_cast<int32>(BulletType), *GetNameSafe(OtherActor));

	// Destroying a replicated Actor on the server also removes it on every client.
	Destroy();
}