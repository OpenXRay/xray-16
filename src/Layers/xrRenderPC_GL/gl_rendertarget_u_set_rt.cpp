#include "stdafx.h"

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, GLuint zb)
{
    if (_1)
    {
        dwWidth = _1->dwWidth;
        dwHeight = _1->dwHeight;
    }
    else
        VERIFY3(false, __FUNCTION__, "TODO: implement 'else' path");
    GLenum buffers[3] = {GL_NONE};
    if (_1)
    {
        buffers[0] = GL_COLOR_ATTACHMENT0;
        RCache.set_RT(_1->pRT, 0);
    }
    if (_2)
    {
        buffers[1] = GL_COLOR_ATTACHMENT1;
        RCache.set_RT(_2->pRT, 1);
    }
    if (_3)
    {
        buffers[2] = GL_COLOR_ATTACHMENT2;
        RCache.set_RT(_3->pRT, 2);
    }
    RCache.set_ZB(zb);
    //	RImplementation.rmNormal				();
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, GLuint zb)
{
    if (_1)
    {
        dwWidth = _1->dwWidth;
        dwHeight = _1->dwHeight;
    }
    else
        VERIFY3(false, __FUNCTION__, "TODO: implement 'else' path");

    GLenum buffers[2] = {GL_NONE};
    if (_1)
    {
        buffers[0] = GL_COLOR_ATTACHMENT0;
        RCache.set_RT(_1->pRT, 0);
    }
    if (_2)
    {
        buffers[1] = GL_COLOR_ATTACHMENT1;
        RCache.set_RT(_2->pRT, 1);
    }
    RCache.set_ZB(zb);
    //	RImplementation.rmNormal				();
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(2, buffers));
}

void CRenderTarget::u_setrt(u32 W, u32 H, GLuint _1, GLuint _2, GLuint _3, GLuint zb)
{
    dwWidth = W;
    dwHeight = H;
    GLenum buffers[3] = {GL_NONE};
    if (_1)
    {
        buffers[0] = GL_COLOR_ATTACHMENT0;
        RCache.set_RT(_1, 0);
    }
    if (_2)
    {
        buffers[1] = GL_COLOR_ATTACHMENT1;
        RCache.set_RT(_2, 1);
    }
    if (_3)
    {
        buffers[2] = GL_COLOR_ATTACHMENT2;
        RCache.set_RT(_3, 2);
    }
    RCache.set_ZB(zb);
    //	RImplementation.rmNormal				();
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
    CHK_GL(glDrawBuffers(3, buffers));
}
