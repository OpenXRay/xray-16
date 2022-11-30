#include "stdafx.h"

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _zb)
{
    dwWidth  = 0;
    dwHeight = 0;

    GLenum buffers[3] = {GL_NONE, GL_NONE, GL_NONE};

    if (_1)
    {
        dwWidth  = _1->dwWidth;
        dwHeight = _1->dwHeight;

        buffers[0] = GL_COLOR_ATTACHMENT0;
        RCache.set_RT(_1->pRT, 0);
    }
    else
    {
        RCache.set_RT(GL_NONE, 0);
    }

    if (_2)
    {
        if (dwWidth && dwHeight)
        {
            VERIFY(_2->dwWidth  == dwWidth);
            VERIFY(_2->dwHeight == dwHeight);
        }
        else
        {
            dwWidth  = _2->dwWidth;
            dwHeight = _2->dwHeight;
        }

        buffers[1] = GL_COLOR_ATTACHMENT1;
        RCache.set_RT(_2->pRT, 1);
    }
    else
    {
        RCache.set_RT(GL_NONE, 1);
    }

    if (_3)
    {
        if (dwWidth && dwHeight)
        {
            VERIFY(_2->dwWidth  == dwWidth);
            VERIFY(_2->dwHeight == dwHeight);
        }
        else
        {
            dwWidth  = _3->dwWidth;
            dwHeight = _3->dwHeight;
        }

        buffers[2] = GL_COLOR_ATTACHMENT2;
        RCache.set_RT(_3->pRT, 2);
    }
    else
    {
        RCache.set_RT(GL_NONE, 2);
    }

    if (_zb)
    {
        if (dwWidth && dwHeight)
        {
            VERIFY(_zb->dwWidth  == dwWidth);
            VERIFY(_zb->dwHeight == dwHeight);
        }
        else
        {
            dwWidth  = _zb->dwWidth;
            dwHeight = _zb->dwHeight;
        }

        RCache.set_ZB(_zb->pZRT);
    }
    else
    {
        RCache.set_ZB(GL_NONE);
    }

    VERIFY(dwWidth  != 0);
    VERIFY(dwHeight != 0);

    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _zb)
{
    dwWidth  = 0;
    dwHeight = 0;

    GLenum buffers[3] = {GL_NONE, GL_NONE, GL_NONE};

    if (_1)
    {
        dwWidth  = _1->dwWidth;
        dwHeight = _1->dwHeight;

        buffers[0] = GL_COLOR_ATTACHMENT0;
        RCache.set_RT(_1->pRT, 0);
    }
    else
    {
        RCache.set_RT(GL_NONE, 0);
    }

    if (_2)
    {
        if (dwWidth && dwHeight)
        {
            VERIFY(_2->dwWidth  == dwWidth);
            VERIFY(_2->dwHeight == dwHeight);
        }
        else
        {
            dwWidth  = _2->dwWidth;
            dwHeight = _2->dwHeight;
        }

        buffers[1] = GL_COLOR_ATTACHMENT1;
        RCache.set_RT(_2->pRT, 1);
    }
    else
    {
        RCache.set_RT(GL_NONE, 1);
    }

    if (_zb)
    {
        if (dwWidth && dwHeight)
        {
            VERIFY(_zb->dwWidth  == dwWidth);
            VERIFY(_zb->dwHeight == dwHeight);
        }
        else
        {
            dwWidth  = _zb->dwWidth;
            dwHeight = _zb->dwHeight;
        }

        RCache.set_ZB(_zb->pZRT);
    }
    else
    {
        RCache.set_ZB(GL_NONE);
    }

    VERIFY(dwWidth  != 0);
    VERIFY(dwHeight != 0);

    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(2, buffers));
}

void CRenderTarget::u_setrt(u32 W, u32 H, GLuint _1, GLuint _2, GLuint _3, GLuint zb)
{
    VERIFY(W != 0);
    VERIFY(H != 0);

    dwWidth  = W;
    dwHeight = H;

    const GLenum buffers[3] = {
        (GLenum)(_1 ? GL_COLOR_ATTACHMENT0 : GL_NONE),
        (GLenum)(_2 ? GL_COLOR_ATTACHMENT1 : GL_NONE),
        (GLenum)(_3 ? GL_COLOR_ATTACHMENT2 : GL_NONE)
    };

    RCache.set_RT(_1, 0);
    RCache.set_RT(_2, 1);
    RCache.set_RT(_3, 2);
    RCache.set_ZB(zb);

    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}
