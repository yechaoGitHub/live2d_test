#include "D3D12Renderer.h"

#include "DirectXTK/DescriptorHeap.h"
#include "DirectXColors.h"
#include "D3DUtil.h"


namespace D3D
{
    using namespace DirectX;
    using namespace Microsoft::WRL;

    GeometryGenerator D3D12Renderer::GEO_GENERATOR_;

    D3D12Renderer::D3D12Renderer(HWND hwnd, int width, int height) :
        window_handle_(hwnd),
        client_width_(width),
        client_height_(height)
    {
    }

    D3D12Renderer::~D3D12Renderer()
    {

    }

    Camera& D3D12Renderer::GetCamera()
    {
        return camera_;
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

        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout;
        input_layout =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_SHADER_BYTECODE shaders[5] = {};
        vs_shader_ = D3D12Manager::CompileShader(L"./Shaders/color.hlsl", "VS", "vs_5_0");
        ps_shader_ = D3D12Manager::CompileShader(L"./Shaders/color.hlsl", "PS", "ps_5_0");

        ID3DBlob* shader_blob[] = { vs_shader_.Get(), ps_shader_ .Get()};
        root_signature_ = D3D12Manager::CreateRootSignatureByReflect(shader_blob, 2);

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

        DescriptorHeap dx_rtv_heap(rtv_heap_.Get());
        D3D12Manager::GetDevice()->CreateRenderTargetView(back_target_buffer_[0].Get(), nullptr, dx_rtv_heap.GetCpuHandle(0));
        D3D12Manager::GetDevice()->CreateRenderTargetView(back_target_buffer_[1].Get(), nullptr, dx_rtv_heap.GetCpuHandle(1));

        DescriptorHeap dx_dsv_heap(dsv_heap_.Get());
        D3D12Manager::GetDevice()->CreateDepthStencilView(depth_stencil_buffer_.Get(), nullptr, dx_dsv_heap.GetCpuHandle(0));

        screen_viewport_.TopLeftX = 0;
        screen_viewport_.TopLeftY = 0;
        screen_viewport_.Width = static_cast<float>(client_width_);
        screen_viewport_.Height = static_cast<float>(client_height_);
        screen_viewport_.MinDepth = 0.0f;
        screen_viewport_.MaxDepth = 1.0f;

        scissor_rect_ = { 0, 0, client_width_, client_height_ };

        camera_.SetLens(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
        camera_.LookAt(XMFLOAT3{ 0.0f, 0.0f, -100.0f }, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{0.0f, 1.0f, 0.0f});
        camera_.UpdateViewMatrix();

        const_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, CalcConstantBufferByteSize(sizeof ObjectConstants));

        srv_heap_ = D3D12Manager::CreateDescriptorHeap(10, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

        model_.SetOriention({ 0.5f, 0.5f, 0.0f });

        timer_.Start();

        InitVertexIndexBuffer();
        InitImageResource();
        InitLight();
        InitResourceBinding();
    }

    void D3D12Renderer::ClearUp()
    {
        FlushCommandQueue();
    }

    void D3D12Renderer::Update()
    {
        timer_.Tick();
        float tick = timer_.DeltaTime();
        HandleInput(tick);

        camera_.UpdateViewMatrix();

        auto xm_world_trans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        auto xm_world_scalar = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        auto world_mat = xm_world_trans * xm_world_scalar;

        auto view = camera_.GetView();
        auto proj = camera_.GetProj();
        auto view_proj = view * proj;

        ObjectConstants obj_constants;
        obj_constants.local_mat = model_.GetModelMatrix4x4();

        XMStoreFloat4x4(&obj_constants.world_mat, world_mat);
        XMStoreFloat4x4(&obj_constants.model_mat, XMMatrixTranspose(model_.GetModelMatrix() * world_mat));
        XMStoreFloat4x4(&obj_constants.view_mat, view);
        XMStoreFloat4x4(&obj_constants.proj_mat, proj);
        XMStoreFloat4x4(&obj_constants.view_proj_mat, XMMatrixTranspose(view_proj));
        XMStoreFloat4x4(&obj_constants.texture_transform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

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
        auto cur_back_buffer_view = DescriptorHeap(rtv_heap_.Get()).GetCpuHandle(back_index);
        auto cur_depth_stencil_view = DescriptorHeap(dsv_heap_.Get()).GetCpuHandle(0);

        command_list_->RSSetViewports(1, &screen_viewport_);
        command_list_->RSSetScissorRects(1, &scissor_rect_);

        D3D12_RESOURCE_BARRIER resource_barrier{};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Transition.pResource = cur_back_buffer.Get();
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list_->ResourceBarrier(1, &resource_barrier);

        command_list_->ClearRenderTargetView(cur_back_buffer_view, Colors::LightSteelBlue, 0, nullptr);
        command_list_->ClearDepthStencilView(cur_depth_stencil_view, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view_);
        command_list_->IASetIndexBuffer(&index_buffer_view_);
        command_list_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        command_list_->SetGraphicsRootSignature(root_signature_.Get());

        ID3D12DescriptorHeap* heap[] = { srv_heap_.Get(),  };
        command_list_->SetDescriptorHeaps(_countof(heap), heap);
        command_list_->SetGraphicsRootDescriptorTable(0, srv_heap_->GetGPUDescriptorHandleForHeapStart());

        command_list_->OMSetRenderTargets(1, &cur_back_buffer_view, true, &cur_depth_stencil_view);

        command_list_->DrawIndexedInstanced(mesh_data_.Indices16.size(), 1, 0, 0, 0);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        command_list_->ResourceBarrier(1, &resource_barrier);

        ThrowIfFailed(command_list_->Close());

        ID3D12CommandList* cmdsLists[] = { command_list_.Get() };
        command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

        FlushCommandQueue();

        swap_chain_->Present(0, 0);
    }

    void D3D12Renderer::OnMouseDown(uint8_t btn, uint32_t x, uint32_t y)
    {
        ::SetCapture(window_handle_);

        start_look_at_ = camera_.GetLook3f();
        start_up_ = camera_.GetUp3f();
        start_right_ = camera_.GetRight3f();
        mouse_click_ = true;
        mouse_cur_x_ = x;
        mouse_cur_y_ = y;
    }

    void D3D12Renderer::OnMouseMove(uint32_t x, uint32_t y)
    {

    }

    void D3D12Renderer::OnMouseUp(uint8_t btn, uint32_t x, uint32_t y)
    {
        ::ReleaseCapture();

        start_look_at_ = {};
        start_up_ = {};
        mouse_click_ = false;
        mouse_cur_x_ = 0;
        mouse_cur_y_ = 0;
    }

    void D3D12Renderer::OnKeyDown(uint32_t key)
    {

    }

    void D3D12Renderer::OnKeyUp(uint32_t key)
    {

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
        mesh_data_ = GEO_GENERATOR_.CreateBox(5.0f, 5.0f, 5.0f, 0);

        uint32_t vertices_size = mesh_data_.Vertices.size() * sizeof(GeometryGenerator::Vertex);
        uint32_t indices_size = mesh_data_.Indices16.size() * sizeof(uint16_t);
        uint64_t upload_buffer_length = (std::max)(vertices_size, indices_size) * 1.5;

        vertex_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_DEFAULT, vertices_size);
        index_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_DEFAULT, indices_size);

        D3D12Manager::PostUploadBufferTask(vertex_buffer_.Get(), 0, mesh_data_.Vertices.data(), vertices_size);

        vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
        vertex_buffer_view_.SizeInBytes = vertices_size;
        vertex_buffer_view_.StrideInBytes = sizeof(GeometryGenerator::Vertex);

        auto last_copy_id = D3D12Manager::PostUploadBufferTask(index_buffer_.Get(), 0, mesh_data_.Indices16.data(), indices_size);

        index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
        index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
        index_buffer_view_.SizeInBytes = indices_size;

        D3D12Manager::WaitCopyTask(last_copy_id);
    }

    void D3D12Renderer::InitImageResource()
    {
        image_resource_ = WICImage::LoadImageFormFile(L"./test.jpeg");
        image_resource_ = WICImage::CovertToD3DPixelFormat(image_resource_.Get());

        uint32_t width{}, height{};
        image_resource_->GetSize(&width, &height);
        texture_ = D3D12Manager::CreateTexture(width, height);

        uint32_t ppb{};
        WICImage::GetImagePixelFormatInfo(image_resource_.Get())->GetBitsPerPixel(&ppb);

        uint32_t img_width{}, img_height{};
        image_resource_->GetSize(&img_width, &img_height);

        uint32_t img_row_pitch = (img_width * ppb + 7u) / 8u;
        img_width *= ppb / 8u;

        auto bmp = WICImage::CreateBmpFormSource(image_resource_.Get());
        ComPtr<IWICBitmapLock> bmp_lock;
        ThrowIfFailed(bmp->Lock(nullptr, WICBitmapLockRead, &bmp_lock));

        UINT buffer_size{};
        UINT stride{};
        BYTE* pv = NULL;
        ThrowIfFailed(bmp_lock->GetStride(&stride));
        ThrowIfFailed(bmp_lock->GetDataPointer(&buffer_size, &pv));

        ImageLayout layout;
        layout.width = img_width;
        layout.height = img_height;
        layout.row_pitch = img_row_pitch;
        auto last_copy_id = D3D12Manager::PostUploadTextureTask(texture_.Get(), 0, 1, pv, &layout);

        D3D12Manager::WaitCopyTask(last_copy_id);
    }

    void D3D12Renderer::InitLight()
    {
        auto buffer_size = CalcConstantBufferByteSize(sizeof(LightConstBuffer));
        const_light_gpu_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, buffer_size);

        LightConstBuffer* light_const_ptr{};
        const_light_gpu_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&light_const_ptr));
        light_const_ptr->directional_light_num = 1;
        light_const_ptr->point_light_num = 0;
        const_light_gpu_buffer_->Unmap(0, nullptr);

        directional_light_gpu_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, sizeof DirectionalLight);

        DirectionalLight* map_ptr{};
        directional_light_gpu_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&map_ptr));
        map_ptr->intensity = 1.0f;
        map_ptr->direction = { 1.0, 0.0, 0.0 };
        map_ptr->color = { 1.0f, 0.0f, 0.0f };

    }

    void D3D12Renderer::InitResourceBinding()
    {
        DescriptorHeap dx_cbv_heap(srv_heap_.Get());

        D3D12_SHADER_RESOURCE_VIEW_DESC dir_light_desc{};
        dir_light_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dir_light_desc.Format = DXGI_FORMAT_UNKNOWN;
        dir_light_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        dir_light_desc.Buffer.FirstElement = 0;
        dir_light_desc.Buffer.NumElements = 1;
        dir_light_desc.Buffer.StructureByteStride = sizeof(DirectionalLight);
        dir_light_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        D3D12Manager::GetDevice()->CreateShaderResourceView(directional_light_gpu_buffer_.Get(), &dir_light_desc, dx_cbv_heap.GetCpuHandle(0));

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = texture_->GetDesc().Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels = texture_->GetDesc().MipLevels;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
        D3D12Manager::GetDevice()->CreateShaderResourceView(texture_.Get(), &srv_desc, dx_cbv_heap.GetCpuHandle(2));

        D3D12_CONSTANT_BUFFER_VIEW_DESC const_buff_view{};
        const_buff_view.BufferLocation = const_buffer_->GetGPUVirtualAddress();
        const_buff_view.SizeInBytes = CalcConstantBufferByteSize(sizeof ObjectConstants);
        D3D12Manager::GetDevice()->CreateConstantBufferView(&const_buff_view, dx_cbv_heap.GetCpuHandle(3));

        const_buff_view.BufferLocation = const_light_gpu_buffer_->GetGPUVirtualAddress();
        const_buff_view.SizeInBytes = CalcConstantBufferByteSize(sizeof LightConstBuffer);
        D3D12Manager::GetDevice()->CreateConstantBufferView(&const_buff_view, dx_cbv_heap.GetCpuHandle(4));
    }

    void D3D12Renderer::HandleInput(float duration)
    {
        float distance = duration * camera_move_speed_;
        if (distance != 0.0f)
        {
            if (::GetAsyncKeyState('W') & 0x8000)
            {
                camera_.Walk(distance);
            }

            if (::GetAsyncKeyState('A') & 0x8000)
            {
                camera_.Strafe(-distance);
            }

            if (::GetAsyncKeyState('S') & 0x8000)
            {
                camera_.Walk(-distance);
            }

            if (::GetAsyncKeyState('D') & 0x8000)
            {
                camera_.Strafe(distance);
            }

            if (::GetAsyncKeyState(VK_SPACE) & 0x8000)
            {
                camera_.Float(distance);
            }

            if (::GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                camera_.Float(-distance);
            }
        }

        if (mouse_click_)
        {
            POINT pt{};
            ThrowIfFalse(::GetCursorPos(&pt));
            ThrowIfFalse(::ScreenToClient(window_handle_, &pt));

            auto dx = pt.x - mouse_cur_x_;
            auto dy = pt.y - mouse_cur_y_;
            if (dx != 0 || dy != 0)
            {
                float dxf = XMConvertToRadians(dx);
                float dyf = XMConvertToRadians(dy);

                auto x_axis = camera_.GetRight3f();
                auto y_axis = camera_.GetUp3f();
                auto qy = XMQuaternionRotationAxis(XMLoadFloat3(&y_axis), dxf);
                auto qx = XMQuaternionRotationAxis(XMLoadFloat3(&x_axis), dyf);

                auto look_at = camera_.GetLook3f();
                auto qr = XMQuaternionMultiply(qx, qy);
                auto v_look_at = XMQuaternionMultiply(XMLoadFloat3(&look_at), qr);
                auto v_up = XMQuaternionMultiply(XMLoadFloat3(&y_axis), qr);

                XMFLOAT3 lookf3 = {};
                XMFLOAT3 upf3 = {};
                XMStoreFloat3(&lookf3, v_look_at);
                XMStoreFloat3(&upf3, v_up);
                camera_.SetOriention(lookf3, upf3);

                mouse_cur_x_ = pt.x;
                mouse_cur_y_ = pt.y;
            }
        }

        camera_.UpdateViewMatrix();
    }

};

