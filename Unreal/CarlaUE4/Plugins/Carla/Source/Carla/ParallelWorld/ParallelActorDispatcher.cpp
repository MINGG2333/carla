// Plugins/Carla/Source/Carla/ParallelWorld/ParallelActorDispatcher.cpp

#include "Carla/ParallelWorld/ParallelActorDispatcher.h"
#include "Carla/ParallelWorld/ParallelWorldManager.h"
#include "Carla/ParallelWorld/ParallelCarlaActor.h"
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h"
#include "Carla/Actor/CarlaActorFactory.h"
#include "Carla/Game/Tagger.h"

TPair<EActorSpawnResultStatus, FCarlaActor*> UParallelActorDispatcher::SpawnActor(
    const FTransform& Transform,
    FActorDescription Description,
    FCarlaActor::IdType DesiredId,
    int32 WorldID  // 新增参数，默认为0
)
{
    // 调用父类的SpawnActor
    auto Result = UActorDispatcher::SpawnActor(Transform, Description, DesiredId);
    
    // 如果生成成功，注册到平行世界管理器
    if (Result.Key == EActorSpawnResultStatus::Success && Result.Value)
    {
        // 默认WorldID为0（默认世界）
        if (WorldID < 0) WorldID = 0;
        
        // 注册Actor到平行世界
        FParallelWorldManager::Get().RegisterActor(Result.Value, WorldID);
        
        UE_LOG(LogCarla, Log, 
            TEXT("Spawned actor %d in world %d"), 
            Result.Value->GetActorId(), WorldID
        );
    }
    
    return Result;
}

TPair<EActorSpawnResultStatus, FParallelCarlaActor*> UParallelActorDispatcher::SpawnActorInWorld(
    const FTransform& Transform,
    FActorDescription ActorDescription,
    int32 WorldID,
    FCarlaActor::IdType DesiredId
)
{
    // 调用重写的SpawnActor方法
    auto Result = SpawnActor(Transform, ActorDescription, DesiredId, WorldID);
    
    if (Result.Key == EActorSpawnResultStatus::Success)
    {
        // 获取对应的ParallelCarlaActor
        FCarlaActor::IdType ActorId = Result.Value->GetActorId();
        
        // 这里需要从ParallelWorldManager获取，但我们需要一个方法
        // 暂时返回null，稍后实现
        return MakeTuple(Result.Key, nullptr);
    }
    
    return MakeTuple(Result.Key, nullptr);
}

AActor* UParallelActorDispatcher::SpawnActorInWorldWithClass(
    UClass* ActorClass,
    const FTransform& Transform,
    int32 WorldID
)
{
    if (!ActorClass)
    {
        UE_LOG(LogCarla, Error, TEXT("Invalid actor class"));
        return nullptr;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogCarla, Error, TEXT("Cannot get world"));
        return nullptr;
    }
    
    // 生成Actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AActor* Actor = World->SpawnActor<AActor>(ActorClass, Transform, SpawnParams);
    
    if (Actor)
    {
        // 注册到ActorDispatcher（这会创建FCarlaActor）
        FActorDescription Description;
        Description.Id = TEXT("custom.actor");
        Description.Class = ActorClass;
        
        FCarlaActor* CarlaActor = RegisterActor(*Actor, Description);
        if (CarlaActor)
        {
            // 注册到平行世界
            FParallelWorldManager::Get().RegisterActor(CarlaActor, WorldID);
            
            // 标记标签
            ATagger::TagActor(*Actor, true);
            
            UE_LOG(LogCarla, Log, 
                TEXT("Spawned custom actor %s in world %d"), 
                *Actor->GetName(), WorldID
            );
        }
    }
    
    return Actor;
}
