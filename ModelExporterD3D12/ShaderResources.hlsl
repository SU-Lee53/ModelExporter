#ifndef _SHADER_RESOURCES_
#define _SHADER_RESOURCES_

struct VS_INPUT
{
    float3  pos             : POSITION;
    float3  normal          : NORMAL;
    float3  tangent         : TANGENT;
    float3  biTangent       : BITANGENT;
    float4  color           : COLOR;
    float2  TexCoord[8]     : TEXCOORD;
    
    int4     blendIndices : BLEND_INDICES;
    float4   blendWeight  : BLEND_WEIGHTS;
    
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
    float4  f4AlbedoColor       : packoffset(c0);
    float4  f4SpecularColor     : packoffset(c1);
    float4  f4AmbientColor      : packoffset(c2);
    float4  f4EmissiveColor     : packoffset(c3);
	
    float   fGlossiness         : packoffset(c4.x);
    float   fSmoothness         : packoffset(c4.y);
    float   fSpecularHighlight  : packoffset(c4.z);
    float   fMetallic           : packoffset(c4.w);
    float   fGlossyReflection   : packoffset(c5.x);
}

Texture2D texAlbedo : register(t0);
Texture2D texSpecular : register(t1);
Texture2D texMetalic : register(t2);
Texture2D texNormal : register(t3);
Texture2D texEmission : register(t4);
Texture2D texDetailAlbedo : register(t5);
Texture2D texDetailNormal : register(t6);

SamplerState samplerDiffuse : register(s0);


// Animation 
#define MAX_ANIMATION_KEYFRAMES     250
#define MAX_ANIMATION_COUNTS        8

cbuffer cbAnimationControllerData : register(b3)
{
    int nCurrentAnimationIndex;
    float fDuration;
    float fTicksPerSecond;
    float fTimeElapsed;
};

cbuffer AnimationTransforms : register(b4)
{
    float4x4 mtxTransforms[MAX_ANIMATION_KEYFRAMES];
};

cbuffer BoneData : register(b5)
{
    int nTransformIndex;
};



#endif