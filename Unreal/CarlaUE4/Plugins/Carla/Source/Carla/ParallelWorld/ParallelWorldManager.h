// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldManager.h

#pragma once

#include "Containers/Map.h"
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Carla/ParallelWorld/WorldProperties.h"
#include "Carla/Actor/CarlaActor.h"
#include "ParallelWorldManager.generated.h"

/**
 * 管理平行世界的UCLASS
 * 简化版本：只存储映射关系，不管理actor生命周期
 */
UCLASS(BlueprintType)
class CARLA_API UParallelWorldManager : public UObject
{
    GENERATED_BODY()
    
public:
    // 单例访问
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    static UParallelWorldManager* GetInstance();
    
    // 初始化/清理
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    void Initialize();
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    void Reset();
    
    // 世界管理
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    int32 CreateWorld(const FString& WorldName = TEXT(""));
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    bool DestroyWorld(int32 WorldID);
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    bool IsValidWorld(int32 WorldID) const;
    
    // Actor管理（使用ActorId）
    void RegisterActor(FCarlaActor::IdType ActorId, int32 WorldID, AActor* Actor = nullptr);
    void UnregisterActor(FCarlaActor::IdType ActorId);
    void UpdateActorWorld(FCarlaActor::IdType ActorId, int32 NewWorldID, AActor* Actor = nullptr);
    
    // 查询
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    int32 GetActorWorldID(FCarlaActor::IdType ActorId) const;
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    TArray<FCarlaActor::IdType> GetActorsInWorld(int32 WorldID) const;
    
    // 获取世界属性
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    FWorldProperties GetWorldProperties(int32 WorldID) const;
    
    // 获取世界数量
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    int32 GetWorldCount() const { return WorldPropertiesMap.Num(); }
    
    // 调试信息
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    void DebugLogWorldInfo() const;
    
    // 检查功能是否启用
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    bool IsParallelWorldEnabled() const { return bEnabled; }
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    void SetParallelWorldEnabled(bool bEnable) { bEnabled = bEnable; }
    
    // 获取所有世界ID
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    TArray<int32> GetAllWorldIDs() const;
    
    // 应用世界设置到actor（如果提供了actor指针）
    void ApplyWorldSettingsToActor(AActor* Actor, int32 WorldID);
    
private:
    // 私有构造函数
    UParallelWorldManager();
    
    // Actor到世界ID的映射
    TMap<FCarlaActor::IdType, int32> ActorToWorldMap;
    
    // 世界ID到Actor列表的映射
    TMap<int32, TArray<FCarlaActor::IdType>> WorldToActorsMap;
    
    // 世界属性存储
    TMap<int32, FWorldProperties> WorldPropertiesMap;
    
    // 下一个可用的WorldID
    int32 NextWorldID;
    
    // 功能是否启用
    bool bEnabled;
    
    // 是否已初始化
    bool bInitialized;
    
    // 静态实例
    static UParallelWorldManager* GParallelWorldManager;
};
