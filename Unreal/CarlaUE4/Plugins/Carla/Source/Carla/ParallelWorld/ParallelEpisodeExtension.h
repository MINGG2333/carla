// Plugins/Carla/Source/Carla/ParallelWorld/ParallelEpisodeExtension.h

#pragma once

#include "Carla/Game/CarlaEpisode.h"
#include "ParallelWorldManager.h"
#include "ParallelActorDispatcher.h"

/**
 * 扩展CarlaEpisode功能的工具类
 * 通过静态函数提供平行世界相关功能
 */
class CARLA_API FParallelEpisodeExtension
{
public:
    // 禁止实例化
    FParallelEpisodeExtension() = delete;
    
    // 在指定世界生成Actor
    static TPair<EActorSpawnResultStatus, FCarlaActor*> SpawnActorInWorld(
        UCarlaEpisode* Episode,
        const FTransform& Transform,
        FActorDescription ActorDescription,
        int32 WorldID = 0,
        FCarlaActor::IdType DesiredId = 0);
    
    // 将Actor移动到指定世界
    static bool MoveActorToWorld(
        UCarlaEpisode* Episode,
        FCarlaActor::IdType ActorId,
        int32 WorldID);
    
    // 获取Actor所在的世界
    static int32 GetActorWorldID(
        UCarlaEpisode* Episode,
        FCarlaActor::IdType ActorId);
    
    // 获取指定世界中的所有Actor
    static TArray<FCarlaActor*> GetActorsInWorld(
        UCarlaEpisode* Episode,
        int32 WorldID);
    
    // 初始化Episode的平行世界支持
    static void InitializeEpisodeForParallelWorlds(UCarlaEpisode* Episode);
    
    // 检查是否支持平行世界
    static bool SupportsParallelWorlds(UCarlaEpisode* Episode);
};
