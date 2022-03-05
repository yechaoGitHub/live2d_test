#pragma once

#include "D3DEvent.h"
#include "CopyTask.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <deque>
#include <mutex>


namespace D3D
{
    class CopyResourceManager
    {
    public:
        CopyResourceManager();
        ~CopyResourceManager();

        void Initialize();
        void StartUp();
        void ShutDown();

        uint64_t PostUploadBufferTask(ID3D12Resource* d3d_dest_resource, uint64_t offset, void* copy_data, uint64_t copy_length);
        uint64_t PostUploadTextureTask(ID3D12Resource* d3d_dest_resource,  uint32_t first_subresource, uint32_t subresource_count, void* copy_data, const ImageLayout* image_layout);

        uint64_t GetCurTaskID();
        uint64_t GetExcuteCount();

    private:
        CopyTask* PopTask();

        void Resize(uint64_t new_size);
        void CopyResourceThreadFunc();
        void FlushCommandQueue();

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      copy_command_allocator_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   copy_command_list_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          copy_command_queue_;
        Microsoft::WRL::ComPtr<ID3D12Fence>                 copy_fence_;

        const uint64_t                                      DEFAULT_UPLOAD_BUFFER_SIZE_ = 1920 * 1080 * 4;

        std::deque<CopyTask*>                               task_queue_;
        std::mutex                                          task_queue_lock_;
        uint64_t                                            assign_task_id_ = 0;
        std::atomic<uint64_t>                               exec_task_id_ = 0;
        D3DEvent                                            event_;

        Microsoft::WRL::ComPtr<ID3D12Resource>              upload_buffer_;
        uint8_t*                                            upload_buffer_map_data_ = nullptr;
        bool                                                running_ = false;
        std::thread                                         copy_resource_thread_;
    };

};


