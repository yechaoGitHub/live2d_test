#pragma once

#include "D3D12Manager.h"
#include "imgui/imgui.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>

namespace D3D
{
    class ImGuiProxy
    {
    public:
        static void Initialize();
        static void Uninitialize();
        static void RegisterKeyboardEvent(const ImGuiKey* keys, int count);
        static void UnregisterKeyboardEvent(const ImGuiKey* keys, int count);

        static void HandleInputEvent(HWND hwnd);
        static void AddKeyEvent(ImGuiKey key, bool down);
        static void AddMousePosEvent(float x, float y);
        static void AddMouseButtonEvent(int mouse_button, bool down);
        static void PopulateCommandList(ID3D12GraphicsCommandList* cmd);

    private:

        enum MouseButtonState
        {
            kNoneClick = 0,
            kLButtonClick = 1,
            kRButtonClick = 2
        };

        static void InitRootSignature();
        static void InitShaderPSO();
        static void InitVertexIndexBuffer();
        static void InitFontTexture();

        static int ImGuiKeyToVk(ImGuiKey key);
        static bool IsVkDown(int vk, bool test_last_click);

        static void HandleMouseInput(HWND hwnd);
        static void HandleKeyboardInput(HWND hwnd);

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
            std::vector<ImGuiKey>                           register_keys;
            int                                             mouse_button_state = kNoneClick;
        } IMGUI_CONTEXT_;

        static const std::unordered_map<ImGuiKey, int>      IMKEY_TO_VK_MAP_;
    };

};


