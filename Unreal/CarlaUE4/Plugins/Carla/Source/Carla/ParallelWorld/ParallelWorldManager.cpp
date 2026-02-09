// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldManager.cpp

#include "Carla/ParallelWorld/ParallelWorldManager.h"
#include "Carla/Actor/ActorRegistry.h"
#include "Engine/World.h"
#include "Misc/Paths.h"

// 静态单例实例
FParallelWorldManager* FParallelWorldManager::SingletonInstance = nullptr;

FParallelWorldManager& FParallelWorldManager::Get()
{
    if (!SingletonInstance)
    {
        // 创建单例实例
        SingletonInstance = new FParallelWorldManager();
        
        // 创建默认世界（世界0）
        SingletonInstance->CreateWorld(TEXT("DefaultWorld"));
        
        UE_LOG(LogCarla, Log, TEXT("ParallelWorldManager initialized"));
    }
    return *SingletonInstance;
}

FParallelWorldManager::FParallelWorldManager()
{
    // 初始化
    NextWorldID = 1; // 0保留给默认世界
}

FParallelWorldManager::~FParallelWorldManager()
{
    // 清理所有世界
    ClearAllWorlds();
}

int32 FParallelWorldManager::CreateWorld(const FString& WorldName)
{
    FScopeLock Lock(&WorldLock);
    
    // 检查是否达到世界数量限制
    if (Worlds.Num() >= MAX_WORLD_COUNT)
    {
        UE_LOG(LogCarla, Error, TEXT("Cannot create more than %d parallel worlds"), MAX_WORLD_COUNT);
        return -1;
    }
    
    // 创建新的世界ID
    int32 NewWorldID = NextWorldID++;
    
    // 创建世界属性
    UWorldProperties* NewWorld = NewObject<UWorldProperties>();
    NewWorld->WorldID = NewWorldID;
    NewWorld->WorldName = WorldName.IsEmpty() ? 
        FString::Printf(TEXT("World_%d"), NewWorldID) : WorldName;
    NewWorld->CollisionChannel = static_cast<EWorldCollisionChannel>(
        FMath::Clamp(NewWorldID, 0, static_cast<int32>(EWorldCollisionChannel::Shared))
    );
    NewWorld->RenderLayerMask = UWorldProperties::GetRenderLayerMaskForWorld(NewWorldID);
    
    // 添加到世界映射
    Worlds.Add(NewWorldID, NewWorld);
    
    // 初始化Actor映射
    ActorsByWorld.Add(NewWorldID, TArray<TSharedPtr<FParallelCarlaActor>>());
    
    UE_LOG(LogCarla, Log, 
        TEXT("Created parallel world %d: %s"), 
        NewWorldID, *NewWorld->WorldName
    );
    
    return NewWorldID;
}

bool FParallelWorldManager::DestroyWorld(int32 WorldID)
{
    FScopeLock Lock(&WorldLock);
    
    // 不能销毁默认世界（世界0）
    if (WorldID == 0)
    {
        UE_LOG(LogCarla, Warning, TEXT("Cannot destroy default world (World 0)"));
        return false;
    }
    
    // 检查世界是否存在
    if (!Worlds.Contains(WorldID))
    {
        UE_LOG(LogCarla, Warning, TEXT("World %d does not exist"), WorldID);
        return false;
    }
    
    // 获取该世界的所有Actor
    TArray<TSharedPtr<FParallelCarlaActor>>* WorldActors = ActorsByWorld.Find(WorldID);
    if (WorldActors)
    {
        // 警告：销毁世界前应该先处理所有Actor
        if (WorldActors->Num() > 0)
        {
            UE_LOG(LogCarla, Warning, 
                TEXT("Destroying world %d with %d actors still in it"), 
                WorldID, WorldActors->Num()
            );
        }
    }
    
    // 移除世界
    Worlds.Remove(WorldID);
    ActorsByWorld.Remove(WorldID);
    ActorToWorldMap.RemoveAll([WorldID](const TPair<FCarlaActor::IdType, int32>& Pair) {
        return Pair.Value == WorldID;
    });
    
    UE_LOG(LogCarla, Log, TEXT("Destroyed parallel world %d"), WorldID);
    
    return true;
}

UWorldProperties* FParallelWorldManager::GetWorldProperties(int32 WorldID)
{
    FScopeLock Lock(&WorldLock);
    
    UWorldProperties** FoundWorld = Worlds.Find(WorldID);
    return FoundWorld ? *FoundWorld : nullptr;
}

void FParallelWorldManager::RegisterActor(FCarlaActor* Actor, int32 WorldID)
{
    if (!Actor)
    {
        UE_LOG(LogCarla, Error, TEXT("Cannot register null actor"));
        return;
    }
    
    FScopeLock Lock(&WorldLock);
    
    // 检查世界是否存在
    if (!Worlds.Contains(WorldID))
    {
        UE_LOG(LogCarla, Warning, 
            TEXT("World %d does not exist, registering actor %d to default world"), 
            WorldID, Actor->GetActorId()
        );
        WorldID = 0; // 默认世界
    }
    
    FCarlaActor::IdType ActorId = Actor->GetActorId();
    
    // 检查是否已注册
    if (ActorToWorldMap.Contains(ActorId))
    {
        UE_LOG(LogCarla, Warning, 
            TEXT("Actor %d is already registered to world %d"), 
            ActorId, ActorToWorldMap[ActorId]
        );
        return;
    }
    
    // 创建并行Actor包装器
    TSharedPtr<FParallelCarlaActor> ParallelActor = 
        MakeShared<FParallelCarlaActor>(
            TSharedPtr<FCarlaActor>(Actor),  // 注意：这里需要小心管理生命周期
            WorldID
        );
    
    // 注册到映射
    ActorToWorldMap.Add(ActorId, WorldID);
    
    // 添加到世界Actor列表
    TArray<TSharedPtr<FParallelCarlaActor>>& WorldActors = ActorsByWorld.FindOrAdd(WorldID);
    WorldActors.Add(ParallelActor);
    
    UE_LOG(LogCarla, Log, 
        TEXT("Registered actor %d to world %d"), 
        ActorId, WorldID
    );
}

void FParallelWorldManager::UnregisterActor(FCarlaActor::IdType ActorId)
{
    FScopeLock Lock(&WorldLock);
    
    // 查找Actor所在的世界
    int32* WorldIDPtr = ActorToWorldMap.Find(ActorId);
    if (!WorldIDPtr)
    {
        UE_LOG(LogCarla, Warning, TEXT("Actor %d is not registered"), ActorId);
        return;
    }
    
    int32 WorldID = *WorldIDPtr;
    
    // 从世界Actor列表中移除
    TArray<TSharedPtr<FParallelCarlaActor>>* WorldActors = ActorsByWorld.Find(WorldID);
    if (WorldActors)
    {
        WorldActors->RemoveAll([ActorId](const TSharedPtr<FParallelCarlaActor>& ParallelActor) {
            return ParallelActor->GetActorId() == ActorId;
        });
    }
    
    // 从映射中移除
    ActorToWorldMap.Remove(ActorId);
    
    UE_LOG(LogCarla, Log, 
        TEXT("Unregistered actor %d from world %d"), 
        ActorId, WorldID
    );
}

int32 FParallelWorldManager::GetActorWorldID(FCarlaActor::IdType ActorId) const
{
    FScopeLock Lock(&WorldLock);
    
    const int32* WorldID = ActorToWorldMap.Find(ActorId);
    return WorldID ? *WorldID : 0; // 0表示未注册或默认世界
}

TArray<FParallelCarlaActor*> FParallelWorldManager::GetActorsInWorld(int32 WorldID) const
{
    FScopeLock Lock(&WorldLock);
    
    TArray<FParallelCarlaActor*> Result;
    
    const TArray<TSharedPtr<FParallelCarlaActor>>* WorldActors = ActorsByWorld.Find(WorldID);
    if (WorldActors)
    {
        for (const TSharedPtr<FParallelCarlaActor>& ParallelActor : *WorldActors)
        {
            Result.Add(ParallelActor.Get());
        }
    }
    
    return Result;
}

void FParallelWorldManager::ClearAllWorlds()
{
    FScopeLock Lock(&WorldLock);
    
    Worlds.Empty();
    ActorsByWorld.Empty();
    ActorToWorldMap.Empty();
    NextWorldID = 1;
    
    UE_LOG(LogCarla, Log, TEXT("Cleared all parallel worlds"));
}

void FParallelWorldManager::DebugPrintWorldInfo() const
{
    FScopeLock Lock(&WorldLock);
    
    UE_LOG(LogCarla, Log, TEXT("=== Parallel Worlds Info ==="));
    UE_LOG(LogCarla, Log, TEXT("Total Worlds: %d"), Worlds.Num());
    
    for (const auto& Pair : Worlds)
    {
        int32 WorldID = Pair.Key;
        UWorldProperties* WorldProps = Pair.Value;
        
        TArray<FParallelCarlaActor*> Actors = GetActorsInWorld(WorldID);
        
        UE_LOG(LogCarla, Log, 
            TEXT("  World %d: %s (Actors: %d)"), 
            WorldID, *WorldProps->WorldName, Actors.Num()
        );
    }
    
    UE_LOG(LogCarla, Log, TEXT("Total Registered Actors: %d"), ActorToWorldMap.Num());
}
