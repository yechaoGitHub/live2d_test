#include "CommonDefine.hlsi"

struct SkyPassVertexOut
{
    float4 sv_position : SV_POSITION;
    float3 position : POSITION;
};

cbuffer VS_MatrixBuffer : register(b0)
{
    float4x4 VIEW_MAT;
};

SkyPassVertexOut VS_Main(VertexIn vin)
{
    SkyPassVertexOut vo = (SkyPassVertexOut) 0.0f;
    vo.sv_position = mul(float4(vin.PosL, 0.0f), VIEW_MAT).xyww;
    vo.position = vin.PosL;
    return vo;
}
