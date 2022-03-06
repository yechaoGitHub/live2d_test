#include "D3DEvent.h"

namespace D3D
{
    D3DEvent::D3DEvent()
    {
        event_ = ::CreateEvent(nullptr, false, false, nullptr);
        ThrowIfFalse(event_ != INVALID_HANDLE_VALUE);
    }

    D3DEvent::~D3DEvent()
    {
        ThrowIfFalse(::CloseHandle(event_));
    }

    void D3DEvent::Notify()
    {
        ThrowIfFalse(::SetEvent(event_));
    }

    void D3DEvent::Wait()
    {
        ThrowIfFalse(::WaitForSingleObject(event_, INFINITE) == WAIT_OBJECT_0);
    }
};
