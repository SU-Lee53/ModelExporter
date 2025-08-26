#include "ShaderResources.hlsl"

float4 SkinningPosition(float3 pos, int indices[4], float weights[4])
{
    float4 skinned = float4(0, 0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        int boneIndex = indices[i];
        float weight = weights[i];
        if (weight > 0)
        {
            skinned += mul(float4(pos, 1.0f), mtxTransforms[boneIndex]) * weight;
        }
    }
    return skinned;
}

float3 SkinningNormal(float3 normal, int indices[4], float weights[4])
{
    float3 skinned = float3(0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        int boneIndex = indices[i];
        float weight = weights[i];
        if (weight > 0)
        {
            // w=0 으로 곱해서 방향벡터만 변환
            skinned += mul(float4(normal, 0.0f), mtxTransforms[boneIndex]).xyz * weight;
        }
    }
    return normalize(skinned);
}


VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // 스키닝된 정점
    float4 skinnedPos = SkinningPosition(input.pos, input.blendIndices, input.blendWeight);
    float3 skinnedNormal = SkinningNormal(input.normal, input.blendIndices, input.blendWeight);
    float3 skinnedTangent = SkinningNormal(input.tangent, input.blendIndices, input.blendWeight);
    float3 skinnedBiTangent = SkinningNormal(input.biTangent, input.blendIndices, input.blendWeight);

    float4 worldPos = mul(skinnedPos, gmtxWorld);
    
    output.worldPos = worldPos;
    output.pos = mul(worldPos, gmtxViewProjection);
    output.worldNormal = mul(float4(skinnedNormal, 0.f), gmtxWorld);
    output.worldTangent = mul(float4(skinnedTangent, 0.f), gmtxWorld);
    output.worldBiTangent = mul(float4(skinnedBiTangent, 0.f), gmtxWorld);
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