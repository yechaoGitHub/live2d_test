#include "WICImage.h"

namespace D3D
{
    using namespace Microsoft::WRL;

    WICImage WICImage::WIC_IMAGE_INSTANCE_;

    WICImage::WICImage() 
    {

    }

    WICImage::~WICImage()
    {
    
    }

    void WICImage::Initialize()
    {
        auto& instance = WIC_IMAGE_INSTANCE_;

        ThrowIfFailed(CoInitialize(nullptr));

        ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&instance.wic_factory_)));
    }

    void WICImage::Uninitialize()
    {
        CoUninitialize();
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> WICImage::LoadImageFormFile(const std::wstring& file_path)
    {
        auto& instance = WIC_IMAGE_INSTANCE_;

        ComPtr<IWICBitmapDecoder> decoder;
        ThrowIfFailed(instance.wic_factory_->CreateDecoderFromFilename(file_path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));

        ComPtr<IWICBitmapFrameDecode> frame;
        ThrowIfFailed(decoder->GetFrame(0, &frame));

        return frame;
    }

    WICPixelFormatGUID WICImage::GetD3DPixelFormat()
    {
        return GUID_WICPixelFormat32bppRGBA;
    }

    Microsoft::WRL::ComPtr<IWICBitmapSource> WICImage::CovertToD3DPixelFormat(IWICBitmapSource* source_img)
    {
        auto& instance = WIC_IMAGE_INSTANCE_;

        WICPixelFormatGUID src_format{};
        ThrowIfFailed(source_img->GetPixelFormat(&src_format));
        if (src_format == GetD3DPixelFormat())
        {
            return source_img;
        }
        
        ComPtr<IWICFormatConverter> converter;
        ThrowIfFailed(WIC_IMAGE_INSTANCE_.wic_factory_->CreateFormatConverter(&converter));

        ThrowIfFailed(converter->Initialize(source_img, GetD3DPixelFormat(), WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom));

        ComPtr<IWICBitmapSource> ret;
        ThrowIfFailed(converter.As(&ret));

        return ret;
    }

};

