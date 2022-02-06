#include "D3D12Renderer.h"

#include "DirectXColors.h"

#include "D3DUtil.h"
#include "DirectXTK/DescriptorHeap.h"

#include <vector>


namespace D3D
{
    D3D12Renderer::D3D12Renderer(HWND hwnd, int width, int height) :
        window_handle_(hwnd),
        client_width_(width),
        client_height_(height)
    {
    }

    D3D12Renderer::~D3D12Renderer()
    {
    }

    void D3D12Renderer::Initialize()
    {
        command_queue_ = D3D12Manager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_FLAG_NONE);
        direct_cmd_list_alloc_ = D3D12Manager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
        command_list_ = D3D12Manager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, direct_cmd_list_alloc_.Get());
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

        D3D12_DESCRIPTOR_RANGE descriptor_range{};
        descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descriptor_range.NumDescriptors = 1;
        descriptor_range.BaseShaderRegister = 0;
        descriptor_range.RegisterSpace = 0;
        descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER root_param{};
        root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_param.DescriptorTable.NumDescriptorRanges = 1;
        root_param.DescriptorTable.pDescriptorRanges = &descriptor_range;
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
    }

    void D3D12Renderer::Render()
    {
        ThrowIfFailed(direct_cmd_list_alloc_->Reset());
        ThrowIfFailed(command_list_->Reset(direct_cmd_list_alloc_.Get(), pipe_line_state_.Get()));

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

        command_list_->OMSetRenderTargets(1, &cur_back_buffer_view, true, &cur_depth_stencil_view);

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
};

