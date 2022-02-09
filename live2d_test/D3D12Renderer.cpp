#include "D3D12Renderer.h"

#include "DirectXColors.h"

#include "D3DUtil.h"
#include "DirectXTK/DescriptorHeap.h"

namespace D3D
{
    using namespace DirectX;

    const std::array<D3D12Renderer::Vertex, 8>  D3D12Renderer::VERTICE_DATA_ = 
    {
            Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
            Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
            Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
            Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
            Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
            Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
            Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
            Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    const std::array<std::uint16_t, 36> D3D12Renderer::INDICE_DATA_ = 
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };


    D3D12Renderer::D3D12Renderer(HWND hwnd, int width, int height) :
        window_handle_(hwnd),
        client_width_(width),
        client_height_(height)
    {
    }

    D3D12Renderer::~D3D12Renderer()
    {
    }

    float D3D12Renderer::AspectRatio() const
    {
        return static_cast<float>(client_width_) / client_height_;
    }

    void D3D12Renderer::Initialize()
    {
        command_queue_ = D3D12Manager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_FLAG_NONE);
        command_list_alloc_ = D3D12Manager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
        command_list_ = D3D12Manager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, command_list_alloc_.Get());
        swap_chain_ = D3D12Manager::CreateSwapChain(
            window_handle_, 
            command_queue_.Get(), 
            2, 
            client_width_, 
            client_height_, 
            DXGI_FORMAT_R8G8B8A8_UNORM, 
            DXGI_USAGE_RENDER_TARGET_OUTPUT, 
            DXGI_SWAP_EFFECT_FLIP_DISCARD);

        command_list_->Close();

        fence_ = D3D12Manager::CreateFence(fence_value_);

        D3D12_ROOT_PARAMETER root_param{};
        root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        root_param.Descriptor.ShaderRegister = 0;
        root_param.Descriptor.RegisterSpace = 0;
        root_signature_ = D3D12Manager::CreateRootSignature(&root_param, 1);

        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout;
        input_layout = 
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_SHADER_BYTECODE shaders[5] = {};
        vs_shader_ = D3D12Manager::CompileShader(L"./Shaders/color.hlsl", "VS", "vs_5_0");
        ps_shader_ = D3D12Manager::CompileShader(L"./Shaders/color.hlsl", "PS", "ps_5_0");

        shaders[0] = { vs_shader_->GetBufferPointer(), (UINT)vs_shader_->GetBufferSize() };
        shaders[1] = { ps_shader_->GetBufferPointer(), (UINT)ps_shader_->GetBufferSize() };

        DXGI_FORMAT rt_format{ DXGI_FORMAT_R8G8B8A8_UNORM };
        pipe_line_state_ = D3D12Manager::CreatePipeLineStateObject({ input_layout.data(), (UINT)input_layout.size() }, root_signature_.Get(), shaders, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, &rt_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT);

        for (int i = 0; i < 2; i++) 
        {
            ThrowIfFailed(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&back_target_buffer_[i])));
        }
        depth_stencil_buffer_ = D3D12Manager::CreateDepthStencilBuffer(client_width_, client_height_, DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0);

        rtv_heap_ = D3D12Manager::CreateDescriptorHeap(2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        dsv_heap_ = D3D12Manager::CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        DirectX::DescriptorHeap dx_rtv_heap(rtv_heap_.Get());
        D3D12Manager::GetDevice()->CreateRenderTargetView(back_target_buffer_[0].Get(), nullptr, dx_rtv_heap.GetCpuHandle(0));
        D3D12Manager::GetDevice()->CreateRenderTargetView(back_target_buffer_[1].Get(), nullptr, dx_rtv_heap.GetCpuHandle(1));

        DirectX::DescriptorHeap dx_dsv_heap(dsv_heap_.Get());
        D3D12Manager::GetDevice()->CreateDepthStencilView(depth_stencil_buffer_.Get(), nullptr, dx_dsv_heap.GetCpuHandle(0));

        screen_viewport_.TopLeftX = 0;
        screen_viewport_.TopLeftY = 0;
        screen_viewport_.Width = static_cast<float>(client_width_);
        screen_viewport_.Height = static_cast<float>(client_height_);
        screen_viewport_.MinDepth = 0.0f;
        screen_viewport_.MaxDepth = 1.0f;

        scissor_rect_ = { 0, 0, client_width_, client_height_ };

        camera_.SetLens(0.25f * DirectX::XM_PI, AspectRatio(), 1.0f, 1000.0f);
        camera_.LookAt(XMFLOAT3{ 0.0f, 0.0f, -10.0f }, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{0.0f, 1.0f, 0.0f});
        camera_.UpdateViewMatrix();

        const_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, CalcConstantBufferByteSize(sizeof ObjectConstants));

        InitVertexIndexBuffer();
        InitImageResource();
    }

    void D3D12Renderer::Update()
    {
        camera_.UpdateViewMatrix();

        auto view = camera_.GetView();
        auto proj = camera_.GetProj();
        auto view_proj = view * proj;

        ObjectConstants obj_constants;
        XMStoreFloat4x4(&obj_constants.world_view_proj, XMMatrixTranspose(view_proj));

        void* map_data{};
        const_buffer_->Map(0, nullptr, &map_data);
        ::memcpy(map_data, &obj_constants, sizeof(ObjectConstants));
        const_buffer_->Unmap(0, nullptr);
    }

    void D3D12Renderer::Render()
    {
        ThrowIfFailed(command_list_alloc_->Reset());
        ThrowIfFailed(command_list_->Reset(command_list_alloc_.Get(), pipe_line_state_.Get()));

        int back_index = GetCurrentRenderTargetIndex();
        auto cur_back_buffer = back_target_buffer_[back_index];
        auto cur_back_buffer_view = DirectX::DescriptorHeap(rtv_heap_.Get()).GetCpuHandle(back_index);
        auto cur_depth_stencil_view = DirectX::DescriptorHeap(dsv_heap_.Get()).GetCpuHandle(0);

        command_list_->RSSetViewports(1, &screen_viewport_);
        command_list_->RSSetScissorRects(1, &scissor_rect_);

        D3D12_RESOURCE_BARRIER resource_barrier{};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Transition.pResource = cur_back_buffer.Get();
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list_->ResourceBarrier(1, &resource_barrier);

        command_list_->ClearRenderTargetView(cur_back_buffer_view, DirectX::Colors::LightSteelBlue, 0, nullptr);
        command_list_->ClearDepthStencilView(cur_depth_stencil_view, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view_);
        command_list_->IASetIndexBuffer(&index_buffer_view_);
        command_list_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        command_list_->SetGraphicsRootSignature(root_signature_.Get());
        command_list_->SetGraphicsRootConstantBufferView(0, const_buffer_->GetGPUVirtualAddress());

        command_list_->OMSetRenderTargets(1, &cur_back_buffer_view, true, &cur_depth_stencil_view);

        command_list_->DrawIndexedInstanced(INDICE_DATA_.size(), 1, 0, 0, 0);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        command_list_->ResourceBarrier(1, &resource_barrier);

        ThrowIfFailed(command_list_->Close());

        ID3D12CommandList* cmdsLists[] = { command_list_.Get() };
        command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

        FlushCommandQueue();

        swap_chain_->Present(0, 0);
    }

    int D3D12Renderer::GetCurrentRenderTargetIndex()
    {
        UINT present_count{};
        ThrowIfFailed(swap_chain_->GetLastPresentCount(&present_count));
        return present_count % 2;
    }

    void D3D12Renderer::FlushCommandQueue()
    {
        fence_value_++;

        ThrowIfFailed(command_queue_->Signal(fence_.Get(), fence_value_));

        if (fence_->GetCompletedValue() < fence_value_)
        {
            HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(fence_->SetEventOnCompletion(fence_value_, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }

    void D3D12Renderer::InitVertexIndexBuffer()
    {
        uint64_t vertices_size = VERTICE_DATA_.size() * sizeof(Vertex);
        uint64_t indices_size = INDICE_DATA_.size() * sizeof(uint16_t);

        uint64_t upload_buffer_length = (std::max)(vertices_size, indices_size) * 1.5;

        upload_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, upload_buffer_length);
        vertex_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_DEFAULT, vertices_size);
        index_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_DEFAULT, indices_size);

        void* upload_map_data{};
        upload_buffer_->Map(0, nullptr, &upload_map_data);
        ::memcpy(upload_map_data, VERTICE_DATA_.data(), vertices_size);

        command_list_alloc_->Reset();
        command_list_->Reset(command_list_alloc_.Get(), nullptr);

        D3D12_RESOURCE_BARRIER resource_barrier{};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resource_barrier.Transition.pResource = vertex_buffer_.Get();
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        command_list_->ResourceBarrier(1, &resource_barrier);
        command_list_->CopyBufferRegion(vertex_buffer_.Get(), 0, upload_buffer_.Get(), 0, vertices_size);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        command_list_->ResourceBarrier(1, &resource_barrier);

        ThrowIfFailed(command_list_->Close());

        ID3D12CommandList* cmdsLists[] = { command_list_.Get() };
        command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

        FlushCommandQueue();

        vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
        vertex_buffer_view_.SizeInBytes = vertices_size;
        vertex_buffer_view_.StrideInBytes = sizeof(Vertex);

        ::memcpy(upload_map_data, INDICE_DATA_.data(), indices_size);

        command_list_alloc_->Reset();
        command_list_->Reset(command_list_alloc_.Get(), nullptr);

        resource_barrier.Transition.pResource = index_buffer_.Get();
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        command_list_->ResourceBarrier(1, &resource_barrier);
        command_list_->CopyBufferRegion(index_buffer_.Get(), 0, upload_buffer_.Get(), 0, indices_size);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        command_list_->ResourceBarrier(1, &resource_barrier);

        ThrowIfFailed(command_list_->Close());

        cmdsLists[0] = { command_list_.Get() };
        command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

        FlushCommandQueue();

        index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
        index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
        index_buffer_view_.SizeInBytes = indices_size;

    }

    void D3D12Renderer::InitImageResource()
    {
        image_resource_ = WICImage::LoadImageFormFile(L"./test.jpeg");
        image_resource_ = WICImage::CovertToD3DPixelFormat(image_resource_.Get());

        uint32_t width{}, height{};
        image_resource_->GetSize(&width, &height);
        texture_ = D3D12Manager::CreateTexture(width, height);


    }
};

