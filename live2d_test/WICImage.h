#pragma once

#include <wincodec.h>
#include <wrl.h>

#include <string>

#include "D3DUtil.h"

namespace D3D 
{
    class WICImage
    {
    public:
        ~WICImage();

        static void Initialize();
        static void Uninitialize();

        static Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> LoadImageFormFile(const std::wstring& file_path);
        static Microsoft::WRL::ComPtr<IWICBitmapSource> CovertToD3DPixelFormat(IWICBitmapSource* source_img);
        static Microsoft::WRL::ComPtr<IWICComponentInfo> GetImageComponenInfo(IWICBitmapSource* img);
        static Microsoft::WRL::ComPtr<IWICPixelFormatInfo> GetImagePixelFormatInfo(IWICBitmapSource* img);

        static WICPixelFormatGUID GetD3DPixelFormat();

    private:
        WICImage();

        static WICImage WIC_IMAGE_INSTANCE_;

        Microsoft::WRL::ComPtr<IWICImagingFactory>  wic_factory_;
    };

}


