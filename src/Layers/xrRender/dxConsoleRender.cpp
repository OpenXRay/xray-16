#include "stdafx.h"
#include "dxConsoleRender.h"

dxConsoleRender::dxConsoleRender()
{
#ifdef USE_DX11
    m_Shader.create("hud" DELIMITER "crosshair");
    m_Geom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
#endif
}

void dxConsoleRender::Copy(IConsoleRender& _in) { *this = *(dxConsoleRender*)&_in; }

#ifdef USE_DX11
ICF void ClearWithShader(const Irect& rect, const Fcolor& color, ref_geom& geom, ref_shader& shader)
{
    u32 vOffset = 0;
    FVF::TL* verts = (FVF::TL*)RCache.Vertex.Lock(4, geom->vb_stride, vOffset);
    verts->set(rect.x1, rect.y2, color.get(), 0, 0);
    verts++;
    verts->set(rect.x1, rect.y1, color.get(), 0, 0);
    verts++;
    verts->set(rect.x2, rect.y2, color.get(), 0, 0);
    verts++;
    verts->set(rect.x2, rect.y1, color.get(), 0, 0);
    verts++;
    RCache.Vertex.Unlock(4, geom->vb_stride);

    RCache.set_Element(shader->E[0]);
    RCache.set_Geometry(geom);

    RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4, 0, 2);
}
#endif // USE_DX11

void dxConsoleRender::OnRender(bool bGame)
{
    Irect rect = { 0, 0, (int)Device.dwWidth, (int)Device.dwHeight };
    if (bGame)
        rect.y2 /= 2;

    const bool result = RCache.ClearRTRect(RCache.get_RT(), color_xrgb(32, 32, 32), 1, &rect);
#ifdef USE_DX11
    if (!result)
        ClearWithShader(rect, color_xrgb(32, 32, 32), m_Geom, m_Shader);
#else
    UNUSED(result);
#endif
}
