cbuffer ConstantBuffer : register(b0)
{
    float4 offset;
    float4 rotation;
    float4 scale;
}

cbuffer SpriteBuffer : register(b1)
{
    float2 UVOffset;
    float2 UVScale;
}

cbuffer EffectBuffer : register(b2)
{
    float time;
    float3 padding;
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
    
    float3 scaledPos = input.position * scale.xyz;
    
    float3 rotatedPos = scaledPos;
    rotatedPos.x = scaledPos.x * cos(rotation.z) - scaledPos.y * sin(rotation.z);
    rotatedPos.y = scaledPos.x * sin(rotation.z) + scaledPos.y * cos(rotation.z);
    
    float3 worldPos = rotatedPos + offset.xyz;
    output.position = float4(worldPos, 1.0f);
    output.uv = input.uv;
	
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.uv * UVScale + UVOffset;
    float4 color = texture0.Sample(samplerState, uv);

    if(time > 0.0f)
    {
        float alpha = 0.5f + 0.5f * sin(time * 30.0f);
        color.a *= alpha;
    }
    return color;
}
