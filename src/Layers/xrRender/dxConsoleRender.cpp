#include "stdafx.h"
#include "dxConsoleRender.h"

dxConsoleRender::dxConsoleRender()
{
#ifndef USE_DX9
    m_Shader.create("hud" DELIMITER "crosshair");
    m_Geom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
#endif
}

void dxConsoleRender::Copy(IConsoleRender& _in) { *this = *(dxConsoleRender*)&_in; }

#ifndef USE_DX9
ICF void ClearWithShader(const D3DRECT& rect, const Fcolor& color, ref_geom& geom, ref_shader& shader)
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
#endif // !USE_DX9

void dxConsoleRender::OnRender(bool bGame)
{
    D3DRECT rect = { 0, 0, static_cast<LONG>(Device.dwWidth), static_cast<LONG>(Device.dwHeight) };
    if (bGame)
        rect.y2 /= 2;

#ifndef USE_DX9
    ClearWithShader(rect, color_xrgb(32, 32, 32), m_Geom, m_Shader);
#else //	USE_DX10
    CHK_DX(HW.pDevice->Clear(1, &rect, D3DCLEAR_TARGET, color_xrgb(32, 32, 32), 1, 0));
#endif //	USE_DX10
}
