#include "stdafx.h"

class sdl_window_test_helper
{
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;

public:
    sdl_window_test_helper()
    {
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

bool TestOpenGLSupport()
{
    // Check if minimal required OpenGL features are available
    const sdl_window_test_helper windowTest;
    if (!windowTest.successful())
        return false;

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
         Log("~ Could not initialize glew:", (pcstr)glewGetErrorString(err));
         return false;
    }

    return true;
}

BOOL xrRender_test_hw()
{
    return TestOpenGLSupport() ? TRUE : FALSE;
}
