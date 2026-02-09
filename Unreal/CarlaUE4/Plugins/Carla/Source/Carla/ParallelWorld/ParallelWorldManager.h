// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldManager.h
#pragma once

#include "CoreMinimal.h"
#include "Carla/ParallelWorld/ParallelCarlaActor.h"  // 需要包含FParallelCarlaActor
#include "Carla/ParallelWorld/WorldProperties.h"     // 需要包含UWorldProperties
#include "Containers/Map.h"          // 用于TMap
#include "HAL/CriticalSection.h"     // 用于FCriticalSection
#include "Misc/ScopeLock.h"          // 用于FScopeLock
#include "Templates/SharedPointer.h" // 用于TSharedPtr
// FParallelWorldManager 不是UCLASS，纯C++类，所以不需要.generated.h

/**
 * 管理多个平行世界，提供Actor到世界的映射
 * 单例模式，全局访问
 */
class CARLA_API FParallelWorldManager
{
public:
    // 单例访问
    static FParallelWorldManager& Get();
    
    // 禁用拷贝和赋值
    FParallelWorldManager(const FParallelWorldManager&) = delete;
    FParallelWorldManager& operator=(const FParallelWorldManager&) = delete;
    
    // 世界管理
    int32 CreateWorld(const FString& WorldName = TEXT(""));
    bool DestroyWorld(int32 WorldID);
    UWorldProperties* GetWorldProperties(int32 WorldID);
    
    // Actor管理
    void RegisterActor(FCarlaActor* Actor, int32 WorldID);
    void UnregisterActor(FCarlaActor::IdType ActorId);
    
    // 查询
    int32 GetActorWorldID(FCarlaActor::IdType ActorId) const;
    TArray<FParallelCarlaActor*> GetActorsInWorld(int32 WorldID) const;
    
    // 世界数量
    int32 GetWorldCount() const { return Worlds.Num(); }
    
    // 调试
    void DebugPrintWorldInfo() const;
    
private:
    // 私有构造函数（单例）
    FParallelWorldManager();
    ~FParallelWorldManager();
    
    // 世界映射：WorldID -> WorldProperties
    TMap<int32, UWorldProperties*> Worlds;
    
    // Actor映射：ActorID -> ParallelCarlaActor
    TMap<FCarlaActor::IdType, TSharedPtr<FParallelCarlaActor>> ParallelActors;
    
    // 世界到Actor列表的映射
    TMap<int32, TArray<TSharedPtr<FParallelCarlaActor>>> ActorsByWorld;
    
    // ActorID到WorldID的映射
    TMap<FCarlaActor::IdType, int32> ActorToWorldMap;
    
    // 下一个可用的WorldID
    int32 NextWorldID = 1; // 0保留给默认世界
    
    // 最大世界数量限制（根据碰撞通道数量）
    static constexpr int32 MAX_WORLD_COUNT = 16;
    
    // 线程安全锁
    mutable FCriticalSection WorldLock;
    
    // 清理所有世界
    void ClearAllWorlds();
    
    // 单例实例指针
    static FParallelWorldManager* SingletonInstance;
};
