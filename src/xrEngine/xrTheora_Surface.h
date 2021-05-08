#ifndef xrTheora_SurfaceH
#define xrTheora_SurfaceH
#pragma once

#ifdef SDL_OUTPUT
#include "SDL.h"
#pragma comment(lib, "SDL.lib")
#endif

// refs
class CTheoraStream;

class ENGINE_API CTheoraSurface
{
#ifdef SDL_OUTPUT
    // SDL Video playback structures
    SDL_Surface* sdl_screen;
    SDL_Overlay* sdl_yuv_overlay;
    SDL_Rect sdl_rect;
#endif
    CTheoraStream* m_rgb;
    CTheoraStream* m_alpha;

    u32 tm_start;
    u32 tm_play;
    u32 tm_total;
    bool ready;
    bool bShaderYUV2RGB;
    int prefetch;

public:
    bool playing;
    bool looped;

protected:
    void Reset();

#ifdef SDL_OUTPUT
    void open_sdl_video();
    void write_sdl_video();
#endif
public:
    CTheoraSurface();
    virtual ~CTheoraSurface();

    bool Valid();
    bool Load(const char* fname);

    bool Update(u32 _time);
    void DecompressFrame(u32* dst, u32 _width, int& count);

    void Play(bool _looped, u32 _time);
    void Pause(bool _pause) { playing = !_pause; }
    void Stop()
    {
        playing = false;
        Reset();
    }
    bool IsPlaying() { return playing; }
    u32 Width(bool bRealSize);
    u32 Height(bool bRealSize);
};

#endif // xrTheora_SurfaceH
