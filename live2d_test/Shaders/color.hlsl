#include "light.hlsi"

cbuffer cbPerObject : register(b0)
{
    float4x4 LOCAL_MAT;         //���ؿռ�������ţ���ת��ƽ��
    float4x4 WORLD_MAT;         //����ռ������Ҫ����ת������������
    float4x4 MODEL_MAT;         //���� * ����
    float4x4 VIEW_MAT;          //��Ļ�ռ����ת������Ļ�ռ�
    float4x4 PROJ_MAT;          //ͶӰ����
    float4x4 VIEW_PROJ_MAT;     //��Ļ�ռ�ͶӰ����
    float4x4 TEX_TRANSFORM;
};

cbuffer LightConstBuffer : register(b1)
{
    uint DIRECTIONAL_LIGHT_NUM;
    uint POINT_LIGHT_NUM;
};

StructuredBuffer<DirectionalLight> DIRECTIONAL_LIGHT_BUFFER : register(t0);
StructuredBuffer<OrgePointLight> DIRECTIONAL_LIGHT_BUFFER : register(t1);

Texture2D gDiffuseMap : register(t2);
SamplerState gsamLinear : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentU : TANGENT;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    vout.PosW = mul(float4(vin.PosL, 1.0f), MODEL_MAT).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) MODEL_MAT);
    vout.PosH = mul(float4(vout.PosW, 1.0f), VIEW_PROJ_MAT);
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), TEX_TRANSFORM);
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = gDiffuseMap.Sample(gsamLinear, pin.TexC);

    for (uint i = 0; i < DIRECTIONAL_LIGHT_NUM; i++)
    {
        float dir_coe = dot(pin.NormalW, normalize(DIRECTIONAL_LIGHT_BUFFER[i].direction));
        color += color * dir_coe * DIRECTIONAL_LIGHT_BUFFER[i].intensity;
    }
    
    return color;
}


