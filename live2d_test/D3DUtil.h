#pragma once

#include <Windows.h>
#include <d3d12.h>

#include <exception>
#include <string>
#include <sstream>


namespace D3D
{
    #define STRUCT_ALIGN_16	__declspec(align(16))
    #define STRUCT_ALIGN(v)	__declspec(align(v))

    static uint32_t CalcConstantBufferByteSize(uint32_t byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }

    D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after, uint32_t subresource);

    inline std::wstring AnsiToWString(const std::string& str)
    {
        WCHAR buffer[512];
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
        return std::wstring(buffer);
    }

    class DxException
    {
    public:
        DxException() = default;
        DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

        std::wstring ToString()const;

        HRESULT ErrorCode = S_OK;
        std::wstring FunctionName;
        std::wstring Filename;
        int LineNumber = -1;
    };


    #ifndef ThrowIfFailed
    #define ThrowIfFailed(x)                                              \
    {                                                                     \
        HRESULT hr__ = (x);                                               \
        std::wstring wfn = AnsiToWString(__FILE__);                       \
        if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
    }
    #endif

    #ifndef ThrowIfFalses
    #define ThrowIfFalse(x)                             \
    {                                                   \
        bool b = (x);                                   \
        if (!b)                                         \
        {                                               \
            std::stringstream error;                    \
            error << L#x;                               \
            error << " failed in ";                     \
            error << __FILE__;                          \
            error << "; line ";                         \
            error << __LINE__;                          \
            throw std::exception(error.str().c_str());  \
        }                                               \
    }
    #endif
};

