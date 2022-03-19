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

    hwnd = D3D::D3D12Manager::CreateD3DWindow(L"live2d", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080);
    window = SDL_CreateWindowFrom(hwnd);
    D3D::D3D12Renderer renderer(hwnd);
    renderer.Initialize();
    renderer.ShowDebugWindow(true);

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

                default:
                break;
            }
        }
        else
        {
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


