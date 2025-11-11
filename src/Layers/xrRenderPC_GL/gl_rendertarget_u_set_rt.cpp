#include "stdafx.h"

namespace xray::render::RENDER_NAMESPACE
{

void CRenderTarget::u_setrtzb(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _zb)
{
    u_setrtzb<true,true,true,true>(cmd_list, _1, _2, _3, _zb);
}

void CRenderTarget::u_setrtzb(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _zb)
{
    u_setrtzb<true,true,false,true>(cmd_list, _1, _2, nullptr, _zb);
}

void CRenderTarget::u_setrtzb(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _zb)
{
    u_setrtzb<true,false,false,true>(cmd_list, _1, nullptr, nullptr, _zb);
}

void CRenderTarget::u_setrt_(CBackend& cmd_list, const ref_rt& _1)
{
    u_setrtzb<true,false,false,false>(cmd_list, _1, nullptr, nullptr, nullptr);
}

void CRenderTarget::u_set_zb(CBackend& cmd_list, const ref_rt& _zb)
{
    u_setrtzb<false,false,false,true>(cmd_list, nullptr, nullptr, nullptr, _zb);
}

template<bool has1, bool has2,bool has3,bool hasZB>
void CRenderTarget::u_setrtzb(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _zb)
{
    VERIFY(cmd_list.context_id < R__NUM_CONTEXTS);
    GLuint _target = 0;
    GLenum buffers[3] = {GL_NONE, GL_NONE, GL_NONE};

    cmd_list.set_FB(HW.pFB);

    if constexpr (has1)
    {
        VERIFY(_1);
        dwWidth[cmd_list.context_id]  = _1->dwWidth;
        dwHeight[cmd_list.context_id] = _1->dwHeight;
        _target = _1->target;
    }
    else if constexpr (has2)
    {
        VERIFY(_2);
        dwWidth[cmd_list.context_id]  = _2->dwWidth;
        dwHeight[cmd_list.context_id] = _2->dwHeight;
        _target = _2->target;
    }
    else if constexpr (has3)
    {
        VERIFY(_3);
        dwWidth[cmd_list.context_id]  = _3->dwWidth;
        dwHeight[cmd_list.context_id] = _3->dwHeight;
        _target = _3->target;
    }
    else if constexpr (hasZB)
    {
        dwWidth[cmd_list.context_id]  = _zb->dwWidth;
        dwHeight[cmd_list.context_id] = _zb->dwHeight;
        _target = _zb->target;
    }

    VERIFY(dwWidth[cmd_list.context_id]  != 0);
    VERIFY(dwHeight[cmd_list.context_id] != 0);

    if constexpr (has1)
    {
        VERIFY(_1);

        buffers[0] = GL_COLOR_ATTACHMENT0;
        cmd_list.set_RT(_1, 0);
    }
    else
        cmd_list.unset_RT(0);

    if constexpr (has2)
    {
        VERIFY(_2);
        VERIFY(_2->dwWidth  == dwWidth[cmd_list.context_id]);
        VERIFY(_2->dwHeight == dwHeight[cmd_list.context_id]);
        VERIFY(_2->target == _target);

        buffers[1] = GL_COLOR_ATTACHMENT1;
        cmd_list.set_RT(_2, 1);
    }
    else
        cmd_list.unset_RT(1);


    if constexpr (has3)
    {
        VERIFY(_3);
        VERIFY(_3->dwWidth  == dwWidth[cmd_list.context_id]);
        VERIFY(_3->dwHeight == dwHeight[cmd_list.context_id]);
        VERIFY(_3->target == _target);

        buffers[2] = GL_COLOR_ATTACHMENT2;
        cmd_list.set_RT(_3, 2);
    }
    else
        cmd_list.unset_RT( 2);

    if constexpr (hasZB)
    {
        VERIFY(_zb);
        VERIFY(_zb->dwWidth  == dwWidth[cmd_list.context_id]);
        cmd_list.set_ZB(_zb);
    }
    else
        cmd_list.unset_ZB();


    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}
} // namespace xray::render::RENDER_NAMESPACE
