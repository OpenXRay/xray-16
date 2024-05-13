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
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "TestOpenGLWindow");
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 0);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1);
        SDL_SetNumberProperty(props, "flags", SDL_WINDOW_HIDDEN | flags);
        m_window = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);

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
#if 1
    GLenum err;
    {
        ZoneScopedN("glewInit()");
        err = glewInit();
    }
    if (GLEW_OK != err && GLEW_ERROR_NO_GLX_DISPLAY != err)
    {
        Log("~ Could not initialize glew:", (pcstr)glewGetErrorString(err));
        return FALSE;
    }
#endif
    return TRUE;
}
