#pragma once

#include "D3D12Manager.h"
#include "DirectXMath.h"
#include "D3DCamera.h"
#include "GeometryGenerator.h"
#include "D3D12BoundResourceManager.h"

namespace D3D
{
    class SkyBoxPass
    {
    public:
        SkyBoxPass();
        ~SkyBoxPass();

        void Initialize();
        void Update(const Camera& camera);
        void PopulateCommandList(ID3D12GraphicsCommandList* cmd);

    private:
        Microsoft::WRL::ComPtr<ID3DBlob>                    vs_shader_;
        Microsoft::WRL::ComPtr<ID3DBlob>                    ps_shader_;
        ID3D12RootSignature*                                root_signature_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>         pso_;
        Microsoft::WRL::ComPtr<ID3D12Resource>              sky_texture_;
        Microsoft::WRL::ComPtr<ID3D12Resource>              view_proj_buffer_;

        D3D12_VERTEX_BUFFER_VIEW                            vert_buffer_view_ = {};
        D3D12_INDEX_BUFFER_VIEW                             index_buffer_view_ = {};
        Microsoft::WRL::ComPtr<ID3D12Resource>              vert_buffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>              index_buffer;

        D3D12BoundResourceManager                           bund_resource_manager_;

        GeometryGenerator::MeshData                         mesh_data_;
    };
};
