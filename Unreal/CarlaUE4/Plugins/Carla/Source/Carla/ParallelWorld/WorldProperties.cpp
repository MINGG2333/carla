// Plugins/Carla/Source/Carla/ParallelWorld/WorldProperties.cpp

#include "Carla/ParallelWorld/WorldProperties.h"
#include "Components/PrimitiveComponent.h"

ECollisionChannel FParallelWorldUtils::GetCollisionChannelForWorld(int32 WorldID)
{
    // 这里需要先定义碰撞通道，可以在CarlaGameModeBase初始化时设置
    // 暂时返回默认通道
    return ECC_WorldStatic;
}

uint32 FParallelWorldUtils::GetRenderLayerMaskForWorld(int32 WorldID)
{
    switch (WorldID)
    {
        case 0: return FParallelRenderLayer::World0;
        case 1: return FParallelRenderLayer::World1;
        case 2: return FParallelRenderLayer::World2;
        case 3: return FParallelRenderLayer::World3;
        default: return FParallelRenderLayer::World0;
    }
}

void FParallelWorldUtils::SetupComponentCollisionForWorld(
    UPrimitiveComponent* Component, 
    int32 WorldID)
{
    if (!Component) return;
    
    // 获取本世界的碰撞通道
    ECollisionChannel MyChannel = GetCollisionChannelForWorld(WorldID);
    
    // 为所有可能的平行世界通道设置响应
    for (int32 OtherWorldID = 0; OtherWorldID < 4; ++OtherWorldID)
    {
        ECollisionChannel OtherChannel = GetCollisionChannelForWorld(OtherWorldID);
        
        if (OtherWorldID == WorldID)
        {
            // 同世界：阻挡
            Component->SetCollisionResponseToChannel(OtherChannel, ECR_Block);
        }
        else
        {
            // 不同世界：忽略
            Component->SetCollisionResponseToChannel(OtherChannel, ECR_Ignore);
        }
    }
    
    // 共享对象（建筑、道路）与所有世界碰撞
    // 使用预设的共享通道
    Component->SetCollisionResponseToChannel(
        GetCollisionChannelForWorld(-1),  // -1表示共享
        ECR_Block
    );
}

EParallelCollisionChannel FParallelWorldUtils::WorldIDToCollisionChannel(int32 WorldID)
{
    // 简单的映射：WorldID 0-3 对应 PCC_World0-PCC_World3
    switch (WorldID)
    {
        case 0: return EParallelCollisionChannel::PCC_World0;
        case 1: return EParallelCollisionChannel::PCC_World1;
        case 2: return EParallelCollisionChannel::PCC_World2;
        case 3: return EParallelCollisionChannel::PCC_World3;
        default:
            // 超出范围的ID使用World0
            return EParallelCollisionChannel::PCC_World0;
    }
}

uint32 FParallelWorldUtils::WorldIDToRenderLayerMask(int32 WorldID)
{
    // 这个函数其实和 GetRenderLayerMaskForWorld 功能相同
    // 为了保持一致性，我们调用已有的函数
    return GetRenderLayerMaskForWorld(WorldID);
}
