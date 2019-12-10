#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#ifndef _EDITOR
#include "xrEngine/Render.h"
#else
#include "Include/xrAPI/xrAPI.h"
#endif

void CResourceManager::reset_begin()
{
    // destroy everything, renderer may use
    GEnv.Render->reset_begin();

    // destroy state-blocks
    for (u32 _it = 0; _it < v_states.size(); _it++)
#ifdef USE_OGL
        v_states[_it]->state.Release();
#else
        _RELEASE(v_states[_it]->state);
#endif // USE_OGL

    // destroy RTs
    for (auto rt_it = m_rtargets.begin(); rt_it != m_rtargets.end(); ++rt_it)
        rt_it->second->reset_begin();
    //  DX10 cut    for (map_RTCIt rtc_it=m_rtargets_c.begin(); rtc_it!=m_rtargets_c.end(); rtc_it++)
    //  DX10 cut        rtc_it->second->reset_begin();

    // destroy DStreams
    RCache.old_QuadIB = RCache.QuadIB;
    RCache.QuadIB.Release();

    RCache.Index.reset_begin();
    RCache.Vertex.reset_begin();
}

bool cmp_rt(const CRT* A, const CRT* B) { return A->_order < B->_order; }
//  DX10 cut bool   cmp_rtc (const CRTC* A,const CRTC* B)   { return A->_order < B->_order; }

void CResourceManager::reset_end()
{
    // create RDStreams
    RCache.Vertex.reset_end();
    RCache.Index.reset_end();
    Evict();
    RCache.CreateQuadIB();

    // remark geom's which point to dynamic VB/IB
    {
        for (u32 _it = 0; _it < v_geoms.size(); _it++)
        {
            SGeometry* _G = v_geoms[_it];
            if (_G->vb == RCache.Vertex.old_pVB)
                _G->vb = RCache.Vertex.Buffer();

            // Here we may recover the buffer using one of
            // RCache's index buffers.
            // Do not remove else.
            if (_G->ib == RCache.Index.old_pIB)
            {
                _G->ib = RCache.Index.Buffer();
            }
            else if (_G->ib == RCache.old_QuadIB)
            {
                _G->ib = RCache.QuadIB;
            }
        }
    }

    // create RTs in the same order as them was first created
    {
// RT
#pragma todo("container is created in stack!")
        xr_vector<CRT*> rt;
        for (auto rt_it = m_rtargets.begin(); rt_it != m_rtargets.end(); ++rt_it)
            rt.push_back(rt_it->second);
        std::sort(rt.begin(), rt.end(), cmp_rt);
        for (u32 _it = 0; _it < rt.size(); _it++)
            rt[_it]->reset_end();
    }
    {
// RTc
#pragma todo("container is created in stack!")
    //DX10 cut        xr_vector<CRTC*>    rt;
    //DX10 cut        for (map_RTCIt rt_it=m_rtargets_c.begin(); rt_it!=m_rtargets_c.end(); rt_it++)
    //rt.push_back(rt_it->second);
    //DX10 cut        std::sort(rt.begin(),rt.end(),cmp_rtc);
    //DX10 cut        for (u32 _it=0; _it<rt.size(); _it++)   rt[_it]->reset_end  ();
    }

    // create state-blocks
    {
        for (u32 _it = 0; _it < v_states.size(); _it++)
#if defined(USE_OGL)
        v_states[_it]->state_code.record(v_states[_it]->state);
#elif defined(USE_DX10) || defined(USE_DX11)
            v_states[_it]->state = ID3DState::Create(v_states[_it]->state_code);
#else // USE_DX10
            v_states[_it]->state = v_states[_it]->state_code.record();
#endif // USE_DX10
    }

    // create everything, renderer may use
    GEnv.Render->reset_end();
    Dump(true);
}

template <class C>
void mdump(C c)
{
    if (0 == c.size())
        return;
    for (auto I = c.begin(); I != c.end(); ++I)
        Msg("*        : %3d: %s", I->second->dwReference, I->second->cName.c_str());
}

CResourceManager::~CResourceManager()
{
    DestroyNecessaryTextures();
    Dump(false);
}

void CResourceManager::Dump(bool bBrief)
{
    Msg("* RM_Dump: textures  : %d", m_textures.size());
    if (!bBrief)
        mdump(m_textures);
    Msg("* RM_Dump: rtargets  : %d", m_rtargets.size());
    if (!bBrief)
        mdump(m_rtargets);
    //  DX10 cut    Msg("* RM_Dump: rtargetsc : %d", m_rtargets_c.size()); if(!bBrief) mdump(m_rtargets_c);
    Msg("* RM_Dump: vs        : %d", m_vs.size());
    if (!bBrief)
        mdump(m_vs);
    Msg("* RM_Dump: ps        : %d", m_ps.size());
    if (!bBrief)
        mdump(m_ps);
    Msg("* RM_Dump: dcl       : %d", v_declarations.size());
    Msg("* RM_Dump: states    : %d", v_states.size());
    Msg("* RM_Dump: tex_list  : %d", lst_textures.size());
    Msg("* RM_Dump: matrices  : %d", lst_matrices.size());
    Msg("* RM_Dump: lst_constants: %d", lst_constants.size());
    Msg("* RM_Dump: v_passes  : %d", v_passes.size());
    Msg("* RM_Dump: v_elements: %d", v_elements.size());
    Msg("* RM_Dump: v_shaders : %d", v_shaders.size());
}
