// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "D3D12Manager.h"
#include "D3D12Renderer.h"

SDL_Window*     window{};
SDL_Renderer*   renderer{};
HWND            hwnd{};

int main(int argc, char** argv)
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);

    D3D::D3D12Manager::Initialize();

    hwnd = D3D::D3D12Manager::CreateD3DWindow(L"live2d", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600);
    window = SDL_CreateWindowFrom(hwnd);
    D3D::D3D12Renderer renderer(hwnd, 800, 600);
    renderer.Initialize();

    SDL_Event windows_event;
    bool quit{ false };
    while (!quit) 
    {
        while (SDL_PollEvent(&windows_event)) 
        {
            switch (windows_event.type)
            {
                case SDL_QUIT:
                    quit = true;
                break;

                case SDL_KEYDOWN:
                break;

                case SDL_KEYUP:
                break;

                case SDL_MOUSEBUTTONUP:
                    OutputDebugStringW(L"mouse click");
                    renderer.Update();
                    renderer.Render();
                break;

                default:
                break;
            }
        }
    }
                 
    SDL_DestroyWindow(window);                              
    SDL_Quit();

    return 0;
}


