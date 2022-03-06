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
        auto& instance = WIC_IMAGE_INSTANCE_;
        instance.wic_factory_ = nullptr;
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

    Microsoft::WRL::ComPtr<IWICBitmap> WICImage::CreateBmpFormSource(IWICBitmapSource* source)
    {
        auto& instance = WIC_IMAGE_INSTANCE_;
        ComPtr<IWICBitmap> ret;
        ThrowIfFailed(instance.wic_factory_->CreateBitmapFromSource(source, WICBitmapCacheOnDemand, &ret));
        return ret;
    }

    Microsoft::WRL::ComPtr<IWICComponentInfo> WICImage::GetImageComponenInfo(IWICBitmapSource* img)
    {
        auto& instance = WIC_IMAGE_INSTANCE_;

        WICPixelFormatGUID format{};
        ThrowIfFailed(img->GetPixelFormat(&format));

        ComPtr<IWICComponentInfo> component_info{};
        ThrowIfFailed(instance.wic_factory_->CreateComponentInfo(format, &component_info));

        return component_info;
    }

    Microsoft::WRL::ComPtr<IWICPixelFormatInfo> WICImage::GetImagePixelFormatInfo(IWICBitmapSource* img)
    {
        auto pixel_format_info = GetImageComponenInfo(img);

        WICComponentType type;
        ThrowIfFailed(pixel_format_info->GetComponentType(&type));

        if (type != WICPixelFormat)
        {
            ThrowIfFailed(E_FAIL);
        }

        Microsoft::WRL::ComPtr<IWICPixelFormatInfo> ret;
        pixel_format_info.As(&ret);

        return ret;
    }

};

