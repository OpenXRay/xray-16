#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRenderDX11/Blender.h"
#include "Layers/xrRenderDX11/Blender_Recorder.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"

namespace xray::render::fg
{
//--------------------------------------------------------------------------------------------------------------
SPass* CResourceManager::_CreatePass(const SPass& proto)
{
    for (SPass* pass : v_passes)
        if (pass->equal(proto))
            return pass;

    SPass* P = v_passes.emplace_back(xr_new<SPass>());
    P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    P->state = proto.state;
    P->ps = proto.ps;
    P->vs = proto.vs;
    P->gs = proto.gs;
#ifdef USE_DX11
    P->hs = proto.hs;
    P->ds = proto.ds;
    P->cs = proto.cs;
#endif
    P->constants = proto.constants;
    P->T = proto.T;
#ifdef _EDITOR
    P->M = proto.M;
#endif
    P->C = proto.C;

    return P;
}

//--------------------------------------------------------------------------------------------------------------
SVS* CResourceManager::_CreateVS(cpcstr, u32) { return nullptr; }
void CResourceManager::_DeleteVS(const SVS*) {}

SPS* CResourceManager::_CreatePS(LPCSTR) { return nullptr; }
void CResourceManager::_DeletePS(const SPS*) {}

SGS* CResourceManager::_CreateGS(LPCSTR) { return nullptr; }
void CResourceManager::_DeleteGS(const SGS*) {}

SHS* CResourceManager::_CreateHS(LPCSTR) { return nullptr; }
void CResourceManager::_DeleteHS(const SHS*) {}

SDS* CResourceManager::_CreateDS(LPCSTR) { return nullptr; }
void CResourceManager::_DeleteDS(const SDS*) {}

SCS* CResourceManager::_CreateCS(LPCSTR) { return nullptr; }
void CResourceManager::_DeleteCS(const SCS*) {}

//--------------------------------------------------------------------------------------------------------------

SDeclaration* CResourceManager::_CreateDecl(const VertexElement* dcl)
{
    for (SDeclaration* D : v_declarations)
    {
        if (dcl_equal(dcl, &D->dcl_code.front()))
            return D;
    }

    SDeclaration* D = v_declarations.emplace_back(xr_new<SDeclaration>());
    u32 dcl_size = GetDeclLength(dcl) + 1;
    D->dcl_code.assign(dcl, dcl + dcl_size);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return D;
}

//--------------------------------------------------------------------------------------------------------------
dx11ConstantBuffer* CResourceManager::_CreateConstantBufferSlang(u32 context_id, const char* name, u32 size)
{
    VERIFY(name);
    VERIFY(size > 0);
    dx11ConstantBuffer* pTempBuffer = xr_new<dx11ConstantBuffer>(name, size);

    for (dx11ConstantBuffer* buf : v_constant_buffer[context_id])
    {
        if (pTempBuffer->Similar(*buf))
        {
            xr_delete(pTempBuffer);
            return buf;
        }
    }

    pTempBuffer->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    v_constant_buffer[context_id].emplace_back(pTempBuffer);
    return pTempBuffer;
}

void CResourceManager::_DeleteConstantBuffer(u32 context_id, const dx11ConstantBuffer* pBuffer)
{
    if (0 == (pBuffer->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_constant_buffer[context_id], pBuffer))
        return;
#ifndef MASTER_GOLD
    Msg("! ERROR: Failed to find compiled constant buffer");
#endif
}

} // namespace xray::render::fg
