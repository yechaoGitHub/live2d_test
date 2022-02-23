#pragma once
#include "D3D12Manager.h"
#include "D3DEvent.h"
#include "CopyTaskRingBuffer.h"

namespace D3D
{
    class CopyResourceManager
    {
    public:
        struct ImageLayout
        {
            uint64_t offset;
            uint32_t width;
            uint32_t height;
            uint32_t row_pitch;
        };

        CopyResourceManager();
        ~CopyResourceManager();

        void Initialize();
        void ShutDown();

        uint64_t PostUploadBufferTask(ID3D12Resource* d3d_dest_resource, uint64_t offset, uint8_t* copy_data, uint64_t copy_length);
        uint64_t PostUploadTextureTask(ID3D12Resource* d3d_dest_resource, uint8_t* copy_data, uint64_t copy_data_length, const ImageLayout* image_layout, uint32_t image_count, uint32_t subresource_start_index, uint64_t base_offset);

    private:
        enum CopyBufferType
        {
            UPLOAD_BUFFER,
            UPLOAD_TEXTURE,
        };

        struct CopyBufferSegment
        {
            uint64_t dest_offset = 0;
            uint64_t src_start = 0;
            uint64_t length = 0;
        };

        struct CopyTextureSegment 
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints;
            uint32_t dest_x = 0;
            uint32_t dest_y = 0;
            uint32_t dest_z = 0;
            D3D12_BOX src_box;
        };

        struct CopyBufferTask
        {
            CopyBufferType type{};
            std::vector<CopyBufferSegment>  buffer_segments;
            std::vector<CopyTextureSegment> texture_segment;
            ID3D12Resource* dest = {};
        };

        void Resize(uint64_t new_size);
        void CopyResourceThreadFunc();
        void UploadBuffer(ID3D12Resource* dest_resource, const std::vector<CopyBufferSegment>& buffer_segment);
        void UploadTexture(ID3D12Resource* dest_resource, const std::vector<CopyTextureSegment>& texture_segment);
        void FlushCommandQueue(uint64_t new_fence);

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      copy_command_allocator_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   copy_command_list_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          copy_command_queue_;
        Microsoft::WRL::ComPtr<ID3D12Fence>                 copy_fence_;

        CopyTaskRingBuffer                                  task_queue_;
        std::atomic<bool>                                   task_lock_;
        uint64_t                                            task_id_ = 0;
        uint64_t                                            task_completion_count_ = 0;

        Microsoft::WRL::ComPtr<ID3D12Resource>              upload_buffer_;
        static const uint64_t                               DEFAULT_UPLOAD_BUFFER_SIZE = 1920 * 1080 * 4;
        std::atomic<int64_t>                                upload_buffer_capacity_ = 0;
        std::atomic<int64_t>                                upload_buffer_size_ = 0;
        std::atomic<int64_t>                                upload_buffer_head_ = 0;
        std::atomic<int64_t>                                upload_buffer_tail_ = 0;
        uint8_t*                                            upload_buffer_map_data_ = nullptr;
        D3DEvent                                            event_;
        bool                                                shut_down_ = false;

        std::thread                                         copy_resource_thread_;
        
    };

};


