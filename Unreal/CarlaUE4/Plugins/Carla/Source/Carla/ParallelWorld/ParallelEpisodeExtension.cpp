// Plugins/Carla/Source/Carla/ParallelWorld/ParallelEpisodeExtension.cpp

#include "Carla/ParallelWorld/ParallelEpisodeExtension.h"
#include "Carla/Game/CarlaEpisode.h"
#include "Carla/ParallelWorld/ParallelWorldManager.h"

TPair<EActorSpawnResultStatus, FCarlaActor*> 
FParallelEpisodeExtension::SpawnActorInWorld(
    UCarlaEpisode* Episode,
    const FTransform& Transform,
    FActorDescription ActorDescription,
    int32 WorldID,
    FCarlaActor::IdType DesiredId)
{
    if (!Episode) 
    {
        return MakeTuple(EActorSpawnResultStatus::UnknownError, nullptr);
    }
    
    // 使用基础Episode生成Actor
    TPair<EActorSpawnResultStatus, FCarlaActor*> Result = 
        Episode->SpawnActorWithInfo(Transform, ActorDescription, DesiredId);
    
    // 如果生成成功，将其注册到指定世界
    if (Result.Key == EActorSpawnResultStatus::Success && Result.Value)
    {
        FCarlaActor::IdType ActorId = Result.Value->GetActorId();
        AActor* Actor = Result.Value->GetActor();
        
        UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
        if (WorldManager && WorldManager->IsParallelWorldEnabled())
        {
            WorldManager->RegisterActor(ActorId, WorldID, Actor);
        }
    }
    
    return Result;
}

bool FParallelEpisodeExtension::MoveActorToWorld(
    UCarlaEpisode* Episode,
    FCarlaActor::IdType ActorId,
    int32 WorldID)
{
    if (!Episode) return false;
    
    // 查找Actor
    FCarlaActor* CarlaActor = Episode->FindCarlaActor(ActorId);
    if (!CarlaActor)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot move actor %d to world %d: Actor not found"), 
            ActorId, WorldID);
        return false;
    }
    
    AActor* Actor = CarlaActor->GetActor();
    
    // 使用ParallelWorldManager更新世界
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (WorldManager && WorldManager->IsParallelWorldEnabled())
    {
        WorldManager->UpdateActorWorld(ActorId, WorldID, Actor);
        return true;
    }
    
    return false;
}

int32 FParallelEpisodeExtension::GetActorWorldID(
    UCarlaEpisode* Episode,
    FCarlaActor::IdType ActorId)
{
    if (!Episode) return 0;
    
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (WorldManager && WorldManager->IsParallelWorldEnabled())
    {
        return WorldManager->GetActorWorldID(ActorId);
    }
    
    return 0;  // 默认世界
}

TArray<FCarlaActor*> FParallelEpisodeExtension::GetActorsInWorld(
    UCarlaEpisode* Episode,
    int32 WorldID)
{
    TArray<FCarlaActor*> Result;
    
    if (!Episode) return Result;
    
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (!WorldManager || !WorldManager->IsParallelWorldEnabled())
    {
        // 如果平行世界未启用，返回所有Actor
        // 注意：这里可能需要修改，根据实际需求
        const auto& Registry = Episode->GetActorRegistry();
        for (const auto& Pair : Registry)
        {
            Result.Add(Pair.Value.Get());
        }
        return Result;
    }
    
    // 获取指定世界中的Actor ID列表
    TArray<FCarlaActor::IdType> ActorIds = WorldManager->GetActorsInWorld(WorldID);
    
    // 将Actor ID转换为FCarlaActor指针
    for (FCarlaActor::IdType ActorId : ActorIds)
    {
        FCarlaActor* CarlaActor = Episode->FindCarlaActor(ActorId);
        if (CarlaActor)
        {
            Result.Add(CarlaActor);
        }
    }
    
    return Result;
}

void FParallelEpisodeExtension::InitializeEpisodeForParallelWorlds(UCarlaEpisode* Episode)
{
    if (!Episode) return;
    
    // 初始化平行世界管理器
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (WorldManager)
    {
        WorldManager->Initialize();
        
        UE_LOG(LogCarla, Log, TEXT("Episode initialized for parallel worlds"));
    }
}

bool FParallelEpisodeExtension::SupportsParallelWorlds(UCarlaEpisode* Episode)
{
    // 检查是否启用了平行世界
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    return WorldManager ? WorldManager->IsParallelWorldEnabled() : false;
}
