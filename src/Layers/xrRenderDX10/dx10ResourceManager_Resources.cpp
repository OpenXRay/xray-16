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
    for (; it != end; it++)
        if (*it == ptr)
        {
            vec.erase(it);
            return TRUE;
        }
    return FALSE;
}

//--------------------------------------------------------------------------------------------------------------
SState* CResourceManager::_CreateState(SimulatorStates& state_code)
{
    // Search equal state-code
    for (u32 it = 0; it < v_states.size(); it++)
    {
        SState* C = v_states[it];
        ;
        SimulatorStates& base = C->state_code;
        if (base.equal(state_code))
            return C;
    }

    // Create New
    v_states.push_back(new SState());
    v_states.back()->dwFlags |= xr_resource_flagged::RF_REGISTERED;
#if defined(USE_DX10) || defined(USE_DX11)
    v_states.back()->state = ID3DState::Create(state_code);
#else //	USE_DX10
    v_states.back()->state = state_code.record();
#endif //	USE_DX10
    v_states.back()->state_code = state_code;
    return v_states.back();
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
    for (u32 it = 0; it < v_passes.size(); it++)
        if (v_passes[it]->equal(proto))
            return v_passes[it];

    SPass* P = new SPass();
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

    v_passes.push_back(P);
    return v_passes.back();
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
SVS* CResourceManager::_CreateVS(LPCSTR _name)
{
    string_path name;
    xr_strcpy(name, _name);
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
    
    return CreateShader<SVS>(name, _name, true);
}

void CResourceManager::_DeleteVS(const SVS* vs)
{
    // XXX: try to use code below
    // DestroyShader(vs);

    if (0 == (vs->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    LPSTR N = LPSTR(*vs->cName);
    map_VS::iterator I = m_vs.find(N);
    if (I != m_vs.end())
    {
        m_vs.erase(I);
        xr_vector<SDeclaration*>::iterator iDecl;
        for (iDecl = v_declarations.begin(); iDecl != v_declarations.end(); ++iDecl)
        {
            xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
            iLayout = (*iDecl)->vs_to_layout.find(vs->signature->signature);
            if (iLayout != (*iDecl)->vs_to_layout.end())
            {
                //	Release vertex layout
                _RELEASE(iLayout->second);
                (*iDecl)->vs_to_layout.erase(iLayout);
            }
        }
        return;
    }
    Msg("! ERROR: Failed to find compiled vertex-shader '%s'", *vs->cName);
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

    return CreateShader<SPS>(name, _name, true);
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
    for (u32 it = 0; it < v_declarations.size(); it++)
    {
        SDeclaration* D = v_declarations[it];
        ;
        if (dcl_equal(dcl, &*D->dcl_code.begin()))
            return D;
    }

    // Create _new
    SDeclaration* D = new SDeclaration();
    u32 dcl_size = D3DXGetDeclLength(dcl) + 1;
    //	Don't need it for DirectX 10 here
    // CHK_DX					(HW.pDevice->CreateVertexDeclaration(dcl,&D->dcl));
    D->dcl_code.assign(dcl, dcl + dcl_size);
    dx10BufferUtils::ConvertVertexDeclaration(D->dcl_code, D->dx10_dcl_code);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    v_declarations.push_back(D);
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
        return NULL;

    for (u32 it = 0; it < v_constant_tables.size(); it++)
        if (v_constant_tables[it]->equal(C))
            return v_constant_tables[it];
    v_constant_tables.push_back(new R_constant_table(C));
    v_constant_tables.back()->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    return v_constant_tables.back();
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
#ifdef USE_DX11
CRT* CResourceManager::_CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, bool useUAV)
#else
CRT* CResourceManager::_CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount)
#endif
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
        m_rtargets.insert(std::make_pair(RT->set_name(Name), RT));
#ifdef USE_DX11
        if (Device.b_is_Ready)
            RT->create(Name, w, h, f, SampleCount, useUAV);
#else
        if (Device.b_is_Ready)
            RT->create(Name, w, h, f, SampleCount);
#endif
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
        m_rtargets_c.insert		(std::make_pair(RT->set_name(Name),RT));
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
    for (u32 it = 0; it < v_geoms.size(); it++)
    {
        SGeometry& G = *(v_geoms[it]);
        if ((G.dcl == dcl) && (G.vb == vb) && (G.ib == ib) && (G.vb_stride == vb_stride))
            return v_geoms[it];
    }

    SGeometry* Geom = new SGeometry();
    Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    Geom->dcl = dcl;
    Geom->vb = vb;
    Geom->vb_stride = vb_stride;
    Geom->ib = ib;
    v_geoms.push_back(Geom);
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
        return 0;
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
    m_textures.insert(std::make_pair(T->set_name(Name), T));
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
    for (; I != E; I++)
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
        return NULL;

    LPSTR N = LPSTR(Name);
    map_Matrix::iterator I = m_matrices.find(N);
    if (I != m_matrices.end())
        return I->second;
    else
    {
        CMatrix* M = new CMatrix();
        M->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        M->dwReference = 1;
        m_matrices.insert(std::make_pair(M->set_name(Name), M));
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
        return NULL;

    LPSTR N = LPSTR(Name);
    map_Constant::iterator I = m_constants.find(N);
    if (I != m_constants.end())
        return I->second;
    else
    {
        CConstant* C = new CConstant();
        C->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        C->dwReference = 1;
        m_constants.insert(std::make_pair(C->set_name(Name), C));
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
    for (u32 it = 0; it < lst_textures.size(); it++)
    {
        STextureList* base = lst_textures[it];
        if (L.equal(*base))
            return base;
    }
    STextureList* lst = new STextureList(L);
    lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    lst_textures.push_back(lst);
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
        return NULL;

    for (u32 it = 0; it < lst_matrices.size(); it++)
    {
        SMatrixList* base = lst_matrices[it];
        if (L.equal(*base))
            return base;
    }
    SMatrixList* lst = new SMatrixList(L);
    lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    lst_matrices.push_back(lst);
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
    for (u32 i = 0; i < L.size(); i++)
        if (L[i])
        {
            bEmpty = FALSE;
            break;
        }
    if (bEmpty)
        return NULL;

    for (u32 it = 0; it < lst_constants.size(); it++)
    {
        SConstantList* base = lst_constants[it];
        if (L.equal(*base))
            return base;
    }
    SConstantList* lst = new SConstantList(L);
    lst->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    lst_constants.push_back(lst);
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

    for (u32 it = 0; it < v_constant_buffer.size(); it++)
    {
        dx10ConstantBuffer* buf = v_constant_buffer[it];
        if (pTempBuffer->Similar(*buf))
        {
            xr_delete(pTempBuffer);
            return buf;
        }
    }

    pTempBuffer->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    v_constant_buffer.push_back(pTempBuffer);
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

    for (u32 it = 0; it < v_input_signature.size(); it++)
    {
        SInputSignature* sign = v_input_signature[it];
        if ((pBlob->GetBufferSize() == sign->signature->GetBufferSize()) &&
            (!(memcmp(pBlob->GetBufferPointer(), sign->signature->GetBufferPointer(), pBlob->GetBufferSize()))))
        {
            return sign;
        }
    }

    SInputSignature* pSign = new SInputSignature(pBlob);

    pSign->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    v_input_signature.push_back(pSign);

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
