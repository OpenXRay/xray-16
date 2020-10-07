#include "stdafx.h"

class DX11TestHelper
{
    SDL_Window* m_window = nullptr;
    CHW m_hw;

public:
    DX11TestHelper()
    {
        m_window = SDL_CreateWindow("TestDX11Window", 0, 0, 1, 1, SDL_WINDOW_HIDDEN);
        if (!m_window)
        {
            Log("~ Cannot create helper window for DirectX 11 test:", SDL_GetError());
            return;
        }
        m_hw.CreateDevice(m_window);
    }

    bool Successful() const
    {
        return m_window && m_hw.Valid;
    }

    const CHW& GetHW() const { return m_hw; }

    ~DX11TestHelper()
    {
        m_hw.DestroyDevice();
        SDL_DestroyWindow(m_window);
    }
};


BOOL xrRender_test_hw()
{
    const DX11TestHelper helper;
    if (!helper.Successful())
        return FALSE;

    const auto level = helper.GetHW().FeatureLevel;
    if (level >= D3D_FEATURE_LEVEL_11_0)
        return TRUE + TRUE; // XXX: remove hack
    if (level >= D3D_FEATURE_LEVEL_10_0)
        return TRUE;
    return FALSE;
}
