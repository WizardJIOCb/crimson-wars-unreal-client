#include "CWNativeGameMode.h"

#include "CWNativePlayerPawn.h"
#include "CWNativeWorldActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ACWNativeGameMode::ACWNativeGameMode()
{
    DefaultPawnClass = ACWNativePlayerPawn::StaticClass();
}

void ACWNativeGameMode::BeginPlay()
{
    Super::BeginPlay();

    bool bHasRuntimeWorldActor = false;
    for (TActorIterator<ACWNativeWorldActor> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(TEXT("CrimsonWarsRuntimeWorld")))
        {
            bHasRuntimeWorldActor = true;
            break;
        }
    }

    if (!bHasRuntimeWorldActor && GetWorld())
    {
        if (ACWNativeWorldActor* WorldActor = GetWorld()->SpawnActor<ACWNativeWorldActor>(ACWNativeWorldActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
        {
            WorldActor->Tags.Add(TEXT("CrimsonWarsRuntimeWorld"));
        }
    }
}
