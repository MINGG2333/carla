// Plugins/Carla/Source/Carla/ParallelWorld/WorldProperties.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldProperties.generated.h"

// 为每个世界分配一个碰撞通道，最多支持8个平行世界
// 注意：UE4最多支持62个自定义碰撞通道，我们从中分配一部分给平行世界
UENUM(BlueprintType)
enum class EParallelCollisionChannel : uint8
{
    PCC_World0 UMETA(DisplayName = "World 0 (Default)"),
    PCC_World1 UMETA(DisplayName = "World 1"),
    PCC_World2 UMETA(DisplayName = "World 2"),
    PCC_World3 UMETA(DisplayName = "World 3"),
    PCC_Shared UMETA(DisplayName = "Shared Objects")
};

// 世界渲染层，使用位掩码实现
struct FParallelRenderLayer
{
    static constexpr int32 World0 = 1 << 0;
    static constexpr int32 World1 = 1 << 1;
    static constexpr int32 World2 = 1 << 2;
    static constexpr int32 World3 = 1 << 3;
    static constexpr int32 Shared = 1 << 4;
};

// 轻量级的世界属性结构体
USTRUCT(BlueprintType)
struct FWorldProperties
{
    GENERATED_BODY()

public:
    // 世界ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Properties")
    int32 WorldID = 0;
    
    // 世界名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Properties")
    FString WorldName = TEXT("DefaultWorld");
    
    // 物理碰撞相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Properties")
    EParallelCollisionChannel CollisionChannel = EParallelCollisionChannel::PCC_World0;
    
    // 渲染相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Properties")
    int32 RenderLayerMask = FParallelRenderLayer::World0;
    
    // 是否为默认世界
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Properties")
    bool bIsDefaultWorld = true;
    
    // 世界是否激活
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Properties")
    bool bIsActive = true;
    
    // 默认构造函数
    FWorldProperties() = default;
    
    // 带参数的构造函数
    FWorldProperties(int32 InWorldID, const FString& InWorldName = TEXT(""))
        : WorldID(InWorldID)
        , WorldName(InWorldName.IsEmpty() ? FString::Printf(TEXT("World_%d"), InWorldID) : InWorldName)
        , bIsDefaultWorld(InWorldID == 0)
        , bIsActive(true)
    {
        // 碰撞通道和渲染层将在外部设置
    }
};

// 工具函数类
class CARLA_API FParallelWorldUtils
{
public:
    // 获取世界的碰撞通道
    static ECollisionChannel GetCollisionChannelForWorld(int32 WorldID);
    
    // 获取世界的渲染层掩码
    static int32 GetRenderLayerMaskForWorld(int32 WorldID);
    
    // 设置组件的碰撞响应
    static void SetupComponentCollisionForWorld(
        UPrimitiveComponent* Component, 
        int32 WorldID
    );
    
    // 转换WorldID到碰撞通道枚举值
    static EParallelCollisionChannel WorldIDToCollisionChannel(int32 WorldID);
    
    // 转换WorldID到渲染层掩码
    static int32 WorldIDToRenderLayerMask(int32 WorldID);
};
