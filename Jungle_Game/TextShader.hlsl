cbuffer ConstantBuffer : register(b0)
{
    float4 offset;
    float4 rotation;
    float4 scale;
}

Texture2D texture0 : register(t0);
SamplerState samplerState : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float3 scaledPos = input.position * scale;
    float3 worldPos = scaledPos + offset;
    output.position = float4(worldPos, 1.0f);
    output.uv = input.uv;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 color = texture0.Sample(samplerState, input.uv);
    
    if (color.r > 0.0f && color.g == 0.0f && color.b == 0.0f)
    {
        return float4(1.0f, 1.0f, 1.0f, color.r);
    }
    
    return color;
}
