#pragma once

#include "D3D12Manager.h"

#include "imgui/imgui.h"

namespace D3D
{
    class ImGuiProxy
    {
    public:
        static void Initialize();


    private:
        static void InitRootSignature();
        static void InitShaderPSO();
        static void InitFontTexture();

        static struct ImGuiProxyContext
        {
            ImGuiContext*                                   imgui_context = nullptr;
            Microsoft::WRL::ComPtr<ID3D12PipelineState>     pso;
            Microsoft::WRL::ComPtr<ID3D12RootSignature>     root_signature;
            Microsoft::WRL::ComPtr<ID3D12Resource>          font_texture;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    srv_heap;
        } IMGUI_CONTEXT_;



    };

};


