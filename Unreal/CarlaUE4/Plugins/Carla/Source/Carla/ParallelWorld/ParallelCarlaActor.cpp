// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaActor.cpp

#include "ParallelCarlaActor.h"
#include "Components/PrimitiveComponent.h"

FParallelCarlaActor::FParallelCarlaActor(
    TSharedPtr<FCarlaActor> InOriginalActor,
    int32 InWorldID
) : OriginalActor(InOriginalActor)
  , WorldID(InWorldID)
  , bWorldSettingsApplied(false)
{
    // 设置世界属性
    WorldProperties.WorldID = InWorldID;
    WorldProperties.WorldName = FString::Printf(TEXT("World_%d"), InWorldID);
    WorldProperties.bIsDefaultWorld = (InWorldID == 0);
    WorldProperties.CollisionChannel = FParallelWorldUtils::WorldIDToCollisionChannel(InWorldID);
    WorldProperties.RenderLayerMask = FParallelWorldUtils::WorldIDToRenderLayerMask(InWorldID);
    
    // 应用世界特定设置
    ApplyWorldSpecificSettings();
}

FParallelCarlaActor::~FParallelCarlaActor()
{
    // 清理工作
}

void FParallelCarlaActor::SetWorldID(int32 NewWorldID)
{
    if (WorldID != NewWorldID)
    {
        WorldID = NewWorldID;
        WorldProperties.WorldID = NewWorldID;
        WorldProperties.WorldName = FString::Printf(TEXT("World_%d"), NewWorldID);
        WorldProperties.bIsDefaultWorld = (NewWorldID == 0);
        WorldProperties.CollisionChannel = FParallelWorldUtils::WorldIDToCollisionChannel(NewWorldID);
        WorldProperties.RenderLayerMask = FParallelWorldUtils::WorldIDToRenderLayerMask(NewWorldID);
        
        // 重新应用设置
        ApplyWorldSpecificSettings();
    }
}

void FParallelCarlaActor::ApplyWorldSpecificSettings()
{
    UpdateCollisionSettings();
    UpdateRenderingSettings();
    bWorldSettingsApplied = true;
}

void FParallelCarlaActor::UpdateCollisionSettings()
{
    AActor* Actor = GetActor();
    if (!Actor) return;
    
    // 遍历所有PrimitiveComponent并设置碰撞
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    
    for (UPrimitiveComponent* Component : Components)
    {
        FParallelWorldUtils::SetupComponentCollisionForWorld(Component, WorldID);
    }
}

void FParallelCarlaActor::UpdateRenderingSettings()
{
    // 渲染设置将在第二阶段实现
    // 这里暂时留空
}

bool FParallelCarlaActor::IsSameActor(const AActor* Actor) const
{
    if (!OriginalActor || !Actor) return false;
    return OriginalActor->GetActor() == Actor;
}
