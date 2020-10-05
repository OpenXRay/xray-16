#include "stdafx.h"

void fix_texture_name(pstr fn);

void simplify_texture(string_path& fn)
{
    static const bool iamGameDesigner = strstr(Core.Params, "-game_designer");
    if (iamGameDesigner)
    {
        if (strstr(fn, "$user"))
            return;
        if (strstr(fn, "ui" DELIMITER))
            return;
        if (strstr(fn, "lmap#"))
            return;
        if (strstr(fn, "act" DELIMITER))
            return;
        if (strstr(fn, "fx" DELIMITER))
            return;
        if (strstr(fn, "glow" DELIMITER))
            return;
        if (strstr(fn, "map" DELIMITER))
            return;
        xr_strcpy(fn, "ed" DELIMITER "ed_not_existing_texture");
    }
}

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
    SState* S = v_states.emplace_back(xr_new<SState>());
    state_code.record(S->state); // S->state will be assigned here
    S->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    S->state_code = state_code;
    return S;
}

void CResourceManager::_DeleteState(const SState* state)
{
    if (0 == (state->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_states, state))
        return;
    Msg("! ERROR: Failed to find compiled stateblock");
}

void CResourceManager::_DeletePass(const SPass* P)
{
    if (0 == (P->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_passes, P))
        return;
    Msg("! ERROR: Failed to find compiled pass");
}

void CResourceManager::_DeleteDecl(const SDeclaration* dcl)
{
    if (0 == (dcl->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_declarations, dcl))
        return;
    Msg("! ERROR: Failed to find compiled vertex-declarator");
}

R_constant_table* CResourceManager::_CreateConstantTable(R_constant_table& C)
{
    if (C.empty())
        return nullptr;

    for (R_constant_table* table : v_constant_tables)
        if (table->equal(C))
            return table;

    R_constant_table* table = v_constant_tables.emplace_back(xr_new<R_constant_table>(C));
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

CRT* CResourceManager::_CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 sampleCount /* = 1 */, Flags32 flags /*= {}*/)
{
    R_ASSERT(Name && Name[0] && w && h);

    // ***** first pass - search already created RT
    pstr N = pstr(Name);
    map_RT::iterator I = m_rtargets.find(N);
    if (I != m_rtargets.end())
        return I->second;
    else
    {
        CRT* RT = xr_new<CRT>();
        RT->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        m_rtargets.emplace(RT->set_name(Name), RT);
        if (RDEVICE.b_is_Ready)
            RT->create(Name, w, h, f, sampleCount, flags);
        return RT;
    }
}

void CResourceManager::_DeleteRT(const CRT* RT)
{
    if (0 == (RT->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    pstr N = pstr(*RT->cName);
    map_RT::iterator I = m_rtargets.find(N);
    if (I != m_rtargets.end())
    {
        m_rtargets.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find render-target '%s'", *RT->cName);
}

//	DX10 cut
/*
CRTC* CResourceManager::_CreateRTC(LPCSTR Name, u32 size, D3DFORMAT f)
{
    R_ASSERT(Name && Name[0] && size);

    // ***** first pass - search already created RTC
    LPSTR N = LPSTR(Name);
    map_RTC::iterator I = m_rtargets_c.find(N);
    if (I != m_rtargets_c.end())
        return I->second;
    else
    {
        CRTC* RT = xr_new<CRTC>();
        RT->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        m_rtargets_c.emplace(RT->set_name(Name), RT);
        if (Device.b_is_Ready)
            RT->create(Name, size, f);
        return RT;
    }
}
void	CResourceManager::_DeleteRTC(const CRTC* RT)
{
    if (0 == (RT->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    LPSTR N = LPSTR(*RT->cName);
    map_RTC::iterator I = m_rtargets_c.find(N);
    if (I != m_rtargets_c.end())
    {
        m_rtargets_c.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find render-target '%s'", *RT->cName);
}
*/

void CResourceManager::DeleteGeom(const SGeometry* Geom)
{
    if (0 == (Geom->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_geoms, Geom))
        return;
    Msg("! ERROR: Failed to find compiled geometry-declaration");
}

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

CTexture* CResourceManager::_CreateTexture(LPCSTR _Name)
{
    // DBG_VerifyTextures	();
    if (0 == xr_strcmp(_Name, "null"))
        return nullptr;
    R_ASSERT(_Name && _Name[0]);
    string_path Name;
    xr_strcpy(Name, _Name); //. andy if (strext(Name)) *strext(Name)=0;
    fix_texture_name(Name);

#ifdef DEBUG
    simplify_texture(Name);
#endif //	DEBUG

    // ***** first pass - search already loaded texture
    pstr N = pstr(Name);
    auto I = m_textures.find(N);
    if (I != m_textures.end())
        return I->second;

    CTexture* T = xr_new<CTexture>();
    T->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    m_textures.emplace(T->set_name(Name), T);
    T->Preload();
    if (RDEVICE.b_is_Ready && !bDeferredLoad)
        T->Load();
    return T;
}

void CResourceManager::_DeleteTexture(const CTexture* T)
{
    // DBG_VerifyTextures();

    if (0 == (T->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    pstr N = pstr(*T->cName);
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

CMatrix* CResourceManager::_CreateMatrix(LPCSTR Name)
{
    R_ASSERT(Name && Name[0]);
    if (0 == xr_stricmp(Name, "$null"))
        return nullptr;

    pstr N = pstr(Name);
    map_Matrix::iterator I = m_matrices.find(N);
    if (I != m_matrices.end())
        return I->second;
    else
    {
        CMatrix* M = xr_new<CMatrix>();
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
    pstr N = pstr(*M->cName);
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

CConstant* CResourceManager::_CreateConstant(LPCSTR Name)
{
    R_ASSERT(Name && Name[0]);
    if (0 == xr_stricmp(Name, "$null"))
        return nullptr;

    pstr N = pstr(Name);
    map_Constant::iterator I = m_constants.find(N);
    if (I != m_constants.end())
        return I->second;
    else
    {
        CConstant* C = xr_new<CConstant>();
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
    pstr N = pstr(*C->cName);
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

    STextureList* lst = lst_textures.emplace_back(xr_new<STextureList>(L));
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

SMatrixList* CResourceManager::_CreateMatrixList(SMatrixList& L)
{
    BOOL bEmpty = TRUE;
    for (const ref_matrix& matrix : L)
    {
        if (matrix)
        {
            bEmpty = FALSE;
            break;
        }
    }

    if (bEmpty)
        return nullptr;

    for (SMatrixList* base : lst_matrices)
    {
        if (L.equal(*base))
            return base;
    }

    SMatrixList* lst = lst_matrices.emplace_back(xr_new<SMatrixList>(L));
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

    SConstantList* lst = lst_constants.emplace_back(xr_new<SConstantList>(L));
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

