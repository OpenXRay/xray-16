#include "stdafx.h"

// TODO: Implement NVRHI-based screenshot functionality
// This replaces the legacy D3D11 screenshot code

namespace xray::render::fg
{

void CRender::Screenshot(ScreenshotMode mode /*= SM_NORMAL*/, pcstr name /*= nullptr*/)
{
    // TODO: Implement using GEnv.Backend->GetBackBuffer() and NVRHI readback
    Msg("! Screenshot not yet implemented for FrameGraph renderer");
}

} // namespace xray::render::fg
