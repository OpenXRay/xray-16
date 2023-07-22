#include "stdafx.h"

static unsigned int g_uGroupTexelDimension = 56;
static unsigned int g_uGroupTexelOverlap = 12;
static unsigned int g_uGroupTexelDimensionAfterOverlap = g_uGroupTexelDimension - 2 * g_uGroupTexelOverlap;

void CRenderTarget::phase_hdao()
{
    if (ps_r_ssao > 0)
    {
        SPass& P = *s_hdao_cs->E[0]->passes[0];
        RCache.set_States(P.state);
        RCache.set_CS(P.cs);
        RCache.set_Constants(P.constants);
        RCache.set_Textures(P.T);

        RCache.set_RT(nullptr, 0);
        RCache.set_RT(nullptr, 1);
        RCache.set_RT(nullptr, 2);
        RCache.set_RT(nullptr, 3);

        // set the cs shader output
        u32 UAVInitialCounts = 1;
        ID3D11UnorderedAccessView* uav[1] = {0};
        ID3D11ShaderResourceView* srv[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        // ID3D11ShaderResourceView* new_srv[2]={rt_ssao_temp1->pTexture->m_pSRView,rt_ssao_temp2->pTexture->m_pSRView};
        // HW.pContext->CSSetShaderResources( 0, 2, new_srv );
        HW.get_context(CHW::IMM_CTX_ID)->CSSetUnorderedAccessViews(0, 1, &rt_ssao_temp->pUAView, &UAVInitialCounts); // TODO: id

        int iGroupsX = (int)ceil((float)dwWidth[RCache.context_id] / (float)g_uGroupTexelDimensionAfterOverlap);
        int iGroupsY = (int)ceil((float)dwHeight[RCache.context_id] / (float)g_uGroupTexelDimensionAfterOverlap);
        RCache.Compute(iGroupsX, iGroupsY, 1);

        HW.get_context(CHW::IMM_CTX_ID)->CSSetUnorderedAccessViews(0, 1, uav, &UAVInitialCounts);
        HW.get_context(CHW::IMM_CTX_ID)->CSSetShaderResources(0, 16, srv);
    }
}
