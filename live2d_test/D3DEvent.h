#pragma once

#include "D3DUtil.h"

namespace D3D
{
    class D3DEvent
    {
    public:
        D3DEvent();
        ~D3DEvent();

        void Notify();
        void Wait();

    private:
        HANDLE event_ = INVALID_HANDLE_VALUE;
    };

};
