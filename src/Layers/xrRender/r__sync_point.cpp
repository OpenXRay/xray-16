#include "stdafx.h"
#include "r__sync_point.h"

#include "QueryHelper.h"

namespace xray::render::fg
{
#ifdef USE_OGL
// Assert this just in case
static_assert(sizeof(void*) == sizeof(GLsync), "void* is used instead of GLsync, sizes should match");

void R_sync_point::Create() {}
void R_sync_point::Destroy() {}

bool R_sync_point::Wait(u32 /*wait_sleep*/, u64 timeout)
{
    ZoneScoped;
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
    q_sync_count = (q_sync_count + 1) % GEnv.Backend->GetCapabilities().iGPUNum;
    CHK_GL(glDeleteSync((GLsync)q_sync_point[q_sync_count]));
}
#elif defined(USE_DX11)
void R_sync_point::Create() {}
void R_sync_point::Destroy() {}
bool R_sync_point::Wait(u32 wait_sleep, u64 timeout)
{
    (void)wait_sleep;
    (void)timeout;
    return true;
}
void R_sync_point::End() {}
#else
#   error No graphics API selected or enabled!
#endif
} // namespace xray::render::fg
