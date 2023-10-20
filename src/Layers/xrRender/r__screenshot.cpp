#include "stdafx.h"

IC u32 convert(float c)
{
    u32 C = iFloor(c);
    if (C > 255)
        C = 255;
    return C;
}

IC void MouseRayFromPoint(Fvector& direction, int x, int y, Fmatrix& m_CamMat)
{
    int halfwidth = Device.dwWidth / 2;
    int halfheight = Device.dwHeight / 2;

    Ivector2 point2;
    point2.set(x - halfwidth, halfheight - y);

    float size_y = VIEWPORT_NEAR * tanf(deg2rad(60.f) * 0.5f);
    float size_x = size_y / (Device.fHeight_2 / Device.fWidth_2);

    float r_pt = float(point2.x) * size_x / (float)halfwidth;
    float u_pt = float(point2.y) * size_y / (float)halfheight;

    direction.mul(m_CamMat.k, VIEWPORT_NEAR);
    direction.mad(direction, m_CamMat.j, u_pt);
    direction.mad(direction, m_CamMat.i, r_pt);
    direction.normalize();
}

void CRender::Screenshot(ScreenshotMode mode, LPCSTR name) { ScreenshotImpl(mode, name, nullptr); }
void CRender::Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer)
{
    if (mode != SM_FOR_MPSENDING)
    {
        Log("~ Not implemented screenshot mode...");
        return;
    }
    ScreenshotImpl(mode, nullptr, &memory_writer);
}

void CRender::ScreenshotAsyncBegin()
{
    VERIFY(!m_bMakeAsyncSS);
    m_bMakeAsyncSS = true;
}

void DoAsyncScreenshot() { RImplementation.Target->DoAsyncScreenshot(); }
