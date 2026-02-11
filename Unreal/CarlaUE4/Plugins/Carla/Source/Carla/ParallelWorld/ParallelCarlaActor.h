// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaActor.h

#pragma once

#include "Carla/Actor/CarlaActor.h"
#include "Carla/ParallelWorld/WorldProperties.h"

/**
 * 简化的平行Actor包装器
 * 仅用于存储世界ID，不管理原始Actor
 */
struct FParallelCarlaActor
{
    // Actor ID
    FCarlaActor::IdType ActorId;
    
    // 世界ID
    int32 WorldID;
    
    // 世界属性
    FWorldProperties WorldProperties;
    
    // 构造函数
    FParallelCarlaActor(FCarlaActor::IdType InActorId = 0, int32 InWorldID = 0)
        : ActorId(InActorId)
        , WorldID(InWorldID)
    {
        WorldProperties.WorldID = InWorldID;
        WorldProperties.WorldName = FString::Printf(TEXT("World_%d"), InWorldID);
        WorldProperties.bIsDefaultWorld = (InWorldID == 0);
        WorldProperties.CollisionChannel = FParallelWorldUtils::WorldIDToCollisionChannel(InWorldID);
        WorldProperties.RenderLayerMask = FParallelWorldUtils::WorldIDToRenderLayerMask(InWorldID);
    }
    
    // 更新世界ID
    void SetWorldID(int32 NewWorldID)
    {
        WorldID = NewWorldID;
        WorldProperties.WorldID = NewWorldID;
        WorldProperties.WorldName = FString::Printf(TEXT("World_%d"), NewWorldID);
        WorldProperties.bIsDefaultWorld = (NewWorldID == 0);
        WorldProperties.CollisionChannel = FParallelWorldUtils::WorldIDToCollisionChannel(NewWorldID);
        WorldProperties.RenderLayerMask = FParallelWorldUtils::WorldIDToRenderLayerMask(NewWorldID);
    }
};
