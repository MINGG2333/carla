// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldManager.cpp

#include "ParallelWorldManager.h"
#include "Carla/Game/CarlaEpisode.h"
#include "Carla/Actor/ActorRegistry.h"

// 单例实例
UParallelWorldManager* UParallelWorldManager::GParallelWorldManager = nullptr;

UParallelWorldManager* UParallelWorldManager::GetInstance()
{
    if (!GParallelWorldManager)
    {
// 创建一个新的实例
        GParallelWorldManager = NewObject<UParallelWorldManager>();
        GParallelWorldManager->AddToRoot();  // 防止垃圾回收
    }
    return GParallelWorldManager;
}

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
    
    // 应用碰撞设置
    // 遍历所有PrimitiveComponent并设置碰撞
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    
    for (UPrimitiveComponent* Component : Components)
    {
        FParallelWorldUtils::SetupComponentCollisionForWorld(Component, WorldID);
    }
    
    // 应用渲染设置（待实现）
    // TODO: 根据WorldProperties->RenderLayerMask设置渲染层
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
