#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"

// Legacy CRT implementation - stubbed out, FrameGraph handles all render targets

namespace xray::render::fg
{

CRT::~CRT()
{
    destroy();
    // release external reference
    RImplementation.Resources->_DeleteRT(this);
}

void CRT::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, u32 slices_num, Flags32 flags)
{
    // Stubbed - FrameGraph handles render targets
    dwWidth = w;
    dwHeight = h;
    fmt = f;
    sampleCount = SampleCount;
    n_slices = slices_num;
}

void CRT::destroy()
{
    // Stubbed - FrameGraph handles render targets
}

void CRT::set_slice_read(int)
{
    // Stubbed
}

void CRT::set_slice_write(u32, int)
{
    // Stubbed
}

void CRT::reset_begin()
{
    destroy();
}

void CRT::reset_end()
{
    create(cName.c_str(), dwWidth, dwHeight, fmt, sampleCount, n_slices, { dwFlags });
}

void CRT::resolve_into(CRT&) const
{
    // Stubbed
}

void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, u32 slices_num, Flags32 flags)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount, slices_num, flags));
}

} // namespace xray::render::fg
