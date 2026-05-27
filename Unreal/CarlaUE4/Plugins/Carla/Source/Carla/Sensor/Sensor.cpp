// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "Carla/Sensor/Sensor.h"
#include "Carla/Sensor/SensorManager.h"

#include "Carla/Actor/ActorDescription.h"
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h"
#include "Carla/Game/CarlaStatics.h"

// jxy: Console variable toggled by PhysScene at snapshot boundaries.
// When true, all ASensor::Tick calls return early, skipping PrePhysTick,
// PostPhysTick, and all downstream scene capture / data streaming.
static TAutoConsoleVariable<bool> CVarSnapshotActive(
    TEXT("carla.SnapshotActive"),
    false,
    TEXT("When true, skip all sensor rendering during snapshot window."),
    ECVF_Default);

ASensor::ASensor(const FObjectInitializer &ObjectInitializer)
  : Super(ObjectInitializer)
{
  auto *Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CamMesh"));
  Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
  Mesh->bHiddenInGame = true;
  Mesh->CastShadow = false;
  RootComponent = Mesh;
}

void ASensor::BeginPlay()
{
  Super::BeginPlay();
  UCarlaEpisode* CurEpisode = UCarlaStatics::GetCurrentEpisode(GetWorld());
  FSensorManager& SensorManager = CurEpisode->GetSensorManager();
  SensorManager.RegisterSensor(this);
}

void ASensor::Set(const FActorDescription &Description)
{
  // set the tick interval of the sensor
  if (Description.Variations.Contains("sensor_tick"))
  {
    SetActorTickInterval(
        UActorBlueprintFunctionLibrary::ActorAttributeToFloat(Description.Variations["sensor_tick"],
        0.0f));
  }
}

void ASensor::Tick(const float DeltaTime)
{
  TRACE_CPUPROFILER_EVENT_SCOPE(ASensor::Tick);
  Super::Tick(DeltaTime);

  // jxy: Skip all sensor rendering during snapshot window.
  if (CVarSnapshotActive.GetValueOnGameThread())
  {
    return;
  }
  // jxy end

  if (bClientsListening)
  {
    if(!Stream.AreClientsListening())
    {
      OnLastClientDisconnected();
      bClientsListening = false;
    }
  }
  else
  {
    if(Stream.AreClientsListening())
    {
      OnFirstClientConnected();
      bClientsListening = true;
    }
  }
  if(!bClientsListening)
  {
    return;
  }
  ReadyToTick = true;
  PrePhysTick(DeltaTime);
}

void ASensor::SetSeed(const int32 InSeed)
{
  check(RandomEngine != nullptr);
  Seed = InSeed;
  RandomEngine->Seed(InSeed);
}

void ASensor::PostActorCreated()
{
  Super::PostActorCreated();

#if WITH_EDITOR
  auto *StaticMeshComponent = Cast<UStaticMeshComponent>(RootComponent);
  if (StaticMeshComponent && !IsRunningCommandlet() && !StaticMeshComponent->GetStaticMesh())
  {
    UStaticMesh *CamMesh = LoadObject<UStaticMesh>(
        NULL,
        TEXT("/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM"),
        NULL,
        LOAD_None,
        NULL);
    StaticMeshComponent->SetStaticMesh(CamMesh);
  }
#endif // WITH_EDITOR
}

void ASensor::EndPlay(EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);
  
  // close all sessions associated to the sensor stream
  auto *GameInstance = UCarlaStatics::GetGameInstance(GetEpisode().GetWorld());
  auto &StreamingServer = GameInstance->GetServer().GetStreamingServer();
  auto StreamId = carla::streaming::detail::token_type(Stream.GetToken()).get_stream_id();
  StreamingServer.CloseStream(StreamId);

  Stream = FDataStream();

  UCarlaEpisode* CurEpisode = UCarlaStatics::GetCurrentEpisode(GetWorld());
  if(CurEpisode)
  {
    FSensorManager& SensorManager = CurEpisode->GetSensorManager();
    SensorManager.DeRegisterSensor(this);
  }
}

void ASensor::PostPhysTickInternal(UWorld *World, ELevelTick TickType, float DeltaSeconds)
{
  TRACE_CPUPROFILER_EVENT_SCOPE(ASensor::PostPhysTickInternal);
  if(ReadyToTick)
  {
    PostPhysTick(World, TickType, DeltaSeconds);
    ReadyToTick = false;
  }
}
