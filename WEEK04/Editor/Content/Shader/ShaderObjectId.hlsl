cbuffer FrameConstants : register(b0)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
}

cbuffer ObjectConstants : register(b1)
{
    row_major float4x4 WorldMatrix;
    uint ObjectId;
    uint IsHovered;
    uint Padding1;
    uint Padding2;
};

struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

VSOutput VSMain(VSInput In)
{
    VSOutput Out;
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    Out.Position = mul(float4(In.Position, 1.0f), MVP);
    return Out;
}

uint PSMain(VSOutput In) : SV_TARGET
{
    return ObjectId;
}