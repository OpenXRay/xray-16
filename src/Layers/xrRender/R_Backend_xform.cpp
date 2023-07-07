#include "stdafx.h"
#pragma hdrstop

#include "R_Backend_xform.h"

void R_xforms::set_W(const Fmatrix& m)
{
    m_w.set(m);
    m_wv.mul_43(m_v, m_w);
    m_wvp.mul(m_p, m_wv);
    if (c_w)
        cmd_list.set_c(c_w, m_w);
    if (c_wv)
        cmd_list.set_c(c_wv, m_wv);
    if (c_wvp)
        cmd_list.set_c(c_wvp, m_wvp);
    m_bInvWValid = false;
    if (c_invw)
        apply_invw();
    cmd_list.set_xform(D3DTS_WORLD, m);
}
void R_xforms::set_V(const Fmatrix& m)
{
    m_v.set(m);
    m_wv.mul_43(m_v, m_w);
    m_vp.mul(m_p, m_v);
    m_wvp.mul(m_p, m_wv);
    if (c_v)
        cmd_list.set_c(c_v, m_v);
    if (c_vp)
        cmd_list.set_c(c_vp, m_vp);
    if (c_wv)
        cmd_list.set_c(c_wv, m_wv);
    if (c_wvp)
        cmd_list.set_c(c_wvp, m_wvp);
    cmd_list.set_xform(D3DTS_VIEW, m);
}
void R_xforms::set_P(const Fmatrix& m)
{
    m_p.set(m);
    m_vp.mul(m_p, m_v);
    m_wvp.mul(m_p, m_wv);
    if (c_p)
        cmd_list.set_c(c_p, m_p);
    if (c_vp)
        cmd_list.set_c(c_vp, m_vp);
    if (c_wvp)
        cmd_list.set_c(c_wvp, m_wvp);
    // always setup projection - D3D relies on it to work correctly :(
    cmd_list.set_xform(D3DTS_PROJECTION, m);
}

void R_xforms::apply_invw()
{
    VERIFY(c_invw);

    if (!m_bInvWValid)
    {
        m_invw.invert_b(m_w);
        m_bInvWValid = true;
    }

    cmd_list.set_c(c_invw, m_invw);
}

void R_xforms::unmap()
{
    c_w = nullptr;
    c_invw = nullptr;
    c_v = nullptr;
    c_p = nullptr;
    c_wv = nullptr;
    c_vp = nullptr;
    c_wvp = nullptr;
}
R_xforms::R_xforms(CBackend& cmd_list_in)
    : cmd_list(cmd_list_in)
{
    unmap();
    m_w.identity();
    m_invw.identity();
    m_v.identity();
    m_p.identity();
    m_wv.identity();
    m_vp.identity();
    m_wvp.identity();
    m_bInvWValid = true;
}
