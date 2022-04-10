#include "SkyBoxPass.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "D3DUtil.h"
#include "DirectXTK/DescriptorHeap.h"
#include "WICImage.h"

namespace D3D
{
    using namespace DirectX;
    using namespace Microsoft::WRL;

    SkyBoxPass::SkyBoxPass()
    {
        GeometryGenerator   mesh_gen;
        mesh_data_ = mesh_gen.CreateSphere(0.5f, 40, 40);
    }

    SkyBoxPass::~SkyBoxPass()
    {
    }

    void SkyBoxPass::Update(const Camera& camera)
    {
        XMFLOAT4X4 view_proj_f4x4{};
        XMStoreFloat4x4(&view_proj_f4x4, XMMatrixTranspose(camera.GetView() * camera.GetProj()));

        uint8_t* map_data{};
        view_proj_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&map_data));
        ::memcpy(map_data, &view_proj_f4x4, sizeof(XMFLOAT4X4));
        view_proj_buffer_->Unmap(0, nullptr);
    }

    void SkyBoxPass::PopulateCommandList(ID3D12GraphicsCommandList* cmd)
    {
        ID3D12DescriptorHeap* heap[] =
        {
            bund_resource_manager_.GetSrvUavCbvDescriptorHeap(),
            bund_resource_manager_.GetSampleDescriptorHeap()
        };

        cmd->SetPipelineState(pso_.Get());
        cmd->SetGraphicsRootSignature(root_signature_);
        cmd->SetDescriptorHeaps(_countof(heap), heap);
        cmd->SetGraphicsRootDescriptorTable(0, bund_resource_manager_.GetSrvUavCbvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
        cmd->SetGraphicsRootDescriptorTable(1, bund_resource_manager_.GetSampleDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
        cmd->IASetVertexBuffers(0, 1, &vert_buffer_view_);
        cmd->IASetIndexBuffer(&index_buffer_view_);
        cmd->DrawIndexedInstanced(mesh_data_.Indices16.size(), 1, 0, 0, 0);
    }

    void SkyBoxPass::Initialize()
    {
        vs_shader_ = D3D12Manager::CompileShader(L"./Shaders/SkyPass_VS.hlsl", "VS_Main", "vs_5_0");
        ps_shader_ = D3D12Manager::CompileShader(L"./Shaders/SkyPass_PS.hlsl", "PS_Main", "ps_5_0");

        ID3DBlob* shader_arr[5] = { vs_shader_.Get(), ps_shader_.Get() };
        bund_resource_manager_.Initialize(shader_arr);

        root_signature_ = bund_resource_manager_.GetRootSignature();

        auto& input_layout = bund_resource_manager_.GetInputElemDescArray();

        D3D12_SHADER_BYTECODE shader_byte[5] = {};
        shader_byte[0] = { vs_shader_->GetBufferPointer(), (UINT)vs_shader_->GetBufferSize() };
        shader_byte[1] = { ps_shader_->GetBufferPointer(), (UINT)ps_shader_->GetBufferSize() };

        D3D12_RASTERIZER_DESC rast_desc = D3D12Manager::DefaultRasterizerDesc();
        D3D12_BLEND_DESC blend_desc = D3D12Manager::DefaultBlendDesc();
        D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = D3D12Manager::DefaultDepthStencilDesc();

        rast_desc.CullMode = D3D12_CULL_MODE_FRONT;
        depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        DXGI_FORMAT rt_format{ DXGI_FORMAT_R8G8B8A8_UNORM };
        pso_ = D3D12Manager::CreatePipeLineStateObject({ input_layout.data(), (UINT)input_layout.size() }, root_signature_, shader_byte, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, &rt_format, 1, DXGI_FORMAT_D24_UNORM_S8_UINT, rast_desc, blend_desc, depth_stencil_desc);

        uint64_t vertexSize = mesh_data_.Vertices.size() * sizeof(GeometryGenerator::Vertex);
        uint64_t indexSize = mesh_data_.Indices16.size() * sizeof(uint16_t);

        vert_buffer = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, vertexSize * 1.5f);
        index_buffer = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, indexSize * 1.5f);

        uint8_t* map_data{ nullptr };
        vert_buffer->Map(0, nullptr, reinterpret_cast<void**>(&map_data));
        ::memcpy(map_data, mesh_data_.Vertices.data(), vertexSize);
        vert_buffer->Unmap(0, nullptr);

        index_buffer->Map(0, nullptr, reinterpret_cast<void**>(&map_data));
        ::memcpy(map_data, mesh_data_.Indices16.data(), indexSize);
        index_buffer->Unmap(0, nullptr);

        vert_buffer_view_.BufferLocation = vert_buffer->GetGPUVirtualAddress();
        vert_buffer_view_.SizeInBytes = vertexSize;
        vert_buffer_view_.StrideInBytes = sizeof(GeometryGenerator::Vertex);

        index_buffer_view_.BufferLocation = index_buffer->GetGPUVirtualAddress();
        index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
        index_buffer_view_.SizeInBytes = indexSize;

        sky_texture_ = D3D12Manager::CreateTexture(1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, 6);

        const wchar_t* pic_path_arr[] = {L"t2.png", L"t1.png",  L"t4.png", L"t3.png", L"t5.png", L"t6.png" };

        for (int i = 0; i < _countof(pic_path_arr); i++)
        {
            auto frame = WICImage::LoadImageFormFile(pic_path_arr[i]);
            uint32_t ppb{};
            WICImage::GetImagePixelFormatInfo(frame.Get())->GetBitsPerPixel(&ppb);
            uint32_t img_width{}, img_height{};
            frame->GetSize(&img_width, &img_height);
            uint32_t img_row_pitch = (img_width * ppb + 7u) / 8u;
            img_width *= ppb / 8u;

            auto bmp = WICImage::CreateBmpFormSource(frame.Get());
            ComPtr<IWICBitmapLock> bmp_lock;
            ThrowIfFailed(bmp->Lock(nullptr, WICBitmapLockRead, &bmp_lock));

            UINT buffer_size{};
            UINT stride{};
            BYTE* pv = NULL;
            ThrowIfFailed(bmp_lock->GetStride(&stride));
            ThrowIfFailed(bmp_lock->GetDataPointer(&buffer_size, &pv));

            ImageLayout layout;
            layout.offset = 0;
            layout.width = img_width;
            layout.height = img_height;
            layout.row_pitch = img_row_pitch;

            auto task_id = D3D12Manager::PostUploadTextureTask(sky_texture_.Get(), i, 1, pv, &layout);
            D3D12Manager::WaitCopyTask(task_id);
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = sky_texture_->GetDesc().Format;;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.TextureCube.MostDetailedMip = 0;
        srv_desc.TextureCube.MipLevels = sky_texture_->GetDesc().MipLevels;
        srv_desc.TextureCube.ResourceMinLODClamp = 0.0f;
        D3D12Manager::GetDevice()->CreateShaderResourceView(sky_texture_.Get(), &srv_desc, bund_resource_manager_.GetDescriptorHandle("CUBE_TEXTURE", 0));
        //D3D12Manager::GetDevice()->CreateShaderResourceView(sky_texture_.Get(), &srv_desc, dx_cbv_heap.GetCpuHandle(0));

        view_proj_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4)));
        D3D12_CONSTANT_BUFFER_VIEW_DESC const_buff_view{};
        const_buff_view.BufferLocation = view_proj_buffer_->GetGPUVirtualAddress();
        const_buff_view.SizeInBytes = CalcConstantBufferByteSize(sizeof(DirectX::XMFLOAT4X4));
        D3D12Manager::GetDevice()->CreateConstantBufferView(&const_buff_view, bund_resource_manager_.GetDescriptorHandle("VS_MatrixBuffer", 0));
        //D3D12Manager::GetDevice()->CreateConstantBufferView(&const_buff_view, dx_cbv_heap.GetCpuHandle(1));

        bund_resource_manager_.BindDefaultSampler("SAMPLER", 0, D3D12BoundResourceManager::kLinearWrap);

    }
};

