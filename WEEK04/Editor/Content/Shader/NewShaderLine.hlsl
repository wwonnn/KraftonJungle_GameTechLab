cbuffer FFrameConstants : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
};

cbuffer FObjectConstants : register(b1)
{
    row_major float4x4 World;
    uint ObjectId;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct VSInput
{
    float3 Position : POSITION;
    float4 Color    : COLOR0;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
};

PSInput VSMain(VSInput In)
{
    PSInput Out;
    float4 worldPos = mul(float4(In.Position, 1.0f), World);
    float4 viewPos = mul(worldPos, View);
    Out.Position = mul(viewPos, Projection);
    
    Out.Color = In.Color;
    return Out;
}

float4 PSMain(PSInput In) : SV_TARGET
{
    return In.Color;
}
