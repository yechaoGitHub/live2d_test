#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <string>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

namespace D3D
{
    //to do 添加一个默认的上传，下载资源的辅助渲染队列
    class D3D12Manager
    {
    public:
        ~D3D12Manager();
   
        static const D3D12Manager& GetManager();

        static void Initialize();

        static ID3D12Device* GetDevice();

        static HWND CreateD3DWindow(
            const wchar_t* wnd_title,
            DWORD wnd_style,
            int wnd_x,
            int wnd_y,
            int wnd_width,
            int wnd_height);

        static Microsoft::WRL::ComPtr<IDXGISwapChain1> CreateSwapChain(HWND handle, ID3D12CommandQueue* command_queue, int buffer_count, int width, int height, DXGI_FORMAT format, int buffer_usage, DXGI_SWAP_EFFECT swap_effect);

        static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_FLAGS flag);

        static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);

        static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* allocator);

        static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipeLineStateObject(
            D3D12_INPUT_LAYOUT_DESC input_desc, 
            ID3D12RootSignature* root_signature, 
            D3D12_SHADER_BYTECODE shaders[5], 
            D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type,
            DXGI_FORMAT rtv_formats[8],
            int rtv_num,
            DXGI_FORMAT dsv_format);

        static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(D3D12_ROOT_PARAMETER* root_param_arr, int count);

        static Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(uint64_t value);

        static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& file_path, const std::string &entry_point, const std::string &target);

        static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(int num, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag);

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilBuffer(int width, int height, DXGI_FORMAT format, float clear_depth, uint8_t clear_stencil);

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(D3D12_HEAP_TYPE type, uint64_t byte_size);

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture(uint32_t width, uint32_t height);

    private:
        D3D12Manager();

        static D3D12Manager D3D12_MANAGER_INSTANCE_;

        static D3D12_RASTERIZER_DESC DefaultRasterizerDesc();
        static D3D12_BLEND_DESC DefaultBlendDesc();
        static D3D12_RENDER_TARGET_BLEND_DESC DefaultRenderTargetBlendDesc();
        static D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc();
        static D3D12_DEPTH_STENCILOP_DESC DefaultDepthStencilopDesc();

        Microsoft::WRL::ComPtr<IDXGIFactory4>               dxgi_factory_;
        Microsoft::WRL::ComPtr<ID3D12Device>                d3d_device_;
    };
};

