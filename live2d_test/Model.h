#pragma once
#include <DirectXMath.h>
#include "MathHelper.h"

namespace D3D
{
    class Model
    {
    public:
        Model();
        ~Model();

        void SetScale(const DirectX::XMFLOAT3& scale);
        void SetOriention(const DirectX::XMFLOAT3& oriention);
        void SetLocation(const DirectX::XMFLOAT3& location);

        void AddScale(const DirectX::XMFLOAT3& scale);
        void AddOriention(const DirectX::XMFLOAT3& oriention);
        void AddLocation(const DirectX::XMFLOAT3& location);

        const DirectX::XMFLOAT3& GetScale();
        const DirectX::XMFLOAT3& GetOriention();
        const DirectX::XMFLOAT3& GetLocation();
        const DirectX::XMFLOAT4X4& GetModelMatrix4x4();
        const DirectX::XMMATRIX& GetModelMatrix();

    private:
        DirectX::XMFLOAT3 scale_ = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 oriention_ = { 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 location_ = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4X4 model_matrix_4x4_ = MathHelper::Identity4x4();
        DirectX::XMMATRIX model_matrix_ = {};

        bool matrix_dirty_ = false;

        void UpdateMatrix();

    };
};

