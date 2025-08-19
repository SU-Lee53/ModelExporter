#include "ShaderResources.hlsl"

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
    output.TexCoord = input.TexCoord[0];
    
    return output;
}

// ====================
// Diffused PS
// ====================
float4 PSDiffused(VS_OUTPUT input) : SV_TARGET
{
    float4 texColor = texAlbedo.Sample(samplerDiffuse, input.TexCoord);
    float4 f4Color = (f4AlbedoColor + f4SpecularColor + f4AmbientColor + f4EmissiveColor);
    return texColor;
}

// ====================
// Albedo PS
// ====================
float4 PSAlbedo(VS_OUTPUT input) : SV_TARGET
{
    return f4AlbedoColor;
}