#include "stdafx.h"

namespace xray::render::fg
{
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

void CResourceManager::_DeleteVS(const SVS*) {}
void CResourceManager::_DeletePS(const SPS*) {}
void CResourceManager::_DeleteGS(const SGS*) {}
void CResourceManager::_DeleteHS(const SHS*) {}
void CResourceManager::_DeleteDS(const SDS*) {}
void CResourceManager::_DeleteCS(const SCS*) {}

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

void CResourceManager::_DeleteDecl(const SDeclaration* dcl)
{
    if (0 == (dcl->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    if (reclaim(v_declarations, dcl))
        return;
    Msg("! ERROR: Failed to find compiled vertex-declarator");
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

SGeometry* CResourceManager::CreateGeom(const VertexElement* decl, VertexBufferHandle vb, IndexBufferHandle ib)
{
    R_ASSERT(decl && vb);

    SDeclaration* dcl = _CreateDecl(decl);
    u32 vb_stride = GetDeclVertexSize(decl, 0);

    // ***** first pass - search already loaded shader
    for (SGeometry* geom : v_geoms)
    {
        SGeometry& G = *geom;
        if (G.dcl == dcl && G.vb == vb && G.ib == ib && G.vb_stride == vb_stride)
            return geom;
    }

    SGeometry* Geom = v_geoms.emplace_back(xr_new<SGeometry>());
    Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    Geom->dcl = dcl;
    Geom->vb = vb;
    Geom->vb_stride = vb_stride;
    Geom->ib = ib;

    return Geom;
}

SGeometry* CResourceManager::CreateLogicalGeom(const VertexElement* decl)
{
    R_ASSERT(decl);
    SDeclaration* declaration = _CreateDecl(decl);
    const u32 stride = GetDeclVertexSize(decl, 0);
    R_ASSERT(stride != 0);
    for (SGeometry* geometry : v_geoms)
    {
        if (geometry->dcl == declaration && !geometry->vb && !geometry->ib
            && geometry->vb_stride == stride)
            return geometry;
    }
    SGeometry* geometry = v_geoms.emplace_back(xr_new<SGeometry>());
    geometry->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    geometry->dcl = declaration;
    geometry->vb_stride = stride;
    return geometry;
}

SGeometry* CResourceManager::CreateGeom(u32 FVF, VertexBufferHandle vb, IndexBufferHandle ib)
{
    thread_local xr_vector<VertexElement> decl;
    [[maybe_unused]] const bool result = CreateDeclFromFVF(FVF, decl);
    VERIFY(result);
    SGeometry* g = CreateGeom(decl.data(), vb, ib);
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

void CResourceManager::DiscardGeometryBuffer(nvrhi::IBuffer* buffer)
{
    if (!buffer)
        return;
    for (SGeometry* geometry : v_geoms)
    {
        if (geometry->vb.Get() == buffer)
            geometry->vb = nullptr;
        if (geometry->ib.Get() == buffer)
            geometry->ib = nullptr;
    }
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
    u32 vb_stride					= GetDeclVertexSize	(test,0);
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
    if (Device.b_is_Ready && !bDeferredLoad)
        T->Load();
    return T;
}

void CResourceManager::_DeleteTexture(const CTexture* T)
{
    // DBG_VerifyTextures();

    if (0 == (T->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;
    pstr N = pstr(T->cName.c_str());
    map_Texture::iterator I = m_textures.find(N);
    if (I != m_textures.end())
    {
        m_textures.erase(I);
        return;
    }
    Msg("! ERROR: Failed to find texture surface '%s'", T->cName.c_str());
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
        R_ASSERT(0 == xr_strcmp(I->first, I->second->cName.c_str()));
    }
}
#endif

} // namespace xray::render::fg
