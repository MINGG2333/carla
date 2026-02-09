// Plugins/Carla/Source/Carla/ParallelWorld/WorldProperties.cpp

#include "Carla/ParallelWorld/WorldProperties.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/CollisionProfile.h"

UWorldProperties::UWorldProperties()
{
    WorldID = 0;
    WorldName = TEXT("DefaultWorld");
    CollisionChannel = EWorldCollisionChannel::World0;
    RenderLayerMask = 1;
}

// 将WorldID转换为UE4的碰撞通道
ECollisionChannel UWorldProperties::GetCollisionChannelForWorld(int32 InWorldID)
{
    // 为每个世界分配一个唯一的碰撞通道
    // 注意：最多支持16个世界，因为UE4碰撞通道有限
    switch (InWorldID)
    {
        case 0:  return ECC_GameTraceChannel1;    // 世界0
        case 1:  return ECC_GameTraceChannel2;    // 世界1
        case 2:  return ECC_GameTraceChannel3;    // 世界2
        case 3:  return ECC_GameTraceChannel4;    // 世界3
        case 4:  return ECC_GameTraceChannel5;    // 世界4
        case 5:  return ECC_GameTraceChannel6;    // 世界5
        case 6:  return ECC_GameTraceChannel7;    // 世界6
        case 7:  return ECC_GameTraceChannel8;    // 世界7
        case 8:  return ECC_GameTraceChannel9;    // 世界8
        case 9:  return ECC_GameTraceChannel10;   // 世界9
        case 10: return ECC_GameTraceChannel11;   // 世界10
        case 11: return ECC_GameTraceChannel12;   // 世界11
        case 12: return ECC_GameTraceChannel13;   // 世界12
        case 13: return ECC_GameTraceChannel14;   // 世界13
        case 14: return ECC_GameTraceChannel15;   // 世界14
        case 15: return ECC_GameTraceChannel16;   // 世界15
        default: return ECC_WorldDynamic;         // 默认世界
    }
}

// 获取共享世界的碰撞通道（所有世界都可见的物体）
ECollisionChannel UWorldProperties::GetSharedCollisionChannel()
{
    return ECC_WorldStatic;  // 使用静态通道作为共享通道
}

// 设置组件的碰撞响应
void UWorldProperties::SetupCollisionResponseForWorld(
    UPrimitiveComponent* Component, 
    int32 InWorldID
)
{
    if (!Component)
    {
        return;
    }
    
    // 获取当前世界的碰撞通道
    ECollisionChannel WorldChannel = GetCollisionChannelForWorld(InWorldID);
    ECollisionChannel SharedChannel = GetSharedCollisionChannel();
    
    // 首先设置为自定义预设
    Component->SetCollisionProfileName(UCollisionProfile::Custom_CollisionProfile_Name);
    
    // 设置对所有世界通道的响应
    for (int32 i = 0; i <= 15; ++i)
    {
        ECollisionChannel Channel = GetCollisionChannelForWorld(i);
        if (i == InWorldID)
        {
            // 同世界：阻挡
            Component->SetCollisionResponseToChannel(Channel, ECR_Block);
        }
        else
        {
            // 不同世界：忽略
            Component->SetCollisionResponseToChannel(Channel, ECR_Ignore);
        }
    }
    
    // 对共享通道：总是阻挡（建筑、道路等）
    Component->SetCollisionResponseToChannel(SharedChannel, ECR_Block);
    
    // 确保物理碰撞启用
    Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

// 获取渲染层掩码
int32 UWorldProperties::GetRenderLayerMaskForWorld(int32 InWorldID)
{
    // 使用位掩码：每个世界占用一个位
    // 例如：世界0 -> 1<<0 = 1，世界1 -> 1<<1 = 2
    if (InWorldID >= 0 && InWorldID < 32)
    {
        return (1 << InWorldID);
    }
    return 1; // 默认世界
}
