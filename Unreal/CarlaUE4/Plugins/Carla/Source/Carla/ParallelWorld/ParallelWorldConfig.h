// Plugins/Carla/Source/Carla/ParallelWorld/ParallelWorldConfig.h

#pragma once

#include "CoreMinimal.h"

/**
 * 平行世界的配置管理
 * 可以从INI文件读取配置
 */
class CARLA_API FParallelWorldConfig
{
public:
    // 单例访问
    static FParallelWorldConfig& Get();
    
    // 读取配置文件
    bool LoadConfig();
    
    // 保存配置文件
    bool SaveConfig();
    
    // 获取/设置是否启用平行世界
    bool IsParallelWorldEnabled() const { return bEnableParallelWorlds; }
    void SetParallelWorldEnabled(bool bEnable) { bEnableParallelWorlds = bEnable; }
    
    // 获取/设置最大平行世界数量
    int32 GetMaxParallelWorlds() const { return MaxParallelWorlds; }
    void SetMaxParallelWorlds(int32 Max) { MaxParallelWorlds = Max; }
    
    // 获取/设置默认世界ID
    int32 GetDefaultWorldID() const { return DefaultWorldID; }
    
    // 获取配置文件名
    static FString GetConfigFileName() { return TEXT("ParallelWorld.ini"); }
    
private:
    FParallelWorldConfig();
    
    // 配置值
    bool bEnableParallelWorlds = true;
    int32 MaxParallelWorlds = 4;
    int32 DefaultWorldID = 0;
    
    // 是否已加载配置
    bool bConfigLoaded = false;
};
