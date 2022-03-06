#include "CopyTask.h"

#include "D3D12Manager.h"
#include "D3DUtil.h"

#include <wrl.h>

namespace D3D
{
    using namespace Microsoft::WRL;

    CopyTask::CopyTask(TaskType type) :
        type_(type)
    {
    }

    CopyTask::~CopyTask()
    {
    }

    void CopyTask::BindData(void* data)
    {
        data_ = data;
    }

    void* CopyTask::GetData()
    {
        return data_;
    }

    void CopyTask::ExcuteCallback()
    {
        if (call_back_)
        {
            call_back_(this);
        }
    }

    UploadBufferTask::UploadBufferTask() :
        CopyTask(UPLOAD_BUFFER)
    {
    }

    UploadBufferTask::~UploadBufferTask()
    {
    }

    void UploadBufferTask::ExcuteCopyTask(ID3D12GraphicsCommandList* command)
    {
        ::memcpy(upload_resource_map_data, src_data, length);

        auto before_barrier = TransitionBarrier(dest_res, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, 0);
        auto after_barrier = TransitionBarrier(dest_res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, 0);

        command->ResourceBarrier(1, &before_barrier);
        command->CopyBufferRegion(dest_res, dest_offset, upload_resource, 0, length);
        command->ResourceBarrier(1, &after_barrier);
    }

    UploadTextureTask::UploadTextureTask() :
        CopyTask(UPLOAD_TEXTURE)
    {
    }

    UploadTextureTask::~UploadTextureTask()
    {
    }

    void UploadTextureTask::ExcuteCopyTask(ID3D12GraphicsCommandList* command)
    {
        auto resource_layout = D3D12Manager::GetCopyableFootprints(dest_res, first_subresource, subresource_tasks.size());

        for (uint32_t i = 0; i < subresource_tasks.size(); i++)
        {
            auto& footprint = resource_layout.fontprints[i];
            auto& image_layout = subresource_tasks[i].src_image_layout;
            auto copy_height = (std::min)(footprint.Footprint.Height, image_layout.height);
            auto copy_width = (std::min)(footprint.Footprint.RowPitch, image_layout.width);

            for (uint32_t row = 0; row < copy_height; row++)
            {
                void* image_src_data = reinterpret_cast<uint8_t*>(src_data) + image_layout.offset + row * image_layout.row_pitch;
                void* upload_dest_data = reinterpret_cast<uint8_t*>(upload_resource_map_data) + footprint.Offset + row * footprint.Footprint.RowPitch;
                ::memcpy(upload_dest_data, image_src_data, copy_width);
            }
        }

        auto before_barrier = TransitionBarrier(dest_res, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, 0);
        auto after_barrier = TransitionBarrier(dest_res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, 0);

        command->ResourceBarrier(1, &before_barrier);

        uint32_t count{};
        for (auto& layout : resource_layout.fontprints)
        {
            D3D12_TEXTURE_COPY_LOCATION src_copy_location{};
            src_copy_location.pResource = upload_resource;
            src_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src_copy_location.PlacedFootprint = layout;

            D3D12_TEXTURE_COPY_LOCATION dest_copy_location{};
            dest_copy_location.pResource = dest_res;
            dest_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dest_copy_location.SubresourceIndex = first_subresource + count;

            command->CopyTextureRegion(&dest_copy_location, 0, 0, 0, &src_copy_location, nullptr);

            count++;
        }

        command->ResourceBarrier(1, &after_barrier);
    }

};

