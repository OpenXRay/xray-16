#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#ifndef _EDITOR
#pragma comment(lib, "d3dx9.lib")
#include "xrEngine/Render.h"
#endif
#pragma warning(pop)

#include <D3DX10Core.h>

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRender/blenders/blender.h"
#include "Layers/xrRender/blenders/blender_recorder.h"
#include "Layers/xrRenderDX10/dx10BufferUtils.h"
#include "Layers/xrRenderDX10/dx10ConstantBuffer.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

SGS* CResourceManager::_CreateGS(LPCSTR Name) { return CreateShader<SGS>(Name); }
void CResourceManager::_DeleteGS(const SGS* GS) { DestroyShader(GS); }

#ifdef USE_DX11
SHS* CResourceManager::_CreateHS(LPCSTR Name) { return CreateShader<SHS>(Name); }
void CResourceManager::_DeleteHS(const SHS* HS) { DestroyShader(HS); }
SDS* CResourceManager::_CreateDS(LPCSTR Name) { return CreateShader<SDS>(Name); }
void CResourceManager::_DeleteDS(const SDS* DS) { DestroyShader(DS); }
SCS* CResourceManager::_CreateCS(LPCSTR Name) { return CreateShader<SCS>(Name); }
void CResourceManager::_DeleteCS(const SCS* CS) { DestroyShader(CS); }
#endif //	USE_DX10

void fix_texture_name(LPSTR fn);

template <class T>
BOOL reclaim(xr_vector<T*>& vec, const T* ptr)
{
    auto it = vec.begin();
    auto end = vec.end();
    for (; it != end; ++it)
    {
        if (*it == ptr)
        {
            vec.erase(it);
            return TRUE;
        }
    }
    return FALSE;
}

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
    SState* state = v_states.emplace_back(new SState());
    state->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    state->state = ID3DState::Create(state_code);
    state->state_code = state_code;
    return state;
}
void CResourceManager::_DeleteState(const SState* state)
{
    if (0 == (state->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_states, state))
        return;
    Msg("! ERROR: Failed to find compiled stateblock");
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

void CResourceManager::_DeletePass(const SPass* P)
{
    if (0 == (P->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_passes, P))
        return;
    Msg("! ERROR: Failed to find compiled pass");
}

//--------------------------------------------------------------------------------------------------------------
SVS* CResourceManager::_CreateVS(cpcstr shader, cpcstr fallbackShader /*= nullptr*/, u32 flags /*= 0*/)
{
    string_path name;
    xr_strcpy(name, shader);
    if (0 == GEnv.Render->m_skinning)
        xr_strcat(name, "_0");
    if (1 == GEnv.Render->m_skinning)
        xr_strcat(name, "_1");
    if (2 == GEnv.Render->m_skinning)
        xr_strcat(name, "_2");
    if (3 == GEnv.Render->m_skinning)
        xr_strcat(name, "_3");
    if (4 == GEnv.Render->m_skinning)
        xr_strcat(name, "_4");
    
    return CreateShader<SVS>(name, shader, fallbackShader, flags);
}

void CResourceManager::_DeleteVS(const SVS* vs)
{
    if (DestroyShader(vs))
    {
        for (const auto& iDecl : v_declarations)
        {
            const auto iLayout = iDecl->vs_to_layout.find(vs->signature->signature);
            if (iLayout != iDecl->vs_to_layout.end())
            {
                //	Release vertex layout
                _RELEASE(iLayout->second);
                iDecl->vs_to_layout.erase(iLayout);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR _name)
{
    string_path name;
    xr_strcpy(name, _name);
    if (0 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_0");
    if (1 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_1");
    if (2 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_2");
    if (3 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_3");
    if (4 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_4");
    if (5 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_5");
    if (6 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_6");
    if (7 == GEnv.Render->m_MSAASample)
        xr_strcat(name, "_7");

    return CreateShader<SPS>(name, _name, nullptr);
}

void CResourceManager::_DeletePS(const SPS* ps) { DestroyShader(ps); }
//--------------------------------------------------------------------------------------------------------------
static BOOL dcl_equal(D3DVERTEXELEMENT9* a, D3DVERTEXELEMENT9* b)
{
    // check sizes
    u32 a_size = D3DXGetDeclLength(a);
    u32 b_size = D3DXGetDeclLength(b);
    if (a_size != b_size)
        return FALSE;
    return 0 == memcmp(a, b, a_size * sizeof(D3DVERTEXELEMENT9));
}

SDeclaration* CResourceManager::_CreateDecl(D3DVERTEXELEMENT9* dcl)
{
    // Search equal code
    for (SDeclaration* D : v_declarations)
    {
        if (dcl_equal(dcl, &D->dcl_code.front()))
            return D;
    }

    // Create _new
    SDeclaration* D = v_declarations.emplace_back(new SDeclaration());
    u32 dcl_size = D3DXGetDeclLength(dcl) + 1;
    //	Don't need it for DirectX 10 here
    // CHK_DX					(HW.pDevice->CreateVertexDeclaration(dcl,&D->dcl));
    D->dcl_code.assign(dcl, dcl + dcl_size);
    dx10BufferUtils::ConvertVertexDeclaration(D->dcl_code, D->dx10_dcl_code);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return D;
}

void CResourceManager::_DeleteDecl(const SDeclaration* dcl)
{
    if (0 == (dcl->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_declarations, dcl))
        return;
    Msg("! ERROR: Failed to find compiled vertex-declarator");
}

//--------------------------------------------------------------------------------------------------------------
R_constant_table* CResourceManager::_CreateConstantTable(R_constant_table& C)
{
    if (C.empty())
        return nullptr;

    for (R_constant_table* table : v_constant_tables)
        if (table->equal(C))
            return table;

    R_constant_table* table = v_constant_tables.emplace_back(new R_constant_table(C));
    table->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return table;
}
void CResourceManager::_DeleteConstantTable(const R_constant_table* C)
{
    if (0 == (C->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_constant_tables, C))
        return;
    Msg("! ERROR: Failed to find compiled constant-table");
}

//--------------------------------------------------------------------------------------------------------------
CRT* CResourceManager::_CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, bool useUAV)
{
    R_ASSERT(Name && Name[0] && w && h);

    // ***** first pass - search already created RT
    LPSTR N = LPSTR(Name);
    map_RT::iterator I = m_rtargets.find(N);
    if (I != m_rtargets.end())
        return I->second;
    else
    {
        CRT* RT = new CRT();
        RT->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        m_rtargets.emplace(RT->set_name(Name), RT);
        if (Device.b_is_Ready)
            RT->create(Name, w, h, f, SampleCount, useUAV);
        return RT;
    }
}
void CResourceManager::_DeleteRT(const CRT* RT)
{
    if (0 == (RT->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    LPSTR N = LPSTR(*RT->cName);
    map_RT::iterator I = m_rtargets.find(N);
    if (I != m_rtargets.end())
    {
        m_rtargets.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find render-target '%s'", *RT->cName);
}
/*	//	DX10 cut
//--------------------------------------------------------------------------------------------------------------
CRTC*	CResourceManager::_CreateRTC		(LPCSTR Name, u32 size,	D3DFORMAT f)
{
    R_ASSERT(Name && Name[0] && size);

    // ***** first pass - search already created RTC
    LPSTR N = LPSTR(Name);
    map_RTC::iterator I = m_rtargets_c.find	(N);
    if (I!=m_rtargets_c.end())	return I->second;
    else
    {
        CRTC *RT				=	new CRTC();
        RT->dwFlags				|=	xr_resource_flagged::RF_REGISTERED;
        m_rtargets_c.emplace	(RT->set_name(Name), RT);
        if (Device.b_is_Ready)	RT->create	(Name,size,f);
        return					RT;
    }
}
void	CResourceManager::_DeleteRTC		(const CRTC* RT)
{
    if (0==(RT->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
    LPSTR N				= LPSTR		(*RT->cName);
    map_RTC::iterator I	= m_rtargets_c.find	(N);
    if (I!=m_rtargets_c.end())	{
        m_rtargets_c.erase(I);
        return;
    }
    Msg	("! ERROR: Failed to find render-target '%s'",*RT->cName);
}
*/
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::DBG_VerifyGeoms()
{
    /*
    for (u32 it=0; it<v_geoms.size(); it++)
    {
    SGeometry* G					= v_geoms[it];

    D3DVERTEXELEMENT9		test	[MAX_FVF_DECL_SIZE];
    u32						size	= 0;
    G->dcl->GetDeclaration			(test,(unsigned int*)&size);
    u32 vb_stride					= D3DXGetDeclVertexSize	(test,0);
    u32 vb_stride_cached			= G->vb_stride;
    R_ASSERT						(vb_stride == vb_stride_cached);
    }
    */
}

SGeometry* CResourceManager::CreateGeom(D3DVERTEXELEMENT9* decl, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib)
{
    R_ASSERT(decl && vb);

    SDeclaration* dcl = _CreateDecl(decl);
    u32 vb_stride = D3DXGetDeclVertexSize(decl, 0);

    // ***** first pass - search already loaded shader
    for (SGeometry* v_geom : v_geoms)
    {
        SGeometry& G = *v_geom;
        if ((G.dcl == dcl) && (G.vb == vb) && (G.ib == ib) && (G.vb_stride == vb_stride))
            return v_geom;
    }

    SGeometry* Geom = v_geoms.emplace_back(new SGeometry());
    Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    Geom->dcl = dcl;
    Geom->vb = vb;
    Geom->vb_stride = vb_stride;
    Geom->ib = ib;

    return Geom;
}
SGeometry* CResourceManager::CreateGeom(u32 FVF, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib)
{
    D3DVERTEXELEMENT9 dcl[MAX_FVF_DECL_SIZE];
    CHK_DX(D3DXDeclaratorFromFVF(FVF, dcl));
    SGeometry* g = CreateGeom(dcl, vb, ib);
    return g;
}

void CResourceManager::DeleteGeom(const SGeometry* Geom)
{
    if (0 == (Geom->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_geoms, Geom))
        return;
    Msg("! ERROR: Failed to find compiled geometry-declaration");
}

//--------------------------------------------------------------------------------------------------------------
CTexture* CResourceManager::_CreateTexture(LPCSTR _Name)
{
    // DBG_VerifyTextures	();
    if (0 == xr_strcmp(_Name, "null"))
        return nullptr;
    R_ASSERT(_Name && _Name[0]);
    string_path Name;
    xr_strcpy(Name, _Name); //. andy if (strext(Name)) *strext(Name)=0;
    fix_texture_name(Name);
    // ***** first pass - search already loaded texture
    LPSTR N = LPSTR(Name);
    auto I = m_textures.find(N);
    if (I != m_textures.end())
        return I->second;

    CTexture* T = new CTexture();
    T->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    m_textures.emplace(T->set_name(Name), T);
    T->Preload();
    if (Device.b_is_Ready && !bDeferredLoad)
        T->Load();
    return T;
}
void CResourceManager::_DeleteTexture(const CTexture* T)
{
    // DBG_VerifyTextures	();

    if (0 == (T->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    LPSTR N = LPSTR(*T->cName);
    map_Texture::iterator I = m_textures.find(N);
    if (I != m_textures.end())
    {
        m_textures.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find texture surface '%s'", *T->cName);
}

#ifdef DEBUG
void CResourceManager::DBG_VerifyTextures()
{
    map_Texture::iterator I = m_textures.begin();
    map_Texture::iterator E = m_textures.end();
    for (; I != E; ++I)
    {
        R_ASSERT(I->first);
        R_ASSERT(I->second);
        R_ASSERT(I->second->cName);
        R_ASSERT(0 == xr_strcmp(I->first, *I->second->cName));
    }
}
#endif

//--------------------------------------------------------------------------------------------------------------
CMatrix* CResourceManager::_CreateMatrix(LPCSTR Name)
{
    R_ASSERT(Name && Name[0]);
    if (0 == xr_stricmp(Name, "$null"))
        return nullptr;

    LPSTR N = LPSTR(Name);
    map_Matrix::iterator I = m_matrices.find(N);
    if (I != m_matrices.end())
        return I->second;
    else
    {
        CMatrix* M = new CMatrix();
        M->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        M->dwReference = 1;
        m_matrices.emplace(M->set_name(Name), M);
        return M;
    }
}
void CResourceManager::_DeleteMatrix(const CMatrix* M)
{
    if (0 == (M->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    LPSTR N = LPSTR(*M->cName);
    map_Matrix::iterator I = m_matrices.find(N);
    if (I != m_matrices.end())
    {
        m_matrices.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find xform-def '%s'", *M->cName);
}
void CResourceManager::ED_UpdateMatrix(LPCSTR Name, CMatrix* data)
{
    CMatrix* M = _CreateMatrix(Name);
    *M = *data;
}
//--------------------------------------------------------------------------------------------------------------
CConstant* CResourceManager::_CreateConstant(LPCSTR Name)
{
    R_ASSERT(Name && Name[0]);
    if (0 == xr_stricmp(Name, "$null"))
        return nullptr;

    LPSTR N = LPSTR(Name);
    map_Constant::iterator I = m_constants.find(N);
    if (I != m_constants.end())
        return I->second;
    else
    {
        CConstant* C = new CConstant();
        C->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        C->dwReference = 1;
        m_constants.emplace(C->set_name(Name), C);
        return C;
    }
}
void CResourceManager::_DeleteConstant(const CConstant* C)
{
    if (0 == (C->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    LPSTR N = LPSTR(*C->cName);
    map_Constant::iterator I = m_constants.find(N);
    if (I != m_constants.end())
    {
        m_constants.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find R1-constant-def '%s'", *C->cName);
}

void CResourceManager::ED_UpdateConstant(LPCSTR Name, CConstant* data)
{
    CConstant* C = _CreateConstant(Name);
    *C = *data;
}

//--------------------------------------------------------------------------------------------------------------
bool cmp_tl(const std::pair<u32, ref_texture>& _1, const std::pair<u32, ref_texture>& _2)
{
    return _1.first < _2.first;
}
STextureList* CResourceManager::_CreateTextureList(STextureList& L)
{
    std::sort(L.begin(), L.end(), cmp_tl);
    for (STextureList* base : lst_textures)
    {
        if (L.equal(*base))
            return base;
    }

    STextureList* lst = lst_textures.emplace_back(new STextureList(L));
    lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return lst;
}
void CResourceManager::_DeleteTextureList(const STextureList* L)
{
    if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(lst_textures, L))
        return;
    Msg("! ERROR: Failed to find compiled list of textures");
}
//--------------------------------------------------------------------------------------------------------------
SMatrixList* CResourceManager::_CreateMatrixList(SMatrixList& L)
{
    BOOL bEmpty = TRUE;
    for (u32 i = 0; i < L.size(); i++)
        if (L[i])
        {
            bEmpty = FALSE;
            break;
        }

    if (bEmpty)
        return nullptr;

    for (SMatrixList* base : lst_matrices)
    {
        if (L.equal(*base))
            return base;
    }

    SMatrixList* lst = lst_matrices.emplace_back(new SMatrixList(L));
    lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return lst;
}
void CResourceManager::_DeleteMatrixList(const SMatrixList* L)
{
    if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(lst_matrices, L))
        return;
    Msg("! ERROR: Failed to find compiled list of xform-defs");
}
//--------------------------------------------------------------------------------------------------------------
SConstantList* CResourceManager::_CreateConstantList(SConstantList& L)
{
    BOOL bEmpty = TRUE;
    for (const ref_constant_obsolette& constant : L)
    {
        if (constant)
        {
            bEmpty = FALSE;
            break;
        }
    }

    if (bEmpty)
        return nullptr;

    for (SConstantList* base : lst_constants)
    {
        if (L.equal(*base))
            return base;
    }

    SConstantList* lst = lst_constants.emplace_back(new SConstantList(L));
    lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return lst;
}
void CResourceManager::_DeleteConstantList(const SConstantList* L)
{
    if (0 == (L->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(lst_constants, L))
        return;
    Msg("! ERROR: Failed to find compiled list of r1-constant-defs");
}
//--------------------------------------------------------------------------------------------------------------
dx10ConstantBuffer* CResourceManager::_CreateConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable)
{
    VERIFY(pTable);
    dx10ConstantBuffer* pTempBuffer = new dx10ConstantBuffer(pTable);

    for (dx10ConstantBuffer* buf : v_constant_buffer)
    {
        if (pTempBuffer->Similar(*buf))
        {
            xr_delete(pTempBuffer);
            return buf;
        }
    }

    pTempBuffer->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    v_constant_buffer.emplace_back(pTempBuffer);
    return pTempBuffer;
}
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::_DeleteConstantBuffer(const dx10ConstantBuffer* pBuffer)
{
    if (0 == (pBuffer->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_constant_buffer, pBuffer))
        return;
    Msg("! ERROR: Failed to find compiled constant buffer");
}

//--------------------------------------------------------------------------------------------------------------
SInputSignature* CResourceManager::_CreateInputSignature(ID3DBlob* pBlob)
{
    VERIFY(pBlob);

    for (SInputSignature* sign : v_input_signature)
    {
        if ((pBlob->GetBufferSize() == sign->signature->GetBufferSize()) &&
            (!(memcmp(pBlob->GetBufferPointer(), sign->signature->GetBufferPointer(), pBlob->GetBufferSize()))))
        {
            return sign;
        }
    }

    SInputSignature* pSign = v_input_signature.emplace_back(new SInputSignature(pBlob));
    pSign->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return pSign;
}
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::_DeleteInputSignature(const SInputSignature* pSignature)
{
    if (0 == (pSignature->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_input_signature, pSignature))
        return;
    Msg("! ERROR: Failed to find compiled constant buffer");
}
