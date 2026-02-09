// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaActor.cpp

#include "Carla/ParallelWorld/ParallelCarlaActor.h"
#include "Carla/Actor/ActorInfo.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Engine.h"

FParallelCarlaActor::FParallelCarlaActor(
    TSharedPtr<FCarlaActor> InOriginalActor,
    int32 InWorldID
) : OriginalActor(InOriginalActor), WorldID(InWorldID)
{
    // 创建世界属性
    WorldProperties = NewObject<UWorldProperties>();
    WorldProperties->WorldID = InWorldID;
    WorldProperties->WorldName = FString::Printf(TEXT("World_%d"), InWorldID);
    WorldProperties->CollisionChannel = static_cast<EWorldCollisionChannel>(
        FMath::Clamp(InWorldID, 0, static_cast<int32>(EWorldCollisionChannel::Shared))
    );
    WorldProperties->RenderLayerMask = UWorldProperties::GetRenderLayerMaskForWorld(InWorldID);
    
    // 应用世界特定的设置
    ApplyWorldCollisionSettings();
    ApplyWorldRenderingSettings();
    
    UE_LOG(LogCarla, Log, 
        TEXT("Created ParallelCarlaActor for actor %d in world %d"), 
        GetActorId(), WorldID
    );
}

FParallelCarlaActor::~FParallelCarlaActor()
{
    // 清理资源
}

void FParallelCarlaActor::SetWorldID(int32 NewWorldID)
{
    if (WorldID != NewWorldID)
    {
        WorldID = NewWorldID;
        WorldProperties->WorldID = NewWorldID;
        WorldProperties->CollisionChannel = static_cast<EWorldCollisionChannel>(
            FMath::Clamp(NewWorldID, 0, static_cast<int32>(EWorldCollisionChannel::Shared))
        );
        WorldProperties->RenderLayerMask = UWorldProperties::GetRenderLayerMaskForWorld(NewWorldID);
        
        // 重新应用设置
        ApplyWorldCollisionSettings();
        ApplyWorldRenderingSettings();
        
        UE_LOG(LogCarla, Log, 
            TEXT("Actor %d moved from world %d to world %d"), 
            GetActorId(), WorldID, NewWorldID
        );
    }
}

FCarlaActor::IdType FParallelCarlaActor::GetActorId() const
{
    return OriginalActor ? OriginalActor->GetActorId() : 0;
}

AActor* FParallelCarlaActor::GetActor() const
{
    return OriginalActor ? OriginalActor->GetActor() : nullptr;
}

const FActorInfo* FParallelCarlaActor::GetActorInfo() const
{
    return OriginalActor ? OriginalActor->GetActorInfo() : nullptr;
}

FCarlaActor::ActorType FParallelCarlaActor::GetActorType() const
{
    return OriginalActor ? OriginalActor->GetActorType() : FCarlaActor::ActorType::INVALID;
}

void FParallelCarlaActor::ApplyWorldCollisionSettings()
{
    if (AActor* Actor = GetActor())
    {
        // 遍历所有Primitive组件
        TArray<UPrimitiveComponent*> Components;
        Actor->GetComponents<UPrimitiveComponent>(Components);
        
        for (UPrimitiveComponent* Component : Components)
        {
            if (Component)
            {
                UWorldProperties::SetupCollisionResponseForWorld(Component, WorldID);
                
                // 特殊处理：车辆和行人的碰撞
                if (GetActorType() == FCarlaActor::ActorType::Vehicle ||
                    GetActorType() == FCarlaActor::ActorType::Walker)
                {
                    // 确保车辆/行人对其他世界的物体不可碰撞
                    for (int32 OtherWorldID = 0; OtherWorldID <= 15; ++OtherWorldID)
                    {
                        if (OtherWorldID != WorldID)
                        {
                            ECollisionChannel OtherChannel = UWorldProperties::GetCollisionChannelForWorld(OtherWorldID);
                            Component->SetCollisionResponseToChannel(OtherChannel, ECR_Ignore);
                        }
                    }
                }
            }
        }
        
        UE_LOG(LogCarla, Verbose, 
            TEXT("Applied collision settings for actor %d in world %d"), 
            GetActorId(), WorldID
        );
    }
}

void FParallelCarlaActor::ApplyWorldRenderingSettings()
{
    if (AActor* Actor = GetActor())
    {
        // 获取渲染层掩码
        int32 RenderMask = WorldProperties->RenderLayerMask;
        
        // 遍历所有Mesh组件
        TArray<UMeshComponent*> MeshComponents;
        Actor->GetComponents<UMeshComponent>(MeshComponents);
        
        for (UMeshComponent* MeshComponent : MeshComponents)
        {
            if (MeshComponent)
            {
                // 设置渲染层（通过自定义深度）
                // 注意：这里需要修改SceneCaptureComponent的过滤设置，稍后实现
                MeshComponent->SetCustomDepthStencilValue(WorldID);
                MeshComponent->SetRenderCustomDepth(true);
            }
        }
        
        UE_LOG(LogCarla, Verbose, 
            TEXT("Applied rendering settings for actor %d in world %d (RenderMask: %d)"), 
            GetActorId(), WorldID, RenderMask
        );
    }
}

// 辅助函数：打印调试信息
void FParallelCarlaActor::DebugPrint() const
{
    FString ActorName = GetActor() ? GetActor()->GetName() : TEXT("None");
    UE_LOG(LogCarla, Log, 
        TEXT("ParallelCarlaActor: ID=%d, World=%d, Actor=%s, Type=%d"),
        GetActorId(), WorldID, *ActorName, static_cast<int32>(GetActorType())
    );
}
