// glHW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/HW.h"
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
    : pDevice(this), pContext(this), m_pSwapChain(this), pBaseRT(0), pBaseZB(0), pPP(0), pFB(0), m_hWnd(nullptr),
      m_hDC(nullptr), m_hRC(nullptr)
{
}

CHW::~CHW() {}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateDevice(SDL_Window* hWnd)
{
    m_hWnd = hWnd;

    R_ASSERT(m_hWnd);

    // Choose the closest pixel format
    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(m_hWnd, &mode);
    mode.format = SDL_PIXELFORMAT_RGBA8888;
    // Apply the pixel format to the device context
    SDL_SetWindowDisplayMode(m_hWnd, &mode);

    // Create the context
    m_hRC = SDL_GL_CreateContext(m_hWnd);
    if (m_hRC == nullptr)
    {
        Msg("Could not create drawing context: %s", SDL_GetError());
        return;
    }

    // Make the new context the current context for this thread
    // NOTE: This assumes the thread calling Create() is the only
    // thread that will use the context.
    if (SDL_GL_MakeCurrent(m_hWnd, m_hRC) != 0)
    {
        Msg("Could not make context current. %s", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Initialize OpenGL Extension Wrangler
    if (glewInit() != GLEW_OK)
    {
        Msg("Could not initialize glew.");
        return;
    }

#ifdef DEBUG
    CHK_GL(glEnable(GL_DEBUG_OUTPUT));
    CHK_GL(glDebugMessageCallback((GLDEBUGPROC)OnDebugCallback, nullptr));
#endif // DEBUG

    // Clip control ensures compatibility with D3D device coordinates.
    // TODO: OGL: Fix these differences in the blenders/shaders.
    CHK_GL(glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE));

    //	Create render target and depth-stencil views here
    UpdateViews();
}

void CHW::DestroyDevice()
{
    if (m_hRC)
    {
        if (SDL_GL_MakeCurrent(nullptr, nullptr) != 0)
            Msg("Could not release drawing context: %s", SDL_GetError());

        SDL_GL_DeleteContext(m_hRC);

        m_hRC = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset()
{
    CHK_GL(glDeleteProgramPipelines(1, &pPP));
    CHK_GL(glDeleteFramebuffers(1, &pFB));
    CHK_GL(glDeleteFramebuffers(1, &pCFB));

    CHK_GL(glDeleteTextures(1, &pBaseRT));
    CHK_GL(glDeleteTextures(1, &pBaseZB));

    UpdateViews();
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

void CHW::ClearRenderTargetView(GLuint pRenderTargetView, const FLOAT ColorRGBA[4])
{
    if (pRenderTargetView == 0)
        return;

    // Attach the render target
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pRenderTargetView, 0));

    // Clear the color buffer without affecting the global state
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(ColorRGBA[0], ColorRGBA[1], ColorRGBA[2], ColorRGBA[3]);
    CHK_GL(glClear(GL_COLOR_BUFFER_BIT));
    glPopAttrib();
}

void CHW::ClearDepthStencilView(GLuint pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    if (pDepthStencilView == 0)
        return;

    // Attach the depth buffer
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, pDepthStencilView, 0));

    u32 mask = 0;
    if (ClearFlags & D3D_CLEAR_DEPTH)
        mask |= (u32)GL_DEPTH_BUFFER_BIT;
    if (ClearFlags & D3D_CLEAR_STENCIL)
        mask |= (u32)GL_STENCIL_BUFFER_BIT;

    glPushAttrib(mask);
    if (ClearFlags & D3D_CLEAR_DEPTH)
    {
        glDepthMask(GL_TRUE);
        glClearDepthf(Depth);
    }
    if (ClearFlags & D3DCLEAR_STENCIL)
    {
        glStencilMask(~0);
        glClearStencil(Stencil);
    }
    CHK_GL(glClear(mask));
    glPopAttrib();
}

HRESULT CHW::Present(UINT /*SyncInterval*/, UINT /*Flags*/)
{
    RImplementation.Target->phase_flip();
    SDL_GL_SwapWindow(m_hWnd);
    return S_OK;
}
