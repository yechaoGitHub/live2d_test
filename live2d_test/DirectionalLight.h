#pragma once

#include "D3DUtil.h"

#include <DirectXMath.h>

namespace D3D
{
#pragma pack(push,1)
    class DirectionalLight
    {
    public:
        float intensity = {};
        DirectX::XMFLOAT3 direction = {};
        DirectX::XMFLOAT3 color = {};
    };
#pragma pack(pop)
};

