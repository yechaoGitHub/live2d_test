#pragma once

#include <d3d12.h>

#include <functional>
#include <stdint.h>
#include <vector>


namespace D3D
{
    struct ImageLayout
    {
        uint64_t offset = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t row_pitch = 0;
    };

    class CopyTask
    {
    public:
        enum TaskType
        {
            UPLOAD_BUFFER,
            UPLOAD_TEXTURE,
        };

        ~CopyTask();

        void BindData(void* data);
        void* GetData();

        template<typename TFunction, typename ...TParam>
        void BindFunction(TFunction func, TParam&&... args)
        {
            call_back_ = std::bind(func, std::move(args...));
        }

        virtual void ExcuteCopyTask(ID3D12GraphicsCommandList* command) = 0;

        void ExcuteCallback();

    protected:
        CopyTask(TaskType type);

    private:
        TaskType type_{};
        void* data_ = nullptr;
        std::function<void(const CopyTask*)> call_back_;
    };

    class UploadBufferTask : public CopyTask
    {
    public:
        UploadBufferTask();
        ~UploadBufferTask();

        void ExcuteCopyTask(ID3D12GraphicsCommandList* command) override;

        ID3D12Resource* dest_res = nullptr;
        uint64_t dest_offset = 0;
        void* src_data = nullptr;
        uint64_t length = 0;

        ID3D12Resource* upload_resource = nullptr;
        void* upload_resource_map_data = nullptr;
    };


    struct UploadTextureSubresourceTask
    {
        uint32_t dest_x = 0;
        uint32_t dest_y = 0;
        uint32_t dest_z = 0;

        ImageLayout src_image_layout;
        D3D12_BOX src_box;
    };

    class UploadTextureTask : public CopyTask
    {
    public:
        UploadTextureTask();
        ~UploadTextureTask();

        void ExcuteCopyTask(ID3D12GraphicsCommandList* command) override;

        ID3D12Resource* dest_res = nullptr;
        void* src_data = nullptr;

        uint32_t first_subresource = 0;
        std::vector<UploadTextureSubresourceTask> subresource_tasks;

        ID3D12Resource* upload_resource = nullptr;
        void* upload_resource_map_data = nullptr;
    };

};

