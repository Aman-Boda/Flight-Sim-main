// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
// --- CHANGE: Ensured all necessary headers are included BEFORE the .generated.h file ---
#include "Components/StaticMeshComponent.h"
#include "HealthComponent.h"
#include "Components/SceneComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "AIAircraftPawn.generated.h" // This MUST be the last include

// --- CHANGE 1: Created an enum for the AI's current state ---
UENUM(BlueprintType)
enum class EAIState : uint8
{
    Seeking,
    Evading
};

UCLASS()
class FLIGHTSIM1_API AAIAircraftPawn : public APawn
{
    GENERATED_BODY()

public:
    // Sets default values for this pawn's properties
    AAIAircraftPawn();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* AircraftMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UHealthComponent* HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* MuzzleLocation;

    // --- AI Properties ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float FlightSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float TurnSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float AvoidanceDistance;

    // --- CHANGE 2: Added properties for evasion ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float EvasionDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float EvasionTurnSpeed;

    // --- CHANGE: Added the missing MaxSpeed property ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float MaxSpeed;

    // --- Weapon Properties ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
    float WeaponRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
    float FireRate;

    UPROPERTY(EditDefaultsOnly, Category = "Weapons")
    UParticleSystem* MuzzleFlashFX;

    UPROPERTY(EditDefaultsOnly, Category = "Weapons")
    USoundBase* FireSound;

private:
    // AI logic functions
    void MoveAndTurn(float DeltaTime);
    void CheckAndFire(float DeltaTime);
    void FireWeapon();

    // --- CHANGE 3: Added functions for handling evasion ---
    UFUNCTION()
    void HandleTakeDamage(AActor* DamagedActor, float Damage);
    void BeginEvasion();
    void EndEvasion();

    // Internal state for firing
    float LastFireTime;

    // Internal state for AI
    EAIState CurrentState;
    FTimerHandle EvasionTimerHandle;
};
