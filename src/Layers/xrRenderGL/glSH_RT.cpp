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
#if defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_WEB) // GLES 3.0 has no GL_MAX_FRAMEBUFFER_WIDTH either
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
    const texture_upload_unit uploadUnit(target);
    glGenTextures(1, &pRT);
    CHK_GL(glBindTexture(target, pRT));
    if (SampleCount > 1)
        CHK_GL(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, glTextureUtils::ConvertTextureFormat(fmt), w,
        h, GL_FALSE));
    else
        CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, glTextureUtils::ConvertTextureFormat(fmt), w, h));

    pTexture = RImplementation.Resources->_CreateTexture(Name);
    pTexture->surface_set(target, pRT);
    pTexture->size_set(w, h);

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

void CRT::resolve_into(CRT& destination) const
{
    glReadBuffer(GL_COLOR_ATTACHMENT0);
#ifndef XR_PLATFORM_WEB // glDrawBuffers below covers it; glDrawBuffer does not exist in GLES
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
#endif

    constexpr GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    RCache.set_RT(pRT, 0);
    RCache.set_RT(destination.pRT, 1);

#ifdef XR_PLATFORM_WEB
    VERIFY(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
#else
    [[maybe_unused]] GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    VERIFY(status == GL_FRAMEBUFFER_COMPLETE);
#endif
    CHK_GL(glDrawBuffers(std::size(buffers), buffers));

    CHK_GL(glBlitFramebuffer(0, 0, dwWidth, dwHeight, 0, 0, destination.dwWidth, destination.dwHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST));
}

#ifdef XR_PLATFORM_WEB
void CRT::copy_into(CRT& destination) const
{
    RCache.set_RT(pRT, 0);
    RCache.set_RT(0, 1);
    RCache.set_RT(0, 2);
    CHK_GL(glReadBuffer(GL_COLOR_ATTACHMENT0));

    const texture_upload_unit uploadUnit(GL_TEXTURE_2D);
    CHK_GL(glBindTexture(GL_TEXTURE_2D, destination.pRT));
    CHK_GL(glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, dwWidth, dwHeight));
}
#endif

void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount /*= 1*/, u32 slices_num /*=1*/, Flags32 flags /*= {}*/)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount, 1, flags));
}
} // namespace xray::render::RENDER_NAMESPACE
