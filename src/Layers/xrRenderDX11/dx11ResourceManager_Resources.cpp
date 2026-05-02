#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRenderDX11/Blender.h"
#include "Layers/xrRenderDX11/Blender_Recorder.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"
#include "Layers/xrRenderDX11/dx11VertexLayoutCache.h"
#include "Layers/xrRenderDX11/ShaderResourceTraits.h"

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
SVS* CResourceManager::_CreateVS(cpcstr shader, u32 flags /*= 0*/)
{
    string_path name;
    xr_strcpy(name, shader);
    switch (RImplementation.m_skinning)
    {
    case 0:
        xr_strcat(name, "_0");
        break;
    case 1:
        xr_strcat(name, "_1");
        break;
    case 2:
        xr_strcat(name, "_2");
        break;
    case 3:
        xr_strcat(name, "_3");
        break;
    case 4:
        xr_strcat(name, "_4");
        break;
    }

    return CreateShader<SVS>(name, shader, flags);
}

void CResourceManager::_DeleteVS(const SVS* vs)
{
    if (DestroyShader(vs))
    {
        // In FrameGraph mode, vertex layouts are handled differently
        // No need to clean up vs_to_layout mappings as there's no signature field
    }
}

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR _name)
{
    string_path name;
    xr_strcpy(name, _name);
    switch (RImplementation.m_MSAASample)
    {
    case 0:
        xr_strcat(name, "_0");
        break;
    case 1:
        xr_strcat(name, "_1");
        break;
    case 2:
        xr_strcat(name, "_2");
        break;
    case 3:
        xr_strcat(name, "_3");
        break;
    case 4:
        xr_strcat(name, "_4");
        break;
    case 5:
        xr_strcat(name, "_5");
        break;
    case 6:
        xr_strcat(name, "_6");
        break;
    case 7:
        xr_strcat(name, "_7");
        break;
    }

    return CreateShader<SPS>(name, _name);
}

void CResourceManager::_DeletePS(const SPS* ps) { DestroyShader(ps); }

//--------------------------------------------------------------------------------------------------------------
SGS* CResourceManager::_CreateGS(LPCSTR Name) { return CreateShader<SGS>(Name); }
void CResourceManager::_DeleteGS(const SGS* GS) { DestroyShader(GS); }

#ifdef USE_DX11
SHS* CResourceManager::_CreateHS(LPCSTR Name) { return CreateShader<SHS>(Name); }
void CResourceManager::_DeleteHS(const SHS* HS) { DestroyShader(HS); }

SDS* CResourceManager::_CreateDS(LPCSTR Name) { return CreateShader<SDS>(Name); }
void CResourceManager::_DeleteDS(const SDS* DS) { DestroyShader(DS); }

SCS* CResourceManager::_CreateCS(LPCSTR Name) { return CreateShader<SCS>(Name); }
void CResourceManager::_DeleteCS(const SCS* CS) { DestroyShader(CS); }
#endif

//--------------------------------------------------------------------------------------------------------------

SDeclaration* CResourceManager::_CreateDecl(const VertexElement* dcl)
{
    // Search equal code
    for (SDeclaration* D : v_declarations)
    {
        if (dcl_equal(dcl, &D->dcl_code.front()))
            return D;
    }

    SDeclaration* D = v_declarations.emplace_back(xr_new<SDeclaration>());
    u32 dcl_size = GetDeclLength(dcl) + 1;
    D->dcl_code.assign(dcl, dcl + dcl_size);

    auto* data = xr_new<DX11DeclBackendData>();
    ConvertVertexDeclarationDX11(D->dcl_code, data->dx11_dcl_code);
    D->backend_data = data;

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
