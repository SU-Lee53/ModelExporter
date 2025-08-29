#include "ShaderResources.hlsl"

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT o;
    float wsum = input.blendWeight.x + input.blendWeight.y + input.blendWeight.z + input.blendWeight.w;
    bool skinned = (wsum > 1e-5);

    float4 modelPos;
    float3 modelNormal;

    if (skinned)
    {
        // 스키닝 (모델 로컬)
        float4 p = float4(input.pos, 1.0f);
        float3 n = input.normal;

        float4 sp = 0;
        float3 sn = 0;
        [unroll]
        for (int i = 0; i < 4; i++)
        {
            uint idx = input.blendIndices[i];
            //float w = input.blendWeight[i];
            float w = input.blendWeight[i];
            float4x4 M = mtxBoneTransforms[idx];

            sp += mul(p, M) * w;
            float3 n3 = mul(float4(n, 0), M).xyz;
            sn += n3 * w;
        }

        modelPos = sp;
        modelNormal = normalize(sn);

        // 스키닝 결과는 "모델 로컬" → 월드만 적용
        float4 worldPos = mul(modelPos, gmtxWorld);
        o.pos = mul(worldPos, gmtxViewProjection);
        o.worldPos = worldPos;
        o.worldNormal = normalize(mul(float4(modelNormal, 0), gmtxWorld));
    }
    else
    {
        // 비스키닝 (기존 경로) : 로컬→월드
        float4x4 mtxToWorld = mul(gmtxLocal, gmtxWorld);
        float4 worldPos = mul(float4(input.pos, 1), mtxToWorld);
        o.pos = mul(worldPos, gmtxViewProjection);
        o.worldPos = worldPos;
        o.worldNormal = normalize(mul(float4(input.normal, 0), mtxToWorld));
    }

    o.TexCoord = input.TexCoord[0];
    o.color = input.color;
    return o;
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