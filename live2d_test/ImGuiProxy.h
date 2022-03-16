#pragma once

#include "D3D12Manager.h"
#include "imgui/imgui.h"
#include <DirectXMath.h>

namespace D3D
{
    class ImGuiProxy
    {
    public:
        static void Initialize();
        static void Unitialize();

        static void PopulateCommandList(ID3D12GraphicsCommandList* cmd);

    private:
        static void InitRootSignature();
        static void InitShaderPSO();
        static void InitVertexIndexBuffer();
        static void InitFontTexture();

        static struct ImGuiProxyContext
        {
            ImGuiContext*                                   imgui_context = nullptr;
            Microsoft::WRL::ComPtr<ID3D12PipelineState>     pso;
            Microsoft::WRL::ComPtr<ID3D12RootSignature>     root_signature;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    srv_heap;
            Microsoft::WRL::ComPtr<ID3D12Resource>          font_texture;
            Microsoft::WRL::ComPtr<ID3D12Resource>          vertex_buffer;
            int                                             vertex_buffer_size = 0;
            Microsoft::WRL::ComPtr<ID3D12Resource>          index_buffer;
            int                                             index_buffer_size = 0;
            DirectX::XMFLOAT4X4                             mvp = {};
        } IMGUI_CONTEXT_;
    };

};


