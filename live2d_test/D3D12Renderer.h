#pragma once

#include "D3D12Manager.h"

namespace D3D
{
    class D3D12Renderer
    {
    public:
        D3D12Renderer(HWND hwnd, int width, int height);
        ~D3D12Renderer();

        void Initialize();

        void Render();

    private:
        int GetCurrentRenderTargetIndex();
        void FlushCommandQueue();

        HWND                                                window_handle_{};
        int                                                 client_width_{};
        int                                                 client_height_{};
        uint64_t                                            fence_value_{};
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          command_queue_;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      direct_cmd_list_alloc_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   command_list_;
        Microsoft::WRL::ComPtr<IDXGISwapChain>              swap_chain_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>         root_signature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>         pipe_line_state_;
        Microsoft::WRL::ComPtr<ID3D12Fence>                 fence_;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        rtv_heap_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        dsv_heap_;
        Microsoft::WRL::ComPtr<ID3D12Resource>              back_target_buffer_[2];
        Microsoft::WRL::ComPtr<ID3D12Resource>              depth_stencil_buffer_;

        Microsoft::WRL::ComPtr<ID3DBlob> vs_shader_;
        Microsoft::WRL::ComPtr<ID3DBlob> ps_shader_;

        D3D12_VIEWPORT  screen_viewport_{};
        D3D12_RECT      scissor_rect_{};


    };

};


