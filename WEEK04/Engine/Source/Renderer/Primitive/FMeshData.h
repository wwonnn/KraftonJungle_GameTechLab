#pragma once

#include "Renderer/Types/VertexTypes.h"
#include "RHI/RHIBuffer.h"

// Forward Declaration
class FD3D11RHI;
// Forward Declaration

enum class EMeshTopology
{
    EMT_Undefined = 0, // = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED
    EMT_Point = 1, // =  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
    EMT_LineList = 2, // = D3D11_PRIMITIVE_TOPOLOGY_LINELIST
    EMT_LineStrip = 3, // = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP
    EMT_TriangleList = 4, // = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    EMT_TriangleStrip = 5, // = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP
};

struct ENGINE_API FMeshData
{
    FMeshData() : SortId(NextSortId++), VertexBufferCount(0), IndexBufferCount(0)
    {
    }

    // NOTE: 이제 Mesh도 에셋 관리자로 중앙집중식 관리하므로 
    //      FMeshData가 직접 Release하지 않음.
    ~FMeshData() { /*Release();*/ }

    uint32 GetSortId() const { return SortId; }

    void Bind(FD3D11RHI* Context);
    
    bool bIsDynamicMesh = false;    
    
    // Dynamic mesh인 경우에는 동적으로 채워넣는 Vertices/Indices를 사용
    TArray<FPrimitiveVertex> Vertices;
    TArray<uint32>           Indices;
    
    // Dynamic mesh인 경우엔 SceneAssetBinder에서 미리 만들어준 VertexBuffer를 고정 사용
    std::shared_ptr<RHI::FRHIVertexBuffer> VertexBuffer = nullptr;
    std::shared_ptr<RHI::FRHIIndexBuffer> IndexBuffer = nullptr;
    uint32 VertexBufferCount;
    uint32 IndexBufferCount;

    // 토폴로지 옵션
    EMeshTopology Topology = EMeshTopology::EMT_Undefined;
    
    /** AABB Box Extent 및 Local Bound Radius 갱신 */
    void  UpdateLocalBound();
    float GetLocalBoundRadius() const { return LocalBoundRadius; }

    FVector GetMinCoord() const { return MinCoord; }
    FVector GetMaxCoord() const { return MaxCoord; }
    FVector GetCenterCoord() const { return (MaxCoord - MinCoord) * 0.5 + MinCoord; }

private:
    uint32               SortId = 0;
    static inline uint32 NextSortId = 0;

    FVector MinCoord = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
    FVector MaxCoord = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    float   LocalBoundRadius = 0.f;
};
