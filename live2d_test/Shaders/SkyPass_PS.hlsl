#include "SkyPass_VS.hlsl"

TextureCube cube_texture : register(t0);
SamplerState default_sampler[6] : register(s0);

//const static float2 invAtan = float2(0.1591, 0.3183);
//float2 SampleSphericalMap(float3 v)
//{
//    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
//    uv *= invAtan;
//    uv += 0.5;
//    uv.y = 1.0 - uv.y;
//    return uv;
//}

float4 PS_Main(SkyPassVertexOut pin) : SV_Target
{
    float3 color = cube_texture.Sample(default_sampler[2], normalize(pin.position));
    return float4(color, 1.0f);
}
