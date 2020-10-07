#include "stdafx.h"
#include "dxThunderboltRender.h"

#include "xrEngine/thunderbolt.h"
#include "dxThunderboltDescRender.h"
#include "dxLensFlareRender.h"

dxThunderboltRender::dxThunderboltRender()
{
    // geom
    hGeom_model.create(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, RCache.Vertex.Buffer(), RCache.Index.Buffer());
    hGeom_gradient.create(FVF::F_LIT, RCache.Vertex.Buffer(), RCache.QuadIB);
}

dxThunderboltRender::~dxThunderboltRender()
{
    hGeom_model.destroy();
    hGeom_gradient.destroy();
}

void dxThunderboltRender::Copy(IThunderboltRender& _in) { *this = *(dxThunderboltRender*)&_in; }
void dxThunderboltRender::Render(CEffect_Thunderbolt& owner)
{
    VERIFY(owner.current);

    // lightning model
    float dv = owner.lightning_phase * 0.5f;
    dv = (owner.lightning_phase > 0.5f) ? Random.randI(2) * 0.5f : dv;

    RCache.set_CullMode(CULL_NONE);
    u32 v_offset, i_offset;

    dxThunderboltDescRender* pThRen = (dxThunderboltDescRender*)&*owner.current->m_pRender;

    u32 vCount_Lock = pThRen->l_model->number_vertices;
    u32 iCount_Lock = pThRen->l_model->number_indices;
    IRender_DetailModel::fvfVertexOut* v_ptr =
        (IRender_DetailModel::fvfVertexOut*)RCache.Vertex.Lock(vCount_Lock, hGeom_model->vb_stride, v_offset);
    u16* i_ptr = RCache.Index.Lock(iCount_Lock, i_offset);
    // XForm verts
    pThRen->l_model->transfer(owner.current_xform, v_ptr, 0xffffffff, i_ptr, 0, 0.f, dv);
    // Flush if needed
    RCache.Vertex.Unlock(vCount_Lock, hGeom_model->vb_stride);
    RCache.Index.Unlock(iCount_Lock);
    RCache.set_xform_world(Fidentity);
    RCache.set_Shader(pThRen->l_model->shader);
    RCache.set_Geometry(hGeom_model);
    RCache.Render(D3DPT_TRIANGLELIST, v_offset, 0, vCount_Lock, i_offset, iCount_Lock / 3);
    RCache.set_CullMode(CULL_CCW);

    // gradient
    Fvector vecSx, vecSy;
    u32 VS_Offset;
    FVF::LIT* pv = (FVF::LIT*)RCache.Vertex.Lock(8, hGeom_gradient.stride(), VS_Offset);
    // top
    {
        u32 c_val = iFloor(owner.current->m_GradientTop->fOpacity * owner.lightning_phase * 255.f);
        u32 c = color_rgba(c_val, c_val, c_val, c_val);
        vecSx.mul(Device.vCameraRight, owner.current->m_GradientTop->fRadius.x * owner.lightning_size);
        vecSy.mul(Device.vCameraTop, -owner.current->m_GradientTop->fRadius.y * owner.lightning_size);
        pv->set(owner.current_xform.c.x + vecSx.x - vecSy.x, owner.current_xform.c.y + vecSx.y - vecSy.y,
            owner.current_xform.c.z + vecSx.z - vecSy.z, c, 0, 0);
        pv++;
        pv->set(owner.current_xform.c.x + vecSx.x + vecSy.x, owner.current_xform.c.y + vecSx.y + vecSy.y,
            owner.current_xform.c.z + vecSx.z + vecSy.z, c, 0, 1);
        pv++;
        pv->set(owner.current_xform.c.x - vecSx.x - vecSy.x, owner.current_xform.c.y - vecSx.y - vecSy.y,
            owner.current_xform.c.z - vecSx.z - vecSy.z, c, 1, 0);
        pv++;
        pv->set(owner.current_xform.c.x - vecSx.x + vecSy.x, owner.current_xform.c.y - vecSx.y + vecSy.y,
            owner.current_xform.c.z - vecSx.z + vecSy.z, c, 1, 1);
        pv++;
    }
    // center
    {
        u32 c_val = iFloor(owner.current->m_GradientTop->fOpacity * owner.lightning_phase * 255.f);
        u32 c = color_rgba(c_val, c_val, c_val, c_val);
        vecSx.mul(Device.vCameraRight, owner.current->m_GradientCenter->fRadius.x * owner.lightning_size);
        vecSy.mul(Device.vCameraTop, -owner.current->m_GradientCenter->fRadius.y * owner.lightning_size);
        pv->set(owner.lightning_center.x + vecSx.x - vecSy.x, owner.lightning_center.y + vecSx.y - vecSy.y,
            owner.lightning_center.z + vecSx.z - vecSy.z, c, 0, 0);
        pv++;
        pv->set(owner.lightning_center.x + vecSx.x + vecSy.x, owner.lightning_center.y + vecSx.y + vecSy.y,
            owner.lightning_center.z + vecSx.z + vecSy.z, c, 0, 1);
        pv++;
        pv->set(owner.lightning_center.x - vecSx.x - vecSy.x, owner.lightning_center.y - vecSx.y - vecSy.y,
            owner.lightning_center.z - vecSx.z - vecSy.z, c, 1, 0);
        pv++;
        pv->set(owner.lightning_center.x - vecSx.x + vecSy.x, owner.lightning_center.y - vecSx.y + vecSy.y,
            owner.lightning_center.z - vecSx.z + vecSy.z, c, 1, 1);
        pv++;
    }
    RCache.Vertex.Unlock(8, hGeom_gradient.stride());
    RCache.set_xform_world(Fidentity);
    RCache.set_Geometry(hGeom_gradient);
    RCache.set_Shader(((dxFlareRender*)&*owner.current->m_GradientTop->m_pFlare)->hShader);
#if !defined(USE_DX9) && !defined(USE_OGL)
    //	Hack. Since lightning gradient uses sun shader override z write settings manually
    RCache.set_Z(TRUE);
    RCache.set_ZFunc(D3DCMP_LESSEQUAL);
#endif // !USE_DX9 && !USE_OGL
    RCache.Render(D3DPT_TRIANGLELIST, VS_Offset, 0, 4, 0, 2);

    RCache.set_Shader(((dxFlareRender*)&*owner.current->m_GradientCenter->m_pFlare)->hShader);
#if !defined(USE_DX9) && !defined(USE_OGL)
    //	Hack. Since lightning gradient uses sun shader override z write settings manually
    RCache.set_Z(TRUE);
    RCache.set_ZFunc(D3DCMP_LESSEQUAL);
#endif // !USE_DX9 && !USE_OGL
    RCache.Render(D3DPT_TRIANGLELIST, VS_Offset + 4, 0, 4, 0, 2);
}
