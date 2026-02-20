// Plugins/Carla/Source/Carla/Game/ParallelWorldUtils.cpp

#include "Carla/Game/ParallelWorldUtils.h"
#include "Engine/EngineTypes.h"  // 添加这个头文件以支持ECollisionChannel
#include "Carla/Util/IniFile.h"
#include "Misc/ConfigCacheIni.h"

// 声明碰撞通道名称
static FName ParallelWorldChannelNames[] = {
    TEXT("ParallelWorld0"),
    TEXT("ParallelWorld1"),
    TEXT("ParallelWorld2"),
    TEXT("ParallelWorld3")
};

ECollisionChannel FParallelWorldUtils::GetCollisionChannelForWorld(int32 WorldID)
{
    // 映射WorldID到实际的碰撞通道
    static TMap<int32, ECollisionChannel> ChannelMap;
    static bool bInitialized = false;
    
    if (!bInitialized)
    {
        // 初始化映射
        const FString ConfigFile = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
        
        // 使用正确的INI读取方式
        FConfigCacheIni* ConfigCache = FConfigCacheIni::ForGame();
        if (ConfigCache && FPaths::FileExists(ConfigFile))
        {
            // 读取碰撞通道配置
            TArray<FString> ChannelResponses;
            ConfigCache->GetArray(TEXT("/Script/Engine.CollisionProfile"), 
                                 TEXT("DefaultChannelResponses"), 
                                 ChannelResponses, ConfigFile);
            
            // 解析通道定义
            for (int32 i = 0; i < ChannelResponses.Num(); ++i)
            {
                FString Response = ChannelResponses[i];
                // 查找ParallelWorld通道
                for (int32 worldIdx = 0; worldIdx < 4; ++worldIdx)
                {
                    if (Response.Contains(ParallelWorldChannelNames[worldIdx].ToString()))
                    {
                        // 提取通道号
                        FString ChannelNumStr;
                        int32 ChannelStart = Response.Find(TEXT("Channel=ECC_GameTraceChannel"));
                        if (ChannelStart != INDEX_NONE)
                        {
                            ChannelStart += 24; // "Channel=ECC_GameTraceChannel".Length()
                            int32 ChannelEnd = Response.Find(TEXT(","), ESearchCase::CaseSensitive, ESearchDir::FromStart, ChannelStart);
                            if (ChannelEnd != INDEX_NONE)
                            {
                                ChannelNumStr = Response.Mid(ChannelStart, ChannelEnd - ChannelStart);
                                int32 ChannelNum = FCString::Atoi(*ChannelNumStr);
                                
                                // 映射WorldID到碰撞通道
                                ECollisionChannel Channel = static_cast<ECollisionChannel>(
                                    ECollisionChannel::ECC_GameTraceChannel1 + ChannelNum - 1);
                                ChannelMap.Add(worldIdx, Channel);
                                
                                UE_LOG(LogCarla, Verbose, TEXT("Mapped World%d to ECC_GameTraceChannel%d"), 
                                       worldIdx, ChannelNum);
                            }
                        }
                        break;
                    }
                }
            }
        }
        
        // 如果没有找到配置，使用默认映射
        if (ChannelMap.Num() == 0)
        {
            UE_LOG(LogCarla, Warning, TEXT("Parallel world collision channels not found in DefaultEngine.ini, using default mapping"));
            
            // 默认映射
            ChannelMap.Add(0, ECollisionChannel::ECC_GameTraceChannel4);  // ParallelWorld0
            ChannelMap.Add(1, ECollisionChannel::ECC_GameTraceChannel5);  // ParallelWorld1
            ChannelMap.Add(2, ECollisionChannel::ECC_GameTraceChannel6);  // ParallelWorld2
            ChannelMap.Add(3, ECollisionChannel::ECC_GameTraceChannel7);  // ParallelWorld3
        }
        
        bInitialized = true;
    }
    
    // 返回对应世界的碰撞通道
    const ECollisionChannel* Channel = ChannelMap.Find(WorldID);
    return Channel ? *Channel : ECollisionChannel::ECC_WorldStatic;
}

int32 FParallelWorldUtils::GetRenderLayerMaskForWorld(int32 WorldID)
{
    // 简单映射：每个世界对应一个渲染层
    switch (WorldID)
    {
        case 0: return 1 << 0;  // 世界0使用第0位
        case 1: return 1 << 1;  // 世界1使用第1位
        case 2: return 1 << 2;  // 世界2使用第2位
        case 3: return 1 << 3;  // 世界3使用第3位
        default: return 1 << 0; // 默认使用世界0
    }
}

void FParallelWorldUtils::SetupComponentCollisionForWorld(
    UPrimitiveComponent* Component, 
    int32 WorldID)
{
    if (!Component) return;
    
    // 获取本世界的碰撞通道
    ECollisionChannel MyChannel = GetCollisionChannelForWorld(WorldID);
    
    // 设置物体类型
    Component->SetCollisionObjectType(MyChannel);
    
    // 设置对所有通道的默认响应为忽略
    Component->SetCollisionResponseToAllChannels(ECR_Ignore);
    
    // 对本世界通道设置为阻挡（同一世界的物体相互碰撞）
    Component->SetCollisionResponseToChannel(MyChannel, ECR_Block);
    
    // 对世界静态物体（建筑、道路）设置为阻挡（所有世界共享静态物体）
    Component->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    
    // 对车辆通道：默认忽略，除非在同一世界
    // 注意：车辆可能在不同世界，所以需要特殊处理
    Component->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
    
    // 对行人通道：默认忽略
    Component->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    
    // 对其他平行世界通道设置为忽略（不同世界的物体不碰撞）
    for (int32 OtherWorldID = 0; OtherWorldID < 4; ++OtherWorldID)
    {
        if (OtherWorldID != WorldID)
        {
            ECollisionChannel OtherChannel = GetCollisionChannelForWorld(OtherWorldID);
            Component->SetCollisionResponseToChannel(OtherChannel, ECR_Ignore);
        }
    }
    
    UE_LOG(LogCarla, VeryVerbose, TEXT("Setup collision for component %s in world %d"), 
           *Component->GetName(), WorldID);
}

EParallelCollisionChannel FParallelWorldUtils::WorldIDToCollisionChannel(int32 WorldID)
{
    switch (WorldID)
    {
        case 0: return EParallelCollisionChannel::PCC_World0;
        case 1: return EParallelCollisionChannel::PCC_World1;
        case 2: return EParallelCollisionChannel::PCC_World2;
        case 3: return EParallelCollisionChannel::PCC_World3;
        default: return EParallelCollisionChannel::PCC_World0;
    }
}

int32 FParallelWorldUtils::WorldIDToRenderLayerMask(int32 WorldID)
{
    return GetRenderLayerMaskForWorld(WorldID);
}

bool FParallelWorldUtils::ValidateCollisionChannelConfig()
{
    const FString ConfigFile = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
    
    if (!FPaths::FileExists(ConfigFile))
    {
        UE_LOG(LogCarla, Error, TEXT("DefaultEngine.ini not found at %s"), *ConfigFile);
        return false;
    }
    
    // 尝试读取一个平行世界通道来验证配置
    ECollisionChannel TestChannel = GetCollisionChannelForWorld(0);
    
    // 检查是否为有效的碰撞通道（不是默认的ECC_WorldStatic）
    if (TestChannel == ECollisionChannel::ECC_WorldStatic)
    {
        UE_LOG(LogCarla, Warning, 
            TEXT("Parallel world collision channels may not be properly configured in DefaultEngine.ini"));
        UE_LOG(LogCarla, Warning, 
            TEXT("Please ensure you have added the following to DefaultEngine.ini:"));
        UE_LOG(LogCarla, Warning, 
            TEXT("[/Script/Engine.CollisionProfile]"));
        
        for (int32 i = 0; i < 4; ++i)
        {
            UE_LOG(LogCarla, Warning, 
                TEXT("+DefaultChannelResponses=(Channel=ECC_GameTraceChannel%d,DefaultResponse=ECR_Ignore,bTraceType=False,bStaticObject=False,Name=\"ParallelWorld%d\")"),
                4 + i, i);
        }
        return false;
    }
    
    return true;
}
