#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"
#include "glTextureUtils.h"

namespace xray::render::RENDER_NAMESPACE
{
CRT::~CRT()
{
    destroy();

    // release external reference
    RImplementation.Resources->_DeleteRT(this);
}

void CRT::set_slice_read(int slice) {}
void CRT::set_slice_write(u32 context_id, int slice) {}

void CRT::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount /*= 1*/, u32 slices_num /*=1*/, Flags32 /*flags = {}*/)
{
    if (pRT) return;

    R_ASSERT(Name && Name[0] && w && h);
    _order = CPU::QPC(); //Device.GetTimerGlobal()->GetElapsed_clk();

    //HRESULT		_hr;

    dwWidth = w;
    dwHeight = h;
    fmt = f;
    sampleCount = SampleCount;

    // Get caps
    GLint max_width, max_height;
#ifdef XR_PLATFORM_APPLE
    // https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_offscreen/opengl_offscreen.html
    CHK_GL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_width));
    max_height = max_width;
 #else
    CHK_GL(glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &max_width));
    CHK_GL(glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &max_height));
 #endif

    // Check width-and-height of render target surface
    // XXX: While seemingly silly, assert w/h are positive?
    if (w > u32(max_width)) return;
    if (h > u32(max_height)) return;

    RImplementation.Resources->Evict();

    target = (SampleCount > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glGenTextures(1, &pRT);
    CHK_GL(glBindTexture(target, pRT));
    glObjectLabel(GL_TEXTURE, pRT, -1, Name);
    if (SampleCount > 1)
        CHK_GL(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, glTextureUtils::ConvertTextureFormat(fmt), w,
        h, GL_FALSE));
    else
        CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, glTextureUtils::ConvertTextureFormat(fmt), w, h));

    pTexture = RImplementation.Resources->_CreateTexture(Name);
    pTexture->surface_set(target, pRT);

    // OpenGL doesn't differentiate between color and depth targets
    pZRT = pRT;
}

void CRT::destroy()
{
    if (pTexture._get())
    {
        pTexture->surface_set(target, 0);
        pTexture = nullptr;
    }
    CHK_GL(glDeleteTextures(1, &pRT));
}

void CRT::reset_begin()
{
    destroy();
}

void CRT::reset_end()
{
    create(cName.c_str(), dwWidth, dwHeight, fmt, sampleCount, { dwFlags });
}

void CRT::resolve_into(ref_rt& destination) const
{
    PIX_EVENT(resolve_into);

    auto actualFB = RCache.get_FB();
    RCache.set_FB(HW.pResolveFB);
    //CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, HW.pResolveFB));
    CHK_GL(glDisable(GL_DEPTH_TEST));
    CHK_GL(glDisable(GL_STENCIL_TEST));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, pRT, 0));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_NONE, GL_NONE, 0));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_NONE, GL_NONE, 0));
    CHK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0));
    VERIFY(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    //Resolve Screen
    RCache.set_FB(actualFB);
    //CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, actualFB));
    CHK_GL(glDisable(GL_DEPTH_TEST));
    CHK_GL(glDisable(GL_STENCIL_TEST));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, destination->target, destination->pRT, 0));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_NONE, GL_NONE, 0));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_NONE, GL_NONE, 0));
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_NONE, GL_NONE, 0));
    VERIFY(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    //CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, HW.pResolveFB));
    RCache.set_FB(HW.pResolveFB); // hm???

    CHK_GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, HW.pResolveFB));
    CHK_GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, actualFB));

    CHK_GL(glBlitFramebuffer(0, 0, dwWidth, dwHeight, 0, 0, destination->dwWidth, destination->dwHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST));

    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, 0));

    RCache.set_FB(actualFB);
}

void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount /*= 1*/, u32 slices_num /*=1*/, Flags32 flags /*= {}*/)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount, 1, flags));
}
} // namespace xray::render::RENDER_NAMESPACE
