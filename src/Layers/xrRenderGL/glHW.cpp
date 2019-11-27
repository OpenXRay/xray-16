// glHW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderGL/glHW.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/XR_IOConsole.h"
#include "Include/xrAPI/xrAPI.h"
#include "xrCore/xr_token.h"

CHW HW;

void CALLBACK OnDebugCallback(GLenum /*source*/, GLenum /*type*/, GLuint id, GLenum severity, GLsizei /*length*/,
    const GLchar* message, const void* /*userParam*/)
{
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
        Log(message, id);
}

CHW::CHW()
    : pDevice(this), pContext(this), m_pSwapChain(this), pBaseRT(0), pBaseZB(0), pPP(0), pFB(0)
{
}

CHW::~CHW() {}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateDevice(SDL_Window* hWnd)
{
    m_window = hWnd;

    R_ASSERT(m_window);

    // Choose the closest pixel format
    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(m_window, &mode);
    mode.format = SDL_PIXELFORMAT_RGBA8888;
    // Apply the pixel format to the device context
    SDL_SetWindowDisplayMode(m_window, &mode);

    // Create the context
    m_context = SDL_GL_CreateContext(m_window);
    if (m_context == nullptr)
    {
        Msg("Could not create drawing context: %s", SDL_GetError());
        return;
    }

    if (MakeContextCurrent(IRender::PrimaryContext) != 0)
    {
        Msg("Could not make context current. %s", SDL_GetError());
        return;
    }

    {
        const Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;

        m_helper_window = SDL_CreateWindow("OpenXRay OpenGL helper window", 0, 0, 1, 1, flags);
        R_ASSERT3(m_helper_window, "Cannot create helper window for OpenGL", SDL_GetError());

        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

        // Create helper context
        m_helper_context = SDL_GL_CreateContext(m_helper_window);
        R_ASSERT3(m_helper_context, "Cannot create OpenGL context", SDL_GetError());

        // just in case
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    }

    if (MakeContextCurrent(IRender::PrimaryContext) != 0)
    {
        Msg("Could not make context current after creating helper context."
            " %s", SDL_GetError());
        return;
    }

    // Initialize OpenGL Extension Wrangler
    if (glewInit() != GLEW_OK)
    {
        Msg("Could not initialize glew.");
        return;
    }

    Console->Execute("rs_v_sync apply");

#ifdef DEBUG
    CHK_GL(glEnable(GL_DEBUG_OUTPUT));
    CHK_GL(glDebugMessageCallback((GLDEBUGPROC)OnDebugCallback, nullptr));
#endif // DEBUG

    int iMaxVTFUnits, iMaxCTIUnits;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &iMaxVTFUnits);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &iMaxCTIUnits);

    AdapterName = reinterpret_cast<pcstr>(glGetString(GL_RENDERER));
    OpenGLVersion = reinterpret_cast<pcstr>(glGetString(GL_VERSION));
    ShadingVersion = reinterpret_cast<pcstr>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    Msg("* GPU vendor: [%s] device: [%s]", glGetString(GL_VENDOR), AdapterName);
    Msg("* GPU OpenGL version: %s", OpenGLVersion);
    Msg("* GPU OpenGL shading language version: %s", ShadingVersion);
    Msg("* GPU OpenGL VTF units: [%d] CTI units: [%d]", iMaxVTFUnits, iMaxCTIUnits);

    ShaderBinarySupported = GLEW_ARB_get_program_binary;

    //	Create render target and depth-stencil views here
    UpdateViews();
}

void CHW::DestroyDevice()
{
    SDL_GL_MakeCurrent(nullptr, nullptr);

    SDL_GL_DeleteContext(m_context);
    m_context = nullptr;

    SDL_GL_DeleteContext(m_helper_context);
    m_helper_context = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset()
{
    CHK_GL(glDeleteProgramPipelines(1, &pPP));
    CHK_GL(glDeleteFramebuffers(1, &pFB));

    CHK_GL(glDeleteTextures(1, &pBaseRT));
    CHK_GL(glDeleteTextures(1, &pBaseZB));

    UpdateViews();
}

void CHW::SetPrimaryAttributes()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    if (!strstr(Core.Params, "-no_gl_context"))
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    }
}

int CHW::MakeContextCurrent(IRender::RenderContext context) const
{
    switch (context)
    {
    case IRender::NoContext:
        return SDL_GL_MakeCurrent(nullptr, nullptr);

    case IRender::PrimaryContext:
        return SDL_GL_MakeCurrent(m_window, m_context);

    case IRender::HelperContext:
        return SDL_GL_MakeCurrent(m_helper_window, m_helper_context);

    default:
        NODEFAULT;
    }
    return -1;
}

void CHW::UpdateViews()
{
    // Create the program pipeline used for rendering with shaders
    glGenProgramPipelines(1, &pPP);
    CHK_GL(glBindProgramPipeline(pPP));

    // Create the default framebuffer
    glGenFramebuffers(1, &pFB);
    CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, pFB));

    // Create a color render target
    glGenTextures(1, &HW.pBaseRT);
    CHK_GL(glBindTexture(GL_TEXTURE_2D, HW.pBaseRT));
    CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, psCurrentVidMode[0], psCurrentVidMode[1]));

    // Create depth/stencil buffer
    glGenTextures(1, &HW.pBaseZB);
    CHK_GL(glBindTexture(GL_TEXTURE_2D, HW.pBaseZB));
    CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, psCurrentVidMode[0], psCurrentVidMode[1]));
}

void CHW::Present()
{
    RImplementation.Target->phase_flip();
    SDL_GL_SwapWindow(m_window);
}

DeviceState CHW::GetDeviceState()
{
    //  TODO: OGL: Implement GetDeviceState
    return DeviceState::Normal;
}

std::pair<u32, u32> CHW::GetSurfaceSize() const
{
    return
    {
        psCurrentVidMode[0],
        psCurrentVidMode[1]
    };
}

void CHW::ClearRenderTarget(GLuint view, Fcolor color) const
{
    if (view == 0)
        return;

    // Attach the render target
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, view, 0));

    // Clear the color buffer without affecting the global state
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(color.r, color.g, color.b, color.a);

    CHK_GL(glClear(GL_COLOR_BUFFER_BIT));
}

int CHW::ClearRenderTargetRect(GLuint view, Fcolor color, u32 numRects, Irect* rects) const
{
    if (view == 0)
        return 0;

    // Attach the render target
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, view, 0));

    CHK_GL(glEnable(GL_SCISSOR_TEST));

    for (u32 i = 0; i < numRects; ++i, ++rects)
    {
        // The window space is inverted compared to DX
        // so we have to invert our vertical coordinates
        const u32 bottom = Device.dwHeight - rects->bottom;

        // The origin of the scissor box is lower-left
        CHK_GL(glScissor(rects->left, bottom, rects->width(), rects->height()));

        // Clear the color buffer without affecting the global state
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(color.r, color.g, color.b, color.a);

        CHK_GL(glClear(GL_COLOR_BUFFER_BIT));
    }

    CHK_GL(glDisable(GL_SCISSOR_TEST));

    return numRects;
}

void CHW::ClearDepth(GLuint view, float depth) const
{
    if (view == 0)
        return;

    // Attach the depth buffer
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, view, 0));

    glDepthMask(GL_TRUE);
    glClearDepthf(depth);

    CHK_GL(glClear(GL_DEPTH_BUFFER_BIT));
}

int CHW::ClearDepthRect(GLuint view, float depth, u32 numRects, Irect* rects) const
{
    if (view == 0)
        return 0;

    // Attach the depth buffer
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, view, 0));

    CHK_GL(glEnable(GL_SCISSOR_TEST));

    for (u32 i = 0; i < numRects; ++i, ++rects)
    {
        // The window space is inverted compared to DX
        // so we have to invert our vertical coordinates
        const u32 bottom = Device.dwHeight - rects->bottom;

        // The origin of the scissor box is lower-left
        CHK_GL(glScissor(rects->left, bottom, rects->width(), rects->height()));

        glDepthMask(GL_TRUE);
        glClearDepthf(depth);

        CHK_GL(glClear(GL_DEPTH_BUFFER_BIT));
    }

    CHK_GL(glDisable(GL_SCISSOR_TEST));

    return numRects;
}

void CHW::ClearStencil(GLuint view, u8 stencil) const
{
    if (view == 0)
        return;

    // Attach the depth buffer
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, view, 0));

    glStencilMask(~0);
    glClearStencil(stencil);

    CHK_GL(glClear(GL_STENCIL_BUFFER_BIT));
}

void CHW::ClearDepthStencil(GLuint view, float depth, u8 stencil) const
{
    if (view == 0)
        return;

    // Attach the depth buffer
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, view, 0));

    glDepthMask(GL_TRUE);
    glClearDepthf(depth);

    glStencilMask(~0);
    glClearStencil(stencil);

    CHK_GL(glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

void CHW::ClearRTAndZB(GLuint rt, Fcolor color, GLuint zb, float depth, u8 stencil) const
{
    ClearRenderTarget(rt, color);
    ClearDepthStencil(zb, depth, stencil);
}
