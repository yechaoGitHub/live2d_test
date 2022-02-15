#include "Model.h"

namespace D3D
{
    using namespace DirectX;

    Model::Model()
    {
        model_matrix_ = XMLoadFloat4x4(&model_matrix_4x4_);
    }

    Model::~Model()
    {
    }

    void Model::SetScale(const DirectX::XMFLOAT3& scale)
    {
        scale_ = scale;
        matrix_dirty_ = true;
    }

    void Model::SetOriention(const DirectX::XMFLOAT3& oriention)
    {
        auto v_oriention = XMLoadFloat3(&oriention);
        XMStoreFloat3(&oriention_, XMVector3Normalize(v_oriention));

        matrix_dirty_ = true;
    }

    void Model::SetLocation(const DirectX::XMFLOAT3& location)
    {
        location_ = location;
        matrix_dirty_ = true;
    }

    const DirectX::XMFLOAT4X4& Model::GetModelMatrix4x4()
    {
        if (matrix_dirty_) 
        {
            UpdateMatrix();
        }

        return model_matrix_4x4_;
    }

    const DirectX::XMMATRIX& Model::GetModelMatrix()
    {
        if (matrix_dirty_)
        {
            UpdateMatrix();
        }

        return model_matrix_;
    }

    const DirectX::XMFLOAT3& Model::GetScale()
    {
        return scale_;
    }

    const DirectX::XMFLOAT3& Model::GetOriention()
    {
        return oriention_;
    }

    const DirectX::XMFLOAT3& Model::GetLocation()
    {
        return location_;
    }

    void Model::AddScale(const DirectX::XMFLOAT3& scale)
    {
        scale_.x += scale.x;
        scale_.y += scale.y;
        scale_.z += scale.z;

        matrix_dirty_ = true;
    }

    void Model::AddOriention(const DirectX::XMFLOAT3& oriention)
    {
        oriention_.x += oriention.x;
        oriention_.y += oriention.y;
        oriention_.z += oriention.z;

        matrix_dirty_ = true;
    }

    void Model::AddLocation(const DirectX::XMFLOAT3& location)
    {
        location_.x += location.x;
        location_.y += location.y;
        location_.z += location.z;

        matrix_dirty_ = true;
    }

    void Model::UpdateMatrix()
    {
        auto xm_scale = XMMatrixScaling(scale_.x, scale_.y, scale_.z);
        auto xm_oriention = XMMatrixRotationRollPitchYaw(oriention_.x, oriention_.y, oriention_.z);
        auto xm_location = XMMatrixTranslation(location_.x, location_.y, location_.z);
        model_matrix_ = xm_scale * xm_oriention * xm_location;

        XMStoreFloat4x4(&model_matrix_4x4_, model_matrix_);
    }
};
