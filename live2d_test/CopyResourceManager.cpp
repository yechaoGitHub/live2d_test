#include "CopyResourceManager.h"

namespace D3D
{
    CopyResourceManager::CopyResourceManager()
    {
    }

    CopyResourceManager::~CopyResourceManager()
    {
        ShutDown();
    }

    void CopyResourceManager::Initialize()
    {
        copy_command_allocator_ = D3D12Manager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY);
        copy_command_list_ = D3D12Manager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, copy_command_allocator_.Get());
        copy_command_queue_ = D3D12Manager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_FLAG_NONE);
        copy_fence_ = D3D12Manager::CreateFence(task_completion_count_);

        upload_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, DEFAULT_UPLOAD_BUFFER_SIZE);
        upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&upload_buffer_map_data_));
        upload_buffer_capacity_ = DEFAULT_UPLOAD_BUFFER_SIZE;
        upload_buffer_size_ = 0;
    }

    void CopyResourceManager::ShutDown()
    {
        shut_down_ = true;
    }

    uint64_t CopyResourceManager::PostUploadBufferTask(ID3D12Resource* d3d_dest_resource, uint64_t offset, uint8_t* copy_data, uint64_t copy_length)
    {
        int64_t origin_capacity{};
        int64_t new_capacity{};
        int64_t origin_head_pos{};
        int64_t new_head_pos{};
        uint64_t task_id{};

        do 
        {
            origin_capacity = upload_buffer_capacity_;
            new_capacity = origin_capacity - copy_length;
            if (new_capacity < 0)
            {
                //Resize();
                return 0;
            }
        } 
        while (!upload_buffer_capacity_.compare_exchange_strong(origin_capacity, new_capacity));

        while (task_lock_.exchange(true))
        {
            std::this_thread::yield();
        }

        task_id = task_id_;
        task_id_++;

        origin_head_pos = upload_buffer_head_;
        new_head_pos = (origin_head_pos + copy_length) / upload_buffer_capacity_;
        upload_buffer_head_ = new_head_pos;

        task_lock_ = false;

        CopyBufferTask* task = new CopyBufferTask();
        task->type = UPLOAD_BUFFER;
        task->dest = d3d_dest_resource;
        if (origin_head_pos + copy_length < upload_buffer_capacity_)
        {
            ::memcpy(upload_buffer_map_data_ + origin_head_pos, copy_data, copy_length);
            task->buffer_segments.push_back({ offset, static_cast<uint64_t>(origin_head_pos) , copy_length });
        }
        else 
        {
            uint64_t to_end_length = upload_buffer_capacity_ - origin_head_pos;
            ::memcpy(upload_buffer_map_data_ + origin_head_pos, copy_data, to_end_length);
            task->buffer_segments.push_back({ offset, static_cast<uint64_t>(origin_head_pos) , to_end_length });

            uint64_t to_head_length = new_head_pos;
            ::memcpy(upload_buffer_map_data_, copy_data + to_end_length, to_head_length);
            task->buffer_segments.push_back({ offset + to_end_length,  static_cast<uint64_t>(0) , to_head_length });
        }

        task_queue_.Emplace(task_id, task);
        event_.Notify();

        return task_id;
    }

    uint64_t CopyResourceManager::PostUploadTextureTask(ID3D12Resource* d3d_dest_resource, uint8_t* copy_data, uint64_t copy_data_lenght, const ImageLayout* image_layout, uint32_t image_count, uint32_t subresource_start_index, uint64_t base_offset)
    {
        int64_t origin_capacity{};
        int64_t new_capacity{};
        int64_t origin_head_pos{};
        int64_t new_head_pos{};
        uint64_t task_id{};

        auto&& layout_info = D3D12Manager::GetCopyableFootprints(d3d_dest_resource, subresource_start_index, image_count, base_offset);

        auto& copy_length = layout_info.total_byte_size;

        do
        {
            origin_capacity = upload_buffer_capacity_;
            new_capacity = origin_capacity - copy_length;
            if (new_capacity < 0)
            {
                //Resize();
                return 0;
            }
        } while (!upload_buffer_capacity_.compare_exchange_strong(origin_capacity, new_capacity));

        while (task_lock_.exchange(true))
        {
            std::this_thread::yield();
        }

        task_id = task_id_;
        task_id_;

        origin_head_pos = upload_buffer_head_;
        new_head_pos = (origin_head_pos + copy_length) / upload_buffer_capacity_;
        upload_buffer_head_ = new_head_pos;

        task_lock_ = false;

        CopyBufferTask* task = new CopyBufferTask();
        task->type = UPLOAD_TEXTURE;
        task->dest = d3d_dest_resource;
        for (uint32_t i = 0; i < image_count; i++) 
        {
            auto& cur_image_layout = image_layout[i];
            auto& cur_footprint = layout_info.fontprints[i];
            auto copy_height = (std::min)(cur_footprint.Footprint.Height, cur_image_layout.height);
            auto image_size = (cur_footprint.Footprint.Height * cur_footprint.Footprint.RowPitch) * 4;


            if (origin_head_pos + image_size <= upload_buffer_capacity_) 
            {
                auto copy_length = (std::min)(cur_image_layout.row_pitch, cur_footprint.Footprint.RowPitch);
                for (uint32_t row = 0; row < copy_height; row++)
                {
                    auto copy_src_pos = copy_data + cur_image_layout.offset + cur_image_layout.row_pitch * row;
                    auto dest_buffer_pos = upload_buffer_map_data_ + origin_head_pos + cur_footprint.Footprint.RowPitch * row;

                    ::memcpy(dest_buffer_pos, copy_src_pos, copy_length);
                }

                CopyTextureSegment texture_segment{};
                texture_segment.footprints.Offset = origin_head_pos + cur_footprint.Offset;
                texture_segment.footprints.Footprint = cur_footprint.Footprint;
                texture_segment.src_box.right = copy_length;
                texture_segment.src_box.bottom = copy_height;
                task->texture_segment.push_back(texture_segment);
            }
            else 
            {
                auto copy_height = (std::min)(cur_footprint.Footprint.Height, cur_image_layout.height);

                uint64_t to_end_length = upload_buffer_capacity_ - origin_head_pos;
                auto to_end_height = to_end_length / cur_footprint.Footprint.RowPitch;
                auto to_end_remain = to_end_length % cur_footprint.Footprint.RowPitch;
                auto to_head_height = copy_height - to_end_height - 1;
                auto to_head_remain = cur_footprint.Footprint.RowPitch - to_end_remain;

                uint8_t* copy_src_pos{};
                uint8_t* dest_buffer_pos{};
                uint32_t copy_length{};

                copy_length = (std::min)(cur_image_layout.row_pitch, cur_footprint.Footprint.RowPitch);
                for (uint32_t row = 0; row < to_end_height; row++)
                {
                    copy_src_pos = copy_data + cur_image_layout.offset + cur_image_layout.row_pitch * row;
                    dest_buffer_pos = upload_buffer_map_data_ + origin_head_pos + cur_footprint.Footprint.RowPitch * row;
                    ::memcpy(dest_buffer_pos, copy_src_pos, copy_length);
                }

                CopyTextureSegment texture_segment{};
                texture_segment.footprints.Offset = origin_head_pos + cur_footprint.Offset;
                texture_segment.footprints.Footprint = cur_footprint.Footprint;
                texture_segment.src_box.right = copy_length;
                texture_segment.src_box.bottom = to_end_height;
                task->texture_segment.push_back(texture_segment);
                
                copy_src_pos = copy_data + cur_image_layout.offset + cur_image_layout.row_pitch * (to_end_height + 1);
                dest_buffer_pos = upload_buffer_map_data_ + origin_head_pos + cur_footprint.Footprint.RowPitch * (to_end_height + 1);

                copy_length = (std::min)(static_cast<uint64_t>(cur_image_layout.row_pitch), to_end_remain);
                ::memcpy(dest_buffer_pos, copy_src_pos, copy_length);

                texture_segment.dest_y = to_end_height + 1;
                texture_segment.src_box.right = copy_length;
                texture_segment.src_box.bottom = to_end_height + 1;
                task->texture_segment.push_back(texture_segment);

                dest_buffer_pos = upload_buffer_map_data_;
                if (copy_length > to_end_remain) 
                {
                    copy_length = cur_image_layout.row_pitch - to_end_remain;
                    ::memcpy(dest_buffer_pos, copy_src_pos, copy_length);

                    texture_segment.dest_y = to_end_height + 1;
                    texture_segment.footprints.Offset = 0;
                    texture_segment.src_box.right = copy_length;
                    texture_segment.src_box.bottom = 1;
                    task->texture_segment.push_back(texture_segment);
                }
                dest_buffer_pos += to_head_remain;

                for (uint32_t row = to_end_height + 1; row < copy_height; row++)
                {
                    copy_src_pos = copy_data + cur_image_layout.offset + cur_image_layout.row_pitch * row;
                    dest_buffer_pos = upload_buffer_map_data_ + origin_head_pos + cur_footprint.Footprint.RowPitch * row;
                    copy_length = (std::min)(cur_image_layout.row_pitch, cur_footprint.Footprint.RowPitch);
                    ::memcpy(dest_buffer_pos, copy_src_pos, copy_length);
                }

                texture_segment.dest_y = to_end_height + 1;
                texture_segment.footprints.Offset = to_head_remain;
                texture_segment.src_box.top = to_end_height + 1;
                texture_segment.src_box.right = copy_length;
                texture_segment.src_box.bottom = copy_height;
                task->texture_segment.push_back(texture_segment);
            }
        }

        task_queue_.Emplace(task_id, task);
        event_.Notify();

        return task_id;
    }

    void CopyResourceManager::Resize(uint64_t new_size)
    {

    }

    void CopyResourceManager::CopyResourceThreadFunc()
    {
        while (shut_down_)
        {
            if (task_queue_.Size()) 
            {
                CopyBufferTask* task{};
                auto task_id = task_queue_.Pop(reinterpret_cast<void**>(&task));

                switch (task->type)
                {
                    case UPLOAD_BUFFER:
                        UploadBuffer(task->dest, task->buffer_segments);
                    break;

                    case UPLOAD_TEXTURE:
                        UploadTexture(task->dest, task->texture_segment);
                    break;

                    default:
                    break;
                }

                ID3D12CommandList* command_list_arr[] = { copy_command_list_.Get()};
                copy_command_queue_->ExecuteCommandLists(1, command_list_arr);
                FlushCommandQueue(task_id);
            }
            else 
            {
                event_.Wait();
            }
        }
    }

    void CopyResourceManager::UploadBuffer(ID3D12Resource* dest_resource, const std::vector<CopyBufferSegment>& buffer_segment)
    {
        copy_command_allocator_->Reset();
        copy_command_list_->Reset(copy_command_allocator_.Get(), nullptr);

        auto before_barrier = TransitionBarrier(dest_resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, 0);
        auto after_barrier = TransitionBarrier(dest_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, 0);

        copy_command_list_->ResourceBarrier(1, &before_barrier);

        for (const auto& segment : buffer_segment)
        {
            copy_command_list_->CopyBufferRegion(dest_resource, segment.dest_offset, dest_resource, segment.src_start, segment.length);
        }

        copy_command_list_->ResourceBarrier(1, &after_barrier);

        ThrowIfFailed(copy_command_list_->Close());
    }

    void CopyResourceManager::UploadTexture(ID3D12Resource* dest_resource, const std::vector<CopyTextureSegment>& texture_segment)
    {

    }

    void CopyResourceManager::FlushCommandQueue(uint64_t new_fence)
    {
        ThrowIfFailed(copy_command_queue_->Signal(copy_fence_.Get(), new_fence));

        if (copy_fence_->GetCompletedValue() < new_fence)
        {
            HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(copy_fence_->SetEventOnCompletion(new_fence, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }

};
