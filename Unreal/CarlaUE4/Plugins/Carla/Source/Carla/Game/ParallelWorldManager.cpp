// Plugins/Carla/Source/Carla/Game/ParallelWorldManager.cpp

#include "Carla/Game/ParallelWorldManager.h"
#include "Carla/Game/CarlaEpisode.h"
#include "Carla/Actor/ActorRegistry.h"
#include "Carla/Game/ParallelWorldUtils.h"

UParallelWorldManager::UParallelWorldManager()
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

void UParallelWorldManager::Initialize()
{
    if (bInitialized) return;
    
    UE_LOG(LogCarla, Log, TEXT("ParallelWorldManager initialized"));
    bInitialized = true;
}

void UParallelWorldManager::Reset()
{
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

int32 UParallelWorldManager::CreateWorld(const FString& WorldName)
{
    if (!bEnabled) return -1;
    
    int32 NewWorldID = NextWorldID++;
    
    FWorldProperties NewWorld(NewWorldID, WorldName);
    NewWorld.bIsDefaultWorld = false;
    NewWorld.CollisionChannel = FParallelWorldUtils::WorldIDToCollisionChannel(NewWorldID);
    NewWorld.RenderLayerMask = FParallelWorldUtils::WorldIDToRenderLayerMask(NewWorldID);
    
    WorldPropertiesMap.Add(NewWorldID, NewWorld);
    WorldToActorsMap.Add(NewWorldID, TArray<FCarlaActor::IdType>());
    
    UE_LOG(LogCarla, Log, TEXT("Created parallel world %d: %s"), 
        NewWorldID, *NewWorld.WorldName);
    
    return NewWorldID;
}

bool UParallelWorldManager::DestroyWorld(int32 WorldID)
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
            UpdateActorWorld(ActorId, 0, nullptr);
        }
    }
    
    // 清理数据结构
    WorldPropertiesMap.Remove(WorldID);
    WorldToActorsMap.Remove(WorldID);
    
    UE_LOG(LogCarla, Log, TEXT("Destroyed parallel world %d"), WorldID);
    return true;
}

bool UParallelWorldManager::IsValidWorld(int32 WorldID) const
{
    return WorldPropertiesMap.Contains(WorldID);
}

void UParallelWorldManager::RegisterActor(FCarlaActor::IdType ActorId, int32 WorldID, AActor* Actor)
{
    if (!bEnabled) return;
    
    // 检查是否已注册
    if (ActorToWorldMap.Contains(ActorId))
    {
        UE_LOG(LogCarla, Warning, TEXT("Actor %d is already registered"), ActorId);
        return;
    }
    
    // 检查目标世界是否存在
    if (!WorldPropertiesMap.Contains(WorldID))
    {
        UE_LOG(LogCarla, Warning, TEXT("World %d does not exist, cannot register actor"), WorldID);
        return;
    }
    
    // 注册到映射
    ActorToWorldMap.Add(ActorId, WorldID);
    
    // 添加到世界Actor列表
    TArray<FCarlaActor::IdType>& ActorList = WorldToActorsMap.FindOrAdd(WorldID);
    ActorList.Add(ActorId);
    
    // 如果提供了Actor指针，应用世界设置
    if (Actor)
    {
        ApplyWorldSettingsToActor(Actor, WorldID);
    }
    
    UE_LOG(LogCarla, Verbose, TEXT("Registered actor %d to world %d"), 
        ActorId, WorldID);
}

void UParallelWorldManager::UnregisterActor(FCarlaActor::IdType ActorId)
{
    if (!bEnabled) return;
    
    // 获取Actor所在世界
    int32* WorldIDPtr = ActorToWorldMap.Find(ActorId);
    if (!WorldIDPtr)
    {
        UE_LOG(LogCarla, Warning, TEXT("Actor %d is not registered"), ActorId);
        return;
    }
    
    int32 WorldID = *WorldIDPtr;
    
    // 从世界Actor列表中移除
    TArray<FCarlaActor::IdType>* ActorList = WorldToActorsMap.Find(WorldID);
    if (ActorList)
    {
        ActorList->Remove(ActorId);
    }
    
    // 从映射中移除
    ActorToWorldMap.Remove(ActorId);
    
    UE_LOG(LogCarla, Verbose, TEXT("Unregistered actor %d from world %d"), 
        ActorId, WorldID);
}

void UParallelWorldManager::UpdateActorWorld(FCarlaActor::IdType ActorId, int32 NewWorldID, AActor* Actor)
{
    if (!bEnabled) return;
    
    // 检查新世界是否存在
    if (!WorldPropertiesMap.Contains(NewWorldID))
    {
        UE_LOG(LogCarla, Warning, TEXT("World %d does not exist"), NewWorldID);
        return;
    }
    
    // 获取Actor当前所在世界
    int32* CurrentWorldIDPtr = ActorToWorldMap.Find(ActorId);
    if (!CurrentWorldIDPtr)
    {
        // Actor未注册，先注册到新世界
        RegisterActor(ActorId, NewWorldID, Actor);
        return;
    }
    
    int32 CurrentWorldID = *CurrentWorldIDPtr;
    
    if (CurrentWorldID == NewWorldID)
    {
        return;  // 已经在目标世界
    }
    
    // 从旧世界列表移除
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
    
    // 如果提供了Actor指针，应用新世界设置
    if (Actor)
    {
        ApplyWorldSettingsToActor(Actor, NewWorldID);
    }
    
    UE_LOG(LogCarla, Log, TEXT("Moved actor %d from world %d to world %d"), 
        ActorId, CurrentWorldID, NewWorldID);
}

int32 UParallelWorldManager::GetActorWorldID(FCarlaActor::IdType ActorId) const
{
    const int32* WorldIDPtr = ActorToWorldMap.Find(ActorId);
    return WorldIDPtr ? *WorldIDPtr : 0;  // 默认返回0（默认世界）
}

TArray<FCarlaActor::IdType> UParallelWorldManager::GetActorsInWorld(int32 WorldID) const
{
    const TArray<FCarlaActor::IdType>* ActorList = WorldToActorsMap.Find(WorldID);
    if (ActorList)
    {
        return *ActorList;
    }
    return TArray<FCarlaActor::IdType>();
}

FWorldProperties UParallelWorldManager::GetWorldProperties(int32 WorldID) const
{
    const FWorldProperties* Properties = WorldPropertiesMap.Find(WorldID);
    if (Properties)
    {
        return *Properties;
    }
    
    // 返回空属性
    FWorldProperties Empty;
    Empty.WorldID = -1;
    return Empty;
}

void UParallelWorldManager::ApplyWorldSettingsToActor(AActor* Actor, int32 WorldID)
{
    if (!Actor || !bEnabled) return;
    
    // 获取世界属性
    const FWorldProperties* WorldProperties = WorldPropertiesMap.Find(WorldID);
    if (!WorldProperties)
    {
        UE_LOG(LogCarla, Warning, TEXT("World %d does not exist, cannot apply settings"), WorldID);
        return;
    }
    
    // 1. 设置碰撞通道
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    
    for (UPrimitiveComponent* Component : Components)
    {
        // 设置物体类型为本世界的碰撞通道
        Component->SetCollisionObjectType(
            FParallelWorldUtils::GetCollisionChannelForWorld(WorldID));
        
        // 设置与其他世界的碰撞响应
        FParallelWorldUtils::SetupComponentCollisionForWorld(Component, WorldID);
    }
    
    // 2. 设置渲染标记（TODO：第二阶段实现）
    // 为Actor添加自定义渲染层标记
    Actor->Tags.Add(FName(*FString::Printf(TEXT("ParallelWorld_%d"), WorldID)));
    
    UE_LOG(LogCarla, Verbose, TEXT("Applied world %d settings to actor %s"), 
        WorldID, *Actor->GetName());
}

void UParallelWorldManager::DebugLogWorldInfo() const
{
    UE_LOG(LogCarla, Log, TEXT("=== Parallel World Manager Info ==="));
    UE_LOG(LogCarla, Log, TEXT("Enabled: %s"), bEnabled ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogCarla, Log, TEXT("Initialized: %s"), bInitialized ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogCarla, Log, TEXT("World Count: %d"), WorldPropertiesMap.Num());
    UE_LOG(LogCarla, Log, TEXT("Registered Actors: %d"), ActorToWorldMap.Num());
    
    // 列出所有世界
    for (const auto& Pair : WorldPropertiesMap)
    {
        const FWorldProperties& World = Pair.Value;
        const TArray<FCarlaActor::IdType>* Actors = WorldToActorsMap.Find(World.WorldID);
        int32 ActorCount = Actors ? Actors->Num() : 0;
        
        UE_LOG(LogCarla, Log, TEXT("  World %d: %s (Actors: %d)"), 
            World.WorldID, *World.WorldName, ActorCount);
    }
    UE_LOG(LogCarla, Log, TEXT("==================================="));
}

TArray<int32> UParallelWorldManager::GetAllWorldIDs() const
{
    TArray<int32> WorldIDs;
    WorldPropertiesMap.GetKeys(WorldIDs);
    return WorldIDs;
}

bool UParallelWorldManager::MoveActorToWorld(FCarlaActor::IdType ActorId, int32_t NewWorldID, AActor* Actor)
{
    if (!bEnabled || !bInitialized)
    {
        return false;
    }
    
    // 检查新世界是否存在
    if (!WorldPropertiesMap.Contains(NewWorldID))
    {
        UE_LOG(LogCarla, Warning, TEXT("World %d does not exist"), NewWorldID);
        return false;
    }
    
    // 获取Actor当前所在世界
    int32* CurrentWorldIDPtr = ActorToWorldMap.Find(ActorId);
    if (!CurrentWorldIDPtr)
    {
        // Actor未注册，先注册到新世界
        RegisterActor(ActorId, NewWorldID, Actor);
        return true;
    }
    
    int32 CurrentWorldID = *CurrentWorldIDPtr;
    
    if (CurrentWorldID == NewWorldID)
    {
        return true; // 已经在目标世界
    }
    
    // 调用现有的UpdateActorWorld方法
    UpdateActorWorld(ActorId, NewWorldID, Actor);
    
    return true;
}
