// Plugins/Carla/Source/Carla/ParallelWorld/ParallelActorDispatcher.cpp

#include "Carla/ParallelWorld/ParallelActorDispatcher.h"
#include "Carla/Game/CarlaEpisode.h"
#include "Carla/ParallelWorld/ParallelWorldManager.h"

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
    
    AActor* Actor = CarlaActor->GetActor();
    
    // 注册到世界管理器
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (WorldManager && WorldManager->IsParallelWorldEnabled())
    {
        WorldManager->UpdateActorWorld(ActorId, WorldID, Actor);
        return true;
    }
    
    return false;
}

int32 FParallelActorDispatcher::GetActorWorldID(FCarlaActor::IdType ActorId)
{
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (WorldManager && WorldManager->IsParallelWorldEnabled())
    {
        return WorldManager->GetActorWorldID(ActorId);
    }
    return 0;  // 默认世界
}

void FParallelActorDispatcher::BatchAssignActorsToWorld(
    UActorDispatcher* BaseDispatcher,
    const TArray<FCarlaActor::IdType>& ActorIds,
    int32 WorldID)
{
    UParallelWorldManager* WorldManager = UParallelWorldManager::GetInstance();
    if (!WorldManager || !WorldManager->IsParallelWorldEnabled())
    {
        return;
    }
    
    for (FCarlaActor::IdType ActorId : ActorIds)
    {
        // 查找Actor
        FCarlaActor* CarlaActor = BaseDispatcher->GetActorRegistry().FindCarlaActor(ActorId);
        if (CarlaActor)
        {
            AActor* Actor = CarlaActor->GetActor();
            WorldManager->UpdateActorWorld(ActorId, WorldID, Actor);
        }
        else
        {
            UE_LOG(LogCarla, Warning, TEXT("Cannot assign actor %d to world %d: Actor not found"), 
                ActorId, WorldID);
        }
    }
}
