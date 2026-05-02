#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
//////////////////////////////////////////////////////////////////////////
class CRT : public xr_resource_named
{
public:
    enum : u32 // extends xr_resource_flagged flags
    {
        /*RF_REGISTERED = xr_resource_flagged::RF_REGISTERED,*/
        CreateUAV = 1 << 1, // Self descriptive. DX11-specific.
        CreateSurface = 1 << 2, // Creates depth-stencil or offscreen plain surface instead of texture. DX9-specific.
        CreateBase = 1 << 3, // Creates basic RTV from backbuffer or DSV (depending on format)
    };

    CRT() = default;
    ~CRT();
    void create(LPCSTR Name, u32 w, u32 h, nvrhi::Format f, u32 SampleCount = 1, u32 slices_num = 1, Flags32 flags = {});
    void destroy();
    void reset_begin();
    void reset_end();
    BOOL valid() { return !!pTexture; }

    void set_slice_read(int slice);
    void set_slice_write(u32 context_id, int slice);

    void resolve_into(CRT& destination) const; // only RTs with same format supported
#ifdef XR_PLATFORM_WEB
    void copy_into(CRT& destination) const; // same format and size
#endif

public:
    ref_texture pTexture;

    u32 dwWidth{};
    u32 dwHeight{};
    nvrhi::Format fmt{};
    u32 sampleCount{};
    u32 n_slices{};

    u64 _order{};
};

struct resptrcode_crt : public resptr_base<CRT>
{
    void create(LPCSTR Name, u32 w, u32 h, nvrhi::Format f, u32 SampleCount = 1, Flags32 flags = {})
    {
        create(Name, w, h, f, SampleCount, 1, flags);
    }
    void create(LPCSTR Name, u32 w, u32 h, nvrhi::Format f, u32 SampleCount, u32 slices_num, Flags32 flags);
    void destroy() { _set(nullptr); }
};
typedef resptr_core<CRT, resptrcode_crt> ref_rt;

}
