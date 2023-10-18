#include "stdafx.h"

void CRenderTarget::u_setrt(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, IDirect3DSurface9* zb)
{
    VERIFY(_1);
    dwWidth[cmd_list.context_id] = _1->dwWidth;
    dwHeight[cmd_list.context_id] = _1->dwHeight;
    if (_1)
        cmd_list.set_RT(_1->pRT, 0);
    else
        cmd_list.set_RT(NULL, 0);
    if (_2)
        cmd_list.set_RT(_2->pRT, 1);
    else
        cmd_list.set_RT(NULL, 1);
    if (_3)
        cmd_list.set_RT(_3->pRT, 2);
    else
        cmd_list.set_RT(NULL, 2);
    cmd_list.set_ZB(zb);
    //RImplementation.rmNormal();
}

void CRenderTarget::u_setrt(CBackend& cmd_list,
    u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3, IDirect3DSurface9* zb)
{
    VERIFY(_1);
    dwWidth[cmd_list.context_id] = W;
    dwHeight[cmd_list.context_id] = H;
    VERIFY(_1);
    cmd_list.set_RT(_1, 0);
    cmd_list.set_RT(_2, 1);
    cmd_list.set_RT(_3, 2);
    cmd_list.set_ZB(zb);
    //RImplementation.rmNormal();
}
