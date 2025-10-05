// Copyright Your Company Name, Inc. All Rights Reserved.

#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "DogfightGameModeBase.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
}

void UHealthComponent::TakeDamage(float DamageAmount)
{
    if (CurrentHealth <= 0.0f)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

    // --- CHANGE 3: Broadcast the OnDamaged event ---
    OnDamaged.Broadcast(GetOwner(), DamageAmount);

    if (CurrentHealth <= 0.0f)
    {
        Die();
    }
}

bool UHealthComponent::IsDead() const
{
    return CurrentHealth <= 0.0f;
}

void UHealthComponent::Die()
{
    ADogfightGameModeBase* GameMode = Cast<ADogfightGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        if (GetOwner() != UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            GameMode->EnemyDestroyed();
        }
        else
        {
            GameMode->PlayerDied();
        }
    }

    if (DeathEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathEffect, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation());
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        UStaticMeshComponent* MeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
        if (MeshComponent)
        {
            MeshComponent->SetSimulatePhysics(false);
        }
        Owner->Destroy();
    }
}
