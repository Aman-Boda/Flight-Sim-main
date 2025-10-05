// Copyright Your Company Name, Inc. All Rights Reserved.

#include "Missile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HealthComponent.h"
#include "Particles/ParticleSystem.h"

// Sets default values
AMissile::AMissile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create the missile's mesh
	MissileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	RootComponent = MissileMesh;
	MissileMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Create and configure the particle trail
	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	// Create and configure the projectile movement component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 40000.f;
	ProjectileMovement->MaxSpeed = 40000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 80000.f;

	// Set default values
	DamageAmount = 100.0f; // Missiles do a lot of damage
	TargetActor = nullptr;

	// Set the lifespan of the missile to 10 seconds
	InitialLifeSpan = 10.0f;
}

// Called when the game starts or when spawned
void AMissile::BeginPlay()
{
	Super::BeginPlay();

	// Bind the OnHit function to the mesh's OnComponentHit event
	MissileMesh->OnComponentHit.AddDynamic(this, &AMissile::OnHit);
}

// Called every frame
void AMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If we have a target, update the projectile movement component
	if (TargetActor)
	{
		ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
	}
}

void AMissile::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void AMissile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// If we hit a valid actor that is not ourselves
	if (OtherActor && OtherActor != this)
	{
		// Try to find a health component on the actor we hit
		UHealthComponent* HealthComponent = OtherActor->FindComponentByClass<UHealthComponent>();
		if (HealthComponent)
		{
			// Apply damage
			HealthComponent->TakeDamage(DamageAmount);
		}
	}

	// Spawn the explosion effect at the impact point
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation());
	}

	// Destroy the missile after it hits something
	Destroy();
}
