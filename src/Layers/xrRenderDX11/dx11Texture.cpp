#include "stdafx.h"
#pragma hdrstop

#include <DirectXTex.h>

constexpr cpcstr NOT_EXISTING_TEXTURE = "ed" DELIMITER "ed_not_existing_texture";

void fix_texture_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".dds") || !xr_stricmp(_ext, ".bmp") ||
        !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

int get_texture_load_lod(LPCSTR fn)
{
    CInifile::Sect& sect = pSettings->r_section("reduce_lod_texture_list");
    auto it_ = sect.Data.cbegin();
    auto it_e_ = sect.Data.cend();

    ENGINE_API bool is_enough_address_space_available();
    static bool enough_address_space_available = is_enough_address_space_available();

    auto it = it_;
    auto it_e = it_e_;

    for (; it != it_e; ++it)
    {
        if (strstr(fn, it->first.c_str()))
        {
            if (psTextureLOD < 1)
            {
                if (enough_address_space_available)
                    return 0;
                else
                    return 1;
            }
            else if (psTextureLOD < 3)
                return 1;
            else
                return 2;
        }
    }

    if (psTextureLOD < 2)
    {
        //if (enough_address_space_available)
        return 0;
        //else
        //    return 1;
    }
    else if (psTextureLOD < 4)
        return 1;
    else
        return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, size_t orig_size)
{
    if (1 == mip_cnt)
        return orig_size;

    int _lod = lod;
    float res = float(orig_size);

    while (_lod > 0)
    {
        --_lod;
        res -= res / 1.333f;
    }
    return iFloor(res);
}

const float _BUMPHEIGH = 8.f;

//////////////////////////////////////////////////////////////////////
// Utility pack
//////////////////////////////////////////////////////////////////////
IC void Reduce(int& w, int& h, int& l, int& skip)
{
    while ((l > 1) && skip)
    {
        w /= 2;
        h /= 2;
        l -= 1;

        skip--;
    }
    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
}

IC void Reduce(size_t& w, size_t& h, size_t& l, int skip)
{
    while ((l > 1) && skip)
    {
        w /= 2;
        h /= 2;
        l -= 1;

        skip--;
    }
    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
}

/*
ID3DTexture2D*  TW_LoadTextureFromTexture
(
 ID3DTexture2D*     t_from,
 D3DFORMAT&             t_dest_fmt,
 int                        levels_2_skip,
 u32&                   w,
 u32&                   h
 )
{
    // Calculate levels & dimensions
    ID3DTexture2D*      t_dest          = NULL;
    D3DSURFACE_DESC         t_from_desc0    ;
    R_CHK                   (t_from->GetLevelDesc   (0,&t_from_desc0));
    int levels_exist        = t_from->GetLevelCount();
    int top_width           = t_from_desc0.Width;
    int top_height          = t_from_desc0.Height;
    Reduce                  (top_width,top_height,levels_exist,levels_2_skip);

    // Create HW-surface
    if (D3DX_DEFAULT==t_dest_fmt)   t_dest_fmt = t_from_desc0.Format;
    R_CHK                   (D3DXCreateTexture(
        HW.pDevice,
        top_width,top_height,
        levels_exist,0,t_dest_fmt,
        D3DPOOL_MANAGED,&t_dest
        ));

    // Copy surfaces & destroy temporary
    ID3DTexture2D* T_src= t_from;
    ID3DTexture2D* T_dst= t_dest;

    int     L_src           = T_src->GetLevelCount  ()-1;
    int     L_dst           = T_dst->GetLevelCount  ()-1;
    for (; L_dst>=0; L_src--,L_dst--)
    {
        // Get surfaces
        IDirect3DSurface9       *S_src, *S_dst;
        R_CHK   (T_src->GetSurfaceLevel (L_src,&S_src));
        R_CHK   (T_dst->GetSurfaceLevel (L_dst,&S_dst));

        // Copy
        R_CHK   (D3DXLoadSurfaceFromSurface(S_dst,NULL,NULL,S_src,NULL,NULL,D3DX_FILTER_NONE,0));

        // Release surfaces
        _RELEASE                (S_src);
        _RELEASE                (S_dst);
    }

    // OK
    w                       = top_width;
    h                       = top_height;
    return                  t_dest;
}

template    <class _It>
IC  void    TW_Iterate_1OP
(
 ID3DTexture2D*     t_dst,
 ID3DTexture2D*     t_src,
 const _It              pred
 )
{
    DWORD mips                          = t_dst->GetLevelCount();
    R_ASSERT                            (mips == t_src->GetLevelCount());
    for (DWORD i = 0; i < mips; i++)    {
        D3DLOCKED_RECT              Rsrc,Rdst;
        D3DSURFACE_DESC             desc,descS;

        t_dst->GetLevelDesc         (i, &desc);
        t_src->GetLevelDesc         (i, &descS);
        VERIFY                      (desc.Format==descS.Format);
        VERIFY                      (desc.Format==D3DFMT_A8R8G8B8);
        t_src->LockRect             (i,&Rsrc,0,0);
        t_dst->LockRect             (i,&Rdst,0,0);
        for (u32 y = 0; y < desc.Height; y++)   {
            for (u32 x = 0; x < desc.Width; x++)    {
                DWORD&  pSrc    = *(((DWORD*)((u8*)Rsrc.pBits + (y * Rsrc.Pitch)))+x);
                DWORD&  pDst    = *(((DWORD*)((u8*)Rdst.pBits + (y * Rdst.Pitch)))+x);
                pDst            = pred(pDst,pSrc);
            }
        }
        t_dst->UnlockRect           (i);
        t_src->UnlockRect           (i);
    }
}
template    <class _It>
IC  void    TW_Iterate_2OP
(
 ID3DTexture2D*     t_dst,
 ID3DTexture2D*     t_src0,
 ID3DTexture2D*     t_src1,
 const _It              pred
 )
{
    DWORD mips                          = t_dst->GetLevelCount();
    R_ASSERT                            (mips == t_src0->GetLevelCount());
    R_ASSERT                            (mips == t_src1->GetLevelCount());
    for (DWORD i = 0; i < mips; i++)    {
        D3DLOCKED_RECT              Rsrc0,Rsrc1,Rdst;
        D3DSURFACE_DESC             desc,descS0,descS1;

        t_dst->GetLevelDesc         (i, &desc);
        t_src0->GetLevelDesc        (i, &descS0);
        t_src1->GetLevelDesc        (i, &descS1);
        VERIFY                      (desc.Format==descS0.Format);
        VERIFY                      (desc.Format==descS1.Format);
        VERIFY                      (desc.Format==D3DFMT_A8R8G8B8);
        t_src0->LockRect            (i,&Rsrc0,  0,0);
        t_src1->LockRect            (i,&Rsrc1,  0,0);
        t_dst->LockRect             (i,&Rdst,   0,0);
        for (u32 y = 0; y < desc.Height; y++)   {
            for (u32 x = 0; x < desc.Width; x++)    {
                DWORD&  pSrc0   = *(((DWORD*)((u8*)Rsrc0.pBits + (y * Rsrc0.Pitch)))+x);
                DWORD&  pSrc1   = *(((DWORD*)((u8*)Rsrc1.pBits + (y * Rsrc1.Pitch)))+x);
                DWORD&  pDst    = *(((DWORD*)((u8*)Rdst.pBits  + (y * Rdst.Pitch)))+x);
                pDst            = pred(pDst,pSrc0,pSrc1);
            }
        }
        t_dst->UnlockRect           (i);
        t_src0->UnlockRect          (i);
        t_src1->UnlockRect          (i);
    }
}

IC u32 it_gloss_rev     (u32 d, u32 s)  {   return  color_rgba  (
    color_get_A(s),     // gloss
    color_get_B(d),
    color_get_G(d),
    color_get_R(d)      );
}
IC u32 it_gloss_rev_base(u32 d, u32 s)  {
    u32     occ     = color_get_A(d)/3;
    u32     def     = 8;
    u32     gloss   = (occ*1+def*3)/4;
    return  color_rgba  (
        gloss,          // gloss
        color_get_B(d),
        color_get_G(d),
        color_get_R(d)
        );
}
IC u32 it_difference    (u32 d, u32 orig, u32 ucomp)    {   return  color_rgba(
    128+(int(color_get_R(orig))-int(color_get_R(ucomp)))*2,     // R-error
    128+(int(color_get_G(orig))-int(color_get_G(ucomp)))*2,     // G-error
    128+(int(color_get_B(orig))-int(color_get_B(ucomp)))*2,     // B-error
    128+(int(color_get_A(orig))-int(color_get_A(ucomp)))*2  );  // A-error
}
IC u32 it_height_rev    (u32 d, u32 s)  {   return  color_rgba  (
    color_get_A(d),                 // diff x
    color_get_B(d),                 // diff y
    color_get_G(d),                 // diff z
    color_get_R(s)  );              // height
}
IC u32 it_height_rev_base(u32 d, u32 s) {   return  color_rgba  (
    color_get_A(d),                 // diff x
    color_get_B(d),                 // diff y
    color_get_G(d),                 // diff z
    (color_get_R(s)+color_get_G(s)+color_get_B(s))/3    );  // height
}
*/
ID3DBaseTexture* CRender::texture_load(LPCSTR fRName, u32& ret_msize)
{
    ret_msize = 0;
    R_ASSERT1_CURE(fRName && fRName[0], true, { return nullptr; });

    DirectX::TexMetadata IMG;
    DirectX::ScratchImage texture;
    ID3DBaseTexture* pTexture2D = NULL;
    string_path fn;
    size_t img_size = 0;
    int img_loaded_lod = 0;
    u32 mip_cnt = u32(-1);
    bool dummyTextureExist;

    // make file name
    string_path fname;
    xr_strcpy(fname, fRName); //. andy if (strext(fname)) *strext(fname)=0;
    fix_texture_name(fname);

    bool force_srgb = 
        !strstr(fname, "_bump")
        && !strstr(fname, "_mask") 
        && !strstr(fname, "_dudv")
        && !strstr(fname, "water_normal")
        && !strstr(fname, "internal_")
        && !strstr(fname, "ui_magnifier2");

    IReader* S = NULL;
    if (!FS.exist(fn, "$game_textures$", fname, ".dds") && strstr(fname, "_bump"))
        goto _BUMP_from_base;
    if (FS.exist(fn, "$level$", fname, ".dds"))
        goto _DDS;
    if (FS.exist(fn, "$game_saves$", fname, ".dds"))
        goto _DDS;
    if (FS.exist(fn, "$game_textures$", fname, ".dds"))
        goto _DDS;

#ifdef _EDITOR
    ELog.Msg(mtError, "Can't find texture '%s'", fname);
    return 0;
#else
    Msg("! Can't find texture '%s'", fname);
    dummyTextureExist = FS.exist(fn, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds");
    if (!ShadowOfChernobylMode)
        R_ASSERT3(dummyTextureExist, "Dummy texture doesn't exist", NOT_EXISTING_TEXTURE);
    if (!dummyTextureExist)
        return nullptr;
#endif

_DDS:
{
    // Load and get header
    S = FS.r_open(fn);
    img_size = S->length();
#ifdef DEBUG
    Msg("* Loaded: %s[%zu]", fn, img_size);
#endif // DEBUG
    R_ASSERT(S);

    R_CHK2(LoadFromDDSMemory(S->pointer(), S->length(), DirectX::DDS_FLAGS_PERMISSIVE, &IMG, texture), fn);

    // Check for LMAP and compress if needed
    xr_strlwr(fn);

    img_loaded_lod = get_texture_load_lod(fn);

    size_t mip_lod = 0;
    if (img_loaded_lod && !IMG.IsCubemap())
    {
        const auto old_mipmap_cnt = IMG.mipLevels;
        Reduce(IMG.width, IMG.height, IMG.mipLevels, img_loaded_lod);
        mip_lod = old_mipmap_cnt - IMG.mipLevels;
    }

    // DirectX requires compressed texture size to be
    // a multiple of 4. Make sure to meet this requirement.
    if (DirectX::IsCompressed(IMG.format))
    {
        IMG.width = (IMG.width + 3u) & ~0x3u;
        IMG.height = (IMG.height + 3u) & ~0x3u;
    }

    R_CHK2(CreateTextureEx(HW.pDevice, texture.GetImages() + mip_lod, texture.GetImageCount(), IMG,
        D3D_USAGE_IMMUTABLE, D3D_BIND_SHADER_RESOURCE, 0, IMG.miscFlags, force_srgb ? DirectX::CREATETEX_FORCE_SRGB : DirectX::CREATETEX_DEFAULT,
        &pTexture2D), fn
    );
    /*R_CHK2(DirectX::CreateDDSTextureFromMemoryEx(HW.pDevice, reinterpret_cast<uint8_t*>(S->pointer()), S->length(), 0, D3D_USAGE_IMMUTABLE, D3D_BIND_SHADER_RESOURCE, 0, 0,
               force_srgb ? DirectX::DDS_LOADER_FORCE_SRGB : DirectX::DDS_LOADER_DEFAULT, &pTexture2D, nullptr),
        fn);*/

    FS.r_close(S);

    // OK
    mip_cnt = IMG.mipLevels;
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return pTexture2D;
}

_BUMP_from_base:
{
    // Msg          ("! auto-generated bump map: %s",fname);
    Msg("! Fallback to default bump map: %s", fname);
    //////////////////
    if (strstr(fname, "_bump#"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump#", ".dds"), "ed_dummy_bump#");
        S = FS.r_open(fn);
        R_ASSERT2(S, fn);
        goto _DDS;
    }
    if (strstr(fname, "_bump"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump", ".dds"), "ed_dummy_bump");
        S = FS.r_open(fn);

        R_ASSERT2(S, fn);
        goto _DDS;
    }
    //////////////////
}

    return nullptr;
}
