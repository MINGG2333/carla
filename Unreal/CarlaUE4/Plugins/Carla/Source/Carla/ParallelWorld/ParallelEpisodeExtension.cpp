// Plugins/Carla/Source/Carla/ParallelWorld/ParallelEpisodeExtension.cpp

#include "ParallelEpisodeExtension.h"
#include "Carla/Game/CarlaEpisode.h"

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
        FParallelWorldManager::GetInstance().RegisterActor(Result.Value, WorldID);
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
    
    // 使用ParallelWorldManager更新世界
    FParallelWorldManager::GetInstance().UpdateActorWorld(ActorId, WorldID);
    return true;
}

int32 FParallelEpisodeExtension::GetActorWorldID(
    UCarlaEpisode* Episode,
    FCarlaActor::IdType ActorId)
{
    if (!Episode) return 0;
    
    return FParallelWorldManager::GetInstance().GetActorWorldID(ActorId);
}

void FParallelEpisodeExtension::InitializeEpisodeForParallelWorlds(UCarlaEpisode* Episode)
{
    if (!Episode) return;
    
    // 初始化平行世界管理器
    FParallelWorldManager::GetInstance().Initialize();
    
    UE_LOG(LogCarla, Log, TEXT("Episode initialized for parallel worlds"));
}

bool FParallelEpisodeExtension::SupportsParallelWorlds(UCarlaEpisode* Episode)
{
    // 检查是否启用了平行世界
    return FParallelWorldManager::GetInstance().IsParallelWorldEnabled();
}
