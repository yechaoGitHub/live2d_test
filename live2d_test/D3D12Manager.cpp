#include "D3D12Manager.h"
#include "D3DUtil.h"
#include "d3dcompiler.h"


namespace D3D
{
    using namespace Microsoft::WRL;

    D3D12Manager D3D12Manager::D3D12_MANAGER_INSTANCE_;

    D3D12Manager::D3D12Manager()
    {
    }

    D3D12Manager::~D3D12Manager()
    {
    }

    const D3D12Manager& D3D12Manager::GetManager()
    {
        return D3D12_MANAGER_INSTANCE_;
    }

    void D3D12Manager::Initialize()
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;

        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&d3d.dxgi_factory_)));

        ComPtr<IDXGIAdapter1> adapter;
        for (int adapter_index = 0; 
            DXGI_ERROR_NOT_FOUND != d3d.dxgi_factory_->EnumAdapters1(adapter_index, &adapter); 
            ++adapter_index)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }
            
            // Check to see if the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }

        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d.d3d_device_)));
    }

    ID3D12Device* D3D12Manager::GetDevice()
    {
        return D3D12_MANAGER_INSTANCE_.d3d_device_.Get();
    }

    HWND D3D12Manager::CreateD3DWindow(
        const wchar_t* wnd_title, 
        DWORD wnd_style, 
        int wnd_x, 
        int wnd_y, 
        int wnd_width, 
        int wnd_height)
    {
        WNDCLASS wc;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = DefWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = 0;
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = L"live2D test window";

        if (!RegisterClass(&wc))
        {
            MessageBox(0, L"RegisterClass Failed.", 0, 0);
            return 0;
        }

        auto handle = CreateWindow(
            L"live2D test window",
            wnd_title,
            wnd_style, 
            wnd_x,
            wnd_y,
            wnd_width,
            wnd_height, 
            0, 0, 0, 0);

        if (!handle)
        {
            MessageBox(0, L"CreateWindow Failed.", 0, 0);
            return 0;
        }

        ShowWindow(handle, SW_SHOW);
        UpdateWindow(handle);

        return handle;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain1> D3D12Manager::CreateSwapChain(HWND handle, ID3D12CommandQueue* command_queue, int buffer_count, int width, int height, DXGI_FORMAT format, int buffer_usage, DXGI_SWAP_EFFECT swap_effect)
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.BufferCount = buffer_count;
        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.BufferUsage = buffer_usage;
        desc.SwapEffect = swap_effect;
        desc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swap_chain;
        ThrowIfFailed(d3d.dxgi_factory_->CreateSwapChainForHwnd(command_queue, handle, &desc, nullptr, nullptr, &swap_chain));

        return swap_chain;
    }

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12Manager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_FLAGS flag)
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Flags = flag;

        ComPtr<ID3D12CommandQueue> command_queue;
        ThrowIfFailed(GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&command_queue)));

        return command_queue;
    }

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3D12Manager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;

        ComPtr<ID3D12CommandAllocator> command_allocator;
        ThrowIfFailed(GetDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocator)));

        return command_allocator;
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> D3D12Manager::CreateCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* allocator)
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;

        ComPtr<ID3D12GraphicsCommandList> command_list;
        ThrowIfFailed(GetDevice()->CreateCommandList(0, type, allocator, nullptr, IID_PPV_ARGS(&command_list)));

        return command_list;
    }

    Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12Manager::CreatePipeLineStateObject(
        D3D12_INPUT_LAYOUT_DESC input_desc, 
        ID3D12RootSignature* root_signature, 
        D3D12_SHADER_BYTECODE shaders[5], 
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type, 
        DXGI_FORMAT rtv_formats[8], 
        int rtv_num,
        DXGI_FORMAT dsv_format)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = input_desc;
        desc.pRootSignature = root_signature;
        desc.VS = shaders[0];
        desc.PS = shaders[1];
        desc.DS = shaders[2];
        desc.HS = shaders[3];
        desc.GS = shaders[4];
        desc.RasterizerState = DefaultRasterizerDesc();
        desc.BlendState = DefaultBlendDesc();
        desc.DepthStencilState = DefaultDepthStencilDesc();
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = rtv_num;
        for (int i = 0; i < rtv_num; i++)
        {
            desc.RTVFormats[i] = rtv_formats[i];
        }
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.DSVFormat = dsv_format;

        ComPtr<ID3D12PipelineState> pso;
        ThrowIfFailed(GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
        return pso;
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12Manager::CreateRootSignature(D3D12_ROOT_PARAMETER* root_param_arr, int count)
    {
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = count;
        desc.pParameters = root_param_arr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr =  D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

        if (errorBlob != nullptr)
        {
            ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        ComPtr<ID3D12RootSignature> root_signature;
        ThrowIfFailed(GetDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&root_signature)));

        return root_signature;
    }

    Microsoft::WRL::ComPtr<ID3D12Fence> D3D12Manager::CreateFence(uint64_t value)
    {
        ComPtr<ID3D12Fence> fence;
        ThrowIfFailed(GetDevice()->CreateFence(value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        return fence;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> D3D12Manager::CompileShader(const std::wstring& file_path, const std::string& entry_point, const std::string& target)
    {
        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ComPtr<ID3DBlob> byteCode = nullptr;
        ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompileFromFile(file_path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry_point.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

        if (errors != nullptr)
            OutputDebugStringA((char*)errors->GetBufferPointer());
        ThrowIfFailed(hr);

        return byteCode;
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12Manager::CreateDescriptorHeap(int num, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = num;
        desc.Type = type;
        desc.Flags = flag;
        desc.NodeMask = 0;
        
        ComPtr<ID3D12DescriptorHeap> heap;
        GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));

        return heap;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Manager::CreateDepthStencilBuffer(int width, int height, DXGI_FORMAT format, float clear_depth, uint8_t clear_stencil)
    {
        D3D12_RESOURCE_DESC depth_stencil_desc;
        depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depth_stencil_desc.Alignment = 0;
        depth_stencil_desc.Width = width;
        depth_stencil_desc.Height = height;
        depth_stencil_desc.DepthOrArraySize = 1;
        depth_stencil_desc.MipLevels = 1;
        depth_stencil_desc.Format = format;
        depth_stencil_desc.SampleDesc.Count = 1;
        depth_stencil_desc.SampleDesc.Quality = 0;
        depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE opt_clear{};
        opt_clear.Format = format;
        opt_clear.DepthStencil.Depth = clear_depth;
        opt_clear.DepthStencil.Stencil = clear_stencil;

        D3D12_HEAP_PROPERTIES heap_property{};
        heap_property.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_property.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_property.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_property.CreationNodeMask = 1;
        heap_property.VisibleNodeMask = 1;

        ComPtr<ID3D12Resource> depth_stencil_buffer;
        ThrowIfFailed(GetDevice()->CreateCommittedResource(
            &heap_property,
            D3D12_HEAP_FLAG_NONE,
            &depth_stencil_desc,
            D3D12_RESOURCE_STATE_COMMON,
            &opt_clear,
            IID_PPV_ARGS(&depth_stencil_buffer)));

        return depth_stencil_buffer;
    }

    D3D12_RASTERIZER_DESC D3D12Manager::DefaultRasterizerDesc()
    {
        static D3D12_RASTERIZER_DESC desc = 
        {  
            D3D12_FILL_MODE_SOLID, 
            D3D12_CULL_MODE_BACK,
            false,
            D3D12_DEFAULT_DEPTH_BIAS,
            D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
            D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
            true,
            false,
            false,
            0,
            D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF 
        };

        return desc;
    }

    D3D12_BLEND_DESC D3D12Manager::DefaultBlendDesc()
    {
        static D3D12_BLEND_DESC desc = 
        {
            false,
            false,
            {
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
                DefaultRenderTargetBlendDesc(),
            }
        };

        return desc;
    }

    D3D12_RENDER_TARGET_BLEND_DESC D3D12Manager::DefaultRenderTargetBlendDesc()
    {
        static D3D12_RENDER_TARGET_BLEND_DESC desc =
        {
            false, false,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };

        return desc;
    }

    D3D12_DEPTH_STENCIL_DESC D3D12Manager::DefaultDepthStencilDesc()
    {
        static D3D12_DEPTH_STENCIL_DESC desc = 
        {
            true,
            D3D12_DEPTH_WRITE_MASK_ALL,
            D3D12_COMPARISON_FUNC_LESS,
            false,
            D3D12_DEFAULT_STENCIL_READ_MASK,
            D3D12_DEFAULT_STENCIL_WRITE_MASK,
            DefaultDepthStencilopDesc(),
            DefaultDepthStencilopDesc()
        };

        return desc;
    }

    D3D12_DEPTH_STENCILOP_DESC D3D12Manager::DefaultDepthStencilopDesc()
    {
        static D3D12_DEPTH_STENCILOP_DESC desc =
        {
            D3D12_STENCIL_OP_KEEP, 
            D3D12_STENCIL_OP_KEEP, 
            D3D12_STENCIL_OP_KEEP, 
            D3D12_COMPARISON_FUNC_ALWAYS
        };

        return desc;
    }

};

