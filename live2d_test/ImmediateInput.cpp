#include "ImmediateInput.h"

namespace D3D
{
    const std::unordered_map<Key, int>  ImmediateInput::KEY_TO_VK_MAP_ =
    {
        {Key_Tab,           VK_TAB},
        {Key_LeftArrow,     VK_LEFT},
        {Key_RightArrow,    VK_RIGHT},
        {Key_UpArrow,       VK_UP},
        {Key_DownArrow,     VK_DOWN},
        {Key_PageUp,        VK_PRIOR},
        {Key_PageDown,      VK_NEXT},
        {Key_Home,          VK_HOME},
        {Key_End,           VK_END},
        {Key_Insert,        VK_INSERT},
        {Key_Delete,        VK_DELETE},
        {Key_Backspace,     VK_BACK},
        {Key_Space,         VK_SPACE},
        {Key_Enter,         VK_RETURN},
        {Key_Escape,        VK_ESCAPE},
        {Key_LeftCtrl,      VK_LCONTROL},
        {Key_LeftShift,     VK_LSHIFT},
        {Key_LeftAlt,       VK_LMENU},
        {Key_LeftSuper,     VK_LWIN},
        {Key_RightCtrl,     VK_RSHIFT},
        {Key_RightShift,    VK_RSHIFT},
        {Key_RightAlt,      VK_RMENU},
        {Key_RightSuper,    VK_RWIN},
        {Key_Menu,          VK_APPS},
        {Key_0,             '0'},
        {Key_1,             '1'},
        {Key_2,             '2'},
        {Key_3,             '3'},
        {Key_4,             '4'},
        {Key_5,             '5'},
        {Key_6,             '6'},
        {Key_7,             '7'},
        {Key_8,             '8'},
        {Key_9,             '9'},
        {Key_A,             'A'},
        {Key_B,             'B'},
        {Key_C,             'C'},
        {Key_D,             'D'},
        {Key_E,             'E'},
        {Key_F,             'F'},
        {Key_G,             'G'},
        {Key_H,             'H'},
        {Key_I,             'I'},
        {Key_J,             'J'},
        {Key_K,             'K'},
        {Key_L,             'L'},
        {Key_M,             'M'},
        {Key_N,             'N'},
        {Key_O,             '0'},
        {Key_P,             'P'},
        {Key_Q,             'Q'},
        {Key_R,             'R'},
        {Key_S,             'S'},
        {Key_T,             'T'},
        {Key_U,             'U'},
        {Key_V,             'V'},
        {Key_W,             'W'},
        {Key_X,             'X'},
        {Key_Y,             'Y'},
        {Key_Z,             'Z'},
        {Key_F1,            VK_F1},
        {Key_F2,            VK_F2},
        {Key_F3,            VK_F3},
        {Key_F4,            VK_F4},
        {Key_F5,            VK_F5},
        {Key_F6,            VK_F6},
        {Key_F7,            VK_F7},
        {Key_F8,            VK_F8},
        {Key_F9,            VK_F9},
        {Key_F10,           VK_F10},
        {Key_F11,           VK_F11},
        {Key_F12,           VK_F12},
        {Key_Apostrophe,    VK_OEM_7},
        {Key_Comma,         VK_OEM_COMMA},
        {Key_Minus,         VK_OEM_MINUS},
        {Key_Period,        VK_OEM_PERIOD},
        {Key_Slash,         VK_OEM_2},
        {Key_Semicolon,     VK_OEM_1},
        {Key_Equal,         VK_OEM_PLUS},
        {Key_LeftBracket,   VK_OEM_4},
        {Key_Backslash,     VK_OEM_5},
        {Key_RightBracket,  VK_OEM_6},
        {Key_GraveAccent,   VK_OEM_3},
        {Key_CapsLock,      VK_CAPITAL},
        {Key_ScrollLock,    VK_SCROLL},
        {Key_NumLock,       VK_NUMLOCK},
        {Key_PrintScreen,   VK_SNAPSHOT},
        {Key_Pause,         VK_PAUSE},
        {Key_Keypad0,       VK_NUMPAD0},
        {Key_Keypad1,       VK_NUMPAD1},
        {Key_Keypad2,       VK_NUMPAD2},
        {Key_Keypad3,       VK_NUMPAD3},
        {Key_Keypad4,       VK_NUMPAD4},
        {Key_Keypad5,       VK_NUMPAD5},
        {Key_Keypad6,       VK_NUMPAD6},
        {Key_Keypad7,       VK_NUMPAD7},
        {Key_Keypad8,       VK_NUMPAD8},
        {Key_Keypad9,       VK_NUMPAD9},
        {Key_KeypadDecimal,     VK_DECIMAL},
        {Key_KeypadDivide,      VK_DIVIDE},
        {Key_KeypadMultiply,    VK_MULTIPLY},
        {Key_KeypadSubtract,    VK_SUBTRACT},
        {Key_KeypadAdd,         VK_ADD},
        {Key_KeypadEnter,       VK_RETURN}
    };

    ImmediateInput::ImmediateInput(HWND hwnd) :
        hwnd_(hwnd)
    {
    }

    ImmediateInput::~ImmediateInput()
    {
    }

    void ImmediateInput::RegisterKeyEvent(const Key* keys, int count)
    {
        for (int i = 0; i < count; i++)
        {
            auto& key = keys[i];
            auto it_find = std::find(register_keys_.begin(), register_keys_.end(), key);
            if (it_find == register_keys_.end())
            {
                register_keys_.push_back(key);
            }
        }
    }

    void ImmediateInput::UnregisterKeyEvent(const Key* keys, int count)
    {
        for (int i = 0; i < count; i++)
        {
            auto& key = keys[i];
            auto it_find = std::find(register_keys_.begin(), register_keys_.end(), key);
            if (it_find != register_keys_.end())
            {
                register_keys_.erase(it_find);
            }
        }
    }

    void ImmediateInput::HandleInput()
    {
        if (::GetForegroundWindow() == hwnd_)
        {
            HandleMouseInput();
            HandleKeyInput();
        }
    }

    bool ImmediateInput::IsMouseBtnHold(MouseButton btn)
    {
        return (mouse_state_ & btn) != 0;
    }

    void ImmediateInput::HandleMouseInput()
    {
        ThrowIfFalse(mouse_callback_.operator bool());

        bool is_lbtn_click = (mouse_state_ & kLButtonClick) != 0;
        bool is_rbtn_click = (mouse_state_ & kRButtonClick) != 0;

        POINT pos{};
        ::GetCursorPos(&pos);
        ::ScreenToClient(hwnd_, &pos);
        if (last_mouse_pos_.x != pos.x ||
            last_mouse_pos_.y != pos.y)
        {
            mouse_callback_(kMouseMove, kNoneMouseButton, pos.x, pos.y);
            last_mouse_pos_ = pos;
        }

        if (IsVkDown(VK_LBUTTON, !is_lbtn_click))
        {
            mouse_state_ |= kLButtonClick;
            mouse_callback_(kButtonPressed, kLMouseButton, pos.x, pos.y);
        }
        else if (is_lbtn_click)
        {
            mouse_state_ &= ~kLButtonClick;
            mouse_callback_(kButtonReleased, kLMouseButton, pos.x, pos.y);
        }

        if (IsVkDown(VK_RBUTTON, !is_rbtn_click))
        {
            mouse_state_ |= kRButtonClick;
            mouse_callback_(kButtonPressed, kRMouseButton, pos.x, pos.y);
        }
        else if (is_rbtn_click)
        {
            mouse_state_ &= ~kRButtonClick;
            mouse_callback_(kButtonReleased, kRMouseButton, pos.x, pos.y);
        }
    }

    void ImmediateInput::HandleKeyInput()
    {
        ThrowIfFalse(key_callback_.operator bool());

        auto it = pressed_keys_.begin();
        while (it != pressed_keys_.end())
        {
            auto key = *it;
            int vk_key = KeyToVk(key);
            ThrowIfFalse(vk_key);

            if (IsVkDown(vk_key, false))
            {
                key_callback_(kKeyReleased, key);
                it = pressed_keys_.erase(it);
            }
            else
            {
                it++;
            }
        }

        for (auto& key : register_keys_)
        {
            int vk_key = KeyToVk(key);
            ThrowIfFalse(vk_key);

            if (IsVkDown(vk_key, true))
            {
                key_callback_(kKeyPressed, key);
                pressed_keys_.push_back(key);
            }
        }
    }

    int ImmediateInput::KeyToVk(Key key)
    {
        auto it = KEY_TO_VK_MAP_.find(key);
        if (it != KEY_TO_VK_MAP_.end())
        {
            return it->second;
        }
        else
        {
            return 0;
        }
    }

    bool ImmediateInput::IsVkDown(int vk, bool test_last_click)
    {
        int ret = ::GetAsyncKeyState(vk);
        if (test_last_click)
        {
            return (ret & 0x8001) != 0;
        }
        else
        {
            return (ret & 0x8000) != 0;
        }
    }
};

