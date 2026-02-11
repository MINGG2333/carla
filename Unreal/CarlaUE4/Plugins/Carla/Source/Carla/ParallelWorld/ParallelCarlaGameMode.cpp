// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaGameMode.cpp

#include "ParallelCarlaGameMode.h"
#include "Carla/Game/CarlaGameModeBase.h"
#include "Carla/Game/CarlaEpisode.h"
#include "ParallelWorld/ParallelWorldManager.h"
#include "ParallelWorld/ParallelWorldConfig.h"

AParallelCarlaGameMode::AParallelCarlaGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 继承父类的所有设置
    PrimaryActorTick.bCanEverTick = true;
}

void AParallelCarlaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    // 首先调用父类的初始化
    Super::InitGame(MapName, Options, ErrorMessage);
    
    // 加载配置
    FParallelWorldConfig& Config = FParallelWorldConfig::Get();
    if (!Config.LoadConfig())
    {
        // 如果配置文件不存在，保存默认配置
        Config.SaveConfig();
    }
    
    // 应用配置
    bEnableParallelWorlds = Config.IsParallelWorldEnabled();
    MaxParallelWorlds = Config.GetMaxParallelWorlds();
    
    if (bEnableParallelWorlds)
    {
        // 初始化平行世界管理器
        UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
        if (WorldManager)
        {
            WorldManager->Initialize();
            WorldManager->SetParallelWorldEnabled(true);
            
            UE_LOG(LogCarla, Log, 
                TEXT("Parallel world system initialized: %d max worlds, enabled: %s"), 
                MaxParallelWorlds,
                WorldManager->IsParallelWorldEnabled() ? TEXT("true") : TEXT("false"));
            
            // 记录当前激活的世界
            WorldManager->DebugLogWorldInfo();
        }
        else
        {
            UE_LOG(LogCarla, Error, TEXT("Failed to get ParallelWorldManager instance"));
        }
        
        // 初始化碰撞通道
        InitializeParallelCollisionChannels();
    }
    else
    {
        UE_LOG(LogCarla, Log, TEXT("Parallel worlds are disabled by configuration"));
    }
}

void AParallelCarlaGameMode::BeginPlay()
{
    // 首先调用父类的BeginPlay
    Super::BeginPlay();
    
    if (bEnableParallelWorlds)
    {
        // 确保Episode已经初始化
        if (Episode)
        {
            UE_LOG(LogCarla, Log, TEXT("ParallelCarlaGameMode BeginPlay - Episode ready"));
            
            // 可以在这里进行一些初始化工作，比如：
            // - 注册现有的Actor到默认世界
            // - 创建一些测试世界
        }
        
        // 记录配置信息
        UE_LOG(LogCarla, Log, TEXT("Parallel World Configuration:"));
        UE_LOG(LogCarla, Log, TEXT("  Enabled: %s"), bEnableParallelWorlds ? TEXT("true") : TEXT("false"));
        UE_LOG(LogCarla, Log, TEXT("  Max Worlds: %d"), MaxParallelWorlds);
    }
}

void AParallelCarlaGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bEnableParallelWorlds)
    {
        // 清理平行世界系统
        UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
        if (WorldManager)
        {
            WorldManager->Reset();
            UE_LOG(LogCarla, Log, TEXT("Parallel world system reset"));
        }
    }
    
    // 调用父类的EndPlay
    Super::EndPlay(EndPlayReason);
}

int32 AParallelCarlaGameMode::CreateParallelWorld(const FString& WorldName)
{
    if (!bEnableParallelWorlds)
    {
        UE_LOG(LogCarla, Warning, TEXT("Parallel worlds are not enabled"));
        return -1;
    }
    
    // 通过管理器创建世界
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (!WorldManager)
    {
        UE_LOG(LogCarla, Error, TEXT("ParallelWorldManager not found"));
        return -1;
    }
    
    // 检查是否达到最大世界数量
    if (WorldManager->GetWorldCount() >= MaxParallelWorlds)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot create more parallel worlds. Max limit: %d"), MaxParallelWorlds);
        return -1;
    }
    
    return WorldManager->CreateWorld(WorldName);
}

bool AParallelCarlaGameMode::DestroyParallelWorld(int32 WorldID)
{
    if (!bEnableParallelWorlds || WorldID == 0)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot destroy default world (ID=0) or parallel worlds are disabled"));
        return false;
    }
    
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (!WorldManager)
    {
        UE_LOG(LogCarla, Error, TEXT("ParallelWorldManager not found"));
        return false;
    }
    
    return WorldManager->DestroyWorld(WorldID);
}

TArray<int32> AParallelCarlaGameMode::GetAvailableWorlds() const
{
    TArray<int32> Result;
    
    if (bEnableParallelWorlds)
    {
        UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
        if (WorldManager)
        {
            Result = WorldManager->GetAllWorldIDs();
        }
    }
    
    return Result;
}

UParallelWorldManager* AParallelCarlaGameMode::GetParallelWorldManager() const
{
    return UParallelWorldManager::GetInstance();
}

int32 AParallelCarlaGameMode::GetParallelWorldCount() const
{
    if (!bEnableParallelWorlds) return 0;
    
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    return WorldManager ? WorldManager->GetWorldCount() : 0;
}

void AParallelCarlaGameMode::InitializeParallelCollisionChannels()
{
    // 初始化碰撞通道
    // 这里需要根据MaxParallelWorlds来分配碰撞通道
    // 可以使用现有的GameTraceChannel，或者动态分配
    
    UE_LOG(LogCarla, Log, TEXT("Initializing collision channels for %d parallel worlds"), MaxParallelWorlds);
    
    // 注意：这里只是记录日志，实际碰撞通道设置需要更多工作
    // 我们可以使用现有的通道或者通过配置文件设置
    
    // 示例：检查碰撞通道配置
    // 这里可以根据需要添加实际配置代码
}
