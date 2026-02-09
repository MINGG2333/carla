// Plugins/Carla/Source/Carla/ParallelWorld/WorldProperties.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"  // 因为UWorldProperties继承自UObject
#include "Engine/EngineTypes.h"     // 用于ECollisionChannel等
#include "WorldProperties.generated.h"

UENUM(BlueprintType)
enum class EWorldCollisionChannel : uint8
{
    World0 UMETA(DisplayName = "World 0"),
    World1 UMETA(DisplayName = "World 1"),
    World2 UMETA(DisplayName = "World 2"),
    // ... 最多支持32个世界（UE4碰撞通道限制）
    Shared UMETA(DisplayName = "Shared Objects")
};

UCLASS()
class CARLA_API UWorldProperties : public UObject
{
    GENERATED_BODY()
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World")
    int32 WorldID = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parallel World")
    FString WorldName = TEXT("DefaultWorld");
    
    // 物理碰撞相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physics")
    EWorldCollisionChannel CollisionChannel = EWorldCollisionChannel::World0;
    
    // 渲染相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rendering")
    int32 RenderLayerMask = 1; // 位掩码
    
    // 静态函数：获取碰撞响应设置
    static void SetupCollisionResponseForWorld(
        UPrimitiveComponent* Component, 
        int32 WorldID
    );
    
    // 静态函数：将WorldID转换为碰撞通道
    static ECollisionChannel GetCollisionChannelForWorld(int32 WorldID);
    
    // 静态函数：获取共享世界的碰撞通道
    static ECollisionChannel GetSharedCollisionChannel();
    
    // 静态函数：获取渲染层掩码
    static int32 GetRenderLayerMaskForWorld(int32 WorldID);
};
