#include "stdafx.h"

#include "Layers/xrRenderDX9/dx9shader_utils.h"

BOOL xrRender_test_hw()
{
    D3DCAPS9 caps;
    CHW _HW;
    _HW.CreateD3D();

    if (!_HW.pD3D || _HW.pD3D->GetAdapterCount() == 0)
        return FALSE;

#ifdef USE_D3DX
    const bool shaderCompilerAvailable = XRay::ModuleHandle{ "d3dx9_31" }.IsLoaded();
    if (!shaderCompilerAvailable)
        return FALSE;
#endif

    _HW.pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    _HW.DestroyD3D();
    u16 ps_ver_major = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf << 8ul)) >> 8);
    u16 ps_instructions = u16(caps.PS20Caps.NumInstructionSlots);
    u16 mrt_count = u16(caps.NumSimultaneousRTs);
    if (ps_ver_major < 2)
        return FALSE;
    if (ps_instructions < 256)
        return FALSE;
    if (mrt_count < 3)
        return FALSE;
    if (ps_ver_major >= 3)
        return TRUE+TRUE; // XXX: remove hack
    return TRUE;
}
