#include "stdafx.h"

static unsigned int g_uGroupTexelDimension = 56;
static unsigned int g_uGroupTexelOverlap = 12;
static unsigned int g_uGroupTexelDimensionAfterOverlap = g_uGroupTexelDimension - 2 * g_uGroupTexelOverlap;

void CRenderTarget::phase_hdao()
{
    if (ps_r_ssao > 0)
    {
        ShaderElement* S;

        if (!RImplementation.o.dx10_msaa)
            S = (&*(s_hdao_cs->E[0]));
        else
            S = (&*(s_hdao_cs_msaa->E[0]));

        SPass& P = *(S->passes[0]);
        RCache.set_States(P.state);
        RCache.set_Constants(P.constants);
        RCache.set_Textures(P.T);
        RCache.set_CS(P.cs);

        // set the cs shader output
        u32 UAVInitialCounts = 1;
        ID3D11UnorderedAccessView* uav[1] = {0};
        ID3D11RenderTargetView* oldrtv[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        ID3D11DepthStencilView* olddsv;
        ID3D11RenderTargetView* rtv[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        ID3D11ShaderResourceView* srv[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        // ID3D11ShaderResourceView* new_srv[2]={rt_ssao_temp1->pTexture->m_pSRView,rt_ssao_temp2->pTexture->m_pSRView};
        HW.pContext->OMGetRenderTargets(8, oldrtv, &olddsv);
        HW.pContext->OMSetRenderTargets(8, rtv, NULL);
        // HW.pContext->CSSetShaderResources( 0, 2, new_srv );
        HW.pContext->CSSetUnorderedAccessViews(0, 1, &rt_ssao_temp->pUAView, &UAVInitialCounts);

        int iGroupsX = (int)ceil((float)dwWidth / (float)g_uGroupTexelDimensionAfterOverlap);
        int iGroupsY = (int)ceil((float)dwHeight / (float)g_uGroupTexelDimensionAfterOverlap);
        RCache.Compute(iGroupsX, iGroupsY, 1);

        HW.pContext->CSSetUnorderedAccessViews(0, 1, uav, &UAVInitialCounts);
        HW.pContext->CSSetShaderResources(0, 16, srv);
        HW.pContext->OMSetRenderTargets(8, oldrtv, olddsv);
    }
}
