#include "stdafx.h"

void CRenderTarget::u_setrt(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, ID3DDepthStencilView* zb)
{
    VERIFY(_1 || zb);
    if (_1)
    {
        dwWidth[cmd_list.context_id] = _1->dwWidth;
        dwHeight[cmd_list.context_id] = _1->dwHeight;
    }
    else
    {
        D3D_DEPTH_STENCIL_VIEW_DESC desc;
        zb->GetDesc(&desc);

        if (!RImplementation.o.msaa)
            VERIFY(desc.ViewDimension == D3D_DSV_DIMENSION_TEXTURE2D || desc.ViewDimension == D3D_DSV_DIMENSION_TEXTURE2DARRAY);

        ID3DResource* pRes;

        zb->GetResource(&pRes);

        ID3DTexture2D* pTex = (ID3DTexture2D*)pRes;

        D3D_TEXTURE2D_DESC TexDesc;

        pTex->GetDesc(&TexDesc);

        dwWidth[cmd_list.context_id] = TexDesc.Width;
        dwHeight[cmd_list.context_id] = TexDesc.Height;
        _RELEASE(pRes);
    }

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
    //	RImplementation.rmNormal				();
}

void CRenderTarget::u_setrt(CBackend& cmd_list, const ref_rt& _1, const ref_rt& _2, ID3DDepthStencilView* zb)
{
    VERIFY(_1 || zb);
    if (_1)
    {
        dwWidth[cmd_list.context_id] = _1->dwWidth;
        dwHeight[cmd_list.context_id] = _1->dwHeight;
    }
    else
    {
        D3D_DEPTH_STENCIL_VIEW_DESC desc;
        zb->GetDesc(&desc);
        if (!RImplementation.o.msaa)
            VERIFY(desc.ViewDimension == D3D_DSV_DIMENSION_TEXTURE2D || desc.ViewDimension == D3D_DSV_DIMENSION_TEXTURE2DARRAY);

        ID3DResource* pRes;

        zb->GetResource(&pRes);

        ID3DTexture2D* pTex = (ID3DTexture2D*)pRes;

        D3D_TEXTURE2D_DESC TexDesc;

        pTex->GetDesc(&TexDesc);

        dwWidth[cmd_list.context_id] = TexDesc.Width;
        dwHeight[cmd_list.context_id] = TexDesc.Height;
        _RELEASE(pRes);
    }

    if (_1)
        cmd_list.set_RT(_1->pRT, 0);
    else
        cmd_list.set_RT(NULL, 0);
    if (_2)
        cmd_list.set_RT(_2->pRT, 1);
    else
        cmd_list.set_RT(NULL, 1);
    cmd_list.set_ZB(zb);
    //	RImplementation.rmNormal				();
}

void CRenderTarget::u_setrt(CBackend& cmd_list, u32 W, u32 H, ID3DRenderTargetView* _1, ID3DRenderTargetView* _2, ID3DRenderTargetView* _3,
    ID3DDepthStencilView* zb)
{
    // VERIFY									(_1);
    dwWidth[cmd_list.context_id] = W;
    dwHeight[cmd_list.context_id] = H;
    // VERIFY									(_1);
    cmd_list.set_RT(_1, 0);
    cmd_list.set_RT(_2, 1);
    cmd_list.set_RT(_3, 2);
    cmd_list.set_ZB(zb);
    //	RImplementation.rmNormal				();
}
