// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaActor.h

#pragma once

#include "Carla/Actor/CarlaActor.h"
#include "Carla/ParallelWorld/WorldProperties.h"

/**
 * 包装原始的FCarlaActor，添加平行世界支持
 * 不修改原始FCarlaActor，通过组合扩展功能
 */
class CARLA_API FParallelCarlaActor
{
public:
    // 构造函数
    FParallelCarlaActor(
        TSharedPtr<FCarlaActor> InOriginalActor,
        int32 InWorldID = 0
    );
    
    // 析构函数
    ~FParallelCarlaActor();
    
    // 获取原始Actor
    TSharedPtr<FCarlaActor> GetOriginalActor() const 
    { 
        return OriginalActor; 
    }
    
    FCarlaActor* GetOriginalActorPtr() const
    {
        return OriginalActor.Get();
    }
    
    // 世界ID管理
    void SetWorldID(int32 NewWorldID);
    int32 GetWorldID() const { return WorldID; }
    
    // 世界属性
    const FWorldProperties& GetWorldProperties() const { return WorldProperties; }
    
    // 代理原始Actor的关键方法（方便访问）
    FCarlaActor::IdType GetActorId() const 
    { 
        return OriginalActor ? OriginalActor->GetActorId() : 0; 
    }
    
    AActor* GetActor() const 
    { 
        return OriginalActor ? OriginalActor->GetActor() : nullptr; 
    }
    
    const FActorInfo* GetActorInfo() const 
    { 
        return OriginalActor ? OriginalActor->GetActorInfo() : nullptr; 
    }
    
    FCarlaActor::ActorType GetActorType() const 
    { 
        return OriginalActor ? OriginalActor->GetActorType() : FCarlaActor::ActorType::INVALID; 
    }
    
    // 世界特定设置
    void ApplyWorldSpecificSettings();
    
    // 更新物理碰撞设置
    void UpdateCollisionSettings();
    
    // 更新渲染设置
    void UpdateRenderingSettings();
    
    // 检查是否有效
    bool IsValid() const { return OriginalActor.IsValid(); }
    
    // 检查是否与给定Actor相同
    bool IsSameActor(const AActor* Actor) const;
    
private:
    // 原始Actor（不修改）
    TSharedPtr<FCarlaActor> OriginalActor;
    
    // 世界ID
    int32 WorldID;
    
    // 世界属性
    FWorldProperties WorldProperties;
    
    // 是否已应用世界设置
    bool bWorldSettingsApplied;
};
