#include "Editor/Animation/AnimationSequencePreviewController.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/AnimSequence.h"
#include "Asset/SkeletalMesh.h"
#include "Component/PostProcess/Light/AmbientLightComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "Editor/EditorEngine.h"
#include "Editor/Selection/SelectionManager.h"
#include "GameFramework/PrimitiveActors.h"
#include "GameFramework/World.h"
#include "Math/Utils.h"
#include "Object/FName.h"
#include "Object/Object.h"
#include "Render/Renderer/Renderer.h"

#include <algorithm>
#include <filesystem>
#include <functional>

namespace
{
    bool IsLiveObject(const UObject* Object)
    {
        return Object != nullptr && UObjectManager::Get().ContainsObject(Object);
    }

    const UAnimDataModel* GetValidAnimDataModel(const UAnimSequence* Sequence)
    {
        if (!IsLiveObject(Sequence))
        {
            return nullptr;
        }

        const UAnimDataModel* DataModel = Sequence->DataModel;
        return IsLiveObject(DataModel) ? DataModel : nullptr;
    }

    FName MakePreviewWorldHandle(const FString& SequencePath)
    {
        const FString NormalizedPath = FPaths::Normalize(SequencePath);
        const size_t PathHash = std::hash<FString>{}(NormalizedPath);
        return FName(("__AnimSequencePreview_" + std::to_string(PathHash)).c_str());
    }

    FString GetMeshStemFromSequencePath(const FString& SequencePath)
    {
        if (SequencePath.empty())
        {
            return {};
        }

        const std::filesystem::path SequenceFsPath(FPaths::ToWide(SequencePath));
        const std::filesystem::path ParentPath = SequenceFsPath.parent_path();
        return ParentPath.empty() ? FString() : FPaths::ToString(ParentPath.filename().generic_wstring());
    }

    FString GetAssetStem(const FString& AssetPath)
    {
        if (AssetPath.empty())
        {
            return {};
        }

        const std::filesystem::path AssetFsPath(FPaths::ToWide(FPaths::Normalize(AssetPath)));
        return FPaths::ToString(AssetFsPath.stem().generic_wstring());
    }

    bool TryBuildMeshPathFromSkeletonPath(
        const FString& SkeletonAssetPath,
        const TArray<FString>& SkeletalMeshPaths,
        FString& OutMeshPath)
    {
        if (SkeletonAssetPath.empty())
        {
            return false;
        }

        std::filesystem::path MeshFsPath(FPaths::ToWide(FPaths::Normalize(SkeletonAssetPath)));
        if (MeshFsPath.empty())
        {
            return false;
        }

        MeshFsPath.replace_extension(L".skmesh");
        const std::filesystem::path BinDir = MeshFsPath.parent_path();
        const std::filesystem::path AssetTypeDir = BinDir.parent_path();
        if (!AssetTypeDir.empty())
        {
            MeshFsPath = AssetTypeDir.parent_path() / L"SkeletalMesh" / BinDir.filename() / MeshFsPath.filename();
        }

        const FString ExpectedMeshPath = FPaths::Normalize(FPaths::ToUtf8(MeshFsPath.generic_wstring()));
        for (const FString& CandidatePath : SkeletalMeshPaths)
        {
            if (FPaths::Normalize(CandidatePath) == ExpectedMeshPath)
            {
                OutMeshPath = CandidatePath;
                return true;
            }
        }

        return false;
    }

    float GetSequenceLengthSeconds(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = GetValidAnimDataModel(Sequence);
        if (!DataModel)
        {
            return 0.0f;
        }

        const float FrameRate = DataModel->FrameRate.AsDecimal();
        if (FrameRate <= 0.0f || DataModel->NumberOfKeys <= 1)
        {
            return 0.0f;
        }

        return static_cast<float>(DataModel->NumberOfKeys - 1) / FrameRate;
    }

    FFrameRate GetSequenceFrameRate(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = GetValidAnimDataModel(Sequence);
        return DataModel ? DataModel->FrameRate : FFrameRate();
    }

    int32 GetSequenceFrameCount(const UAnimSequence* Sequence)
    {
        const UAnimDataModel* DataModel = GetValidAnimDataModel(Sequence);
        if (!DataModel)
        {
            return 0;
        }

        return std::max(DataModel->NumberOfFrames, DataModel->NumberOfKeys);
    }

    float FrameIndexToTime(int32 FrameIndex, const FFrameRate& FrameRate)
    {
        const int32 SafeFrameIndex = std::max(FrameIndex, 0);
        const double FPS = FrameRate.AsDecimal();
        if (FPS <= 0.0)
        {
            return static_cast<float>(SafeFrameIndex);
        }

        return static_cast<float>(static_cast<double>(SafeFrameIndex) / FPS);
    }
}

FAnimationSequencePreviewController::~FAnimationSequencePreviewController()
{
    Shutdown();
}

bool FAnimationSequencePreviewController::HasSequence() const
{
    return IsLiveObject(Sequence);
}

bool FAnimationSequencePreviewController::Initialize(
    UEditorEngine* InEditorEngine,
    const FString& InSequencePath,
    UAnimSequence* InSequence)
{
    Shutdown();

    EditorEngine = InEditorEngine;
    SequencePath = FPaths::Normalize(InSequencePath);
    Sequence = InSequence;
    PreviewResourceIndex = InvalidPreviewResourceIndex;
    CurrentTime = 0.0f;
    PlayRate = 1.0f;
    bPlaying = false;
    bLooping = true;
    bInitialized = false;
    bPoseDirty = false;
    PreviewStatusText = "Preview is not initialized.";
    TimelineStatusText = "Timeline controls are available after preview initialization.";

    if (!EditorEngine || !Sequence)
    {
        PreviewStatusText = "Preview initialization failed: sequence document is missing editor context or animation data.";
        TimelineStatusText = "Timeline is unavailable until the animation sequence is loaded.";
        return false;
    }

    if (!ResolvePreviewMeshPath(PreviewMeshPath))
    {
        if (Sequence && !Sequence->GetSkeletonAssetPath().empty())
        {
            PreviewStatusText =
                "Preview mesh could not be resolved from SkeletonAssetPath: " + Sequence->GetSkeletonAssetPath();
        }
        else
        {
            PreviewStatusText =
                "Preview mesh could not be resolved because the sequence has no SkeletonAssetPath metadata yet.";
        }
        TimelineStatusText = "Playback controls are available after a preview mesh is resolved.";
        return false;
    }

    PreviewMesh = FResourceManager::Get().LoadSkeletalMesh(PreviewMeshPath);
    if (!PreviewMesh || !PreviewMesh->HasValidMeshData())
    {
        PreviewStatusText = "Failed to load preview skeletal mesh: " + PreviewMeshPath;
        TimelineStatusText = "Playback controls are available after a valid preview mesh loads.";
        return false;
    }

    if (!InitializePreviewWorld() || !InitializePreviewActor())
    {
        Shutdown();
        TimelineStatusText = "Playback controls are unavailable because preview world initialization failed.";
        return false;
    }

    SetViewportSize(ViewportWidth, ViewportHeight);
    SetLooping(true);
    SetPlayRate(1.0f);
    Pause();
    SetCurrentTime(0.0f);

    bInitialized = true;
    PreviewStatusText = "Preview mesh: " + PreviewMeshPath;
    TimelineStatusText = "Use the timeline below to scrub the current pose or play the clip in place.";
    return true;
}

void FAnimationSequencePreviewController::Shutdown()
{
    bInitialized = false;
    bPlaying = false;
    bPoseDirty = false;

    PreviewComponent = nullptr;
    PreviewActor = nullptr;
    PreviewMesh = nullptr;
    PreviewMeshPath.clear();

    PreviewViewport.SetRenderTargetSet(nullptr);
    PreviewViewport.SetEditorIdPickActors({});
    PreviewViewport.SetClient(nullptr);
    PreviewViewportClient.SetBonePickHandler(nullptr);
    PreviewViewportClient.DestroyCamera();
    PreviewViewportClient.SetGizmo(nullptr);
    PreviewViewportClient.SetSelectionManager(nullptr);
    PreviewViewportClient.SetViewport(nullptr);
    PreviewViewportClient.SetState(nullptr);
    PreviewViewportClient.SetWorld(nullptr);
    PreviewViewportClient.ReleaseTransientEditorState();

    if (EditorEngine && PreviewResourceIndex != InvalidPreviewResourceIndex)
    {
        EditorEngine->GetRenderer().ReleaseViewerViewportResource(PreviewResourceIndex);
    }
    PreviewResourceIndex = InvalidPreviewResourceIndex;

    if (EditorEngine && PreviewWorldHandle.IsValid())
    {
        if (EditorEngine->GetWorldContextFromHandle(PreviewWorldHandle))
        {
            EditorEngine->DestroyWorldContext(PreviewWorldHandle);
        }
    }

    PreviewWorld = nullptr;
    PreviewWorldHandle = FName();
    Sequence = nullptr;
    SequencePath.clear();
    EditorEngine = nullptr;
}

void FAnimationSequencePreviewController::Tick(float DeltaTime)
{
    if (!HasValidPreview())
    {
        return;
    }

    PreviewViewportClient.Tick(DeltaTime);

    if (!bPlaying && !bPoseDirty)
    {
        return;
    }

    RefreshPreviewPose(bPlaying ? DeltaTime : 0.0f);
}

void FAnimationSequencePreviewController::SetCurrentTime(float InTime)
{
    CurrentTime = std::clamp(InTime, 0.0f, GetLength());
    if (!IsLiveObject(PreviewComponent))
    {
        bPoseDirty = true;
        return;
    }

    PreviewComponent->SetPreviewTime(CurrentTime);
    CurrentTime = PreviewComponent->GetPreviewTime();
    if (PreviewWorld)
    {
        PreviewWorld->SyncSpatialIndex();
    }
    bPoseDirty = false;
}

float FAnimationSequencePreviewController::GetLength() const
{
    if (IsLiveObject(PreviewComponent))
    {
        return PreviewComponent->GetPreviewLength();
    }

    return GetSequenceLengthSeconds(Sequence);
}

FFrameRate FAnimationSequencePreviewController::GetFrameRate() const
{
    return GetSequenceFrameRate(Sequence);
}

int32 FAnimationSequencePreviewController::GetFrameCount() const
{
    return GetSequenceFrameCount(Sequence);
}

int32 FAnimationSequencePreviewController::GetCurrentFrameIndex() const
{
    const int32 FrameCount = GetFrameCount();
    if (FrameCount <= 0)
    {
        return 0;
    }

    const double FrameRate = GetFrameRate().AsDecimal();
    if (FrameRate <= 0.0)
    {
        return std::clamp(static_cast<int32>(std::lround(CurrentTime)), 0, std::max(0, FrameCount - 1));
    }

    const int32 FrameIndex = static_cast<int32>(std::lround(static_cast<double>(CurrentTime) * FrameRate));
    return std::clamp(FrameIndex, 0, std::max(0, FrameCount - 1));
}

void FAnimationSequencePreviewController::Play()
{
    bPlaying = HasValidPreview();
    if (IsLiveObject(PreviewComponent))
    {
        PreviewComponent->SetPreviewPlaying(true);
    }
}

void FAnimationSequencePreviewController::Pause()
{
    bPlaying = false;
    if (IsLiveObject(PreviewComponent))
    {
        PreviewComponent->SetPreviewPlaying(false);
    }
}

void FAnimationSequencePreviewController::Stop()
{
    Pause();
    JumpToStart();
}

void FAnimationSequencePreviewController::StepToNextFrame()
{
    const int32 FrameCount = GetFrameCount();
    if (FrameCount <= 0)
    {
        return;
    }

    Pause();
    SetCurrentTime(FrameIndexToTime(std::min(GetCurrentFrameIndex() + 1, FrameCount - 1), GetFrameRate()));
}

void FAnimationSequencePreviewController::StepToPreviousFrame()
{
    const int32 FrameCount = GetFrameCount();
    if (FrameCount <= 0)
    {
        return;
    }

    Pause();
    SetCurrentTime(FrameIndexToTime(std::max(GetCurrentFrameIndex() - 1, 0), GetFrameRate()));
}

void FAnimationSequencePreviewController::JumpToStart()
{
    Pause();
    SetCurrentTime(0.0f);
}

void FAnimationSequencePreviewController::JumpToEnd()
{
    const int32 FrameCount = GetFrameCount();
    if (FrameCount > 1)
    {
        Pause();
        SetCurrentTime(FrameIndexToTime(FrameCount - 1, GetFrameRate()));
        return;
    }

    Pause();
    SetCurrentTime(GetLength());
}

void FAnimationSequencePreviewController::SetLooping(bool bInLooping)
{
    bLooping = bInLooping;
    if (IsLiveObject(PreviewComponent))
    {
        PreviewComponent->SetPreviewLooping(bLooping);
    }
}

void FAnimationSequencePreviewController::SetPlayRate(float InPlayRate)
{
    PlayRate = InPlayRate;
    if (IsLiveObject(PreviewComponent))
    {
        PreviewComponent->SetPreviewPlayRate(PlayRate);
    }
}

bool FAnimationSequencePreviewController::HasValidPreview() const
{
    return IsLiveObject(Sequence) &&
        IsLiveObject(PreviewMesh) &&
        PreviewWorld != nullptr &&
        IsLiveObject(PreviewComponent);
}

void FAnimationSequencePreviewController::SetViewportSize(int32 InWidth, int32 InHeight)
{
    ViewportWidth = std::max(InWidth, 1);
    ViewportHeight = std::max(InHeight, 1);

    FViewportRect ViewportRect = {};
    ViewportRect.X = 0;
    ViewportRect.Y = 0;
    ViewportRect.Width = ViewportWidth;
    ViewportRect.Height = ViewportHeight;

    PreviewViewport.SetRect(ViewportRect);
    PreviewViewportClient.SetViewportSize(static_cast<float>(ViewportWidth), static_cast<float>(ViewportHeight));

    if (!EditorEngine || !HasValidPreview())
    {
        PreviewViewport.SetRenderTargetSet(nullptr);
        return;
    }

    if (PreviewResourceIndex == InvalidPreviewResourceIndex)
    {
        PreviewResourceIndex = NextPreviewResourceIndex++;
    }

    FViewportRenderResource& Resource =
        EditorEngine->GetRenderer().AcquireViewerViewportResource(
            PreviewResourceIndex,
            static_cast<uint32>(ViewportWidth),
            static_cast<uint32>(ViewportHeight));
    PreviewViewport.SetRenderTargetSet(&Resource.GetView());
}

ID3D11ShaderResourceView* FAnimationSequencePreviewController::GetPreviewSRV() const
{
    return PreviewViewport.GetOutSRV();
}

bool FAnimationSequencePreviewController::ResolvePreviewMeshPath(FString& OutMeshPath) const
{
    OutMeshPath.clear();

    const TArray<FString> SkeletalMeshPaths = FResourceManager::Get().GetSkeletalMeshPaths();
    const FString SkeletonAssetPath = Sequence ? FPaths::Normalize(Sequence->GetSkeletonAssetPath()) : FString();
    if (!SkeletonAssetPath.empty())
    {
        if (TryBuildMeshPathFromSkeletonPath(SkeletonAssetPath, SkeletalMeshPaths, OutMeshPath))
        {
            return true;
        }

        const FString SkeletonStem = GetAssetStem(SkeletonAssetPath);
        TArray<FString> CandidatePaths;
        CandidatePaths.reserve(SkeletalMeshPaths.size());
        for (const FString& CandidatePath : SkeletalMeshPaths)
        {
            if (SkeletonStem.empty() || GetAssetStem(CandidatePath) == SkeletonStem)
            {
                CandidatePaths.push_back(CandidatePath);
            }
        }

        auto TryResolveFromCandidates = [&](const TArray<FString>& Paths) -> bool
        {
            for (const FString& CandidatePath : Paths)
            {
                USkeletalMesh* CandidateMesh = FResourceManager::Get().FindSkeletalMesh(CandidatePath);
                if (!CandidateMesh)
                {
                    CandidateMesh = FResourceManager::Get().LoadSkeletalMesh(CandidatePath);
                }

                if (!CandidateMesh)
                {
                    continue;
                }

                if (FPaths::Normalize(CandidateMesh->GetSkeletonAssetPath()) == SkeletonAssetPath)
                {
                    OutMeshPath = CandidatePath;
                    return true;
                }
            }

            return false;
        };

        if (TryResolveFromCandidates(CandidatePaths))
        {
            return true;
        }

        if (CandidatePaths.size() != SkeletalMeshPaths.size() && TryResolveFromCandidates(SkeletalMeshPaths))
        {
            return true;
        }
    }

    const FString MeshStem = GetMeshStemFromSequencePath(SequencePath);
    if (MeshStem.empty())
    {
        return false;
    }

    for (const FString& CandidatePath : SkeletalMeshPaths)
    {
        if (GetAssetStem(CandidatePath) == MeshStem)
        {
            OutMeshPath = CandidatePath;
            return true;
        }
    }

    return false;
}

bool FAnimationSequencePreviewController::InitializePreviewWorld()
{
    if (!EditorEngine)
    {
        return false;
    }

    PreviewWorldHandle = MakePreviewWorldHandle(SequencePath);
    FWorldContext& PreviewContext =
        EditorEngine->CreateWorldContext(EWorldType::ViewerPreview, PreviewWorldHandle, "Animation Sequence Preview");
    PreviewContext.bPaused = true;
    EditorEngine->ApplySpatialIndexMaintenanceSettings(PreviewContext.World);
    PreviewWorld = PreviewContext.World;

    if (!PreviewWorld)
    {
        PreviewStatusText = "Preview world creation failed.";
        return false;
    }

    InitializeViewportClient();
    if (PreviewContext.SelectionManager)
    {
        PreviewViewportClient.SetGizmo(PreviewContext.SelectionManager->GetGizmo());
        PreviewViewportClient.SetSelectionManager(PreviewContext.SelectionManager);
    }

    ADirectionalLightActor* DirectionalLight = PreviewWorld->SpawnActor<ADirectionalLightActor>();
    if (DirectionalLight)
    {
        DirectionalLight->InitDefaultComponents();
        DirectionalLight->SetFName(FName("Anim Preview Directional Light"));
        DirectionalLight->SetActorLocation(FVector(100000.0f, 100000.0f, 100000.0f));
        DirectionalLight->SetActorRotation(FVector(0.0f, 44.0f, 0.0f));
    }

    AAmbientLightActor* AmbientLight = PreviewWorld->SpawnActor<AAmbientLightActor>();
    if (AmbientLight)
    {
        AmbientLight->InitDefaultComponents();
        AmbientLight->SetFName(FName("Anim Preview Ambient Light"));
        AmbientLight->SetActorLocation(FVector(100000.0f, 100000.0f, 100000.0f));

        if (UAmbientLightComponent* AmbientLightComponent = AmbientLight->FindComponent<UAmbientLightComponent>())
        {
            AmbientLightComponent->Intensity = 0.7f;
        }
    }

    return true;
}

bool FAnimationSequencePreviewController::InitializePreviewActor()
{
    if (!PreviewWorld || !PreviewMesh)
    {
        return false;
    }

    PreviewActor = PreviewWorld->SpawnActor<ASkeletalMeshActor>();
    if (!PreviewActor)
    {
        PreviewStatusText = "Preview actor creation failed.";
        return false;
    }

    PreviewActor->InitDefaultComponents();
    PreviewActor->SetFName(FName("AnimationSequencePreviewActor"));
    PreviewActor->SetActorLocation(FVector::ZeroVector);

    PreviewComponent = PreviewActor->GetSkeletalMeshComponent();
    if (!PreviewComponent)
    {
        PreviewStatusText = "Preview skeletal mesh component is unavailable.";
        return false;
    }

    PreviewComponent->SetSkeletalMesh(PreviewMesh);
    PreviewComponent->SetPreviewSequence(Sequence);
    PreviewComponent->SetPreviewLooping(bLooping);
    PreviewComponent->SetPreviewPlayRate(PlayRate);
    PreviewComponent->SetPreviewPlaying(false);
    PreviewComponent->EnsureSkinningUpdated();

    PreviewWorld->SyncSpatialIndex();
    ConfigurePreviewCamera();
    return true;
}

void FAnimationSequencePreviewController::InitializeViewportClient()
{
    PreviewViewport.SetClient(&PreviewViewportClient);

    PreviewViewportClient.Initialize(EditorEngine->GetWindow(), EditorEngine);
    PreviewViewportClient.SetWorld(PreviewWorld);
    PreviewViewportClient.SetViewport(&PreviewViewport);
    PreviewViewportClient.SetState(&PreviewViewport.GetState());
    PreviewViewportClient.SetSceneEditingShortcutsEnabled(false);
    PreviewViewportClient.SetViewportType(EEditorViewportType::EVT_Perspective);
    PreviewViewportClient.CreateCamera();
    PreviewViewportClient.ApplyCameraMode();

    PreviewViewport.GetState().ViewMode = EViewMode::Lit_BlinnPhong;
    PreviewViewport.GetState().LightCullMode = ELightCullMode::None;
}

void FAnimationSequencePreviewController::ConfigurePreviewCamera()
{
    if (!PreviewMesh)
    {
        return;
    }

    FViewportCamera* Camera = PreviewViewportClient.GetCamera();
    if (!Camera)
    {
        return;
    }

    const FAABB& LocalBounds = PreviewMesh->GetLocalBounds();
    const FVector Center = LocalBounds.GetCenter();
    const FVector Extent = LocalBounds.GetExtent();
    const float MaxExtent = std::max(10.0f, std::max(Extent.X, std::max(Extent.Y, Extent.Z)));
    const float Distance = MaxExtent * 3.0f;
    const FVector CameraLocation = Center + FVector(Distance, -Distance * 0.85f, Distance * 0.4f);

    Camera->SetLocation(CameraLocation);
    Camera->SetLookAt(Center);
    Camera->SetNearPlane(0.1f);
    Camera->SetFarPlane(std::max(2000.0f, Distance * 10.0f));
    PreviewViewportClient.SyncCameraTarget();
}

void FAnimationSequencePreviewController::RefreshPreviewPose(float DeltaTime)
{
    if (!IsLiveObject(PreviewComponent))
    {
        return;
    }

    PreviewComponent->SetPreviewLooping(bLooping);
    PreviewComponent->SetPreviewPlayRate(PlayRate);
    PreviewComponent->SetPreviewPlaying(bPlaying);
    PreviewComponent->TickPreviewAnimation(DeltaTime);
    CurrentTime = PreviewComponent->GetPreviewTime();

    if (PreviewWorld)
    {
        PreviewWorld->SyncSpatialIndex();
    }

    bPoseDirty = false;
}
