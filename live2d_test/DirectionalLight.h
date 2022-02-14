#pragma once

#include "D3DUtil.h"

#include <DirectXMath.h>

namespace D3D
{
#pragma pack(push,1)
    class DirectionalLight
    {
    public:
        STRUCT_ALIGN_16 DirectX::XMFLOAT3 direction = {};
        STRUCT_ALIGN_16 DirectX::XMFLOAT3 color = {};
        float intensity = 0.0f;
    };
#pragma pack(pop)
};

