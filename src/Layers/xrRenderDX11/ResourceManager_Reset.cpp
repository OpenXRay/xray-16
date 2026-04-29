#include "stdafx.h"
#pragma hdrstop

#include "xrCore/buffer_vector.h"

#include "ResourceManager.h"
#ifdef _EDITOR
#include "Include/xrAPI/xrAPI.h"
#endif

namespace xray::render::RENDER_NAMESPACE
{
void CResourceManager::reset_begin()
{
    ZoneScoped;
    for (SState* sstate : v_states)
        _RELEASE(sstate->state);

    RImplementation.old_QuadIB = RImplementation.QuadIB;
    RImplementation.QuadIB.Release();

    RImplementation.Index.reset_begin();
    RImplementation.Vertex.reset_begin();
}

void CResourceManager::reset_end()
{
    ZoneScoped;
    RImplementation.Vertex.reset_end();
    RImplementation.Index.reset_end();
    Evict();
    RImplementation.CreateQuadIB();

    {
        for (u32 _it = 0; _it < v_geoms.size(); _it++)
        {
            SGeometry* geom = v_geoms[_it];
            if (geom->vb == RImplementation.Vertex.old_pVB)
                geom->vb = RImplementation.Vertex.Buffer();

            if (geom->ib == RImplementation.Index.old_pIB)
            {
                geom->ib = RImplementation.Index.Buffer();
            }
            else if (geom->ib == RImplementation.old_QuadIB)
            {
                geom->ib = RImplementation.QuadIB;
            }
        }
    }

    for (SState* sstate : v_states)
    {
        sstate->state_code.record(sstate->state);
    }
}

template <class C>
void mdump(C c)
{
    if (0 == c.size())
        return;
    for (auto I = c.begin(); I != c.end(); ++I)
        Msg("*        : %3d: %s", I->second->ref_count.load() , I->second->cName.c_str());
}

CResourceManager::~CResourceManager()
{
    DestroyNecessaryTextures();
#ifndef MASTER_GOLD
    Dump(false);
#endif
}

void CResourceManager::Dump(bool bBrief)
{
    Msg("* RM_Dump: textures  : %d", m_textures.size());
    if (!bBrief)
        mdump(m_textures);
    Msg("* RM_Dump: rtargets  : %d", m_rtargets.size());
    if (!bBrief)
        mdump(m_rtargets);
    Msg("* RM_Dump: vs        : %d", m_vs.size());
    if (!bBrief)
        mdump(m_vs);
    Msg("* RM_Dump: ps        : %d", m_ps.size());
    if (!bBrief)
        mdump(m_ps);
    Msg("* RM_Dump: gs        : %d", m_gs.size());
    if (!bBrief)
        mdump(m_gs);
#ifdef USE_DX11
    Msg("* RM_Dump: cs        : %d", m_cs.size());
    if (!bBrief)
        mdump(m_cs);
    Msg("* RM_Dump: hs        : %d", m_hs.size());
    if (!bBrief)
        mdump(m_hs);
    Msg("* RM_Dump: ds        : %d", m_ds.size());
    if (!bBrief)
        mdump(m_ds);
#endif
    Msg("* RM_Dump: dcl       : %d", v_declarations.size());
    Msg("* RM_Dump: states    : %d", v_states.size());
    Msg("* RM_Dump: tex_list  : %d", lst_textures.size());
    Msg("* RM_Dump: matrices  : %d", lst_matrices.size());
    Msg("* RM_Dump: lst_constants: %d", lst_constants.size());
    Msg("* RM_Dump: v_passes  : %d", v_passes.size());
    Msg("* RM_Dump: v_elements: %d", v_elements.size());
    Msg("* RM_Dump: v_shaders : %d", v_shaders.size());
}
}
