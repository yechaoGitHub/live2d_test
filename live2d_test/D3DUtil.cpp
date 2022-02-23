#include "D3DUtil.h"
#include <comdef.h>

D3D::DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

std::wstring D3D::DxException::ToString() const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

D3D12_RESOURCE_BARRIER D3D::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after, uint32_t subresource)
{
    D3D12_RESOURCE_BARRIER ret;
    ret.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ret.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ret.Transition.pResource = resource;
    ret.Transition.StateBefore = state_before;
    ret.Transition.StateAfter = state_after;
    ret.Transition.Subresource = subresource;

    return ret;
}
