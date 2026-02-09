// Plugins/Carla/Source/Carla/ParallelWorld/WorldProperties.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldProperties.generated.h"

// 为每个世界分配一个碰撞通道，最多支持8个平行世界
// 注意：UE4最多支持62个自定义碰撞通道，我们从中分配一部分给平行世界
UENUM(BlueprintType)
enum class EParallelCollisionChannel : uint8
{
    PCC_World0 UMETA(DisplayName = "World 0 (Default)"),
    PCC_World1 UMETA(DisplayName = "World 1"),
    PCC_World2 UMETA(DisplayName = "World 2"),
    PCC_World3 UMETA(DisplayName = "World 3"),
    PCC_Shared UMETA(DisplayName = "Shared Objects")
};

// 世界渲染层，使用位掩码实现
struct FParallelRenderLayer
{
    static constexpr uint32 World0 = 1 << 0;
    static constexpr uint32 World1 = 1 << 1;
    static constexpr uint32 World2 = 1 << 2;
    static constexpr uint32 World3 = 1 << 3;
    static constexpr uint32 Shared = 1 << 4;
};

// 轻量级的世界属性结构体，不继承UObject以简化
struct FWorldProperties
{
    // 世界ID
    int32 WorldID = 0;
    
    // 世界名称
    FString WorldName = TEXT("DefaultWorld");
    
    // 物理碰撞相关
    EParallelCollisionChannel CollisionChannel = EParallelCollisionChannel::PCC_World0;
    
    // 渲染相关
    uint32 RenderLayerMask = FParallelRenderLayer::World0;
    
    // 是否为默认世界
    bool bIsDefaultWorld = true;
    
    // 世界是否激活
    bool bIsActive = true;
};

// 工具函数类
class CARLA_API FParallelWorldUtils
{
public:
    // 获取世界的碰撞通道
    static ECollisionChannel GetCollisionChannelForWorld(int32 WorldID);
    
    // 获取世界的渲染层掩码
    static uint32 GetRenderLayerMaskForWorld(int32 WorldID);
    
    // 设置组件的碰撞响应
    static void SetupComponentCollisionForWorld(
        UPrimitiveComponent* Component, 
        int32 WorldID
    );
    
    // 转换WorldID到碰撞通道枚举值
    static EParallelCollisionChannel WorldIDToCollisionChannel(int32 WorldID);
    
    // 转换WorldID到渲染层掩码
    static uint32 WorldIDToRenderLayerMask(int32 WorldID);
};
