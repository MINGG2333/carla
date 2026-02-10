// Plugins/Carla/Source/Carla/ParallelWorld/ParallelCarlaGameMode.h

#pragma once

#include "GameFramework/GameModeBase.h"
#include "Carla/Game/CarlaGameModeBase.h"  // 继承自原有GameMode
#include "ParallelWorld/ParallelWorldManager.h"
#include "ParallelCarlaGameMode.generated.h"

/**
 * 扩展CarlaGameMode以支持平行世界
 * 可以直接替代原有的CarlaGameMode
 */
UCLASS()
class CARLA_API AParallelCarlaGameMode : public ACarlaGameModeBase  // 注意：继承自ACarlaGameModeBase
{
    GENERATED_BODY()
    
public:
    AParallelCarlaGameMode(const FObjectInitializer& ObjectInitializer);
    
    // 重写关键函数来集成平行世界系统
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    // 平行世界相关功能（通过CarlaServer暴露给Python）
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    int32 CreateParallelWorld(const FString& WorldName = TEXT(""));
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    bool DestroyParallelWorld(int32 WorldID);
    
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    TArray<int32> GetAvailableWorlds() const;
    
    // 获取平行世界管理器
    UFUNCTION(BlueprintCallable, Category="Parallel World")
    FParallelWorldManager& GetParallelWorldManager() const;
    
protected:
    // 是否启用平行世界（可通过配置文件控制）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World")
    bool bEnableParallelWorlds = true;
    
    // 最大平行世界数量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World", meta=(ClampMin="1", ClampMax="8"))
    int32 MaxParallelWorlds = 4;
    
private:
    // 初始化平行世界碰撞通道
    void InitializeParallelCollisionChannels();
};
