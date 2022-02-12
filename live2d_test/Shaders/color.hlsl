Texture2D gDiffuseMap : register(t0);
SamplerState gsamLinear : register(s0);

cbuffer cbPerObject : register(b0)
{
    float4x4 LOCAL_MAT;         //模型空间矩阵，缩放，旋转，平移
    float4x4 WORLD_MAT;         //世界空间矩阵，主要用来转换到世界坐标
    float4x4 VIEW_MAT;          //屏幕空间矩阵，转换到屏幕空间
    float4x4 PROJ_MAT;          //投影矩阵
    float4x4 VIEW_PROJ_MAT;     //屏幕空间投影矩阵
    float4x4 TEX_TRANSFORM;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
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
	
    vout.PosW = mul(float4(vin.PosL, 1.0f), WORLD_MAT).xyz;
	// Transform to homogeneous clip space.
    vout.PosH = mul(float4(vout.PosW, 1.0f), VIEW_PROJ_MAT);
    
    vout.NormalW = mul(vin.NormalL, (float3x3)VIEW_MAT);
	

    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 0.0f, 1.0f);

}


