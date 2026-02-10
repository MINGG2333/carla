// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaGameMode.cpp

#include "ParallelCarlaGameMode.h"
#include "Carla/Game/CarlaGameModeBase.h"
#include "Carla/Game/CarlaEpisode.h"

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
    Config.LoadConfig();
    
    // 应用配置
    bEnableParallelWorlds = Config.IsParallelWorldEnabled();
    MaxParallelWorlds = Config.GetMaxParallelWorlds();
    
    if (bEnableParallelWorlds)
    {
        // 初始化平行世界管理器
        FParallelWorldManager::GetInstance().Initialize();
        FParallelWorldManager::GetInstance().SetParallelWorldEnabled(true);
        
        // 初始化碰撞通道
        InitializeParallelCollisionChannels();
        
        UE_LOG(LogCarla, Log, 
            TEXT("Parallel world system initialized: %d max worlds"), 
            MaxParallelWorlds);
        
        // 记录当前激活的世界
        FParallelWorldManager::GetInstance().DebugLogWorldInfo();
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
        }
        
        // 可以在这里创建一些默认的平行世界
        // 例如：CreateParallelWorld("World1");
    }
}

void AParallelCarlaGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bEnableParallelWorlds)
    {
        // 清理平行世界系统
        FParallelWorldManager::GetInstance().Reset();
        UE_LOG(LogCarla, Log, TEXT("Parallel world system reset"));
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
    
    // 检查是否达到最大世界数量
    if (FParallelWorldManager::GetInstance().GetWorldCount() >= MaxParallelWorlds)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot create more parallel worlds. Max limit: %d"), MaxParallelWorlds);
        return -1;
    }
    
    return FParallelWorldManager::GetInstance().CreateWorld(WorldName);
}

bool AParallelCarlaGameMode::DestroyParallelWorld(int32 WorldID)
{
    if (!bEnableParallelWorlds || WorldID == 0)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot destroy default world (ID=0) or parallel worlds are disabled"));
        return false;
    }
    
    return FParallelWorldManager::GetInstance().DestroyWorld(WorldID);
}

TArray<int32> AParallelCarlaGameMode::GetAvailableWorlds() const
{
    TArray<int32> Result;
    
    if (bEnableParallelWorlds)
    {
        // 这里需要扩展ParallelWorldManager来提供世界列表
        // 暂时返回空数组
        UE_LOG(LogCarla, Warning, TEXT("GetAvailableWorlds not implemented yet"));
    }
    
    return Result;
}

FParallelWorldManager& AParallelCarlaGameMode::GetParallelWorldManager() const
{
    return FParallelWorldManager::GetInstance();
}

void AParallelCarlaGameMode::InitializeParallelCollisionChannels()
{
    // 初始化碰撞通道
    // 这里需要根据MaxParallelWorlds来分配碰撞通道
    // 可以使用现有的GameTraceChannel，或者动态分配
    
    UE_LOG(LogCarla, Log, TEXT("Initializing collision channels for %d parallel worlds"), MaxParallelWorlds);
    
    // 注意：这里只是记录日志，实际碰撞通道设置需要更多工作
    // 我们可以使用现有的通道或者通过配置文件设置
}
