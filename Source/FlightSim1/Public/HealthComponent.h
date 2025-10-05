// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// --- CHANGE 1: Declared a new delegate (event dispatcher) ---
// This will broadcast an event whenever the component takes damage.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamagedSignature, AActor*, DamagedActor, float, Damage);

class UParticleSystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLIGHTSIM1_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UHealthComponent();

    // --- CHANGE 2: Added the delegate property ---
    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnDamagedSignature OnDamaged;

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Function to be called when this component takes damage
    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeDamage(float DamageAmount);

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDead() const;

protected:
    // The maximum health of the actor
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
    float MaxHealth;

    // The current health of the actor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    float CurrentHealth;

    // The particle effect to spawn upon death
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
    UParticleSystem* DeathEffect;

private:
    // Function to handle the death of the actor
    void Die();
};
