// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldManager.h

#pragma once

#include "Carla/ParallelWorld/ParallelCarlaActor.h"
#include "Containers/Map.h"
#include "CoreMinimal.h"

/**
 * 管理平行世界的单例类
 * 参考CARLA中其他管理器的实现方式（如TrafficLightManager）
 */
class CARLA_API FParallelWorldManager
{
public:
    // 单例访问（参考CARLA中其他管理器的实现）
    static FParallelWorldManager& GetInstance();
    
    // 初始化/清理
    void Initialize();
    void Reset();
    
    // 世界管理
    int32 CreateWorld(const FString& WorldName = TEXT(""));
    bool DestroyWorld(int32 WorldID);
    bool IsValidWorld(int32 WorldID) const;
    
    // Actor管理
    void RegisterActor(FCarlaActor* Actor, int32 WorldID);
    void UnregisterActor(FCarlaActor::IdType ActorId);
    void UpdateActorWorld(FCarlaActor::IdType ActorId, int32 NewWorldID);
    
    // 查询
    int32 GetActorWorldID(FCarlaActor::IdType ActorId) const;
    TArray<FCarlaActor::IdType> GetActorsInWorld(int32 WorldID) const;
    
    // 获取世界属性
    const FWorldProperties* GetWorldProperties(int32 WorldID) const;
    
    // 获取平行Actor
    TSharedPtr<FParallelCarlaActor> GetParallelActor(FCarlaActor::IdType ActorId);
    TSharedPtr<FParallelCarlaActor> GetParallelActor(const AActor* Actor);
    
    // 调试信息
    void DebugLogWorldInfo() const;
    FString GetDebugInfoString() const;
    
    // 检查功能是否启用
    bool IsParallelWorldEnabled() const { return bEnabled; }
    void SetParallelWorldEnabled(bool bEnable) { bEnabled = bEnable; }
    
private:
    // 私有构造函数
    FParallelWorldManager();
    
    // 禁用复制
    FParallelWorldManager(const FParallelWorldManager&) = delete;
    FParallelWorldManager& operator=(const FParallelWorldManager&) = delete;
    
    // Actor映射：ActorID -> ParallelCarlaActor
    TMap<FCarlaActor::IdType, TSharedPtr<FParallelCarlaActor>> ParallelActors;
    
    // Actor到世界ID的快速查找（用于性能优化）
    TMap<FCarlaActor::IdType, int32> ActorToWorldMap;
    
    // 世界ID到Actor列表的映射
    TMap<int32, TArray<FCarlaActor::IdType>> WorldToActorsMap;
    
    // 世界属性存储
    TMap<int32, FWorldProperties> WorldPropertiesMap;
    
    // 下一个可用的WorldID
    int32 NextWorldID;
    
    // 功能是否启用
    bool bEnabled;
    
    // 是否已初始化
    bool bInitialized;
};
