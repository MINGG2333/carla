// Plugins/Carla/Source/Carla/Game/ParallelWorldSettings.h
#pragma once

#include "CoreMinimal.h"
#include "ParallelWorldSettings.generated.h"

USTRUCT(BlueprintType)
struct CARLA_API FParallelWorldSettings
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World")
    bool bEnableParallelWorlds = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World", meta=(ClampMin="1", ClampMax="8"))
    int32 MaxParallelWorlds = 4;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World")
    bool bAutoAssignToDefaultWorld = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World")
    bool bDebugLogging = true;
    
    FParallelWorldSettings() = default;
};
