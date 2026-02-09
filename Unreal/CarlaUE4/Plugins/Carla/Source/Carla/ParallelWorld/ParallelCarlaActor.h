// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaActor.h
#pragma once

#include "CoreMinimal.h"
#include "Carla/Actor/CarlaActor.h"  // 需要包含FCarlaActor
#include "Carla/ParallelWorld/WorldProperties.h"  // 需要包含UWorldProperties
#include "Containers/Array.h"        // 用于 TArray
#include "UObject/WeakObjectPtr.h"   // 用于 TWeakObjectPtr
// FParallelCarlaActor 不是UCLASS，纯C++类，所以不需要.generated.h

/**
 * 包装原始的FCarlaActor，添加平行世界功能
 * 保持与FCarlaActor相同的接口，但通过组合扩展功能
 */
class CARLA_API FParallelCarlaActor
{
public:
    // 构造函数
    FParallelCarlaActor(
        TSharedPtr<FCarlaActor> OriginalActor,
        int32 InWorldID = 0
    );
    
    virtual ~FParallelCarlaActor();
    
    // 获取原始Actor（用于向后兼容）
    TSharedPtr<FCarlaActor> GetOriginalActor() const { return OriginalActor; }
    
    // 世界ID管理
    void SetWorldID(int32 NewWorldID);
    int32 GetWorldID() const { return WorldID; }
    
    // 世界属性
    UWorldProperties* GetWorldProperties() const { return WorldProperties; }
    
    // 代理原始Actor的关键方法
    FCarlaActor::IdType GetActorId() const;
    AActor* GetActor() const;
    const FActorInfo* GetActorInfo() const;
    FCarlaActor::ActorType GetActorType() const;
    
    // 物理设置：应用世界特定的碰撞设置
    void ApplyWorldCollisionSettings();
    
    // 渲染设置：应用世界特定的渲染设置
    void ApplyWorldRenderingSettings();
    
    // 调试函数
    void DebugPrint() const;
    
private:
    // 原始Actor（不修改）
    TSharedPtr<FCarlaActor> OriginalActor;
    
    // 世界ID
    int32 WorldID = 0;
    
    // 世界属性
    UWorldProperties* WorldProperties = nullptr;
};
