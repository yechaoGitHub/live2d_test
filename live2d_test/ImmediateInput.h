#pragma once

#include <Windows.h>

#include <functional>
#include <unordered_map>
#include <vector>

#include "D3DUtil.h"
#include "InputDefine.h"


namespace D3D
{
    class ImmediateInput
    {
    public:
        using MouseEventCallback = void(MouseAction, MouseButton, int, int);
        using KeyEventCallback = void(KeyAction, Key);

        ImmediateInput(HWND hwnd);
        ~ImmediateInput();

        void RegisterKeyEvent(const Key* keys, int count);
        void UnregisterKeyEvent(const Key* keys, int count);

        template<typename TFunction, typename ...TParam>
        void RegisterMouseEventHandle(TFunction&& func, TParam&&... param)
        {
            mouse_callback_ = std::bind(std::move(func), std::move(param)...);
        }

        template<typename TFunction, typename ...TParam>
        void RegisterKeyEventHandle(TFunction&& func, TParam&&... param)
        {
            key_callback_ = std::bind(std::move(func), std::move(param)...);
        }

        void HandleInput();
        bool IsMouseBtnHold(MouseButton btn);

    private:
        enum MouseButtonState { kLButtonClick = 1, kRButtonClick };

        void HandleMouseInput();
        void HandleKeyInput();
        static int KeyToVk(Key key);
        static bool IsVkDown(int vk, bool test_last_click);

        static const std::unordered_map<Key, int>      KEY_TO_VK_MAP_;

        HWND                                hwnd_ = {};
        std::vector<Key>                    register_keys_;
        std::vector<Key>                    pressed_keys_;
        std::function<MouseEventCallback>   mouse_callback_;
        std::function<KeyEventCallback>     key_callback_;
        int                                 mouse_state_ = 0;
        POINT                               last_mouse_pos_ = {};
    };
};



