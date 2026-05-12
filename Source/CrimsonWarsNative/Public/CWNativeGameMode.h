#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CWNativeGameMode.generated.h"

UCLASS()
class CRIMSONWARSNATIVE_API ACWNativeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACWNativeGameMode();

    virtual void BeginPlay() override;
};
