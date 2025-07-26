
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
    float4 worldPos : POSITION;
    float4 worldNormal : NORMAL;
    float4 worldTangent : TANGENT;
    float4 worldBiTangent : BITANGENT;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD;
};

cbuffer cbCameraData : register(b0)
{
    matrix gmtxViewProjection;
};

cbuffer cbWorldTransformData : register(b1)
{
    matrix gmtxLocal;
    matrix gmtxWorld;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    matrix mtxToWorld = mul(gmtxLocal, gmtxWorld);
    output.worldPos = mul(float4(input.pos, 1.0f), mtxToWorld);
    output.pos = mul(output.worldPos, gmtxViewProjection);
    output.worldNormal = mul(float4(input.normal, 0.f), mtxToWorld);
    output.worldTangent = mul(float4(input.tangent, 0.f), mtxToWorld);
    output.worldBiTangent = mul(float4(input.biTangent, 0.f), mtxToWorld);
    output.color = input.color;
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.worldNormal;
}