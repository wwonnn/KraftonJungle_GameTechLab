#include "FMeshData.h"

#include "Renderer/D3D11/D3D11RHI.h"
#include "RHI/D3D11/D3D11Buffer.h"

void FMeshData::Bind(FD3D11RHI* Context)
{
    if (!Context) return;

    if (bIsDynamicMesh)
    {
        // Vertex Buffer Update/Create
        if (!Vertices.empty())
        {
            uint32 RequiredByteWidth = static_cast<uint32>(sizeof(FPrimitiveVertex) * Vertices.size());
            bool bNeedRecreate = !VertexBuffer || VertexBuffer->GetDesc().ByteWidth < RequiredByteWidth;

            if (bNeedRecreate)
            {
                ID3D11Buffer* RawBuffer = nullptr;
                if (Context->CreateVertexBuffer(Vertices.data(), RequiredByteWidth, sizeof(FPrimitiveVertex), true, &RawBuffer))
                {
                    RHI::FBufferDesc Desc;
                    Desc.ByteWidth = RequiredByteWidth;
                    Desc.Stride = sizeof(FPrimitiveVertex);
                    Desc.BindFlags = RHI::EBufferBindFlags::VertexBuffer;
                    Desc.Usage = RHI::EBufferUsage::Dynamic;
                    Desc.CPUAccessFlags = RHI::ECPUAccessFlags::Write;

                    VertexBuffer = std::make_shared<RHI::D3D11::FD3D11VertexBuffer>(Desc, RawBuffer);
                }
            }
            else
            {
                auto* D3D11VB = static_cast<RHI::D3D11::FD3D11VertexBuffer*>(VertexBuffer.get());
                Context->UpdateDynamicBuffer(D3D11VB->GetBuffer(), Vertices.data(), RequiredByteWidth);
            }
            VertexBufferCount = static_cast<uint32>(Vertices.size());
        }

        // Index Buffer Update/Create
        if (!Indices.empty())
        {
            uint32 RequiredByteWidth = static_cast<uint32>(sizeof(uint32) * Indices.size());
            bool bNeedRecreate = !IndexBuffer || IndexBuffer->GetDesc().ByteWidth < RequiredByteWidth;

            if (bNeedRecreate)
            {
                ID3D11Buffer* RawBuffer = nullptr;
                if (Context->CreateIndexBuffer(Indices.data(), RequiredByteWidth, true, &RawBuffer))
                {
                    RHI::FBufferDesc Desc;
                    Desc.ByteWidth = RequiredByteWidth;
                    Desc.Stride = sizeof(uint32);
                    Desc.BindFlags = RHI::EBufferBindFlags::IndexBuffer;
                    Desc.Usage = RHI::EBufferUsage::Dynamic;
                    Desc.CPUAccessFlags = RHI::ECPUAccessFlags::Write;

                    IndexBuffer = std::make_shared<RHI::D3D11::FD3D11IndexBuffer>(Desc, RHI::EIndexFormat::UInt32, RawBuffer);
                }
            }
            else
            {
                auto* D3D11IB = static_cast<RHI::D3D11::FD3D11IndexBuffer*>(IndexBuffer.get());
                Context->UpdateDynamicBuffer(D3D11IB->GetBuffer(), Indices.data(), RequiredByteWidth);
            }
            IndexBufferCount = static_cast<uint32>(Indices.size());
        }
    }

    // Bind Buffers
    if (VertexBuffer)
    {
        UINT Stride = sizeof(FPrimitiveVertex);
        UINT Offset = 0;
        Context->SetVertexBuffer(0, VertexBuffer.get(), Stride, Offset);
    }

    if (IndexBuffer)
    {
        Context->SetIndexBuffer(IndexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
    }

    // Topology
    if (Topology != EMeshTopology::EMT_Undefined)
    {
        D3D11_PRIMITIVE_TOPOLOGY D3DTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        switch (Topology)
        {
        case EMeshTopology::EMT_Point:         D3DTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
        case EMeshTopology::EMT_LineList:      D3DTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
        case EMeshTopology::EMT_LineStrip:     D3DTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
        case EMeshTopology::EMT_TriangleList:  D3DTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case EMeshTopology::EMT_TriangleStrip: D3DTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
        default: break;
        }

        if (D3DTopology != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
        {
            Context->SetPrimitiveTopology(D3DTopology);
        }
    }
}

void FMeshData::UpdateLocalBound()
{
    if (Vertices.empty())
    {
        MinCoord = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
        MaxCoord = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        LocalBoundRadius = 0.f;
    }
    else
    {
        MinCoord = FVector(FLT_MAX, FLT_MAX, FLT_MAX);
        MaxCoord = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        // TODO: Ritter's Algorithm으로 개선
        for (const FPrimitiveVertex& Vertex : Vertices)
        {
            if (Vertex.Position.X < MinCoord.X)
                MinCoord.X = Vertex.Position.X;
            if (Vertex.Position.X > MaxCoord.X)
                MaxCoord.X = Vertex.Position.X;
            if (Vertex.Position.Y < MinCoord.Y)
                MinCoord.Y = Vertex.Position.Y;
            if (Vertex.Position.Y > MaxCoord.Y)
                MaxCoord.Y = Vertex.Position.Y;
            if (Vertex.Position.Z < MinCoord.Z)
                MinCoord.Z = Vertex.Position.Z;
            if (Vertex.Position.Z > MaxCoord.Z)
                MaxCoord.Z = Vertex.Position.Z;
        }

        LocalBoundRadius = ((MaxCoord - MinCoord) * 0.5f).Size();
    }
}
