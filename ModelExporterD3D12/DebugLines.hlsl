cbuffer cbCamera : register(b0)
{
    float4x4 gViewProj;
}

struct VSIn
{
    float3 pos : POSITION;
    float4 col : COLOR;
};
struct VSOut
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

VSOut VSMain(VSIn i)
{
    VSOut o;
    o.pos = mul(float4(i.pos, 1), gViewProj);
    o.col = i.col;
    return o;
}
float4 PSMain(VSOut i) : SV_Target
{
    return i.col;
}
