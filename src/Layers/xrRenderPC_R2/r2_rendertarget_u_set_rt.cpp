#include "stdafx.h"

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, IDirect3DSurface9* zb)
{
    VERIFY(_1);
    dwWidth = _1->dwWidth;
    dwHeight = _1->dwHeight;
    if (_1)
        RCache.set_RT(_1->pRT, 0);
    else
        RCache.set_RT(NULL, 0);
    if (_2)
        RCache.set_RT(_2->pRT, 1);
    else
        RCache.set_RT(NULL, 1);
    if (_3)
        RCache.set_RT(_3->pRT, 2);
    else
        RCache.set_RT(NULL, 2);
    RCache.set_ZB(zb);
    //RImplementation.rmNormal();
}

void CRenderTarget::u_setrt(
    u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3, IDirect3DSurface9* zb)
{
    VERIFY(_1);
    dwWidth = W;
    dwHeight = H;
    VERIFY(_1);
    RCache.set_RT(_1, 0);
    RCache.set_RT(_2, 1);
    RCache.set_RT(_3, 2);
    RCache.set_ZB(zb);
    //RImplementation.rmNormal();
}
