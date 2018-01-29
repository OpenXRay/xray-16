// glHW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/HW.h"
#include "xrEngine/XR_IOConsole.h"
#include "Include/xrAPI/xrAPI.h"
#include "xrCore/xr_token.h"

#ifndef _EDITOR
void fill_vid_mode_list(CHW* _hw);
void free_vid_mode_list();

void fill_render_mode_list();
void free_render_mode_list();
#else
void	fill_vid_mode_list			(CHW* _hw)	{}
void	free_vid_mode_list			()			{}
void	fill_render_mode_list		()			{}
void	free_render_mode_list		()			{}
#endif

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

#ifndef _EDITOR
    free_vid_mode_list();
#endif
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
}

void CHW::updateWindowProps(HWND m_hWnd)
{
    bool bWindowed = true;
#ifndef _EDITOR
    if (!GEnv.isDedicatedServer)
        bWindowed = !psDeviceFlags.is(rsFullscreen);
#endif

    u32 dwWindowStyle = 0;
    // Set window properties depending on what mode were in.
    if (bWindowed)
    {
        if (m_move_window)
        {
            dwWindowStyle = WS_BORDER | WS_VISIBLE;
            if (!strstr(Core.Params, "-no_dialog_header"))
                dwWindowStyle |= WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX;
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
            RECT DesktopRect;

            GetClientRect(GetDesktopWindow(), &DesktopRect);

            SetRect(&m_rcWindowBounds,
                    (DesktopRect.right - psCurrentVidMode[0]) / 2,
                    (DesktopRect.bottom - psCurrentVidMode[1]) / 2,
                    (DesktopRect.right + psCurrentVidMode[0]) / 2,
                    (DesktopRect.bottom + psCurrentVidMode[1]) / 2);

            AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

            SetWindowPos(m_hWnd, HWND_NOTOPMOST, m_rcWindowBounds.left, m_rcWindowBounds.top,
                         m_rcWindowBounds.right - m_rcWindowBounds.left,
                         m_rcWindowBounds.bottom - m_rcWindowBounds.top,
                         SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_DRAWFRAME);
        }
    }
    else
    {
        SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = WS_POPUP | WS_VISIBLE);
    }

    ShowCursor(FALSE);
    SetForegroundWindow(m_hWnd);
}


struct _uniq_mode
{
    _uniq_mode(LPCSTR v): _val(v) {}
    LPCSTR _val;
    bool operator()(LPCSTR _other) { return !xr_stricmp(_val, _other); }
};

#ifndef _EDITOR

void free_vid_mode_list()
{
    for (int i = 0; GEnv.vid_mode_token[i].name; i++)
    {
        xr_free(GEnv.vid_mode_token[i].name);
    }
    xr_free(GEnv.vid_mode_token);
    GEnv.vid_mode_token = nullptr;
}

void fill_vid_mode_list(CHW* /*_hw*/)
{
    if (GEnv.vid_mode_token != nullptr) return;
    xr_vector<LPCSTR> _tmp;

    DWORD iModeNum = 0;
    DEVMODE dmi;
    ZeroMemory(&dmi, sizeof dmi);
    dmi.dmSize = sizeof dmi;

    while (EnumDisplaySettings(nullptr, iModeNum++, &dmi) != 0)
    {
        string32 str;

        if (dmi.dmPelsWidth < 800)
            continue;

        sprintf_s(str, sizeof str, "%dx%d", dmi.dmPelsWidth, dmi.dmPelsHeight);

        if (_tmp.end() != find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
            continue;

        _tmp.push_back(nullptr);
        _tmp.back() = xr_strdup(str);
    }

    u32 _cnt = _tmp.size() + 1;

    GEnv.vid_mode_token = xr_alloc<xr_token>(_cnt);

    GEnv.vid_mode_token[_cnt - 1].id = -1;
    GEnv.vid_mode_token[_cnt - 1].name = nullptr;

#ifdef DEBUG
	Msg("Available video modes[%d]:", _tmp.size());
#endif // DEBUG
    for (u32 i = 0; i < _tmp.size(); ++i)
    {
        GEnv.vid_mode_token[i].id = i;
        GEnv.vid_mode_token[i].name = _tmp[i];
#ifdef DEBUG
		Msg("[%s]", _tmp[i]);
#endif // DEBUG
    }
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
#endif
