#pragma once

struct SDL_Window;
class IRenderBackend;

class FGRenderHost
{
public:
    static IRenderBackend* CreateBackend(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, bool enableValidation);
    static void ResizeBackend(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight);
};
