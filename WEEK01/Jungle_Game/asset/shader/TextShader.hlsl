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
    float3 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float3 scaledPos = input.position * scale;
    float3 worldPos = scaledPos + offset;
    output.position = float4(worldPos, 1.0f);
    output.uv = input.uv;
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 color = texture0.Sample(samplerState, input.uv);
    float alpha = color.r;
    
    if (color.r < 0.01f)
    {
        discard;
    }
    
    return float4(input.color, alpha);

}
