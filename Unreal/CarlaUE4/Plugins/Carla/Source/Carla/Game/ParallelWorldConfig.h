// Plugins/Carla/Source/Carla/Game/ParallelWorldConfig.h

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
    
    // 获取/设置是否自动分配新Actor到默认世界
    bool GetAutoAssignToDefaultWorld() const { return bAutoAssignToDefaultWorld; }
    void SetAutoAssignToDefaultWorld(bool bAuto) { bAutoAssignToDefaultWorld = bAuto; }
    
    // 获取/设置是否启用调试日志
    bool GetDebugLogging() const { return bDebugLogging; }
    void SetDebugLogging(bool bDebug) { bDebugLogging = bDebug; }
    
    // 获取配置是否已加载
    bool IsConfigLoaded() const { return bConfigLoaded; }
    
    // 获取配置文件名
    static FString GetConfigFileName() { return TEXT("ParallelWorld.ini"); }
    
private:
    FParallelWorldConfig();
    
    // 配置值
    bool bEnableParallelWorlds = true;
    int32 MaxParallelWorlds = 4;
    int32 DefaultWorldID = 0;
    bool bAutoAssignToDefaultWorld = true;
    bool bDebugLogging = false;
    
    // 是否已加载配置
    bool bConfigLoaded = false;
};
