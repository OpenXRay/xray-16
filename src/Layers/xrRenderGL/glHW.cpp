// glHW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/HW.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/XR_IOConsole.h"
#include "Include/xrAPI/xrAPI.h"
#include "xrCore/xr_token.h"

extern ENGINE_API xr_vector<xr_token> AvailableVideoModes;

void fill_vid_mode_list(CHW* _hw);
void free_vid_mode_list();

CHW HW;

void CALLBACK OnDebugCallback(GLenum /*source*/, GLenum /*type*/, GLuint id, GLenum severity,
                              GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
{
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
        Log(message, id);
}

CHW::CHW() :
    pDevice(this),
    pContext(this),
    m_pSwapChain(this),
    pBaseRT(0),
    pBaseZB(0),
    pPP(0),
    pFB(0),
    m_hWnd(nullptr),
    m_hDC(nullptr),
    m_hRC(nullptr),
    m_move_window(true) {}

CHW::~CHW() {}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateDevice(HWND hWnd, bool move_window)
{
    m_hWnd = hWnd;
    m_move_window = move_window;

    R_ASSERT(m_hWnd);

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER, // Flags
        PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
        32, // Color depth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24, // Number of bits for the depthbuffer
        8, // Number of bits for the stencilbuffer
        0, // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    // Get the device context
    m_hDC = GetDC(m_hWnd);
    if (m_hDC == nullptr)
    {
        Msg("Could not get device context.");
        return;
    }

    // Choose the closest pixel format
    int iPixelFormat = ChoosePixelFormat(m_hDC, &pfd);
    if (iPixelFormat == 0)
    {
        Msg("No pixel format found.");
        return;
    }

    // Apply the pixel format to the device context
    if (!SetPixelFormat(m_hDC, iPixelFormat, &pfd))
    {
        Msg("Could not set pixel format.");
        return;
    }

    // Create the context
    m_hRC = wglCreateContext(m_hDC);
    if (m_hRC == nullptr)
    {
        Msg("Could not create drawing context.");
        return;
    }

    // Make the new context the current context for this thread
    // NOTE: This assumes the thread calling Create() is the only
    // thread that will use the context.
    if (!wglMakeCurrent(m_hDC, m_hRC))
    {
        Msg("Could not make context current.");
        return;
    }

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

#ifndef _EDITOR
    updateWindowProps(m_hWnd);
    fill_vid_mode_list(this);
#endif
}

void CHW::DestroyDevice()
{
    if (m_hRC)
    {
        if (!wglMakeCurrent(nullptr, nullptr))
            Msg("Could not release drawing context.");

        if (!wglDeleteContext(m_hRC))
            Msg("Could not delete context.");

        m_hRC = nullptr;
    }

    if (m_hDC)
    {
        if (!ReleaseDC(m_hWnd, m_hDC))
            Msg("Could not release device context.");

        m_hDC = nullptr;
    }

    free_vid_mode_list();
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset(HWND hwnd)
{
    BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);

    CHK_GL(glDeleteProgramPipelines(1, &pPP));
    CHK_GL(glDeleteFramebuffers(1, &pFB));
    CHK_GL(glDeleteFramebuffers(1, &pCFB));

    CHK_GL(glDeleteTextures(1, &pBaseRT));
    CHK_GL(glDeleteTextures(1, &pBaseZB));

    UpdateViews();

    updateWindowProps(hwnd);
    ShowWindow(hwnd, SW_SHOWNORMAL);
}

void CHW::updateWindowProps(HWND m_hWnd)
{
    const bool bWindowed = !psDeviceFlags.is(rsFullscreen);

    u32 dwWindowStyle = 0;
    // Set window properties depending on what mode were in.
    if (bWindowed)
    {
        if (m_move_window)
        {
            const bool drawBorders = strstr(Core.Params, "-draw_borders");
            dwWindowStyle = WS_VISIBLE;
            if (drawBorders)
                dwWindowStyle |= WS_BORDER | WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX;
            SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle);
            // When moving from fullscreen to windowed mode, it is important to
            // adjust the window size after recreating the device rather than
            // beforehand to ensure that you get the window size you want.  For
            // example, when switching from 640x480 fullscreen to windowed with
            // a 1000x600 window on a 1024x768 desktop, it is impossible to set
            // the window size to 1000x600 until after the display mode has
            // changed to 1024x768, because windows cannot be larger than the
            // desktop.

            RECT m_rcWindowBounds;
            float fYOffset = 0.f;
            bool centerScreen = false;
            if (strstr(Core.Params, "-center_screen"))
                centerScreen = true;

            if (centerScreen)
            {
                RECT DesktopRect;
                GetClientRect(GetDesktopWindow(), &DesktopRect);

                SetRect(&m_rcWindowBounds,
                    (DesktopRect.right - psCurrentVidMode[0]) / 2,
                    (DesktopRect.bottom - psCurrentVidMode[1]) / 2,
                    (DesktopRect.right + psCurrentVidMode[0]) / 2,
                    (DesktopRect.bottom + psCurrentVidMode[1]) / 2);
            }
            else
            {
                if (drawBorders)
                    fYOffset = GetSystemMetrics(SM_CYCAPTION); // size of the window title bar
                SetRect(&m_rcWindowBounds, 0, 0, psCurrentVidMode[0], psCurrentVidMode[1]);
            }

            AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

            SetWindowPos(m_hWnd, HWND_NOTOPMOST,
                         m_rcWindowBounds.left, m_rcWindowBounds.top + fYOffset,
                         m_rcWindowBounds.right - m_rcWindowBounds.left,
                         m_rcWindowBounds.bottom - m_rcWindowBounds.top,
                         SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_DRAWFRAME);
        }
    }
    else
    {
        SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = WS_POPUP | WS_VISIBLE);
    }

    SetForegroundWindow(m_hWnd);
}

struct uniqueRenderingMode
{
    uniqueRenderingMode(pcstr v) : value(v) {}
    pcstr value;
    bool operator()(const xr_token other) const { return !xr_stricmp(value, other.name); }
};

void free_vid_mode_list()
{
    for (auto& mode : AvailableVideoModes)
        xr_free(mode.name);
    AvailableVideoModes.clear();
}

void fill_vid_mode_list(CHW* /*_hw*/)
{
    if (!AvailableVideoModes.empty())
        return;

    DWORD iModeNum = 0;
    DEVMODE dmi;
    ZeroMemory(&dmi, sizeof dmi);
    dmi.dmSize = sizeof dmi;

    int i = 0;
    auto& AVM = AvailableVideoModes;
    while (EnumDisplaySettings(nullptr, iModeNum++, &dmi) != 0)
    {
        string32 str;

        xr_sprintf(str, sizeof(str), "%dx%d", dmi.dmPelsWidth, dmi.dmPelsHeight);

        if (AVM.cend() != find_if(AVM.cbegin(), AVM.cend(), uniqueRenderingMode(str)))
            continue;

        AVM.emplace_back(xr_token(xr_strdup(str), i));
        ++i;
    }
    AVM.emplace_back(xr_token(nullptr, -1));

    Msg("Available video modes[%d]:", AVM.size());
    for (const auto& mode : AVM)
        Msg("[%s]", mode.name);
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
    return SwapBuffers(m_hDC) ? S_OK : E_FAIL;
}
