// Plugins/Carla/Source/Carla/ParallelWorld/ParallelActorDispatcher.h

#pragma once

#include "Carla/Actor/ActorDispatcher.h"

// 前向声明
class UParallelWorldManager;

/**
 * 扩展ActorDispatcher以支持平行世界
 * 通过子类化而不是修改原类
 */
class CARLA_API FParallelActorDispatcher
{
public:
    // 在指定世界生成Actor
    static TPair<EActorSpawnResultStatus, FCarlaActor*> SpawnActorInWorld(
        UActorDispatcher* BaseDispatcher,
        const FTransform& Transform,
        FActorDescription ActorDescription,
        int32 WorldID,
        FCarlaActor::IdType DesiredId = 0
    );
    
    // 将现有Actor分配到指定世界
    static bool AssignActorToWorld(
        UActorDispatcher* BaseDispatcher,
        FCarlaActor::IdType ActorId,
        int32 WorldID
    );
    
    // 获取Actor所在的世界
    static int32 GetActorWorldID(FCarlaActor::IdType ActorId);
    
    // 批量分配Actor到世界
    static void BatchAssignActorsToWorld(
        UActorDispatcher* BaseDispatcher,
        const TArray<FCarlaActor::IdType>& ActorIds,
        int32 WorldID
    );
    
private:
    // 禁止实例化
    FParallelActorDispatcher() = delete;
};
