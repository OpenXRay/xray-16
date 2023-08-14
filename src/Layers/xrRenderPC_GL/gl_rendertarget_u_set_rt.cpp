#include "stdafx.h"

void CRenderTarget::u_setrt(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _zb)
{
    dwWidth[cmd_list.context_id] = 0;
    dwHeight[cmd_list.context_id] = 0;

    GLenum buffers[3] = {GL_NONE, GL_NONE, GL_NONE};

    cmd_list.set_FB(HW.pFB);

    if (_1)
    {
        dwWidth[cmd_list.context_id]  = _1->dwWidth;
        dwHeight[cmd_list.context_id] = _1->dwHeight;

        buffers[0] = GL_COLOR_ATTACHMENT0;
        cmd_list.set_RT(_1->pRT, 0);
    }
    else
    {
        cmd_list.set_RT(GL_NONE, 0);
    }

    if (_2)
    {
        if (dwWidth[cmd_list.context_id] && dwHeight[cmd_list.context_id])
        {
            VERIFY(_2->dwWidth  == dwWidth[cmd_list.context_id]);
            VERIFY(_2->dwHeight == dwHeight[cmd_list.context_id]);
        }
        else
        {
            dwWidth[cmd_list.context_id]  = _2->dwWidth;
            dwHeight[cmd_list.context_id] = _2->dwHeight;
        }

        buffers[1] = GL_COLOR_ATTACHMENT1;
        cmd_list.set_RT(_2->pRT, 1);
    }
    else
    {
        cmd_list.set_RT(GL_NONE, 1);
    }

    if (_3)
    {
        if (dwWidth[cmd_list.context_id] && dwHeight[cmd_list.context_id])
        {
            VERIFY(_2->dwWidth  == dwWidth[cmd_list.context_id]);
            VERIFY(_2->dwHeight == dwHeight[cmd_list.context_id]);
        }
        else
        {
            dwWidth[cmd_list.context_id]  = _3->dwWidth;
            dwHeight[cmd_list.context_id] = _3->dwHeight;
        }

        buffers[2] = GL_COLOR_ATTACHMENT2;
        cmd_list.set_RT(_3->pRT, 2);
    }
    else
    {
        cmd_list.set_RT(GL_NONE, 2);
    }

    if (_zb)
    {
        if (dwWidth[cmd_list.context_id] && dwHeight[cmd_list.context_id])
        {
            VERIFY(_zb->dwWidth  == dwWidth[cmd_list.context_id]);
            VERIFY(_zb->dwHeight == dwHeight[cmd_list.context_id]);
        }
        else
        {
            dwWidth[cmd_list.context_id]  = _zb->dwWidth;
            dwHeight[cmd_list.context_id] = _zb->dwHeight;
        }

        cmd_list.set_ZB(_zb->pZRT);
    }
    else
    {
        cmd_list.set_ZB(GL_NONE);
    }

    VERIFY(dwWidth  != 0);
    VERIFY(dwHeight != 0);

    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}

void CRenderTarget::u_setrt(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _zb)
{
    dwWidth[cmd_list.context_id]  = 0;
    dwHeight[cmd_list.context_id] = 0;

    GLenum buffers[3] = {GL_NONE, GL_NONE, GL_NONE};

    cmd_list.set_FB(HW.pFB);

    if (_1)
    {
        dwWidth[cmd_list.context_id]  = _1->dwWidth;
        dwHeight[cmd_list.context_id] = _1->dwHeight;

        buffers[0] = GL_COLOR_ATTACHMENT0;
        cmd_list.set_RT(_1->pRT, 0);
    }
    else
    {
        cmd_list.set_RT(GL_NONE, 0);
    }

    if (_2)
    {
        if (dwWidth[cmd_list.context_id] && dwHeight[cmd_list.context_id])
        {
            VERIFY(_2->dwWidth  == dwWidth[cmd_list.context_id]);
            VERIFY(_2->dwHeight == dwHeight[cmd_list.context_id]);
        }
        else
        {
            dwWidth[cmd_list.context_id]  = _2->dwWidth;
            dwHeight[cmd_list.context_id] = _2->dwHeight;
        }

        buffers[1] = GL_COLOR_ATTACHMENT1;
        cmd_list.set_RT(_2->pRT, 1);
    }
    else
    {
        cmd_list.set_RT(GL_NONE, 1);
    }

    if (_zb)
    {
        if (dwWidth[cmd_list.context_id] && dwHeight[cmd_list.context_id])
        {
            VERIFY(_zb->dwWidth  == dwWidth[cmd_list.context_id]);
            VERIFY(_zb->dwHeight == dwHeight[cmd_list.context_id]);
        }
        else
        {
            dwWidth[cmd_list.context_id]  = _zb->dwWidth;
            dwHeight[cmd_list.context_id] = _zb->dwHeight;
        }

        cmd_list.set_ZB(_zb->pZRT);
    }
    else
    {
        cmd_list.set_ZB(GL_NONE);
    }

    VERIFY(dwWidth[cmd_list.context_id]  != 0);
    VERIFY(dwHeight[cmd_list.context_id] != 0);

    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(2, buffers));
}

void CRenderTarget::u_setrt(CBackend& cmd_list, u32 W, u32 H, GLuint _1, GLuint _2, GLuint _3, GLuint zb)
{
    VERIFY(W != 0);
    VERIFY(H != 0);

    dwWidth[cmd_list.context_id]  = W;
    dwHeight[cmd_list.context_id] = H;

    const GLenum buffers[3] = {
        (GLenum)(_1 ? GL_COLOR_ATTACHMENT0 : GL_NONE),
        (GLenum)(_2 ? GL_COLOR_ATTACHMENT1 : GL_NONE),
        (GLenum)(_3 ? GL_COLOR_ATTACHMENT2 : GL_NONE)
    };

    cmd_list.set_FB(HW.pFB);

    cmd_list.set_RT(_1, 0);
    cmd_list.set_RT(_2, 1);
    cmd_list.set_RT(_3, 2);
    cmd_list.set_ZB(zb);

    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}
