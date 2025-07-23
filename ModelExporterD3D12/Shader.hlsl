
struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : POSITION;
    float3 tangent : POSITION;
    float3 biTangent : POSITION;
    float4 color : COLOR;
    float2 TexCoord[8] : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer cbCameraData : register(b0)
{
    matrix gmtxViewProjection;
};

cbuffer cbWorldTransformData : register(b1)
{
    matrix gmtxWorld;
}

VS_OUTPUT VSDiffused(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.pos = mul(mul(float4(input.pos, 1), gmtxWorld), gmtxViewProjection);
    output.color = input.color;
    
    return output;
}

float4 PSDiffused(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}