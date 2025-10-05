// Copyright Your Company Name, Inc. All Rights Reserved.

#include "FighterJetPawn.h"
#include "HealthComponent.h"
#include "Missile.h"
#include "AIAircraftPawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

// Sets default values
AFighterJetPawn::AFighterJetPawn()
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // --- Component Initialization ---
    AircraftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AircraftMesh"));
    RootComponent = AircraftMesh;
    AircraftMesh->SetSimulatePhysics(true);
    AircraftMesh->SetMassOverrideInKg(NAME_None, 15000.0f, true);
    AircraftMesh->SetAngularDamping(0.5f);
    AircraftMesh->SetLinearDamping(0.1f);

    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
    MuzzleLocation->SetupAttachment(AircraftMesh);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);

    // --- Camera Settings for Chase View ---
    SpringArm->TargetArmLength = 17000.0f;
    SpringArm->SocketOffset = FVector(-700.0f, 0.0f, 700.0f);
    SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 5.0f;
    SpringArm->bInheritPitch = true;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = true;


    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

    // --- Default Physics Values ---
    MaxThrust = 100000000.0f;
    ThrustAcceleration = 0.5f;

    PitchSpeed = 30.0f;
    RollSpeed = 50.0f;
    YawSpeed = 10.0f;
    GroundSteerSpeed = 80.0f;

    LiftCoefficient = 0.1f;
    DragCoefficient = 0.005f;

    // --- Weapon Defaults ---
    WeaponRange = 50000.0f;
    FireRate = 0.1f;

    // --- Initial State ---
    CurrentThrottle = 0.0f;
    PitchInput = 0.0f;
    RollInput = 0.0f;
    YawInput = 0.0f;
    GroundSteerInput = 0.0f;
    bIsOnGround = false;
    Airspeed = 0.0f;
    Altitude = 0.0f;
    bIsFiring = false;
    LastFireTime = 0.0f;
    LockedTarget = nullptr;

    // --- Find the HUD Widget Blueprint ---
    static ConstructorHelpers::FClassFinder<UUserWidget> HUDWidgetFinder(TEXT("/Game/Blueprints/WBP_FighterHUD"));
    if (HUDWidgetFinder.Succeeded())
    {
        HUDWidgetClass = HUDWidgetFinder.Class;
    }
}

// Called when the game starts or when spawned
void AFighterJetPawn::BeginPlay()
{
    Super::BeginPlay();

    // --- Create and display the HUD ---
    if (HUDWidgetClass)
    {
        HUDWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
        if (HUDWidgetInstance)
        {
            HUDWidgetInstance->AddToViewport();
        }
    }
}

// Called every frame
void AFighterJetPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    CheckIfOnGround();
    ApplyAerodynamics(DeltaTime);

    // --- Automatically update the locked target every frame ---
    UpdateLockedTarget();

    // --- Update HUD values every frame ---
    if (AircraftMesh)
    {
        Airspeed = AircraftMesh->GetPhysicsLinearVelocity().Size() * 0.036f;
        Altitude = GetActorLocation().Z / 100.0f;
    }

    if (bIsFiring && (GetWorld()->GetTimeSeconds() - LastFireTime) > FireRate)
    {
        FireWeapon();
        LastFireTime = GetWorld()->GetTimeSeconds();
    }

    if (HealthComponent && HealthComponent->IsDead())
    {
        HandleDeath();
    }
}

// Called to bind functionality to input
void AFighterJetPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Axis Mappings
    PlayerInputComponent->BindAxis("Throttle", this, &AFighterJetPawn::Throttle);
    PlayerInputComponent->BindAxis("Pitch", this, &AFighterJetPawn::Pitch);
    PlayerInputComponent->BindAxis("Roll", this, &AFighterJetPawn::Roll);
    PlayerInputComponent->BindAxis("Yaw", this, &AFighterJetPawn::Yaw);
    PlayerInputComponent->BindAxis("GroundSteer", this, &AFighterJetPawn::GroundSteer);

    // Action Mappings
    PlayerInputComponent->BindAction("FireWeapon", IE_Pressed, this, &AFighterJetPawn::StartFiring);
    PlayerInputComponent->BindAction("FireWeapon", IE_Released, this, &AFighterJetPawn::StopFiring);
    PlayerInputComponent->BindAction("FireMissile", IE_Pressed, this, &AFighterJetPawn::FireMissile);
}

void AFighterJetPawn::Throttle(float Value)
{
    CurrentThrottle = FMath::Clamp(CurrentThrottle + Value * ThrustAcceleration * GetWorld()->GetDeltaSeconds(), 0.0f, 1.0f);
}

void AFighterJetPawn::Pitch(float Value)
{
    PitchInput = Value;
}

void AFighterJetPawn::Roll(float Value)
{
    RollInput = Value;
}

void AFighterJetPawn::Yaw(float Value)
{
    YawInput = Value;
}

void AFighterJetPawn::GroundSteer(float Value)
{
    GroundSteerInput = Value;
}

void AFighterJetPawn::StartFiring()
{
    bIsFiring = true;
}

void AFighterJetPawn::StopFiring()
{
    bIsFiring = false;
}

void AFighterJetPawn::FireWeapon()
{
    if (!AircraftMesh) return;

    if (MuzzleFlashFX)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation->GetComponentLocation(), MuzzleLocation->GetComponentRotation());
    }

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    }

    FVector StartLocation = AircraftMesh->GetComponentLocation();
    FVector ForwardVector = AircraftMesh->GetForwardVector();
    FVector EndLocation = StartLocation + (ForwardVector * WeaponRange);

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

void AFighterJetPawn::FireMissile()
{
    if (!MissileClass || !LockedTarget)
    {
        // Can't fire if we don't have a missile class or a locked target
        return;
    }

    FVector SpawnLocation = MuzzleLocation->GetComponentLocation();
    FRotator SpawnRotation = GetActorRotation();

    AMissile* SpawnedMissile = GetWorld()->SpawnActor<AMissile>(MissileClass, SpawnLocation, SpawnRotation);
    if (SpawnedMissile)
    {
        // Set the missile's target to our automatically locked target
        SpawnedMissile->SetTarget(LockedTarget);
    }
}

void AFighterJetPawn::UpdateLockedTarget()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAIAircraftPawn::StaticClass(), FoundActors);

    AActor* BestTarget = nullptr;
    float BestTargetScore = -1.0f; // Use a score instead of just distance

    for (AActor* Actor : FoundActors)
    {
        // Check if the actor pointer is valid. This is the correct way to ensure the actor exists.
        if (!Actor)
        {
            continue;
        }

        // Calculate direction and distance to the potential target
        FVector DirectionToTarget = (Actor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        float DistanceToTarget = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());

        // Calculate the dot product to see how far in front of us the target is
        // A value of 1 means it's directly in front, -1 is directly behind.
        float DotProduct = FVector::DotProduct(GetActorForwardVector(), DirectionToTarget);

        // We only consider targets that are in front of us (DotProduct > 0)
        if (DotProduct > 0)
        {
            // Simple scoring: prioritize targets that are more directly in front of us
            // and closer.
            float Score = DotProduct / DistanceToTarget;

            if (Score > BestTargetScore)
            {
                BestTargetScore = Score;
                BestTarget = Actor;
            }
        }
    }

    LockedTarget = BestTarget;
}


void AFighterJetPawn::CheckIfOnGround()
{
    if (!AircraftMesh) return;
    FVector Start = AircraftMesh->GetComponentLocation();
    FVector End = Start - FVector(0.0f, 0.0f, 300.0f);
    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility,
        CollisionParams
    );

    if (bHit && !bIsOnGround)
    {
        float ImpactSpeed = -AircraftMesh->GetPhysicsLinearVelocity().Z;
        if (ImpactSpeed > 500.0f)
        {
            HealthComponent->TakeDamage(100.0f);
        }
    }

    bIsOnGround = bHit;
}


void AFighterJetPawn::ApplyAerodynamics(float DeltaTime)
{
    if (!AircraftMesh) return;
    FVector Velocity = AircraftMesh->GetPhysicsLinearVelocity();
    FVector ThrustForce = AircraftMesh->GetForwardVector() * CurrentThrottle * MaxThrust;
    AircraftMesh->AddForce(ThrustForce);
    if (Airspeed > 0.01f)
    {
        FVector DragForce = -Velocity.GetSafeNormal() * Airspeed * Airspeed * DragCoefficient;
        AircraftMesh->AddForce(DragForce);
    }
    if (!bIsOnGround)
    {
        FVector LiftDirection = FVector::CrossProduct(Velocity.GetSafeNormal(), AircraftMesh->GetRightVector()).GetSafeNormal();
        float LiftMagnitude = Airspeed * Airspeed * LiftCoefficient;
        FVector LiftForce = LiftDirection * LiftMagnitude;
        AircraftMesh->AddForce(LiftForce);
    }
    FVector RightVector = AircraftMesh->GetRightVector();
    FVector UpVector = AircraftMesh->GetUpVector();
    FVector ForwardVector = AircraftMesh->GetForwardVector();
    AircraftMesh->AddTorqueInDegrees(RightVector * PitchInput * PitchSpeed, NAME_None, true);
    AircraftMesh->AddTorqueInDegrees(ForwardVector * RollInput * RollSpeed, NAME_None, true);
    if (bIsOnGround)
    {
        AircraftMesh->AddTorqueInDegrees(UpVector * GroundSteerInput * GroundSteerSpeed, NAME_None, true);
    }
    else
    {
        AircraftMesh->AddTorqueInDegrees(UpVector * YawInput * YawSpeed, NAME_None, true);
    }
}

void AFighterJetPawn::HandleDeath()
{
    UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}
