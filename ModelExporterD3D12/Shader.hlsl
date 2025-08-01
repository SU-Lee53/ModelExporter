
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

cbuffer cbMaterialData : register(b2)
{
    float4  f4AlbedoColor;     // c0
    float4  f4SpecularColor;   // c1
    float4  f4AmbientColor;    // c2
    float4  f4EmissiveColor;   // c3
			
    float   fGlossiness;          // c4.x
    float   fSmoothness;          // c4.y
    float   fSpecularHighlight;   // c4.z
    float   fMetallic;            // c4.w
    float   fGlossyReflection;    // c5.x
}

Texture2D texAlbedo : register(t0);
Texture2D texSpecular : register(t1);
Texture2D texMetalic : register(t2);
Texture2D texNormal : register(t3);
Texture2D texEmission : register(t4);
Texture2D texDetailAlbedo : register(t5);
Texture2D texDetailNormal : register(t6);

SamplerState samplerDiffuse : register(s0);

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