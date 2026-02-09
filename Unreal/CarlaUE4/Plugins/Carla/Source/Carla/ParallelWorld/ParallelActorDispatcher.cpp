// Plugins/Carla/Source/Carla/ParallelWorld/ParallelActorDispatcher.cpp

#include "ParallelActorDispatcher.h"
#include "Carla/Game/CarlaEpisode.h"

TPair<EActorSpawnResultStatus, FCarlaActor*> 
FParallelActorDispatcher::SpawnActorInWorld(
    UActorDispatcher* BaseDispatcher,
    const FTransform& Transform,
    FActorDescription ActorDescription,
    int32 WorldID,
    FCarlaActor::IdType DesiredId)
{
    // 使用基础分发器生成Actor
    TPair<EActorSpawnResultStatus, FCarlaActor*> Result = 
        BaseDispatcher->SpawnActor(Transform, ActorDescription, DesiredId);
    
    // 如果生成成功，分配到指定世界
    if (Result.Key == EActorSpawnResultStatus::Success && Result.Value)
    {
        FParallelWorldManager::GetInstance().RegisterActor(Result.Value, WorldID);
    }
    
    return Result;
}

bool FParallelActorDispatcher::AssignActorToWorld(
    UActorDispatcher* BaseDispatcher,
    FCarlaActor::IdType ActorId,
    int32 WorldID)
{
    // 查找Actor
    FCarlaActor* CarlaActor = BaseDispatcher->GetActorRegistry().FindCarlaActor(ActorId);
    if (!CarlaActor)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot assign actor %d to world %d: Actor not found"), 
            ActorId, WorldID);
        return false;
    }
    
    // 注册到世界管理器
    FParallelWorldManager::GetInstance().RegisterActor(CarlaActor, WorldID);
    return true;
}
