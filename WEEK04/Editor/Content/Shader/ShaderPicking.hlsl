cbuffer FrameConstants : register(b0)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
}

cbuffer ObjectConstants : register(b1)
{
    row_major float4x4 WorldMatrix;
    uint ObjectId;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

cbuffer PickingConstants : register(b0)
{
    uint ObjectId;
    float3 Padding;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR;
    float2 UV       : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    output.position = mul(float4(input.position, 1.0f), MVP);
    return output;
}

float4 mainPS(VS_OUTPUT input)
{
    return float4(1,0,0,1);
}

// uint mainPS(VS_OUTPUT input) : SV_TARGET
// {
//     return 100; // TEST
//     //return ObjectId;
// }