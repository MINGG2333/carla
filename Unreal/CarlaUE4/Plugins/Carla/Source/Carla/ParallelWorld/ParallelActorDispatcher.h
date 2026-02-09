// Plugins/Carla/Source/Carla/ParallelWorld/ParallelActorDispatcher.h
#pragma once

#include "CoreMinimal.h"
#include "Carla/Actor/ActorDispatcher.h"  // 继承自UActorDispatcher
#include "Carla/ParallelWorld/ParallelCarlaActor.h"  // 使用FParallelCarlaActor
#include "ParallelActorDispatcher.generated.h"  // UCLASS需要这个

/**
 * 扩展UActorDispatcher，添加平行世界支持
 * 通过继承并重写关键方法实现
 */
UCLASS()
class CARLA_API UParallelActorDispatcher : public UActorDispatcher
{
    GENERATED_BODY()
    
public:
    // 重写SpawnActor，添加WorldID参数
    virtual TPair<EActorSpawnResultStatus, FCarlaActor*> SpawnActor(
        const FTransform& Transform,
        FActorDescription ActorDescription,
        FCarlaActor::IdType DesiredId = 0,
        int32 WorldID = 0  // 新增参数
    ) override;
    
    // 新的SpawnActor接口（带有WorldID）
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    TPair<EActorSpawnResultStatus, FParallelCarlaActor*> SpawnActorInWorld(
        const FTransform& Transform,
        FActorDescription ActorDescription,
        int32 WorldID,
        FCarlaActor::IdType DesiredId = 0
    );
    
    // 在指定世界生成Actor（蓝图版本）
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    AActor* SpawnActorInWorldWithClass(
        UClass* ActorClass,
        const FTransform& Transform,
        int32 WorldID
    );
    
    // 注册Actor到世界
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    void RegisterActorToWorld(AActor* Actor, FActorDescription Description, int32 WorldID);
};
