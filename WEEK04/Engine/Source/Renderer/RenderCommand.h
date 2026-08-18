#pragma once

#include "Primitive/FMeshData.h"
#include "Renderer/RenderState/RenderState.h"
#include "Engine/Asset/Material.h"


class UMaterial;

enum class ERenderLayer
{
    Default,
    Overlay,
    UI,
};

struct ENGINE_API FRenderCommand
{
    FMeshData*        MeshData = nullptr;
    FMatrix           WorldMatrix;
    UMaterial* Material = nullptr;
    uint64            SortKey = 0;
    Geometry::FAABB  WorldAABB;
    bool             bDrawAABB = false;
    bool             bIgnoreWireFrame = false;

    FVector2 UVOffset = {0.f, 0.f};

    ERenderLayer RenderLayer = ERenderLayer::Default;
    uint32       ObjectId = 0;

    FRasterizerStateOption   RasterizerOption;
    FDepthStencilStateOption DepthStencilOption;
    FBlendStateOption        BlendOption;
    EMeshTopology            Topology = EMeshTopology::EMT_TriangleList;

    uint32 FirstIndex = 0;
    // Vertex Buffer에 존재하는 총 vertex 갯수와는 별개로 draw call에서 지정하는 렌더링할 vertex의 갯수
    uint32 DrawVertexCount = 0;
    // Legacy/compatibility alias for call sites that still use index terminology.
    uint32 DrawIndexCount = 0;
    
    bool bIsVisible = true;
    bool bIsPickable = true;
    bool bIsSelected = false;

    FVector4 MultiplyColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    FVector4 AdditiveColor = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

    void SetStates(const UMaterial* InMaterial, EMeshTopology InTopology);
    void SetDefaultStates();

    static uint64 MakeSortKey(const UMaterial* InMaterial, const FMeshData* InMeshData);
};

/**
 * 한 프레임 동안 수집된 모든 렌더링 명령을 담는 큐
 */
struct ENGINE_API FRenderCommandQueue
{
    /** 일반 메시 렌더링 명령 목록 (텍스트, SubUV 포함 통합) */
    TArray<FRenderCommand> Commands;

    /** 프레임의 카메라 행렬 */
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;

    void Reserve(size_t Count)
    {
        Commands.reserve(Count);
    }

    void AddCommand(const FRenderCommand& Cmd)
    {
        Commands.push_back(Cmd);
    }

    /** 큐 초기화 */
    void Clear()
    {
        Commands.clear();
    }
};