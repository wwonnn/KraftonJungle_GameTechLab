#include "../Common/Common.hlsli"

cbuffer SelectionMaskBuffer : register(b12)
{
    uint UseAlphaTest;
    float AlphaCutoff;
    float2 UVOffset;
    float2 UVScale;
}

Texture2D MaskTexture : register(t0);
SamplerState MaskSampler : register(s0);

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

VSOutputTextured VSStaticMesh(VSInputStaticMesh Input)
{
    VSOutputTextured Output;
    Output.Position = ApplyMVP(Input.Position);
    Output.UV = UVOffset + Input.UV * UVScale;
    return Output;
}

// Support GPU skinning when compiled with SKELETAL_MESH
#ifdef SKELETAL_MESH
StructuredBuffer<float4x4> SkinningMatrices : register(t16);

VSOutputTextured VSSkeletalMesh(VSInputSkeletalMesh Input)
{
    VSOutputTextured Output;
    float4 Pos = float4(Input.Position, 1.0f);
    float4 weights = Input.BoneWeights / dot(Input.BoneWeights, 1.0f);

    float4 SkinnedPos =
        mul(SkinningMatrices[Input.BoneIndices.x], Pos) * weights.x +
        mul(SkinningMatrices[Input.BoneIndices.y], Pos) * weights.y +
        mul(SkinningMatrices[Input.BoneIndices.z], Pos) * weights.z +
        mul(SkinningMatrices[Input.BoneIndices.w], Pos) * weights.w;

    Output.Position = ApplyMVP(SkinnedPos.xyz);
    Output.UV = UVOffset + Input.UV * UVScale;
    return Output;
}
#else
VSOutputTextured VSSkeletalMesh(VSInputSkeletalMesh Input)
{
    VSOutputTextured Output;
    Output.Position = ApplyMVP(Input.Position);
    Output.UV = UVOffset + Input.UV * UVScale;
    return Output;
}
#endif

VSOutputTextured VSBillboard(VSInputPrimitive Input)
{
    VSOutputTextured Output;
    Output.Position = ApplyMVP(Input.Position);
    float2 LocalUV = float2(0.5f - Input.Position.y, 0.5f - Input.Position.z);
    Output.UV = UVOffset + LocalUV * UVScale;
    return Output;
}

float4 PSPrimitive(VSOutputPrimitive Input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

float4 PSTextured(VSOutputTextured Input) : SV_TARGET
{
    if (UseAlphaTest != 0)
    {
        float Alpha = MaskTexture.Sample(MaskSampler, Input.UV).a;
        if (Alpha <= AlphaCutoff)
        {
            discard;
        }
    }

    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
