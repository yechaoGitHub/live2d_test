#include "D3D12Manager.h"
#include "D3DUtil.h"
#include "d3dcompiler.h"

#include <map>
#include <regex>
#include <set>

#include "D3D12BoundResourceManager.h"

namespace D3D
{
    using namespace Microsoft::WRL;

    D3D12Manager D3D12Manager::D3D12_MANAGER_INSTANCE_;

    D3D12Manager::D3D12Manager()
    {
    }

    std::string D3D12Manager::GetShaderResourceIdentify(const std::string& raw_name)
    {
        static std::regex id_reg("(\\w+)");
        std::smatch m;
        if (std::regex_search(raw_name, m, id_reg))
        {
            return m[0].str();
        }
        else
        {
            return "";
        }
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

#if defined(DEBUG) || defined(_DEBUG)
        // Enable the D3D12 debug layer.
        {
            ComPtr<ID3D12Debug> debugController;
            ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
            debugController->EnableDebugLayer();
        }
#endif

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

        d3d.copy_resource_manager_.Initialize();
        d3d.copy_resource_manager_.StartUp();
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
        DXGI_FORMAT dsv_format,
        D3D12_RASTERIZER_DESC rast_desc,
        D3D12_BLEND_DESC blend_desc,
        D3D12_DEPTH_STENCIL_DESC depth_stencil_desc)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = input_desc;
        desc.pRootSignature = root_signature;
        desc.VS = shaders[0];
        desc.PS = shaders[1];
        desc.DS = shaders[2];
        desc.HS = shaders[3];
        desc.GS = shaders[4];
        desc.RasterizerState = rast_desc;
        desc.BlendState = blend_desc;
        desc.DepthStencilState = depth_stencil_desc;
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

    Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12Manager::CreateRootSignature(const D3D12_ROOT_PARAMETER* root_param_arr, int count, const D3D12_STATIC_SAMPLER_DESC* static_sampler, uint32_t sampler_count)
    {
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = count;
        desc.pParameters = root_param_arr;
        desc.NumStaticSamplers = sampler_count;
        desc.pStaticSamplers = static_sampler;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr =  ::D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

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

    //Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12Manager::CreateRootSignatureByReflect(ID3DBlob** shader_arr, uint32_t shader_count)
    //{
    //    std::set<std::string> ids;
    //    DescriptorTableBindPointDesc arr_bind_point_desc[4];

    //    for (uint32_t i = 0; i < shader_count; i++)
    //    {
    //        ComPtr<ID3D12ShaderReflection> shader_reflect;
    //        ThrowIfFailed(D3DReflect(shader_arr[i]->GetBufferPointer(), shader_arr[i]->GetBufferSize(), IID_PPV_ARGS(&shader_reflect)));

    //        D3D12_SHADER_DESC shader_desc{};
    //        ThrowIfFailed(shader_reflect->GetDesc(&shader_desc));

    //        for (uint32_t b = 0; b < shader_desc.BoundResources; b++)
    //        {
    //            D3D12_SHADER_INPUT_BIND_DESC bound_desc{};
    //            ThrowIfFailed(shader_reflect->GetResourceBindingDesc(b, &bound_desc));

    //            auto id = GetShaderResourceIdentify(bound_desc.Name);
    //            if (id.empty() || ids.find(bound_desc.Name) != ids.end())
    //            {
    //                continue;
    //            }

    //            ids.insert(id);

    //            DescriptorTableBindPointDesc* ptr_bind_point_desc{};

    //            switch (bound_desc.Type)
    //            {
    //                case D3D_SIT_CBUFFER:
    //                    ptr_bind_point_desc = &arr_bind_point_desc[D3D12_DESCRIPTOR_RANGE_TYPE_CBV];
    //                break;

    //                case D3D_SIT_TBUFFER:
    //                break;

    //                case D3D_SIT_TEXTURE:
    //                case D3D_SIT_STRUCTURED:
    //                    ptr_bind_point_desc = &arr_bind_point_desc[D3D12_DESCRIPTOR_RANGE_TYPE_SRV];
    //                break;

    //                case D3D_SIT_SAMPLER:
    //                    ptr_bind_point_desc = &arr_bind_point_desc[D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER];
    //                break;

    //                case D3D_SIT_UAV_RWTYPED:
    //                break;

    //                case D3D_SIT_UAV_RWSTRUCTURED:
    //                break;

    //                case D3D_SIT_BYTEADDRESS:
    //                break;

    //                case D3D_SIT_UAV_RWBYTEADDRESS:
    //                break;

    //                case D3D_SIT_UAV_APPEND_STRUCTURED:
    //                break;

    //                case D3D_SIT_UAV_CONSUME_STRUCTURED:
    //                break;

    //                case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
    //                break;

    //                case D3D_SIT_RTACCELERATIONSTRUCTURE:
    //                break;

    //                case D3D_SIT_UAV_FEEDBACKTEXTURE:
    //                break;

    //                default:
    //                    ThrowIfFalse(0);
    //                break;
    //            }

    //            if (ptr_bind_point_desc)
    //            {
    //                auto& bind_point = bound_desc.BindPoint;
    //                ptr_bind_point_desc->max_bind_point = (std::max)(ptr_bind_point_desc->max_bind_point, bind_point);
    //                ptr_bind_point_desc->min_bind_point = (std::min)(ptr_bind_point_desc->min_bind_point, bind_point);
    //                ptr_bind_point_desc->count++;
    //            }
    //        }
    //    }

    //    std::vector<D3D12_DESCRIPTOR_RANGE> vec_descriptor_range;
    //    D3D12_DESCRIPTOR_RANGE sampler_range{};
    //    for (uint32_t i = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; i <= D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; i++)
    //    {
    //        auto& bind_point_desc = arr_bind_point_desc[i];
    //        if (bind_point_desc.count == 0)
    //        {
    //            continue;
    //        }

    //        if (i == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
    //        {
    //            sampler_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    //            sampler_range.NumDescriptors = bind_point_desc.max_bind_point - bind_point_desc.min_bind_point + 1;
    //            sampler_range.BaseShaderRegister = bind_point_desc.min_bind_point;
    //            sampler_range.RegisterSpace = 0;
    //            sampler_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    //        }
    //        else
    //        {
    //            D3D12_DESCRIPTOR_RANGE descriptor_range{};
    //            descriptor_range.RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(i);
    //            descriptor_range.NumDescriptors = bind_point_desc.max_bind_point - bind_point_desc.min_bind_point + 1;
    //            descriptor_range.BaseShaderRegister = bind_point_desc.min_bind_point;
    //            descriptor_range.RegisterSpace = 0;
    //            descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //            vec_descriptor_range.push_back(descriptor_range);
    //        }
    //    }

    //    D3D12_ROOT_PARAMETER root_param[2] = {};
    //    uint32_t root_param_count{};
    //    root_param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    root_param[0].DescriptorTable.NumDescriptorRanges = vec_descriptor_range.size();
    //    root_param[0].DescriptorTable.pDescriptorRanges = vec_descriptor_range.data();
    //    root_param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //    root_param_count++;
    //    //if (sampler_range.NumDescriptors > 0)
    //    //{
    //    //    root_param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    //    root_param[1].DescriptorTable.NumDescriptorRanges = 1;
    //    //    root_param[1].DescriptorTable.pDescriptorRanges = &sampler_range;
    //    //    root_param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //    //    root_param_count++;
    //    //}

    //    auto static_samplers = GetDefaultStaticSamplers();

    //    return CreateRootSignature(root_param, root_param_count, static_samplers.data(), static_samplers.size());
    //}

    //std::array<const D3D12_STATIC_SAMPLER_DESC, 6> D3D12Manager::GetDefaultStaticSamplers(uint32_t base_register)
    //{
    //    const D3D12_STATIC_SAMPLER_DESC point_wrap =
    //    {
    //        D3D12_FILTER_MIN_MAG_MIP_POINT,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        0.0f,
    //        16,
    //        D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
    //        0.0f,
    //        D3D12_FLOAT32_MAX,
    //        0 + base_register,
    //        0,
    //        D3D12_SHADER_VISIBILITY_ALL
    //    };

    //    const D3D12_STATIC_SAMPLER_DESC point_clamp =
    //    {
    //        D3D12_FILTER_MIN_MAG_MIP_POINT,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        0.0f,
    //        16,
    //        D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
    //        0.0f,
    //        D3D12_FLOAT32_MAX,
    //        1 + base_register,
    //        0,
    //        D3D12_SHADER_VISIBILITY_ALL
    //    };

    //    const D3D12_STATIC_SAMPLER_DESC linear_wrap =
    //    {
    //        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        0.0f,
    //        16,
    //        D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
    //        0.0f,
    //        D3D12_FLOAT32_MAX,
    //        2 + base_register,
    //        0,
    //        D3D12_SHADER_VISIBILITY_ALL
    //    };

    //    const D3D12_STATIC_SAMPLER_DESC linear_clamp =
    //    {
    //        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        0.0f,
    //        16,
    //        D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
    //        0.0f,
    //        D3D12_FLOAT32_MAX,
    //        3 + base_register,
    //        0,
    //        D3D12_SHADER_VISIBILITY_ALL
    //    };

    //    const D3D12_STATIC_SAMPLER_DESC anisotropic_wrap =
    //    {
    //        D3D12_FILTER_ANISOTROPIC,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    //        0.0f,
    //        8,
    //        D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
    //        0.0f,
    //        D3D12_FLOAT32_MAX,
    //        4 + base_register,
    //        0,
    //        D3D12_SHADER_VISIBILITY_ALL
    //    };

    //    const D3D12_STATIC_SAMPLER_DESC anisotropic_clamp =
    //    {
    //        D3D12_FILTER_ANISOTROPIC,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    //        0.0f,
    //        8,
    //        D3D12_COMPARISON_FUNC_LESS_EQUAL,
    //        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
    //        0.0f,
    //        D3D12_FLOAT32_MAX,
    //        5 + base_register,
    //        0,
    //        D3D12_SHADER_VISIBILITY_ALL
    //    };

    //    return {
    //        point_wrap, point_clamp,
    //        linear_wrap, linear_clamp,
    //        anisotropic_wrap, anisotropic_clamp };
    //}


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
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &opt_clear,
            IID_PPV_ARGS(&depth_stencil_buffer)));

        return depth_stencil_buffer;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE type, uint64_t byte_size)
    {
        D3D12_HEAP_PROPERTIES heap_properties{};
        heap_properties.Type = type;
        heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resource_desc{};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment = 0;
        resource_desc.Width = byte_size;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_RESOURCE_STATES init_state{};
        switch (type)
        {
            case D3D12_HEAP_TYPE_DEFAULT:
                init_state = D3D12_RESOURCE_STATE_COMMON;
            break;

            case D3D12_HEAP_TYPE_UPLOAD:
                init_state = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;

            case D3D12_HEAP_TYPE_READBACK:
                init_state = D3D12_RESOURCE_STATE_COPY_DEST;
            break;

            default:
                init_state = D3D12_RESOURCE_STATE_COMMON;
            break;
        }

        ComPtr<ID3D12Resource> buffer;
        ThrowIfFailed(GetDevice()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, init_state, nullptr, IID_PPV_ARGS(&buffer)));
        return buffer;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Manager::CreateTexture(uint32_t width, uint32_t height, DXGI_FORMAT format, uint16_t array_size)
    {
        D3D12_HEAP_PROPERTIES heap_properties{};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resource_desc{};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment = 0;
        resource_desc.Width = width;
        resource_desc.Height = height;
        resource_desc.DepthOrArraySize = array_size;
        resource_desc.MipLevels = 1;
        resource_desc.Format = format;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ComPtr<ID3D12Resource> texture;
        ThrowIfFailed(GetDevice()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture)));
        return texture;
    }

    D3D12Manager::ResourceLayout D3D12Manager::GetCopyableFootprints(ID3D12Resource* resource, uint32_t first_resource_index, uint32_t num_resources, uint64_t base_offset)
    {
        ResourceLayout ret;
        ret.fontprints.resize(num_resources);
        ret.num_rows.resize(num_resources);
        ret.row_size_in_bytes.resize(num_resources);

        auto desc = resource->GetDesc();
        GetDevice()->GetCopyableFootprints(&desc, first_resource_index, num_resources, base_offset, ret.fontprints.data(), ret.num_rows.data(), ret.row_size_in_bytes.data(), &ret.total_byte_size);

        return ret;
    }

    uint64_t D3D12Manager::PostUploadBufferTask(ID3D12Resource* d3d_dest_resource, uint64_t dest_offset, void* copy_data, uint64_t copy_lenght, D3D12_RESOURCE_STATES res_state_before, D3D12_RESOURCE_STATES res_state_after)
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;
        return d3d.copy_resource_manager_.PostUploadBufferTask(d3d_dest_resource, dest_offset, copy_data, copy_lenght, res_state_before, res_state_after);
    }

    uint64_t D3D12Manager::PostUploadTextureTask(ID3D12Resource* d3d_dest_resource, uint32_t first_subresource, uint32_t subresource_count, void* copy_data, const ImageLayout* image_layout, D3D12_RESOURCE_STATES res_state_before, D3D12_RESOURCE_STATES res_state_after)
    {
        auto& d3d = D3D12_MANAGER_INSTANCE_;
        return d3d.copy_resource_manager_.PostUploadTextureTask(d3d_dest_resource, first_subresource, subresource_count, copy_data, image_layout, res_state_before, res_state_after);
    }

    uint64_t D3D12Manager::GetCurCopyTaskID()
    {
        auto& copy_manager = D3D12_MANAGER_INSTANCE_.copy_resource_manager_;
        return copy_manager.GetCurTaskID();
    }

    uint64_t D3D12Manager::GetCopyExcuteCount()
    {
        auto& copy_manager = D3D12_MANAGER_INSTANCE_.copy_resource_manager_;
        return copy_manager.GetExcuteCount();
    }

    bool D3D12Manager::WaitCopyTask(uint64_t copy_task_id)
    {
        auto& copy_manager = D3D12_MANAGER_INSTANCE_.copy_resource_manager_;

        ThrowIfFailed(copy_manager.GetCurTaskID() >= copy_task_id);

        while (copy_task_id > copy_manager.GetExcuteCount())
        {
            std::this_thread::yield();
        }

        return true;
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

