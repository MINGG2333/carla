// Plugins/Carla/Source/Carla/Game/ParallelWorldConfig.cpp

#include "Carla/Game/ParallelWorldConfig.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"

FParallelWorldConfig& FParallelWorldConfig::Get()
{
    static FParallelWorldConfig Instance;
    return Instance;
}

FParallelWorldConfig::FParallelWorldConfig()
{
    // 默认配置
    bEnableParallelWorlds = true;
    MaxParallelWorlds = 4;
    DefaultWorldID = 0;
    bAutoAssignToDefaultWorld = true;
    bDebugLogging = false;
    bConfigLoaded = false;
}

bool FParallelWorldConfig::LoadConfig()
{
    const FString ConfigFile = FPaths::ProjectConfigDir() / GetConfigFileName();
    
    if (!FPaths::FileExists(ConfigFile))
    {
        UE_LOG(LogCarla, Log, TEXT("Parallel world config file not found: %s"), *ConfigFile);
        UE_LOG(LogCarla, Log, TEXT("Using default configuration"));
        return false;
    }
    
    GConfig->GetBool(TEXT("ParallelWorldSettings"), TEXT("bEnableParallelWorlds"), bEnableParallelWorlds, ConfigFile);
    GConfig->GetInt(TEXT("ParallelWorldSettings"), TEXT("MaxParallelWorlds"), MaxParallelWorlds, ConfigFile);
    GConfig->GetBool(TEXT("ParallelWorldSettings"), TEXT("bAutoAssignToDefaultWorld"), bAutoAssignToDefaultWorld, ConfigFile);
    GConfig->GetBool(TEXT("ParallelWorldSettings"), TEXT("bDebugLogging"), bDebugLogging, ConfigFile);
    
    bConfigLoaded = true;
    
    UE_LOG(LogCarla, Log, TEXT("Loaded parallel world config from %s"), *ConfigFile);
    UE_LOG(LogCarla, Log, TEXT("  Enabled: %s"), bEnableParallelWorlds ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCarla, Log, TEXT("  Max Worlds: %d"), MaxParallelWorlds);
    UE_LOG(LogCarla, Log, TEXT("  Auto Assign: %s"), bAutoAssignToDefaultWorld ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCarla, Log, TEXT("  Debug Logging: %s"), bDebugLogging ? TEXT("true") : TEXT("false"));
    
    return true;
}

bool FParallelWorldConfig::SaveConfig()
{
    const FString ConfigFile = FPaths::ProjectConfigDir() / GetConfigFileName();
    
    GConfig->SetBool(TEXT("ParallelWorldSettings"), TEXT("bEnableParallelWorlds"), bEnableParallelWorlds, ConfigFile);
    GConfig->SetInt(TEXT("ParallelWorldSettings"), TEXT("MaxParallelWorlds"), MaxParallelWorlds, ConfigFile);
    GConfig->SetBool(TEXT("ParallelWorldSettings"), TEXT("bAutoAssignToDefaultWorld"), bAutoAssignToDefaultWorld, ConfigFile);
    GConfig->SetBool(TEXT("ParallelWorldSettings"), TEXT("bDebugLogging"), bDebugLogging, ConfigFile);
    
    GConfig->Flush(false, ConfigFile);
    
    UE_LOG(LogCarla, Log, TEXT("Saved parallel world config to %s"), *ConfigFile);
    UE_LOG(LogCarla, Log, TEXT("  Enabled: %s"), bEnableParallelWorlds ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCarla, Log, TEXT("  Max Worlds: %d"), MaxParallelWorlds);
    
    return true;
}
