#include "stdafx.h"
#include "r__sync_point.h"

#include "QueryHelper.h"

#if defined(XR_PLATFORM_ANDROID)
#include <chrono>
#include <thread>
#endif

namespace xray::render::RENDER_NAMESPACE
{
#ifdef USE_OGL
// Assert this just in case
static_assert(sizeof(void*) == sizeof(GLsync), "void* is used instead of GLsync, sizes should match");

void R_sync_point::Create() { q_sync_count = 0; }
void R_sync_point::Destroy()
{
    for (auto& fence : q_sync_point)
    {
        if (fence) glDeleteSync(static_cast<GLsync>(fence));
        fence = nullptr;
    }
    q_sync_count = 0;
}

bool R_sync_point::Wait(u32 /*wait_sleep*/, u64 timeout)
{
    ZoneScoped;
    // Wait only when reusing a slot, never on a fence just submitted by this frame.
    if (!q_sync_point[q_sync_count])
        return true;

#if defined(XR_PLATFORM_ANDROID)
    const auto started = std::chrono::steady_clock::now();
    bool reported_backlog = false;
    for (;;)
    {
        // Poll in short intervals so a slow GPU cannot accumulate an
        // unbounded number of submitted frames. The timeout is a diagnostic
        // threshold, not permission to discard a fence that is still in use.
        const auto status = glClientWaitSync((GLsync)q_sync_point[q_sync_count],
            GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);
        if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED)
            return true;
        if (status == GL_WAIT_FAILED)
        {
            Log("! R_sync_point::Wait raised GL_WAIT_FAILED");
            return false;
        }
        if (status != GL_TIMEOUT_EXPIRED)
            NODEFAULT;

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        if (!reported_backlog && elapsed >= static_cast<long long>(timeout))
        {
            Msg("[gpu-queue] frame fence exceeded %llu ms; throttling submissions until it completes",
                static_cast<unsigned long long>(timeout));
            reported_backlog = true;
        }
        std::this_thread::yield();
    }
#else
    const auto status = glClientWaitSync((GLsync)q_sync_point[q_sync_count],
        GL_SYNC_FLUSH_COMMANDS_BIT, timeout * 1000 * 1000);
    switch (status)
    {
    case GL_ALREADY_SIGNALED:
    case GL_CONDITION_SATISFIED:
        return true;
    case GL_TIMEOUT_EXPIRED:
        return false;
    case GL_WAIT_FAILED:
        Log("! R_sync_point::Wait raised GL_WAIT_FAILED");
        [[fallthrough]];
    default:
        NODEFAULT;
    }
    return false;
#endif
}

void R_sync_point::End()
{
    if (q_sync_point[q_sync_count])
        CHK_GL(glDeleteSync(static_cast<GLsync>(q_sync_point[q_sync_count])));
    CHK_GL(q_sync_point[q_sync_count] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
    const u32 bufferedFrames = GLAD_GL_ES_VERSION_3_0 ? 2u : std::max(1u, HW.Caps.iGPUNum);
    static_assert(CHWCaps::MAX_GPUS >= 2);
    q_sync_count = (q_sync_count + 1) % bufferedFrames;
}
#elif defined(USE_DX11)
void R_sync_point::Create()
{
    for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
        R_CHK(CreateQuery((ID3DQuery**)&q_sync_point[i], D3D_QUERY_EVENT));
    // Prevent error on first get data
    CHK_DX(EndQuery((ID3DQuery*)q_sync_point[0]));
}

void R_sync_point::Destroy()
{
    for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
        R_CHK(ReleaseQuery((ID3DQuery*)q_sync_point[i]));
}

bool R_sync_point::Wait(u32 wait_sleep, u64 timeout)
{
    ZoneScoped;
    CTimer T;
    T.Start();
    BOOL result = FALSE;
    HRESULT hr = S_FALSE;
    while ((hr = GetData((ID3DQuery*)q_sync_point[q_sync_count], &result, sizeof(result))) == S_FALSE)
    {
        if (!SwitchToThread())
            Sleep(wait_sleep);
        if (T.GetElapsed_ms() > timeout)
        {
            result = FALSE;
            break;
        }
    }
    return result;
}

void R_sync_point::End()
{
    q_sync_count = (q_sync_count + 1) % HW.Caps.iGPUNum;
    CHK_DX(EndQuery((ID3DQuery*)q_sync_point[q_sync_count]));
}
#else
#   error No graphics API selected or enabled!
#endif
} // namespace xray::render::RENDER_NAMESPACE
