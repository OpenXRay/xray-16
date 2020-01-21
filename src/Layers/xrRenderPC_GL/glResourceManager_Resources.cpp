#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
#include "../../xrEngine/Render.h"
#endif

#include "../xrRender/ResourceManager.h"
#include "../xrRender/tss.h"
#include "../xrRender/blenders/Blender.h"
#include "../xrRender/blenders/Blender_Recorder.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

//--------------------------------------------------------------------------------------------------------------
SState* CResourceManager::_CreateState(SimulatorStates& state_code)
{
    // Search equal state-code 
    for (SState* C : v_states)
    {
        SimulatorStates& base = C->state_code;
        if (base.equal(state_code))
            return C;
    }

    // Create New
    SState* S = v_states.emplace_back(new SState());
    state_code.record(S->state);
    S->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    S->state_code = state_code;
    return S;
}

//--------------------------------------------------------------------------------------------------------------
SPass* CResourceManager::_CreatePass(const SPass& proto)
{
    for (SPass* pass : v_passes)
        if (pass->equal(proto))
            return pass;

    SPass* P = v_passes.emplace_back(new SPass());
    P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    P->state = proto.state;
    P->ps = proto.ps;
    P->vs = proto.vs;
    P->gs = proto.gs;
    P->constants = proto.constants;
    P->T = proto.T;
#ifdef _EDITOR
    P->M = proto.M;
#endif
    P->C = proto.C;

    return P;
}

//--------------------------------------------------------------------------------------------------------------
static BOOL dcl_equal(D3DVERTEXELEMENT9* a, D3DVERTEXELEMENT9* b)
{
    // check sizes
    u32 a_size = GetDeclLength(a);
    u32 b_size = GetDeclLength(b);
    if (a_size != b_size) return FALSE;
    return 0 == memcmp(a, b, a_size * sizeof(D3DVERTEXELEMENT9));
}

SDeclaration* CResourceManager::_CreateDecl(u32 FVF)
{
    // Search equal code
    for (SDeclaration* D : v_declarations)
    {
        if (D->dcl_code.empty() && D->FVF == FVF)
            return D;
    }

    SDeclaration* D = v_declarations.emplace_back(new SDeclaration());
    glGenVertexArrays(1, &D->dcl);

    D->FVF = FVF;
    ConvertVertexDeclaration(FVF, D);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return D;
}

SDeclaration* CResourceManager::_CreateDecl(D3DVERTEXELEMENT9* dcl)
{
    // Search equal code
    for (SDeclaration* D : v_declarations)
    {
        if (!D->dcl_code.empty() && dcl_equal(dcl, &D->dcl_code.front()))
            return D;
    }

    SDeclaration* D = v_declarations.emplace_back(new SDeclaration());
    glGenVertexArrays(1, &D->dcl);

    D->FVF = 0;
    u32 dcl_size = GetDeclLength(dcl) + 1;
    D->dcl_code.assign(dcl, dcl + dcl_size);
    ConvertVertexDeclaration(dcl, D);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return D;
}

//--------------------------------------------------------------------------------------------------------------
SVS* CResourceManager::_CreateVS(cpcstr shader, cpcstr fallbackShader /*= nullptr*/, u32 flags /*= 0*/)
{
    string_path name;
    xr_strcpy(name, shader);
    switch (GEnv.Render->m_skinning)
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

    return CreateShader<SVS>(name, shader, fallbackShader, flags);
}

void CResourceManager::_DeleteVS(const SVS* vs) { DestroyShader(vs); }

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR _name)
{
    string_path name;
    xr_strcpy(name, _name);
    switch (GEnv.Render->m_MSAASample)
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

    return CreateShader<SPS>(name, _name, nullptr);
}

void CResourceManager::_DeletePS(const SPS* ps) { DestroyShader(ps); }

//--------------------------------------------------------------------------------------------------------------
SGS* CResourceManager::_CreateGS(LPCSTR Name) { return CreateShader<SGS>(Name); }
void CResourceManager::_DeleteGS(const SGS* gs) { DestroyShader(gs); }

SHS* CResourceManager::_CreateHS(LPCSTR Name) { return CreateShader<SHS>(Name); }
void CResourceManager::_DeleteHS(const SHS* HS) { DestroyShader(HS); }

SDS* CResourceManager::_CreateDS(LPCSTR Name) { return CreateShader<SDS>(Name); }
void CResourceManager::_DeleteDS(const SDS* DS) { DestroyShader(DS); }

SCS* CResourceManager::_CreateCS(LPCSTR Name) { return CreateShader<SCS>(Name); }
void CResourceManager::_DeleteCS(const SCS* CS) { DestroyShader(CS); }

//--------------------------------------------------------------------------------------------------------------
SGeometry* CResourceManager::CreateGeom(VertexElement* decl, GLuint vb, GLuint ib)
{
    R_ASSERT(decl && vb);

    SDeclaration* dcl = _CreateDecl(decl);
    u32 vb_stride = GetDeclVertexSize(decl, 0);

    // ***** first pass - search already loaded shader
    for (SGeometry* geom : v_geoms)
    {
        SGeometry& G = *geom;
        if (G.dcl == dcl && G.vb == vb && G.ib == ib && G.vb_stride == vb_stride) return geom;
    }

    SGeometry* Geom = v_geoms.emplace_back(new SGeometry());
    Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    Geom->dcl = dcl;
    Geom->vb = vb;
    Geom->vb_stride = vb_stride;
    Geom->ib = ib;

    return Geom;
}

SGeometry* CResourceManager::CreateGeom(u32 FVF, GLuint vb, GLuint ib)
{
    R_ASSERT(FVF && vb);

    SDeclaration* dcl = _CreateDecl(FVF);
    u32 vb_stride = GetFVFVertexSize(FVF);

    // ***** first pass - search already loaded shader
    for (SGeometry* geom : v_geoms)
    {
        SGeometry& G = *geom;
        if (G.dcl == dcl && G.vb == vb && G.ib == ib && G.vb_stride == vb_stride) return geom;
    }

    SGeometry* Geom = v_geoms.emplace_back(new SGeometry());
    Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    Geom->dcl = dcl;
    Geom->vb = vb;
    Geom->vb_stride = vb_stride;
    Geom->ib = ib;

    return Geom;
}
