// Copyright Your Company Name, Inc. All Rights Reserved.

#include "DogfightGameModeBase.h"
#include "AIAircraftPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h" // Needed for widgets

void ADogfightGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    // Call the function to spawn enemies
    SpawnEnemies();

    AliveEnemiesCount = NumberOfEnemiesToSpawn;
}

void ADogfightGameModeBase::SpawnEnemies()
{
    if (!AIPawnClass)
    {
        return;
    }

    for (int32 i = 0; i < NumberOfEnemiesToSpawn; ++i)
    {
        float Angle = FMath::FRandRange(0.f, 360.f);
        FVector SpawnLocation = FVector(SpawnRadius * FMath::Cos(Angle), SpawnRadius * FMath::Sin(Angle), 5000.0f);
        FRotator SpawnRotation = FRotator(0.0f, FMath::FRandRange(0.f, 360.f), 0.0f);

        GetWorld()->SpawnActor<AAIAircraftPawn>(AIPawnClass, SpawnLocation, SpawnRotation);
    }
}

void ADogfightGameModeBase::EnemyDestroyed()
{
    AliveEnemiesCount--;
    CheckWinCondition();
}

void ADogfightGameModeBase::CheckWinCondition()
{
    if (AliveEnemiesCount <= 0)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("YOU WIN!"));
        }
    }
}

// --- CHANGE 3: Implemented the PlayerDied function ---
void ADogfightGameModeBase::PlayerDied()
{
    if (GameOverWidgetClass)
    {
        UUserWidget* GameOverWidget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
        if (GameOverWidget)
        {
            GameOverWidget->AddToViewport();
        }
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (PlayerController)
    {
        PlayerController->SetPause(true);
        PlayerController->bShowMouseCursor = true;
    }
}
