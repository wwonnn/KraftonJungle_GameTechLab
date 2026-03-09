// ShaderW0.hlsl
cbuffer constants : register(b0)
{
    float3 Offset;
    float Scale;
    float3 Rotation;
    float ColorFactor;
    float Pad;
}

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 회전 계산
    float3 r = Rotation;
    float3 p = input.position;

    // XYZ축 회전 행렬
    float3x3 rotX =
    {
        1, 0, 0,
        0, cos(r.x), -sin(r.x),
        0, sin(r.x), cos(r.x)
    };
    float3x3 rotY =
    {
        cos(r.y), 0, sin(r.y),
        0, 1, 0,
        -sin(r.y), 0, cos(r.y)
    };
    float3x3 rotZ =
    {
        cos(r.z), -sin(r.z), 0,
        sin(r.z), cos(r.z), 0,
        0, 0, 1
    };

    // 회전 적용
    float3 rotatedPos = mul(mul(mul(p, rotX), rotY), rotZ);
    
    // Pass the position directly to the pixel shader (no transformation)
    output.position = float4(rotatedPos * Scale + Offset, 1.0);
    
    // Pass the color to the pixel shader
    output.color = input.color * ColorFactor;
    output.color.a = input.color.a;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
