// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldConfig.cpp

#include "ParallelWorldConfig.h"
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
    bConfigLoaded = false;
}

bool FParallelWorldConfig::LoadConfig()
{
    const FString ConfigFile = FPaths::ProjectConfigDir() / GetConfigFileName();
    
    if (!FPaths::FileExists(ConfigFile))
    {
        UE_LOG(LogCarla, Log, TEXT("Parallel world config file not found: %s"), *ConfigFile);
        return false;
    }
    
    GConfig->GetBool(TEXT("ParallelWorldSettings"), TEXT("bEnableParallelWorlds"), bEnableParallelWorlds, ConfigFile);
    GConfig->GetInt(TEXT("ParallelWorldSettings"), TEXT("MaxParallelWorlds"), MaxParallelWorlds, ConfigFile);
    
    bConfigLoaded = true;
    UE_LOG(LogCarla, Log, TEXT("Loaded parallel world config: Enabled=%s, MaxWorlds=%d"), 
        bEnableParallelWorlds ? TEXT("true") : TEXT("false"), MaxParallelWorlds);
    
    return true;
}

bool FParallelWorldConfig::SaveConfig()
{
    const FString ConfigFile = FPaths::ProjectConfigDir() / GetConfigFileName();
    
    GConfig->SetBool(TEXT("ParallelWorldSettings"), TEXT("bEnableParallelWorlds"), bEnableParallelWorlds, ConfigFile);
    GConfig->SetInt(TEXT("ParallelWorldSettings"), TEXT("MaxParallelWorlds"), MaxParallelWorlds, ConfigFile);
    
    GConfig->Flush(false, ConfigFile);
    
    UE_LOG(LogCarla, Log, TEXT("Saved parallel world config: Enabled=%s, MaxWorlds=%d"), 
        bEnableParallelWorlds ? TEXT("true") : TEXT("false"), MaxParallelWorlds);
    
    return true;
}
