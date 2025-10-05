// Copyright Your Company Name, Inc. All Rights Reserved.

#include "AIAircraftPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "HealthComponent.h"
#include "FighterJetPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h" // --- CHANGE 1: Added include for TimerManager ---

// Sets default values
AAIAircraftPawn::AAIAircraftPawn()
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Create and set the root component
    AircraftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AircraftMesh"));
    RootComponent = AircraftMesh;
    AircraftMesh->SetSimulatePhysics(true);

    // Create the Health Component
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    // Create Muzzle Component and set weapon defaults
    MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
    MuzzleLocation->SetupAttachment(AircraftMesh);

    // Set default AI values
    FlightSpeed = 5000.0f;
    TurnSpeed = 2.0f;
    AvoidanceDistance = 15000.0f;
    MaxSpeed = 10000.0f;
    EvasionDuration = 2.0f;
    EvasionTurnSpeed = 8.0f;

    // Set default weapon values
    WeaponRange = 50000.0f;
    FireRate = 0.2f;
    LastFireTime = 0.0f;

    // Set initial state
    CurrentState = EAIState::Seeking;
}

// Called when the game starts or when spawned
void AAIAircraftPawn::BeginPlay()
{
    Super::BeginPlay();

    if (HealthComponent)
    {
        HealthComponent->OnDamaged.AddDynamic(this, &AAIAircraftPawn::HandleTakeDamage);
    }
}

// Called every frame
void AAIAircraftPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Execute AI logic every frame
    MoveAndTurn(DeltaTime);

    if (CurrentState == EAIState::Seeking)
    {
        CheckAndFire(DeltaTime);
    }
}

void AAIAircraftPawn::MoveAndTurn(float DeltaTime)
{
    // 1. Apply forward thrust
    FVector ForwardForce = GetActorForwardVector() * FlightSpeed;
    AircraftMesh->AddForce(ForwardForce);

    // 2. Clamp the velocity to the speed limit
    FVector CurrentVelocity = AircraftMesh->GetPhysicsLinearVelocity();
    if (CurrentVelocity.Size() > MaxSpeed)
    {
        AircraftMesh->SetPhysicsLinearVelocity(CurrentVelocity.GetSafeNormal() * MaxSpeed);
    }

    // 3. Find the player and turn
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerPawn)
    {
        if (CurrentState == EAIState::Seeking)
        {
            float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
            FRotator TargetRotation;

            if (DistanceToPlayer > AvoidanceDistance)
            {
                FVector DirectionToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
                TargetRotation = UKismetMathLibrary::MakeRotFromX(DirectionToPlayer);
            }
            else
            {
                FVector DirectionAwayFromPlayer = GetActorLocation() - PlayerPawn->GetActorLocation();
                TargetRotation = UKismetMathLibrary::MakeRotFromX(DirectionAwayFromPlayer);
            }

            FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, TurnSpeed);
            SetActorRotation(NewRotation);
        }
        else // If evading
        {
            FRotator EvasionRotation = FRotator(0.0f, EvasionTurnSpeed, 0.0f);
            AddActorLocalRotation(EvasionRotation);
        }
    }
}

// --- CHANGE 2: Added the definitions for the missing functions ---
void AAIAircraftPawn::HandleTakeDamage(AActor* DamagedActor, float Damage)
{
    if (CurrentState != EAIState::Evading)
    {
        BeginEvasion();
    }
}

void AAIAircraftPawn::BeginEvasion()
{
    CurrentState = EAIState::Evading;
    GetWorldTimerManager().SetTimer(EvasionTimerHandle, this, &AAIAircraftPawn::EndEvasion, EvasionDuration, false);
}

void AAIAircraftPawn::EndEvasion()
{
    CurrentState = EAIState::Seeking;
}

void AAIAircraftPawn::CheckAndFire(float DeltaTime)
{
    if ((GetWorld()->GetTimeSeconds() - LastFireTime) < FireRate)
    {
        return;
    }

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerPawn)
    {
        FVector DirectionToPlayer = (PlayerPawn->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        float DotProduct = FVector::DotProduct(GetActorForwardVector(), DirectionToPlayer);

        if (DotProduct > 0.9f)
        {
            FireWeapon();
            LastFireTime = GetWorld()->GetTimeSeconds();
        }
    }
}

void AAIAircraftPawn::FireWeapon()
{
    if (MuzzleFlashFX)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation->GetComponentLocation(), MuzzleLocation->GetComponentRotation());
    }

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    }

    FVector StartLocation = MuzzleLocation->GetComponentLocation();
    FVector EndLocation = StartLocation + (GetActorForwardVector() * WeaponRange);
    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        CollisionParams
    );

    if (bHit)
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor)
        {
            UHealthComponent* TargetHealthComponent = HitActor->FindComponentByClass<UHealthComponent>();
            if (TargetHealthComponent)
            {
                TargetHealthComponent->TakeDamage(10.0f);
            }
        }
    }
}
