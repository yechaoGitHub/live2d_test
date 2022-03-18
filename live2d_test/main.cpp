// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "D3D12Manager.h"
#include "D3D12Renderer.h"
#include "WICImage.h"
#include "ImGuiProxy.h"

SDL_Window*     window{};
SDL_Renderer*   renderer{};
HWND            hwnd{};

int main(int argc, char** argv)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);

    D3D::D3D12Manager::Initialize();
    D3D::WICImage::Initialize();
    D3D::ImGuiProxy::Initialize();

    ImGuiKey key_event[] = { ImGuiKey_LeftShift, ImGuiKey_RightShift, ImGuiKey_LeftSuper, ImGuiKey_RightSuper, ImGuiKey_ModCtrl, ImGuiKey_ModShift, ImGuiKey_ModAlt, ImGuiKey_ModSuper };
    D3D::ImGuiProxy::RegisterKeyboardEvent(key_event, _countof(key_event));

    hwnd = D3D::D3D12Manager::CreateD3DWindow(L"live2d", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080);
    window = SDL_CreateWindowFrom(hwnd);
    RECT work_rt{};
    ::GetClientRect(hwnd, &work_rt);
    RECT wnd_rt{};
    ::GetWindowRect(hwnd, &wnd_rt);

    D3D::D3D12Renderer renderer(hwnd, work_rt.right, work_rt.bottom);
    renderer.Initialize();

    ImGui::GetMainViewport()->PlatformHandleRaw = (void*)hwnd;

    SDL_Event windows_event;
    bool quit{ false };
    while (!quit)
    {
        if (SDL_PollEvent(&windows_event))
        {
            switch (windows_event.type)
            {
                case SDL_QUIT:
                    quit = true;
                break;

                case SDL_KEYDOWN:
                    renderer.OnKeyDown(windows_event.key.keysym.sym);
                break;

                case SDL_KEYUP:
                    renderer.OnKeyUp(windows_event.key.keysym.sym);
                break;

                case SDL_MOUSEBUTTONDOWN:
                    renderer.OnMouseDown(windows_event.button.button, windows_event.button.x, windows_event.button.y);
                break;

                case SDL_MOUSEMOTION:
                    renderer.OnMouseMove(windows_event.button.x, windows_event.button.y);
                break;

                case SDL_MOUSEBUTTONUP:
                    renderer.OnMouseUp(windows_event.button.button, windows_event.button.x, windows_event.button.y);
                break;

                default:
                break;
            }
        }
        else
        {
            ImGuiIO& io = ImGui::GetIO();
            RECT rect = { 0, 0, 0, 0 };
            ::GetClientRect(hwnd, &rect);
            io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

            ImGui::NewFrame();
            ImGui::ShowDemoWindow();
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();

            D3D::ImGuiProxy::HandleInputEvent(hwnd);

            ImGui::Render();

            renderer.Update();
            renderer.Render();
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    D3D::ImGuiProxy::Uninitialize();
    renderer.ClearUp();
    renderer.~D3D12Renderer();
    D3D::WICImage::Uninitialize();

    return 0;
}


