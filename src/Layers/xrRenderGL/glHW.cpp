// glHW.cpp: implementation of the OpenGL specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "glHW.h"
#include "xrEngine/XR_IOConsole.h"

namespace xray::render::RENDER_NAMESPACE
{
CHW HW;
#if defined(XR_PLATFORM_ANDROID)
static bool androidDebugCallbackInstalled = false;
#endif

void CALLBACK OnDebugCallback(GLenum /*source*/, GLenum /*type*/, GLuint id, GLenum severity, GLsizei /*length*/,
    const GLchar* message, const void* /*userParam*/)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;
#if defined(XR_PLATFORM_ANDROID)
    static u32 reportedMessages = 0;
    if (reportedMessages++ >= 32)
        return;
#endif
    Log(message, id);
}

static_assert(std::is_same_v<decltype(&OnDebugCallback), GLDEBUGPROC>);

void UpdateVSync()
{
    if (psDeviceFlags.test(rsVSync))
    {
        // Try adaptive vsync first
        if (SDL_GL_SetSwapInterval(-1) == -1)
            SDL_GL_SetSwapInterval(1);
    }
    else
    {
        SDL_GL_SetSwapInterval(0);
    }
}

CHW::CHW()
{
    if (!ThisInstanceIsGlobal())
        return;

    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this);
}

CHW::~CHW()
{
    if (!ThisInstanceIsGlobal())
        return;

    Device.seqAppActivate.Remove(this);
    Device.seqAppDeactivate.Remove(this);
}

void CHW::OnAppActivate()
{
#if defined(XR_PLATFORM_ANDROID)
    // SDL recreates the EGL window surface when the Activity returns to the
    // foreground. Rebind the existing context to that new surface before the
    // first frame; otherwise SwapWindow may keep presenting to the abandoned
    // surface and the user sees a permanent black screen.
    if (!m_window || !m_context)
        return;

    if (SDL_GL_MakeCurrent(m_window, m_context) != 0)
    {
        Msg("! Android GLES: cannot restore EGL surface: %s", SDL_GetError());
        m_surfaceNeedsReset = true;
        return;
    }

    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(m_window, &drawableWidth, &drawableHeight);
    m_surfaceNeedsReset = !glIsFramebuffer(pFB) || drawableWidth <= 0 || drawableHeight <= 0 ||
        drawableWidth != static_cast<int>(psDeviceMode.Width) ||
        drawableHeight != static_cast<int>(psDeviceMode.Height);
    Msg("* Android GLES: foreground surface [%dx%d], reset=[%d]", drawableWidth, drawableHeight,
        m_surfaceNeedsReset);
    UpdateVSync();
    return;
#else
    if (m_window)
    {
        SDL_RestoreWindow(m_window);
    }
#endif
}

void CHW::OnAppDeactivate()
{
#if defined(XR_PLATFORM_ANDROID)
    // SDL_MinimizeWindow() deliberately launches the Android HOME intent.
    // A transient focus loss therefore used to eject the user to the
    // launcher/home screen while the native engine kept running. Pausing the
    // scheduler and audio is handled by the normal app-deactivate sequence;
    // do not turn that lifecycle event into task navigation.
    Msg("* Android GLES: window surface moved to background");
    return;
#else
    if (m_window)
    {
        if (psDeviceMode.WindowStyle == rsFullscreen || psDeviceMode.WindowStyle == rsFullscreenBorderless)
            SDL_MinimizeWindow(m_window);
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateDevice(SDL_Window* hWnd)
{
    ZoneScoped;

    m_window = hWnd;

    R_ASSERT(m_window);

    // Choose the closest pixel format
    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(m_window, &mode);
    mode.format = SDL_PIXELFORMAT_RGBA8888;
    // Apply the pixel format to the device context
    SDL_SetWindowDisplayMode(m_window, &mode);

    Caps.fTarget = D3DFMT_A8R8G8B8;
    Caps.fDepth = D3DFMT_D24S8;

    // Create the context
    m_context = SDL_GL_CreateContext(m_window);
    if (m_context == nullptr)
    {
        Log("! OpenGL: could not create drawing context:", SDL_GetError());
        return;
    }

    if (MakeContextCurrent(IRender::PrimaryContext) != 0)
    {
        Log("! OpenGL: could not make context current:", SDL_GetError());
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
        return;
    }

    int version;
    {
#if defined(XR_PLATFORM_ANDROID)
        // Android creates an OpenGL ES context.  Loading it through the
        // desktop GL loader marks ES 3.x as desktop GL 3.x, leaving ES 3.0
        // entry points such as glTexStorage2D unresolved.  The first render
        // target then calls a null function pointer during device creation.
        ZoneScopedN("gladLoadGLES2");
        version = gladLoadGLES2(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
#else
        ZoneScopedN("gladLoadGL");
        version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
#endif
    }
    if (version == 0)
    {
        Log("! OpenGL: could not initialize GLAD.");
        if (const auto err = SDL_GetError())
            Log("SDL Error:", err);
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
        return;
    }

#if defined(XR_PLATFORM_ANDROID)
    // Debug callbacks belong to the GL context, not to the CHW instance.
    androidDebugCallbackInstalled = false;
#endif

    if (ThisInstanceIsGlobal())
    {
        UpdateVSync();

        bool debugOutput = false;
#ifdef DEBUG
        debugOutput = true;
#endif
#if defined(XR_PLATFORM_ANDROID)
        debugOutput = debugOutput || (Core.Params && strstr(Core.Params, "-android-gl-debug"));
#endif
        if (debugOutput && glDebugMessageCallback)
        {
            CHK_GL(glEnable(GL_DEBUG_OUTPUT));
            CHK_GL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
            CHK_GL(glDebugMessageCallback((GLDEBUGPROC)OnDebugCallback, nullptr));
#if defined(XR_PLATFORM_ANDROID)
            androidDebugCallbackInstalled = true;
#endif
            Msg("* OpenGL debug callback enabled");
        }
    }

    int iMaxVTFUnits, iMaxCTIUnits;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &iMaxVTFUnits);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &iMaxCTIUnits);

    AdapterName = reinterpret_cast<pcstr>(glGetString(GL_RENDERER));
    OpenGLVersionString = reinterpret_cast<pcstr>(glGetString(GL_VERSION));
    ShadingVersion = reinterpret_cast<pcstr>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    Msg("* GPU vendor: [%s] device: [%s]", glGetString(GL_VENDOR), AdapterName);
    Msg("* GPU OpenGL version: %s", OpenGLVersionString);
    Msg("* GPU OpenGL shading language version: %s", ShadingVersion);
    Msg("* GPU OpenGL VTF units: [%d] CTI units: [%d]", iMaxVTFUnits, iMaxCTIUnits);
#if defined(XR_PLATFORM_ANDROID)
    Msg("* GLES runtime: ES3.1=[%d] ES3.2=[%d] io_blocks=[%d/%d] clip_cull_distance=[%d] shader5=[%d/%d] implicit_conversions=[%d] blend_func_extended=[%d]",
        GLAD_GL_ES_VERSION_3_1, GLAD_GL_ES_VERSION_3_2, GLAD_GL_EXT_shader_io_blocks, GLAD_GL_OES_shader_io_blocks,
        GLAD_GL_EXT_clip_cull_distance, GLAD_GL_EXT_gpu_shader5, GLAD_GL_OES_gpu_shader5,
        GLAD_GL_EXT_shader_implicit_conversions, GLAD_GL_EXT_blend_func_extended);

    GLint maxDrawBuffers = 0;
    GLint maxColorAttachments = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    if (!GLAD_GL_ES_VERSION_3_1 || maxDrawBuffers < 4 || maxColorAttachments < 4)
    {
        string512 reason;
        xr_sprintf(reason,
            "OpenXRay requires OpenGL ES 3.1 and at least 4 draw buffers/color attachments.\n\n"
            "Device reports: %s; draw buffers=%d; color attachments=%d.",
            OpenGLVersionString ? OpenGLVersionString : "unknown OpenGL ES version",
            maxDrawBuffers, maxColorAttachments);
        Msg("! Android GLES capability check failed: %s", reason);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            "OpenXRay: unsupported graphics device", reason, m_window);
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
        return;
    }
    Msg("* Android GLES capability check passed: ES 3.1+, draw buffers=[%d], color attachments=[%d]",
        maxDrawBuffers, maxColorAttachments);
#endif

    ComputeShadersSupported = false; // XXX: Implement compute shaders support

    if (glGenFramebuffers && glBindFramebuffer)
        UpdateViews();
}

void CHW::DestroyDevice()
{
    CHK_GL(glDeleteFramebuffers(1, &pFB));
    pFB = 0;

    const auto context = SDL_GL_GetCurrentContext();
    if (context == m_context)
        SDL_GL_MakeCurrent(nullptr, nullptr);

    SDL_GL_DeleteContext(m_context);
    m_context = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset()
{
    ZoneScoped;

    CHK_GL(glDeleteFramebuffers(1, &pFB));
    pFB = 0;
    UpdateViews();

#if defined(XR_PLATFORM_ANDROID)
    m_surfaceNeedsReset = false;
#endif

    UpdateVSync();
}

void CHW::SetPrimaryAttributes(u32& windowFlags)
{
    windowFlags |= SDL_WINDOW_OPENGL;

#if defined(XR_PLATFORM_ANDROID)
    // Android exposes OpenGL ES through SDL.  The desktop core profile and
    // the 4.1 request used by the Linux renderer are not valid on Android.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#if defined(XR_PLATFORM_ANDROID)
    // Android's SDL GLES backend owns the context profile.  Do not inspect
    // Core.Params here: the renderer smoke path intentionally reaches this
    // function before the full engine command-line state is guaranteed to be
    // valid.  The previous strstr(Core.Params, ...) call turned that state
    // into a SIGSEGV on ARMv7 (fault address 0x0 inside libc strstr/strchr).
    // The shipped game shaders redeclare gl_PerVertex, which requires the
    // GLES 3.1 / GLSL ES 3.10 interface-block model.  Requesting GLES 3.0
    // here makes the Adreno compiler reject the first shader and the engine
    // later reports the misleading generic video-card message.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    const pcstr commandLine = Core.Params ? Core.Params : "";
    if (!strstr(commandLine, "-no_gl_context"))
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    }
#endif
}

IRender::RenderContext CHW::GetCurrentContext() const
{
    const auto context = SDL_GL_GetCurrentContext();
    if (context == m_context)
        return IRender::PrimaryContext;
    return IRender::NoContext;
}

int CHW::MakeContextCurrent(IRender::RenderContext context) const
{
    switch (context)
    {
    case IRender::NoContext:
        return SDL_GL_MakeCurrent(nullptr, nullptr);

    case IRender::PrimaryContext:
        return SDL_GL_MakeCurrent(m_window, m_context);

    default:
        NODEFAULT;
    }
    return -1;
}

void CHW::UpdateViews()
{
#if defined(XR_PLATFORM_ANDROID)
    // SDL's Android GLES window owns framebuffer 0.  OpenXRay renders the
    // deferred scene into its own FBO and presents it to framebuffer 0 in
    // Present(), so keep a real engine FBO here just like the desktop path.
    glGenFramebuffers(1, &pFB);
    CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, pFB));
    BackBufferCount = 1;
#else
    // Create the default framebuffer
    glGenFramebuffers(1, &pFB);
    CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, pFB));

    BackBufferCount = 1;
#endif
}

void CHW::BeginScene() { }
void CHW::EndScene() { }

void CHW::Present()
{
#if defined(XR_PLATFORM_ANDROID)
    const u64 presentStart = SDL_GetPerformanceCounter();

    // Resolve the engine's final color target into the EGL window surface.
    // Keep this explicit: framebuffer 0 cannot host the deferred renderer's
    // MRT attachments, while swapping without this copy presents untouched
    // black buffers.
    static u32 reportedPendingErrors = 0;
    for (GLenum pending = glGetError(); pending != GL_NO_ERROR; pending = glGetError())
    {
        if (reportedPendingErrors < 16)
            Msg("! OpenGL ES: pending error 0x%x before Present", pending);
        ++reportedPendingErrors;
    }
    if (reportedPendingErrors && !androidDebugCallbackInstalled && glDebugMessageCallback)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback((GLDEBUGPROC)OnDebugCallback, nullptr);
        androidDebugCallbackInstalled = true;
        Msg("* OpenGL ES: diagnostic callback enabled after renderer error");
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, pFB);
    const GLenum sourceStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    GLint sourceTexture = 0;
    glGetFramebufferAttachmentParameteriv(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &sourceTexture);

    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(m_window, &drawableWidth, &drawableHeight);

    static bool presentStateLogged = false;
    static u32 incompletePresentCount = 0;
    const bool displayable = sourceStatus == GL_FRAMEBUFFER_COMPLETE && sourceTexture != 0 &&
        drawableWidth > 0 && drawableHeight > 0;
    if (!displayable)
        ++incompletePresentCount;
    const bool reportIncomplete = !displayable &&
        (incompletePresentCount <= 8 || incompletePresentCount % 300 == 0);
    if (!presentStateLogged || reportIncomplete)
    {
        Msg("* Android present: FBO=[%u] status=[0x%x] color0=[%d] source=[%ux%u] drawable=[%dx%d]",
            pFB, sourceStatus, sourceTexture, Device.dwWidth, Device.dwHeight, drawableWidth, drawableHeight);
        presentStateLogged = true;
    }

    if (displayable)
    {
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        const GLenum filter = Device.dwWidth == static_cast<u32>(drawableWidth) &&
            Device.dwHeight == static_cast<u32>(drawableHeight) ? GL_NEAREST : GL_LINEAR;
        glBlitFramebuffer(
            0, 0, Device.dwWidth, Device.dwHeight,
            0, 0, drawableWidth, drawableHeight,
            GL_COLOR_BUFFER_BIT, filter);

        // The render target is owned by the engine and may be read again on
        // the next frame (including after SDL recreates the window surface).
        // Do not discard its contents from the presentation path; resource
        // owners may invalidate an attachment after its last actual use.
    }
    else if (reportIncomplete)
    {
        Msg("! Android present skipped: the final engine framebuffer is not displayable");
    }

    static u32 reportedPresentErrors = 0;
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
    {
        if (reportedPresentErrors < 16)
            Msg("! OpenGL ES: Present failed with 0x%x", error);
        ++reportedPresentErrors;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, pFB);
    const u64 swapStart = SDL_GetPerformanceCounter();
    SDL_GL_SwapWindow(m_window);
    const u64 presentEnd = SDL_GetPerformanceCounter();

    static u32 lastFrameReport = 0;
    static u32 previousPresentTick = 0;
    static u64 intervalFrameTime = 0;
    static u32 intervalFrameCount = 0;
    static u32 intervalMaxFrame = 0;
    static u64 intervalPresentTime = 0;
    static u64 intervalSwapTime = 0;
    static u32 intervalPresentCount = 0;
    intervalPresentTime += swapStart - presentStart;
    intervalSwapTime += presentEnd - swapStart;
    ++intervalPresentCount;
    const u32 now = SDL_GetTicks();
    if (previousPresentTick)
    {
        const u32 elapsed = now - previousPresentTick;
        intervalFrameTime += elapsed;
        ++intervalFrameCount;
        intervalMaxFrame = std::max(intervalMaxFrame, elapsed);
    }
    if (now - lastFrameReport >= 5000)
    {
        const auto& stats = Device.GetStats();
        const float averageFrame = intervalFrameCount ?
            static_cast<float>(intervalFrameTime) / intervalFrameCount : 0.f;
        const float counterToMilliseconds = SDL_GetPerformanceFrequency() ?
            1000.f / SDL_GetPerformanceFrequency() : 0.f;
        const float averagePresent = intervalPresentCount ?
            counterToMilliseconds * intervalPresentTime / intervalPresentCount : 0.f;
        const float averageSwap = intervalPresentCount ?
            counterToMilliseconds * intervalSwapTime / intervalPresentCount : 0.f;
        const u32 drawCalls = GEnv.Render ? GEnv.Render->GetCacheStatCalls() : 0;
        const u32 polygons = GEnv.Render ? GEnv.Render->GetCacheStatPolys() : 0;
        Msg("[frame-trace] fps=%.1f frame-avg=%.1fms frame-max=%ums samples=%u "
            "update=%.1fms render=%.1fms wait=%.1fms present=%.1fms swap=%.1fms "
            "calls=%u polys=%u stats=%d "
            "internal=%ux%u drawable=%dx%d",
            stats.fFPS, averageFrame, intervalMaxFrame, intervalFrameCount,
            stats.fFrameMoveReal, stats.fRenderReal, stats.fParallelWaitReal,
            averagePresent, averageSwap,
            drawCalls, polygons,
            g_bEnableStatGather ? 1 : 0,
            Device.dwWidth, Device.dwHeight, drawableWidth, drawableHeight);
        lastFrameReport = now;
        intervalFrameTime = 0;
        intervalFrameCount = 0;
        intervalMaxFrame = 0;
        intervalPresentTime = 0;
        intervalSwapTime = 0;
        intervalPresentCount = 0;
    }
    previousPresentTick = now;
#else
#if 0 // kept for historical reasons
    RImplementation.Target->phase_flip();
#else
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(
        0, 0, Device.dwWidth, Device.dwHeight,
        0, 0, Device.dwWidth, Device.dwHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
#endif
    SDL_GL_SwapWindow(m_window);
#endif
    CurrentBackBuffer = (CurrentBackBuffer + 1) % BackBufferCount;
}

DeviceState CHW::GetDeviceState() const
{
#if defined(XR_PLATFORM_ANDROID)
    if (m_surfaceNeedsReset)
        return DeviceState::NeedReset;
#endif
    // TODO: OGL: Implement desktop context-loss detection.
    return DeviceState::Normal;
}

std::pair<u32, u32> CHW::GetSurfaceSize()
{
#if defined(XR_PLATFORM_ANDROID)
    return
    {
        Device.dwWidth,
        Device.dwHeight
    };
#else
    return
    {
        psDeviceMode.Width,
        psDeviceMode.Height
    };
#endif
}

bool CHW::ThisInstanceIsGlobal() const
{
    return this == &HW;
}

void CHW::BeginPixEvent(pcstr name) const
{
    if (glPushDebugGroup)
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
}

void CHW::EndPixEvent() const
{
    if (glPushDebugGroup)
        glPopDebugGroup();
}
} // namespace xray::render::RENDER_NAMESPACE
