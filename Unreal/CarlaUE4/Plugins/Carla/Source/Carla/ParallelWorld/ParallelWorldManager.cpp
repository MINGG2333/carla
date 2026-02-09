// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldManager.cpp

#include "ParallelWorldManager.h"
#include "Carla/Game/CarlaEpisode.h"
#include "Carla/Actor/ActorRegistry.h"

// 单例实例
static FParallelWorldManager* GParallelWorldManager = nullptr;

FParallelWorldManager& FParallelWorldManager::GetInstance()
{
    if (!GParallelWorldManager)
    {
        GParallelWorldManager = new FParallelWorldManager();
    }
    return *GParallelWorldManager;
}

FParallelWorldManager::FParallelWorldManager()
    : NextWorldID(1)  // 0是默认世界
    , bEnabled(true)
    , bInitialized(false)
{
    // 创建默认世界
    FWorldProperties DefaultWorld;
    DefaultWorld.WorldID = 0;
    DefaultWorld.WorldName = TEXT("DefaultWorld");
    DefaultWorld.bIsDefaultWorld = true;
    DefaultWorld.CollisionChannel = EParallelCollisionChannel::PCC_World0;
    DefaultWorld.RenderLayerMask = FParallelRenderLayer::World0;
    
    WorldPropertiesMap.Add(0, DefaultWorld);
}

void FParallelWorldManager::Initialize()
{
    if (bInitialized) return;
    
    UE_LOG(LogCarla, Log, TEXT("ParallelWorldManager initialized"));
    bInitialized = true;
}

void FParallelWorldManager::Reset()
{
    ParallelActors.Empty();
    ActorToWorldMap.Empty();
    WorldToActorsMap.Empty();
    
    // 保留默认世界
    WorldPropertiesMap.Empty();
    
    FWorldProperties DefaultWorld;
    DefaultWorld.WorldID = 0;
    DefaultWorld.WorldName = TEXT("DefaultWorld");
    DefaultWorld.bIsDefaultWorld = true;
    DefaultWorld.CollisionChannel = EParallelCollisionChannel::PCC_World0;
    DefaultWorld.RenderLayerMask = FParallelRenderLayer::World0;
    
    WorldPropertiesMap.Add(0, DefaultWorld);
    
    NextWorldID = 1;
    UE_LOG(LogCarla, Log, TEXT("ParallelWorldManager reset"));
}

int32 FParallelWorldManager::CreateWorld(const FString& WorldName)
{
    if (!bEnabled) return -1;
    
    int32 NewWorldID = NextWorldID++;
    
    FWorldProperties NewWorld;
    NewWorld.WorldID = NewWorldID;
    NewWorld.WorldName = WorldName.IsEmpty() ? 
        FString::Printf(TEXT("World_%d"), NewWorldID) : WorldName;
    NewWorld.bIsDefaultWorld = false;
    NewWorld.CollisionChannel = FParallelWorldUtils::WorldIDToCollisionChannel(NewWorldID);
    NewWorld.RenderLayerMask = FParallelWorldUtils::WorldIDToRenderLayerMask(NewWorldID);
    
    WorldPropertiesMap.Add(NewWorldID, NewWorld);
    WorldToActorsMap.Add(NewWorldID, TArray<FCarlaActor::IdType>());
    
    UE_LOG(LogCarla, Log, TEXT("Created parallel world %d: %s"), 
        NewWorldID, *NewWorld.WorldName);
    
    return NewWorldID;
}

bool FParallelWorldManager::DestroyWorld(int32 WorldID)
{
    if (WorldID == 0)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot destroy default world (ID=0)"));
        return false;
    }
    
    if (!WorldPropertiesMap.Contains(WorldID))
    {
        UE_LOG(LogCarla, Warning, TEXT("World %d does not exist"), WorldID);
        return false;
    }
    
    // 获取该世界的所有Actor
    TArray<FCarlaActor::IdType>* ActorList = WorldToActorsMap.Find(WorldID);
    if (ActorList)
    {
        // 将Actor移回默认世界
        for (FCarlaActor::IdType ActorId : *ActorList)
        {
            UpdateActorWorld(ActorId, 0);
        }
    }
    
    // 清理数据结构
    WorldPropertiesMap.Remove(WorldID);
    WorldToActorsMap.Remove(WorldID);
    
    UE_LOG(LogCarla, Log, TEXT("Destroyed parallel world %d"), WorldID);
    return true;
}

void FParallelWorldManager::RegisterActor(FCarlaActor* Actor, int32 WorldID)
{
    if (!Actor || !bEnabled) return;
    
    FCarlaActor::IdType ActorId = Actor->GetActorId();
    
    // 检查是否已注册
    if (ParallelActors.Contains(ActorId))
    {
        UE_LOG(LogCarla, Warning, TEXT("Actor %d is already registered"), ActorId);
        return;
    }
    
    // 创建并行Actor包装器
    TSharedPtr<FParallelCarlaActor> ParallelActor = 
        MakeShared<FParallelCarlaActor>(
            TSharedPtr<FCarlaActor>(Actor),  // 注意：这里共享指针，需要确保生命周期
            WorldID
        );
    
    // 注册到映射
    ParallelActors.Add(ActorId, ParallelActor);
    ActorToWorldMap.Add(ActorId, WorldID);
    
    // 添加到世界Actor列表
    TArray<FCarlaActor::IdType>& ActorList = WorldToActorsMap.FindOrAdd(WorldID);
    ActorList.Add(ActorId);
    
    UE_LOG(LogCarla, Verbose, TEXT("Registered actor %d to world %d"), 
        ActorId, WorldID);
}

void FParallelWorldManager::UnregisterActor(FCarlaActor::IdType ActorId)
{
    if (!bEnabled) return;
    
    // 从世界Actor列表中移除
    int32* WorldIDPtr = ActorToWorldMap.Find(ActorId);
    if (WorldIDPtr)
    {
        TArray<FCarlaActor::IdType>* ActorList = WorldToActorsMap.Find(*WorldIDPtr);
        if (ActorList)
        {
            ActorList->Remove(ActorId);
        }
    }
    
    // 从映射中移除
    ParallelActors.Remove(ActorId);
    ActorToWorldMap.Remove(ActorId);
    
    UE_LOG(LogCarla, Verbose, TEXT("Unregistered actor %d"), ActorId);
}

void FParallelWorldManager::UpdateActorWorld(FCarlaActor::IdType ActorId, int32 NewWorldID)
{
    if (!bEnabled) return;
    
    // 获取当前世界ID
    int32* CurrentWorldIDPtr = ActorToWorldMap.Find(ActorId);
    if (!CurrentWorldIDPtr)
    {
        UE_LOG(LogCarla, Warning, TEXT("Actor %d not found"), ActorId);
        return;
    }
    
    int32 CurrentWorldID = *CurrentWorldIDPtr;
    if (CurrentWorldID == NewWorldID) return;
    
    // 从旧世界列表中移除
    TArray<FCarlaActor::IdType>* OldActorList = WorldToActorsMap.Find(CurrentWorldID);
    if (OldActorList)
    {
        OldActorList->Remove(ActorId);
    }
    
    // 添加到新世界列表
    TArray<FCarlaActor::IdType>& NewActorList = WorldToActorsMap.FindOrAdd(NewWorldID);
    NewActorList.Add(ActorId);
    
    // 更新映射
    ActorToWorldMap[ActorId] = NewWorldID;
    
    // 更新ParallelActor的世界ID
    TSharedPtr<FParallelCarlaActor>* ParallelActorPtr = ParallelActors.Find(ActorId);
    if (ParallelActorPtr && ParallelActorPtr->IsValid())
    {
        (*ParallelActorPtr)->SetWorldID(NewWorldID);
    }
    
    UE_LOG(LogCarla, Log, TEXT("Moved actor %d from world %d to world %d"), 
        ActorId, CurrentWorldID, NewWorldID);
}

int32 FParallelWorldManager::GetActorWorldID(FCarlaActor::IdType ActorId) const
{
    const int32* WorldIDPtr = ActorToWorldMap.Find(ActorId);
    return WorldIDPtr ? *WorldIDPtr : 0;  // 默认返回0（默认世界）
}

TArray<FCarlaActor::IdType> FParallelWorldManager::GetActorsInWorld(int32 WorldID) const
{
    const TArray<FCarlaActor::IdType>* ActorList = WorldToActorsMap.Find(WorldID);
    return ActorList ? *ActorList : TArray<FCarlaActor::IdType>();
}

const FWorldProperties* FParallelWorldManager::GetWorldProperties(int32 WorldID) const
{
    return WorldPropertiesMap.Find(WorldID);
}

TSharedPtr<FParallelCarlaActor> FParallelWorldManager::GetParallelActor(FCarlaActor::IdType ActorId)
{
    TSharedPtr<FParallelCarlaActor>* Result = ParallelActors.Find(ActorId);
    return Result ? *Result : nullptr;
}

void FParallelWorldManager::DebugLogWorldInfo() const
{
    UE_LOG(LogCarla, Log, TEXT("=== Parallel World Manager Info ==="));
    UE_LOG(LogCarla, Log, TEXT("Enabled: %s"), bEnabled ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogCarla, Log, TEXT("World Count: %d"), WorldPropertiesMap.Num());
    
    for (const auto& Pair : WorldPropertiesMap)
    {
        const FWorldProperties& Props = Pair.Value;
        TArray<FCarlaActor::IdType> Actors = GetActorsInWorld(Props.WorldID);
        
        UE_LOG(LogCarla, Log, TEXT("  World %d: %s (%d actors)"), 
            Props.WorldID, *Props.WorldName, Actors.Num());
    }
    
    UE_LOG(LogCarla, Log, TEXT("Total Registered Actors: %d"), ParallelActors.Num());
}
