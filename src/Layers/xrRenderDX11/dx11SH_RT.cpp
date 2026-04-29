#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"

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
    dwWidth = w;
    dwHeight = h;
    fmt = f;
    sampleCount = SampleCount;
    n_slices = slices_num;
}

void CRT::destroy()
{
}

void CRT::set_slice_read(int)
{
}

void CRT::set_slice_write(u32, int)
{
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
}

void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, u32 slices_num, Flags32 flags)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount, slices_num, flags));
}

}
