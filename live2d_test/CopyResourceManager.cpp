#include "CopyResourceManager.h"

#include "D3D12Manager.h" 

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
        copy_fence_ = D3D12Manager::CreateFence(exec_task_id_);

        upload_buffer_ = D3D12Manager::CreateBuffer(D3D12_HEAP_TYPE_UPLOAD, DEFAULT_UPLOAD_BUFFER_SIZE_);
        upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&upload_buffer_map_data_));
    }

    void CopyResourceManager::StartUp()
    {
        ThrowIfFalse(!running_);
        ThrowIfFalse(copy_resource_thread_.get_id() == std::thread::id());
        running_ = true;
        copy_resource_thread_ = std::thread(&CopyResourceManager::CopyResourceThreadFunc, this);
    }

    void CopyResourceManager::ShutDown()
    {
        running_ = false;
        event_.Notify();
        copy_resource_thread_.join();
    }

    uint64_t CopyResourceManager::PostUploadBufferTask(ID3D12Resource* d3d_dest_resource, uint64_t offset, void* copy_data, uint64_t copy_length)
    {
        UploadBufferTask* task = new UploadBufferTask();

        task->dest_res = d3d_dest_resource;
        task->dest_offset = offset;
        task->src_data = copy_data;
        task->length = copy_length;
        task->upload_resource = upload_buffer_.Get();
        task->upload_resource_map_data = upload_buffer_map_data_;

        uint64_t ret{};
        {
            std::lock_guard<std::mutex> guard(task_queue_lock_);
            task_queue_.push_back(task);
            assign_task_id_++;
            ret = assign_task_id_;
        }

        event_.Notify();

        return assign_task_id_;
    }

    uint64_t CopyResourceManager::PostUploadTextureTask(ID3D12Resource* d3d_dest_resource, uint32_t first_subresource, uint32_t subresource_count, void* copy_data, const ImageLayout* image_layout)
    {
        UploadTextureTask* task = new UploadTextureTask();

        task->dest_res = d3d_dest_resource;
        task->src_data = copy_data;
        task->first_subresource = first_subresource;

        for (uint32_t i = 0; i < subresource_count; i++)
        {
            UploadTextureSubresourceTask subresource_task;
            subresource_task.src_image_layout = image_layout[i];
        }

        uint64_t ret{};
        {
            std::lock_guard<std::mutex> guard(task_queue_lock_);
            task_queue_.push_back(task);
            assign_task_id_++;
            ret = assign_task_id_;
        }

        event_.Notify();

        return ret;
    }

    uint64_t CopyResourceManager::GetCurTaskID()
    {
        return assign_task_id_;
    }

    uint64_t CopyResourceManager::GetExcuteCount()
    {
        return exec_task_id_;
    }

    CopyTask* CopyResourceManager::PopTask()
    {
        CopyTask* task{};

        {
            std::lock_guard<std::mutex> guard(task_queue_lock_);
            if (!task_queue_.empty()) 
            {
                task = task_queue_.front();
                task_queue_.pop_front();
            }
        }

        return task;
    }

    void CopyResourceManager::Resize(uint64_t new_size)
    {

    }

    void CopyResourceManager::CopyResourceThreadFunc()
    {
        while (running_)
        {
            auto task = PopTask();
            if (task) 
            {
                copy_command_allocator_->Reset();
                copy_command_list_->Reset(copy_command_allocator_.Get(), nullptr);
                task->ExcuteCopyTask(copy_command_list_.Get());
                copy_command_list_->Close();

                FlushCommandQueue();
            }
            else 
            {
                event_.Wait();
            }
        }
    }

    void CopyResourceManager::FlushCommandQueue()
    {
        ID3D12CommandList* commands[] = { copy_command_list_ .Get() };
        copy_command_queue_->ExecuteCommandLists(1, commands);

        auto new_fence = exec_task_id_ + 1;
        ThrowIfFailed(copy_command_queue_->Signal(copy_fence_.Get(), new_fence));

        if (copy_fence_->GetCompletedValue() < new_fence)
        {
            HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(copy_fence_->SetEventOnCompletion(new_fence, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }

        exec_task_id_++;
    }

};
