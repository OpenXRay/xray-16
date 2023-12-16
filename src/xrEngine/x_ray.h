#ifndef __X_RAY_H__
#define __X_RAY_H__

#include "xrCore/Threading/Event.hpp"

struct SDL_Window;
struct SDL_Surface;

// definition
class ENGINE_API CApplication final
{
    SDL_Window* m_window{};
    Event m_should_exit;
    bool m_thread_operational{};

    size_t m_current_surface_idx{};
    xr_vector<SDL_Surface*> m_surfaces;

private:
    static void SplashProc(void* self_ptr);

    void ShowSplash(bool topmost);
    void HideSplash();

public:
    // Other
    CApplication(pcstr commandLine);
    ~CApplication();

    int Run();
};

#endif //__XR_BASE_H__
