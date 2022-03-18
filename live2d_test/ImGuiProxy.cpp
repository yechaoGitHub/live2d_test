#include "ImGuiProxy.h"
#include "d3dcompiler.h"


namespace D3D
{
    using namespace Microsoft::WRL;

    ImGuiProxy::ImGuiProxyContext ImGuiProxy::IMGUI_CONTEXT_;

    #define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)

    const std::unordered_map<ImGuiKey, int> ImGuiProxy::IMKEY_TO_VK_MAP_ =
    {
        {ImGuiKey_Tab,          VK_TAB},
        {ImGuiKey_LeftArrow,    VK_LEFT},
        {ImGuiKey_RightArrow,   VK_RIGHT},
        {ImGuiKey_UpArrow,      VK_UP},
        {ImGuiKey_DownArrow,    VK_DOWN},
        {ImGuiKey_PageUp,       VK_PRIOR},
        {ImGuiKey_PageDown,     VK_NEXT},
        {ImGuiKey_Home,         VK_HOME},
        {ImGuiKey_End,          VK_END},
        {ImGuiKey_Insert,       VK_INSERT},
        {ImGuiKey_Delete,       VK_DELETE},
        {ImGuiKey_Backspace,    VK_BACK},
        {ImGuiKey_Space,        VK_SPACE},
        {ImGuiKey_Enter,        VK_RETURN},
        {ImGuiKey_Escape,       VK_ESCAPE},
        {ImGuiKey_LeftCtrl,     VK_LCONTROL},
        {ImGuiKey_LeftShift,    VK_LSHIFT},
        {ImGuiKey_LeftAlt,      VK_LMENU},
        {ImGuiKey_LeftSuper,    VK_LWIN},
        {ImGuiKey_RightCtrl,    VK_RSHIFT},
        {ImGuiKey_RightShift,   VK_RSHIFT},
        {ImGuiKey_RightAlt,     VK_RMENU},
        {ImGuiKey_RightSuper,   VK_RWIN},
        {ImGuiKey_Menu,         VK_APPS},
        {ImGuiKey_0,            '0'},
        {ImGuiKey_1,            '1'},
        {ImGuiKey_2,            '2'},
        {ImGuiKey_3,            '3'},
        {ImGuiKey_4,            '4'},
        {ImGuiKey_5,            '5'},
        {ImGuiKey_6,            '6'},
        {ImGuiKey_7,            '7'},
        {ImGuiKey_8,            '8'},
        {ImGuiKey_9,            '9'},
        {ImGuiKey_A,            'A'},
        {ImGuiKey_B,            'B'},
        {ImGuiKey_C,            'C'},
        {ImGuiKey_D,            'D'},
        {ImGuiKey_E,            'E'},
        {ImGuiKey_F,            'F'},
        {ImGuiKey_G,            'G'},
        {ImGuiKey_H,            'H'},
        {ImGuiKey_I,            'I'},
        {ImGuiKey_J,            'J'},
        {ImGuiKey_K,            'K'},
        {ImGuiKey_L,            'L'},
        {ImGuiKey_M,            'M'},
        {ImGuiKey_N,            'N'},
        {ImGuiKey_O,            '0'},
        {ImGuiKey_P,            'P'},
        {ImGuiKey_Q,            'Q'},
        {ImGuiKey_R,            'R'},
        {ImGuiKey_S,            'S'},
        {ImGuiKey_T,            'T'},
        {ImGuiKey_U,            'U'},
        {ImGuiKey_V,            'V'},
        {ImGuiKey_W,            'W'},
        {ImGuiKey_X,            'X'},
        {ImGuiKey_Y,            'Y'},
        {ImGuiKey_Z,            'Z'},
        {ImGuiKey_F1,           VK_F1},
        {ImGuiKey_F2,           VK_F2},
        {ImGuiKey_F3,           VK_F3},
        {ImGuiKey_F4,           VK_F4},
        {ImGuiKey_F5,           VK_F5},
        {ImGuiKey_F6,           VK_F6},
        {ImGuiKey_F7,           VK_F7},
        {ImGuiKey_F8,           VK_F8},
        {ImGuiKey_F9,           VK_F9},
        {ImGuiKey_F10,          VK_F10},
        {ImGuiKey_F11,          VK_F11},
        {ImGuiKey_F12,          VK_F12},
        {ImGuiKey_Apostrophe,   VK_OEM_7},
        {ImGuiKey_Comma,        VK_OEM_COMMA},
        {ImGuiKey_Minus,        VK_OEM_MINUS},
        {ImGuiKey_Period,       VK_OEM_PERIOD},
        {ImGuiKey_Slash,        VK_OEM_2},
        {ImGuiKey_Semicolon,    VK_OEM_1},
        {ImGuiKey_Equal,        VK_OEM_PLUS},
        {ImGuiKey_LeftBracket,  VK_OEM_4},
        {ImGuiKey_Backslash,    VK_OEM_5},
        {ImGuiKey_RightBracket, VK_OEM_6},
        {ImGuiKey_GraveAccent,  VK_OEM_3},
        {ImGuiKey_CapsLock,     VK_CAPITAL},
        {ImGuiKey_ScrollLock,   VK_SCROLL},
        {ImGuiKey_NumLock,      VK_NUMLOCK},
        {ImGuiKey_PrintScreen,  VK_SNAPSHOT},
        {ImGuiKey_Pause,        VK_PAUSE},
        {ImGuiKey_Keypad0,      VK_NUMPAD0},
        {ImGuiKey_Keypad1,      VK_NUMPAD1},
        {ImGuiKey_Keypad2,      VK_NUMPAD2},
        {ImGuiKey_Keypad3,      VK_NUMPAD3},
        {ImGuiKey_Keypad4,      VK_NUMPAD4},
        {ImGuiKey_Keypad5,      VK_NUMPAD5},
        {ImGuiKey_Keypad6,      VK_NUMPAD6},
        {ImGuiKey_Keypad7,      VK_NUMPAD7},
        {ImGuiKey_Keypad8,      VK_NUMPAD8},
        {ImGuiKey_Keypad9,      VK_NUMPAD9},
        {ImGuiKey_KeypadDecimal,    VK_DECIMAL},
        {ImGuiKey_KeypadDivide,     VK_DIVIDE},
        {ImGuiKey_KeypadMultiply,   VK_MULTIPLY},
        {ImGuiKey_KeypadSubtract,   VK_SUBTRACT},
        {ImGuiKey_KeypadAdd,        VK_ADD},
        {ImGuiKey_KeypadEnter,      IM_VK_KEYPAD_ENTER}
    };

    void ImGuiProxy::Initialize()
    {
        ThrowIfFalse(!IMGUI_CONTEXT_.imgui_context);
        IMGUI_CONTEXT_.imgui_context = ImGui::CreateContext();

        InitFontTexture();
        InitRootSignature();
        InitShaderPSO();
        InitVertexIndexBuffer();
    }

    void ImGuiProxy::Uninitialize()
    {
        ImGui::DestroyContext(IMGUI_CONTEXT_.imgui_context);
        IMGUI_CONTEXT_.imgui_context = nullptr;
        IMGUI_CONTEXT_.pso.Reset();
        IMGUI_CONTEXT_.root_signature.Reset();
        IMGUI_CONTEXT_.srv_heap.Reset();
        IMGUI_CONTEXT_.font_texture.Reset();
        IMGUI_CONTEXT_.vertex_buffer.Reset();
        IMGUI_CONTEXT_.vertex_buffer_size = 0;
        IMGUI_CONTEXT_.index_buffer.Reset();
        IMGUI_CONTEXT_.index_buffer_size = 0;
        IMGUI_CONTEXT_.mvp = {};
        IMGUI_CONTEXT_.register_keys.clear();
    }

    void ImGuiProxy::RegisterKeyboardEvent(const ImGuiKey* keys, int count)
    {
        auto& register_keys = IMGUI_CONTEXT_.register_keys;

        for (int i = 0; i < count; i++)
        {
            auto& key = keys[i];
            auto it_find = std::find(register_keys.begin(), register_keys.end(), key);
            if (it_find == register_keys.end())
            {
                register_keys.push_back(key);
            }
        }
    }

    void ImGuiProxy::UnregisterKeyboardEvent(const ImGuiKey* keys, int count)
    {
        auto& register_keys = IMGUI_CONTEXT_.register_keys;
        for (int i = 0; i < count; i++)
        {
            auto& key = keys[i];
            auto it_find = std::find(register_keys.begin(), register_keys.end(), key);
            if (it_find != register_keys.end())
            {
                register_keys.erase(it_find);
            }
        }
    }

    void ImGuiProxy::InitRootSignature()
    {
        D3D12_DESCRIPTOR_RANGE desc_range = {};
        desc_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        desc_range.NumDescriptors = 1;
        desc_range.BaseShaderRegister = 0;
        desc_range.RegisterSpace = 0;
        desc_range.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER param[2] = {};

        param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param[0].Constants.ShaderRegister = 0;
        param[0].Constants.RegisterSpace = 0;
        param[0].Constants.Num32BitValues = 16;
        param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 1;
        param[1].DescriptorTable.pDescriptorRanges = &desc_range;
        param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC static_sampler = {};
        static_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        static_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        static_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        static_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        static_sampler.MipLODBias = 0.f;
        static_sampler.MaxAnisotropy = 0;
        static_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        static_sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        static_sampler.MinLOD = 0.f;
        static_sampler.MaxLOD = 0.f;
        static_sampler.ShaderRegister = 0;
        static_sampler.RegisterSpace = 0;
        static_sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(param);
        desc.pParameters = param;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &static_sampler;
        desc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = ::D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

        if (errorBlob != nullptr)
        {
            ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        ComPtr<ID3D12RootSignature> root_signature;
        ThrowIfFailed(D3D12Manager::GetDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&IMGUI_CONTEXT_.root_signature)));
    }

    void ImGuiProxy::InitShaderPSO()
    {
        ThrowIfFalse(IMGUI_CONTEXT_.root_signature.Get());

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
        memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        psoDesc.NodeMask = 1;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature = IMGUI_CONTEXT_.root_signature.Get();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        ComPtr<ID3DBlob> vertex_shader_blob;
        ComPtr<ID3DBlob> pixel_shader_blob;

        // Create the vertex shader
        {
            static const char* vertex_shader =
                "cbuffer vertexBuffer : register(b0) \
            {\
              float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
              float2 pos : POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
              output.col = input.col;\
              output.uv  = input.uv;\
              return output;\
            }";

            ThrowIfFailed(D3DCompile(vertex_shader, strlen(vertex_shader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vertex_shader_blob, NULL));

            psoDesc.VS = { vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize() };

            // Create the input layout
            static D3D12_INPUT_ELEMENT_DESC local_layout[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            psoDesc.InputLayout = { local_layout, 3 };
        }

        // Create the pixel shader
        {
            static const char* pixel_shader =
                "struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            SamplerState sampler0 : register(s0);\
            Texture2D texture0 : register(t0);\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";

            ThrowIfFailed(D3DCompile(pixel_shader, strlen(pixel_shader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &pixel_shader_blob, NULL));

            psoDesc.PS = { pixel_shader_blob->GetBufferPointer(), pixel_shader_blob->GetBufferSize() };
        }

        // Create the blending setup
        {
            D3D12_BLEND_DESC& desc = psoDesc.BlendState;
            desc.AlphaToCoverageEnable = false;
            desc.RenderTarget[0].BlendEnable = true;
            desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        // Create the rasterizer state
        {
            D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
            desc.FillMode = D3D12_FILL_MODE_SOLID;
            desc.CullMode = D3D12_CULL_MODE_NONE;
            desc.FrontCounterClockwise = FALSE;
            desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            desc.DepthClipEnable = true;
            desc.MultisampleEnable = FALSE;
            desc.AntialiasedLineEnable = FALSE;
            desc.ForcedSampleCount = 0;
            desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        }

        // Create depth-stencil State
        {
            D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.BackFace = desc.FrontFace;
        }

        ThrowIfFailed(D3D12Manager::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&IMGUI_CONTEXT_.pso)));
    }

    void ImGuiProxy::InitVertexIndexBuffer()
    {
        IMGUI_CONTEXT_.vertex_buffer_size = 1024 * 1024 * 4;
        IMGUI_CONTEXT_.vertex_buffer = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, IMGUI_CONTEXT_.vertex_buffer_size);
        IMGUI_CONTEXT_.index_buffer_size = 1024 * 1024;
        IMGUI_CONTEXT_.index_buffer = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, IMGUI_CONTEXT_.index_buffer_size);
    }

    void ImGuiProxy::InitFontTexture()
    {
        ThrowIfFalse(!IMGUI_CONTEXT_.font_texture.Get());

        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Upload texture to graphics system
        {
            D3D12_HEAP_PROPERTIES props;
            ::memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment = 0;
            desc.Width = width;
            desc.Height = height;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            ThrowIfFailed(D3D12Manager::GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&IMGUI_CONTEXT_.font_texture)));

            UINT upload_pitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
            ImageLayout img_layout{};
            img_layout.width = width * 4;
            img_layout.height = height;
            img_layout.row_pitch = upload_pitch;
            auto task_id = D3D12Manager::PostUploadTextureTask(IMGUI_CONTEXT_.font_texture.Get(), 0, 1, pixels, &img_layout, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            ThrowIfFalse(task_id > 0);

            D3D12Manager::WaitCopyTask(task_id);

            IMGUI_CONTEXT_.srv_heap = D3D12Manager::CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            // Create texture view
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            D3D12Manager::GetDevice()->CreateShaderResourceView(IMGUI_CONTEXT_.font_texture.Get(), &srvDesc, IMGUI_CONTEXT_.srv_heap->GetCPUDescriptorHandleForHeapStart());
        }

        io.Fonts->SetTexID((ImTextureID)IMGUI_CONTEXT_.srv_heap->GetGPUDescriptorHandleForHeapStart().ptr);
    }

    int ImGuiProxy::ImGuiKeyToVk(ImGuiKey key)
    {
        auto it = IMKEY_TO_VK_MAP_.find(key);
        if (it != IMKEY_TO_VK_MAP_.end())
        {
            return it->second;
        }
        else
        {
            return 0;
        }
    }

    bool ImGuiProxy::IsVkDown(int vk, bool test_last_click)
    {
        int ret = ::GetAsyncKeyState(vk);
        if (test_last_click)
        {
            return (ret & 0x8001) != 0;
        }
        else
        {
            return (ret & 0x8000) != 0;
        }
    }

    void ImGuiProxy::HandleMouseInput(HWND hwnd)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto& mouse_button_state = IMGUI_CONTEXT_.mouse_button_state;

        bool is_lbtn_click = (mouse_button_state & kLButtonClick) != 0;
        bool is_rbtn_click = (mouse_button_state & kRButtonClick) != 0;
        const bool is_app_focused = (::GetForegroundWindow() == hwnd);

        if (is_rbtn_click | is_lbtn_click ||
            is_app_focused)
        {
            POINT pos{};
            if (::GetCursorPos(&pos) && ::ScreenToClient(hwnd, &pos))
            {
                io.AddMousePosEvent((float)pos.x, (float)pos.y);
            }

            if (IsVkDown(VK_LBUTTON, !is_lbtn_click))
            {
                io.AddMouseButtonEvent(0, true);
                mouse_button_state |= kLButtonClick;
            }
            else if(is_lbtn_click)
            {
                io.AddMouseButtonEvent(0, false);
                mouse_button_state &= ~kLButtonClick;
            }

            if(IsVkDown(VK_RBUTTON, !is_rbtn_click))
            {
                io.AddMouseButtonEvent(1, true);
                mouse_button_state |= kRButtonClick;
            }
            else if(is_rbtn_click)
            {
                io.AddMouseButtonEvent(1, false);
                mouse_button_state &= ~kRButtonClick;
            }
        }
    }

    void ImGuiProxy::HandleKeyboardInput(HWND hwnd)
    {
        const bool is_app_focused = (::GetForegroundWindow() == hwnd);
        if (is_app_focused)
        {
            auto& register_keys = IMGUI_CONTEXT_.register_keys;

            for (auto& key : register_keys)
            {
                int vk_key = ImGuiKeyToVk(key);
                if (vk_key && IsVkDown(vk_key, false))
                {
                    AddKeyEvent(key, true);
                }
            }
        }
    }

    bool ImGuiProxy::HandleInputEvent(HWND hwnd, float tick)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = tick;

        HandleMouseInput(hwnd);
        HandleKeyboardInput(hwnd);

        return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    }

    void ImGuiProxy::AddKeyEvent(ImGuiKey key, bool down)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(key, down);
    }

    void ImGuiProxy::AddMousePosEvent(float x, float y)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(x, y);
    }

    void ImGuiProxy::AddMouseButtonEvent(int mouse_button, bool down)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseButtonEvent(mouse_button, down);
    }

    void ImGuiProxy::PopulateCommandList(ID3D12GraphicsCommandList* cmd)
    {
        auto draw_data = ImGui::GetDrawData();
        if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
            return;

        auto& vert_buffer = IMGUI_CONTEXT_.vertex_buffer;
        auto& vert_buffer_size = IMGUI_CONTEXT_.vertex_buffer_size;
        auto& index_buffer = IMGUI_CONTEXT_.index_buffer;
        auto& index_buffer_size = IMGUI_CONTEXT_.index_buffer_size;

        auto require_vert_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        if (require_vert_buffer_size > IMGUI_CONTEXT_.vertex_buffer_size)
        {
            vert_buffer.Reset();
            vert_buffer_size = require_vert_buffer_size * 1.5;
            vert_buffer = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, vert_buffer_size);
        }

        auto require_index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if (require_index_buffer_size > IMGUI_CONTEXT_.index_buffer_size)
        {
            index_buffer.Reset();
            index_buffer_size = require_index_buffer_size * 1.5;
            index_buffer = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, index_buffer_size);
        }

        D3D12_RANGE vert_range{};
        vert_range.End = require_vert_buffer_size;
        ImDrawVert* map_vert_data{};
        ThrowIfFailed(vert_buffer->Map(0, &vert_range, reinterpret_cast<void**>(&map_vert_data)));

        D3D12_RANGE index_range{};
        index_range.End = require_index_buffer_size;
        ImDrawIdx* map_index_data{};
        ThrowIfFailed(index_buffer->Map(0, &index_range, reinterpret_cast<void**>(&map_index_data)));

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(map_vert_data, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(map_index_data, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            map_vert_data += cmd_list->VtxBuffer.Size;
            map_index_data += cmd_list->IdxBuffer.Size;
        }

        vert_buffer->Unmap(0, nullptr);
        index_buffer->Unmap(0, nullptr);

        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        auto mat = DirectX::XMMatrixOrthographicOffCenterLH(L, R, B, T, -20.0f, 20.0f);
        DirectX::XMStoreFloat4x4(&IMGUI_CONTEXT_.mvp, mat);

        D3D12_VIEWPORT vp{};
        vp.Width = draw_data->DisplaySize.x;
        vp.Height = draw_data->DisplaySize.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        cmd->RSSetViewports(1, &vp);

        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.BufferLocation = vert_buffer->GetGPUVirtualAddress();
        vbv.SizeInBytes = require_vert_buffer_size;
        vbv.StrideInBytes = sizeof(ImDrawVert);
        cmd->IASetVertexBuffers(0, 1, &vbv);

        D3D12_INDEX_BUFFER_VIEW ibv;
        memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
        ibv.BufferLocation = index_buffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = require_index_buffer_size;
        ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        cmd->IASetIndexBuffer(&ibv);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->SetPipelineState(IMGUI_CONTEXT_.pso.Get());
        cmd->SetGraphicsRootSignature(IMGUI_CONTEXT_.root_signature.Get());

        ID3D12DescriptorHeap* heap[] = { IMGUI_CONTEXT_.srv_heap.Get() };
        cmd->SetDescriptorHeaps(1, heap);
        cmd->SetGraphicsRoot32BitConstants(0, 16, &IMGUI_CONTEXT_.mvp, 0);

        // Setup blend factor
        const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
        cmd->OMSetBlendFactor(blend_factor);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        ImVec2 clip_off = draw_data->DisplayPos;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    //if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    //    ImGui_ImplDX12_SetupRenderState(draw_data, ctx, fr);
                    //else
                    //    pcmd->UserCallback(cmd_list, pcmd);
                    ThrowIfFalse(0);
                }
                else
                {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                    ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                    if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                        continue;

                    // Apply Scissor/clipping rectangle, Bind texture, Draw
                    const D3D12_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                    D3D12_GPU_DESCRIPTOR_HANDLE texture_handle = {};
                    texture_handle.ptr = (UINT64)pcmd->GetTexID();

                    cmd->SetGraphicsRootDescriptorTable(1, texture_handle);
                    cmd->RSSetScissorRects(1, &r);
                    cmd->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }
};
