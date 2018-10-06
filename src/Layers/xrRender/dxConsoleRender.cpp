#include "stdafx.h"
#include "dxConsoleRender.h"

dxConsoleRender::dxConsoleRender()
{
#if defined(USE_DX10) || defined(USE_DX11)
    m_Shader.create("hud" DELIMITER "crosshair");
    m_Geom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
#endif
}

void dxConsoleRender::Copy(IConsoleRender& _in) { *this = *(dxConsoleRender*)&_in; }
void dxConsoleRender::OnRender(bool bGame)
{
    VERIFY(HW.pDevice);

    D3DRECT R = {0, 0, static_cast<LONG>(Device.dwWidth), static_cast<LONG>(Device.dwHeight)};
    if (bGame)
        R.y2 /= 2;

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    u32 vOffset = 0;
    //	TODO: DX10: Implement console background clearing for DX10
    FVF::TL* verts = (FVF::TL*)RCache.Vertex.Lock(4, m_Geom->vb_stride, vOffset);
    verts->set(R.x1, R.y2, color_xrgb(32, 32, 32), 0, 0);
    verts++;
    verts->set(R.x1, R.y1, color_xrgb(32, 32, 32), 0, 0);
    verts++;
    verts->set(R.x2, R.y2, color_xrgb(32, 32, 32), 0, 0);
    verts++;
    verts->set(R.x2, R.y1, color_xrgb(32, 32, 32), 0, 0);
    verts++;
    RCache.Vertex.Unlock(4, m_Geom->vb_stride);

    RCache.set_Element(m_Shader->E[0]);
    RCache.set_Geometry(m_Geom);

    RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4, 0, 2);
#else //	USE_DX10
    CHK_DX(HW.pDevice->Clear(1, &R, D3DCLEAR_TARGET, color_xrgb(32, 32, 32), 1, 0));
#endif //	USE_DX10
}
