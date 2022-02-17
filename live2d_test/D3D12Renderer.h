#pragma once

#include "D3D12Manager.h"
#include "D3DCamera.h"
#include "MathHelper.h"
#include "WICImage.h"
#include "GeometryGenerator.h"
#include "DirectionalLight.h"
#include "Model.h"
#include "GameTimer.h"

#include <vector>
#include <array>

namespace D3D
{
    class D3D12Renderer
    {
#pragma pack(push,1)
        struct ObjectConstants
        {
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 local_mat = MathHelper::Identity4x4();          //本地空间矩阵，缩放，旋转，平移
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 world_mat = MathHelper::Identity4x4();          //世界空间矩阵，主要用来转换到世界坐标
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 model_mat = MathHelper::Identity4x4();          //模型矩阵，等于本地 * 世界
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 view_mat = MathHelper::Identity4x4();           //屏幕空间矩阵，转换到屏幕空间
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 proj_mat = MathHelper::Identity4x4();           //投影矩阵
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 view_proj_mat = MathHelper::Identity4x4();      //屏幕空间投影矩阵
            STRUCT_ALIGN_16 DirectX::XMFLOAT4X4 texture_transform = MathHelper::Identity4x4();
        };

        struct LightConstBuffer 
        {
            STRUCT_ALIGN_16 uint32_t light_nums = 0;
            STRUCT_ALIGN_16 DirectionalLight dir_light_arr[1];
        };
#pragma pack(pop)

    public:
        D3D12Renderer(HWND hwnd, int width, int height);
        ~D3D12Renderer();

        Camera& GetCamera();

        float AspectRatio() const;

        void Initialize();
        void ClearUp();
        void Update();
        void Render();

        static std::array<const D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

        virtual void OnMouseDown(uint8_t btn, uint32_t x, uint32_t y);
        virtual void OnMouseMove(uint32_t x, uint32_t y);
        virtual void OnMouseUp(uint8_t btn, uint32_t x, uint32_t y);

        virtual void OnKeyDown(uint32_t key);
        virtual void OnKeyUp(uint32_t key);

    private:
        int GetCurrentRenderTargetIndex();
        void FlushCommandQueue();
        void InitVertexIndexBuffer();
        void InitImageResource();
        void InitLight();

        void HandleInput(float duration);

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
        float                                               camera_move_speed_ = 1.0f;

        Microsoft::WRL::ComPtr<IWICBitmapSource>            image_resource_;

        static const uint32_t                               MAX_LIGHT_NUM_ = 10;
        Microsoft::WRL::ComPtr<ID3D12Resource>              light_buffer_resource_;
        LightConstBuffer                                    light_const_buffer_;
        DirectionalLight                                    dir_light_;

        Model                                               model_;

        GameTimer                                           timer_;

        bool                                                mouse_click_ = false;
        int32_t                                             mouse_start_x_ = 0;
        int32_t                                             mouse_start_y_ = 0;
        DirectX::XMFLOAT3                                   start_look_at_ = {};
        DirectX::XMFLOAT3                                   start_up_ = {};
    };

};


