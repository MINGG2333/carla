// Plugins/Carla/Source/Carla/Game/ParallelWorldUtils.h

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Carla/Game/ParallelWorldTypes.h"

// 工具函数类
class CARLA_API FParallelWorldUtils
{
public:
    // 获取世界的碰撞通道
    static ECollisionChannel GetCollisionChannelForWorld(int32 WorldID);
    
    // 获取世界的渲染层掩码
    static int32 GetRenderLayerMaskForWorld(int32 WorldID);
    
    // 设置组件的碰撞响应
    static void SetupComponentCollisionForWorld(
        UPrimitiveComponent* Component, 
        int32 WorldID
    );
    
    // 转换WorldID到碰撞通道枚举值
    static EParallelCollisionChannel WorldIDToCollisionChannel(int32 WorldID);
    
    // 转换WorldID到渲染层掩码
    static int32 WorldIDToRenderLayerMask(int32 WorldID);
    
    // 验证碰撞通道配置（仅用于调试）
    static bool ValidateCollisionChannelConfig();
};
