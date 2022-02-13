#pragma once

#include "D3D12Manager.h"
#include "D3DCamera.h"
#include "MathHelper.h"
#include "WICImage.h"
#include "GeometryGenerator.h"

#include <vector>
#include <array>

namespace D3D
{
    class D3D12Renderer
    {
        struct ObjectConstants
        {
            DirectX::XMFLOAT4X4 local_mat = MathHelper::Identity4x4();          //本地空间矩阵，缩放，旋转，平移
            DirectX::XMFLOAT4X4 world_mat = MathHelper::Identity4x4();          //世界空间矩阵，主要用来转换到世界坐标
            DirectX::XMFLOAT4X4 model_mat = MathHelper::Identity4x4();          //模型矩阵，等于本地 * 世界
            DirectX::XMFLOAT4X4 view_mat = MathHelper::Identity4x4();           //屏幕空间矩阵，转换到屏幕空间
            DirectX::XMFLOAT4X4 proj_mat = MathHelper::Identity4x4();           //投影矩阵
            DirectX::XMFLOAT4X4 view_proj_mat = MathHelper::Identity4x4();      //屏幕空间投影矩阵
            DirectX::XMFLOAT4X4 texture_transform = MathHelper::Identity4x4();
        };

    public:
        D3D12Renderer(HWND hwnd, int width, int height);
        ~D3D12Renderer();

        float AspectRatio() const;

        void Initialize();
        void Update();
        void Render();

        static std::array<const D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    private:
        int GetCurrentRenderTargetIndex();
        void FlushCommandQueue();
        void InitVertexIndexBuffer();
        void InitImageResource();

        HWND                                                window_handle_{};
        int                                                 client_width_{};
        int                                                 client_height_{};
        uint64_t                                            fence_value_{};
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          command_queue_;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      command_list_alloc_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   command_list_;
        Microsoft::WRL::ComPtr<IDXGISwapChain>              swap_chain_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>         root_signature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>         pipe_line_state_;
        Microsoft::WRL::ComPtr<ID3D12Fence>                 fence_;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        rtv_heap_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        dsv_heap_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        tex_heap_;

        Microsoft::WRL::ComPtr<ID3D12Resource>              back_target_buffer_[2];
        Microsoft::WRL::ComPtr<ID3D12Resource>              depth_stencil_buffer_;
        Microsoft::WRL::ComPtr<ID3D12Resource>              texture_;

        D3D12_VERTEX_BUFFER_VIEW                            vertex_buffer_view_{};
        Microsoft::WRL::ComPtr<ID3D12Resource>              vertex_buffer_;
        D3D12_INDEX_BUFFER_VIEW                             index_buffer_view_{};
        Microsoft::WRL::ComPtr<ID3D12Resource>              index_buffer_;
        Microsoft::WRL::ComPtr<ID3D12Resource>              upload_buffer_;
        Microsoft::WRL::ComPtr<ID3D12Resource>              const_buffer_;

        static GeometryGenerator                            GEO_GENERATOR_;
        GeometryGenerator::MeshData                         mesh_data_;

        Microsoft::WRL::ComPtr<ID3DBlob>                    vs_shader_;
        Microsoft::WRL::ComPtr<ID3DBlob>                    ps_shader_;

        D3D12_VIEWPORT                                      screen_viewport_{};
        D3D12_RECT                                          scissor_rect_{};

        Camera                                              camera_;

        Microsoft::WRL::ComPtr<IWICBitmapSource>            image_resource_;

    };

};


