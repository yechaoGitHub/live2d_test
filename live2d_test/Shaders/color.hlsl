#include "Light.hlsi"
#include "CommonDefine.hlsi"

cbuffer LightConstBuffer : register(b1)
{
    uint DIRECTIONAL_LIGHT_NUM;
    uint POINT_LIGHT_NUM;
};

StructuredBuffer<DirectionalLight> DIRECTIONAL_LIGHT_BUFFER : register(t0);
StructuredBuffer<OrgePointLight> POINT_LIGHT_BUFFER : register(t1);

Texture2D TEXTURE : register(t2);
SamplerState SAMPLER : register(s0);

float4 PS_Main(VertexOut pin) : SV_Target
{
    float3 text_color = TEXTURE.Sample(SAMPLER, pin.TexC);
    float3 out_color = float4(0.0, 0.0, 0.0, 1.0);
    for (uint i = 0; i < DIRECTIONAL_LIGHT_NUM; i++)
    {
        float dir_coe = dot(pin.NormalW, normalize(-DIRECTIONAL_LIGHT_BUFFER[i].direction));
        float3 dir_color = DIRECTIONAL_LIGHT_BUFFER[i].color * DIRECTIONAL_LIGHT_BUFFER[i].intensity * dir_coe;
        out_color += text_color * dir_color;
    }

    return float4(out_color, 1.0f);
}

