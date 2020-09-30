#include "stdafx.h"
#include "r__sync_point.h"

#include "QueryHelper.h"

#ifdef USE_OGL
// Assert this just in case
static_assert(sizeof(void*) == sizeof(GLsync), "void* is used instead of GLsync, sizes should match");

void R_sync_point::Create() {}
void R_sync_point::Destroy() {}

bool R_sync_point::Wait(u32 /*wait_sleep*/, u64 timeout)
{
    CHK_GL(q_sync_point[q_sync_count] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));

    const auto status = glClientWaitSync((GLsync)q_sync_point[q_sync_count],
        GL_SYNC_FLUSH_COMMANDS_BIT, timeout * 1000 * 1000);

    bool result = false;
    switch (status)
    {
    case GL_ALREADY_SIGNALED:
    case GL_CONDITION_SATISFIED:
        result = true;
        break;

    case GL_TIMEOUT_EXPIRED:
        // This is bad, but we skip it.
        break;

    case GL_WAIT_FAILED:
        Log("! R_sync_point::Wait raised GL_WAIT_FAILED");
        [[fallthrough]];
    default:
        NODEFAULT;
    }
    return result;
}

void R_sync_point::End()
{
    q_sync_count = (q_sync_count + 1) % HW.Caps.iGPUNum;
    CHK_GL(glDeleteSync((GLsync)q_sync_point[q_sync_count]));
}
#else
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

#endif // USE_OGL
