#include "stdafx.h"

class sdl_window_test_helper
{
    SDL_Window* m_window{};
    SDL_GLContext m_context{};

public:
    sdl_window_test_helper()
    {
        ZoneScoped;
        u32 flags{};
        HW.SetPrimaryAttributes(flags);
        m_window = SDL_CreateWindow("TestOpenGLWindow", 0, 0, 1, 1, SDL_WINDOW_HIDDEN | flags);
        if (!m_window)
        {
            Log("~ Cannot create helper window for OpenGL:", SDL_GetError());
            return;
        }

        m_context = SDL_GL_CreateContext(m_window);
        if (!m_context)
        {
            Log("~ Cannot create OpenGL context:", SDL_GetError());
            return;
        }
    }

    [[nodiscard]]
    bool successful() const
    {
        return m_window && m_context;
    }

    ~sdl_window_test_helper()
    {
        SDL_GL_DeleteContext(m_context);
        SDL_DestroyWindow(m_window);
    }
};

BOOL xrRender_test_hw()
{
    ZoneScoped;

    // Check if minimal required OpenGL features are available
    const sdl_window_test_helper windowTest;
    if (!windowTest.successful())
        return FALSE;

    return TRUE;
}
