#include "../Common/Common.hlsli"

cbuffer EditorPickingBuffer : register(b12)
{
    uint PickingId;
    uint UseAlphaTest;
    float AlphaCutoff;
    float PickingPadding0;
    float2 UVOffset;
    float2 UVScale;
}

Texture2D PickTexture : register(t0);
SamplerState PickSampler : register(s0);

// GPU skinning matrices bound at t16 when available
StructuredBuffer<float4x4> SkinningMatrices : register(t16);

struct VSInputPrimitive
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VSInputStaticMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    float4 Tangent : TANGENT;
};

struct VSInputSkeletalMesh
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    float4 Tangent : TANGENT;
    float4 Color : COLOR;
    uint4 BoneIndices : BLENDINDICES;
    float4 BoneWeights : BLENDWEIGHT;
};

struct VSOutputPrimitive
{
    float4 Position : SV_POSITION;
};

struct VSOutputTextured
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

VSOutputPrimitive VSPrimitive(VSInputPrimitive Input)
{
    VSOutputPrimitive Output;
    Output.Position = ApplyMVP(Input.Position);
    return Output;
}

VSOutputTextured VSBillboard(VSInputPrimitive Input)
{
    VSOutputTextured Output;
    Output.Position = ApplyMVP(Input.Position);
    float2 LocalUV = float2(0.5f - Input.Position.y, 0.5f - Input.Position.z);
    Output.UV = UVOffset + LocalUV * UVScale;
    return Output;
}

VSOutputTextured VSStaticMesh(VSInputStaticMesh Input)
{
    VSOutputTextured Output;
    Output.Position = ApplyMVP(Input.Position);
    Output.UV = UVOffset + Input.UV * UVScale;
    return Output;
}

VSOutputTextured VSSkeletalMesh(VSInputSkeletalMesh Input)
{
    VSOutputTextured Output;
#ifdef SKELETAL_MESH
    float4 Pos = float4(Input.Position, 1.0f);
    float weightSum = Input.BoneWeights.x + Input.BoneWeights.y + Input.BoneWeights.z + Input.BoneWeights.w;
    float4 weights = weightSum > 0.0f ? Input.BoneWeights / weightSum : float4(1, 0, 0, 0);

    float4 skinnedPos = mul(SkinningMatrices[Input.BoneIndices.x], Pos) * weights.x;
    skinnedPos += mul(SkinningMatrices[Input.BoneIndices.y], Pos) * weights.y;
    skinnedPos += mul(SkinningMatrices[Input.BoneIndices.z], Pos) * weights.z;
    skinnedPos += mul(SkinningMatrices[Input.BoneIndices.w], Pos) * weights.w;

    Output.Position = ApplyMVP(skinnedPos.xyz);
#else
    Output.Position = ApplyMVP(Input.Position);
#endif
    Output.UV = UVOffset + Input.UV * UVScale;
    return Output;
}

uint PSOpaque(VSOutputPrimitive Input) : SV_TARGET
{
    return PickingId;
}

uint PSTextured(VSOutputTextured Input) : SV_TARGET
{
    if (UseAlphaTest != 0)
    {
        float Alpha = PickTexture.Sample(PickSampler, Input.UV).a;
        if (Alpha <= AlphaCutoff)
        {
            discard;
        }
    }
    return PickingId;
}
