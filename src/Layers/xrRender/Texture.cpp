// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

using namespace DirectX;

constexpr cpcstr NOT_EXISTING_TEXTURE = "ed" DELIMITER "ed_not_existing_texture";

void fix_texture_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".dds") ||
        !xr_stricmp(_ext, ".bmp") || !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

#ifndef _EDITOR
ENGINE_API bool is_enough_address_space_available();
#else
bool is_enough_address_space_available() { return true; }
#endif

int get_texture_load_lod(LPCSTR fn)
{
    CInifile::Sect& sect = pSettings->r_section("reduce_lod_texture_list");
    auto it_ = sect.Data.cbegin();
    auto it_e_ = sect.Data.cend();

    auto it = it_;
    auto it_e = it_e_;

    static bool enough_address_space_available = is_enough_address_space_available();

    for (; it != it_e; ++it)
    {
        if (strstr(fn, it->first.c_str()))
        {
            if (psTextureLOD < 1)
            {
                if (enough_address_space_available || GEnv.Render->GenerationIsR1())
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
        //if (enough_address_space_available || GEnv.Render->GenerationIsR1())
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
void TW_Save(ScratchImage& T, LPCSTR name, LPCSTR prefix, LPCSTR postfix)
{
    string256 fn;
    strconcat(sizeof(fn), fn, name, "_", prefix, "-", postfix);
    for (int it = 0; it < int(xr_strlen(fn)); it++)
        if (_DELIMITER == fn[it])
            fn[it] = '_';
    string256 fn2;
    strconcat(sizeof(fn2), fn2, "debug" DELIMITER, fn, ".dds");
    Log("* debug texture save: ", fn2);
    wchar_t fnw[256];
    mbtowc(fnw, fn2, sizeof(fnw));
    SaveToDDSFile(T.GetImages(), T.GetImageCount(), T.GetMetadata(), DDS_FLAGS_FORCE_DX9_LEGACY, fnw);
}

ID3DTexture2D* TW_LoadTextureFromTexture(ScratchImage& t_from, D3DFORMAT t_dest_fmt)
{
    // Calculate levels & dimensions
    ID3DTexture2D* t_dest = nullptr;
    TexMetadata t_from_desc0;
    t_from_desc0 = t_from.GetMetadata();
    int levels_exist = t_from_desc0.mipLevels;
    int top_width = t_from_desc0.width;
    int top_height = t_from_desc0.height;

    // Create HW-surface
    R_ASSERT(t_dest_fmt != D3DFMT_UNKNOWN && t_dest_fmt != (D3DFORMAT)-1);
    R_CHK(HW.pDevice->CreateTexture(top_width, top_height, levels_exist, 0, t_dest_fmt,
        (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED), &t_dest, NULL));

    // Copy surfaces & destroy temporary
    for (int level = levels_exist; level >= 0; level--)
    {
        // Lock rect
        const Image* Isrc = t_from.GetImage(level, 0, 0);
        D3DLOCKED_RECT Rdst;
        t_dest->LockRect(level, &Rdst, nullptr, 0);

        // Copy
        R_ASSERT(Rdst.Pitch == Isrc->rowPitch);
        memcpy(Rdst.pBits, Isrc->pixels, Isrc->rowPitch * Isrc->height);

        // Unlock rect
        t_dest->UnlockRect(level);
    }

    // OK
    return t_dest;
}

template <class _It>
void TW_Iterate_1OP(ScratchImage& t_dst, const ScratchImage& t_src, const _It pred)
{
    u32 count = t_dst.GetImageCount();
    R_ASSERT(count == t_src.GetImageCount());
    for (u32 i = 0; i < count; i++)
    {
        const Image* Idst = t_dst.GetImages() + i;
        const Image* Isrc = t_src.GetImages() + i;

        VERIFY(Idst->format == Isrc->format);
        VERIFY(Idst->format == DXGI_FORMAT_B8G8R8A8_UNORM);
        for (u32 y = 0; y < Idst->height; y++)
        {
            for (u32 x = 0; x < Idst->width; x++)
            {
                u32& pSrc = *(((u32*)((u8*)Isrc->pixels + (y * Isrc->rowPitch))) + x);
                u32& pDst = *(((u32*)((u8*)Idst->pixels + (y * Idst->rowPitch))) + x);
                pDst = pred(pDst, pSrc);
            }
        }
    }
}
template <class _It>
void TW_Iterate_2OP(ScratchImage& t_dst, ScratchImage& t_src0, ScratchImage& t_src1, const _It pred)
{
    u32 count = t_dst.GetImageCount();
    R_ASSERT(count == t_src0.GetImageCount());
    R_ASSERT(count == t_src1.GetImageCount());
    for (u32 i = 0; i < count; i++)
    {
        const Image* Idst = t_dst.GetImages() + i;
        const Image* Isrc0 = t_src0.GetImages() + i;
        const Image* Isrc1 = t_src1.GetImages() + i;

        VERIFY(Idst->format == Isrc0->format);
        VERIFY(Idst->format == Isrc1->format);
        VERIFY(Idst->format == DXGI_FORMAT_B8G8R8A8_UNORM);
        for (u32 y = 0; y < Idst->height; y++)
        {
            for (u32 x = 0; x < Idst->width; x++)
            {
                u32& pSrc0 = *(((u32*)((u8*)Isrc0->pixels + (y * Isrc0->rowPitch))) + x);
                u32& pSrc1 = *(((u32*)((u8*)Isrc1->pixels + (y * Isrc1->rowPitch))) + x);
                u32& pDst = *(((u32*)((u8*)Idst->pixels + (y * Idst->rowPitch))) + x);
                pDst = pred(pDst, pSrc0, pSrc1);
            }
        }
    }
}

IC u32 it_gloss_rev(u32 d, u32 s)
{
    return color_rgba(color_get_A(s), // gloss
        color_get_B(d), color_get_G(d), color_get_R(d));
}
IC u32 it_gloss_rev_base(u32 d, u32 /*s*/)
{
    u32 occ = color_get_A(d) / 3;
    u32 def = 8;
    u32 gloss = (occ * 1 + def * 3) / 4;
    return color_rgba(gloss, // gloss
        color_get_B(d), color_get_G(d), color_get_R(d));
}
IC u32 it_difference(u32 /*d*/, u32 orig, u32 ucomp)
{
    return color_rgba(128 + (int(color_get_R(orig)) - int(color_get_R(ucomp))) * 2, // R-error
        128 + (int(color_get_G(orig)) - int(color_get_G(ucomp))) * 2, // G-error
        128 + (int(color_get_B(orig)) - int(color_get_B(ucomp))) * 2, // B-error
        128 + (int(color_get_A(orig)) - int(color_get_A(ucomp))) * 2); // A-error
}
IC u32 it_height_rev(u32 d, u32 s)
{
    return color_rgba(color_get_A(d), // diff x
        color_get_B(d), // diff y
        color_get_G(d), // diff z
        color_get_R(s)); // height
}
IC u32 it_height_rev_base(u32 d, u32 s)
{
    return color_rgba(color_get_A(d), // diff x
        color_get_B(d), // diff y
        color_get_G(d), // diff z
        (color_get_R(s) + color_get_G(s) + color_get_B(s)) / 3); // height
}

ID3DBaseTexture* CRender::texture_load(LPCSTR fRName, u32& ret_msize)
{
    HRESULT result;
    ID3DBaseTexture* pTexture = nullptr;
    string_path fn;
    size_t img_size = 0;
    int img_loaded_lod = 0;
    u32 mip_cnt = u32(-1);
    bool dummyTextureExist;

    // validation
    R_ASSERT(fRName);
    R_ASSERT(fRName[0]);

    // make file name
    string_path fname;
    xr_strcpy(fname, fRName); //. andy if (strext(fname)) *strext(fname)=0;
    fix_texture_name(fname);
    IReader* S = nullptr;
    // if (FS.exist(fn,"$game_textures$",fname, ".dds") && strstr(fname,"_bump")) goto _BUMP;
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
    goto _DDS;

//xrDebug::Fatal(DEBUG_INFO,"Can't find texture '%s'",fname);

#endif

_DDS:
{
    // Load and get header
    D3DSURFACE_DESC IMG;
    S = FS.r_open(fn);
#ifdef DEBUG
    Msg("* Loaded: %s[%d]", fn, S->length());
#endif // DEBUG
    img_size = S->length();
    R_ASSERT(S);
    result = CreateDDSTextureFromMemoryEx(HW.pDevice, (uint8_t*)S->pointer(), S->length(), 0,
        (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED), false, &pTexture);
    FS.r_close(S);

    if (FAILED(result))
    {
        Msg("! Can't load texture '%s'", fn);
        string_path temp;
        R_ASSERT(FS.exist(temp, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds"));
        R_ASSERT(xr_strcmp(temp, fn));
        xr_strcpy(fn, temp);
        goto _DDS;
    }

    if (!RImplementation.o.no_ram_textures && pTexture->GetType() != D3DRTYPE_CUBETEXTURE)
    {
        // Reduce the number of LODs to keep in VRAM
        xr_strlwr(fn);
        img_loaded_lod = get_texture_load_lod(fn);
        pTexture->SetLOD(img_loaded_lod);
    }

    // OK
    mip_cnt = pTexture->GetLevelCount();
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return pTexture;
}
/*
_BUMP:
{
    // Load SYS-MEM-surface, bound to device restrictions
    D3DXIMAGE_INFO IMG;
    IReader* S = FS.r_open (fn);
    msize = S->length ();
    ID3DTexture2D* T_height_gloss;
    R_CHK(D3DXCreateTextureFromFileInMemoryEx(
        HW.pDevice, S->pointer(), S->length(),
        D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM, D3DX_DEFAULT,D3DX_DEFAULT,
        0, &IMG, 0, &T_height_gloss));
    FS.r_close(S);
    //TW_Save(T_height_gloss, fname, "debug-0", "original");

    // Create HW-surface, compute normal map
    ID3DTexture2D* T_normal_1 = 0;
    R_CHK(D3DXCreateTexture
        (HW.pDevice, IMG.Width, IMG.Height, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &T_normal_1));
    R_CHK(D3DXComputeNormalMap(T_normal_1, T_height_gloss, 0, 0, D3DX_CHANNEL_RED,_BUMPHEIGH));
    //TW_Save(T_normal_1, fname, "debug-1", "normal");

    // Transfer gloss-map
    TW_Iterate_1OP(T_normal_1, T_height_gloss, it_gloss_rev);
    //TW_Save(T_normal_1, fname, "debug-2", "normal-G");

    // Compress
    fmt = D3DFMT_DXT5;
    ID3DTexture2D* T_normal_1C = TW_LoadTextureFromTexture(T_normal_1, fmt, psTextureLOD, dwWidth, dwHeight);
    //TW_Save(T_normal_1C, fname, "debug-3", "normal-G-C");

#if RENDER==R_R2
    // Decompress (back)
    fmt = D3DFMT_A8R8G8B8;
    ID3DTexture2D* T_normal_1U = TW_LoadTextureFromTexture(T_normal_1C, fmt, 0, dwWidth, dwHeight);
    // TW_Save(T_normal_1U, fname, "debug-4", "normal-G-CU");

    // Calculate difference
    ID3DTexture2D*  T_normal_1D = 0;
    R_CHK(D3DXCreateTexture(HW.pDevice, dwWidth, dwHeight, T_normal_1U->GetLevelCount(), 0, D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM, &T_normal_1D));
    TW_Iterate_2OP(T_normal_1D, T_normal_1, T_normal_1U, it_difference);
    //TW_Save(T_normal_1D, fname, "debug-5", "normal-G-diff");

    // Reverse channels back + transfer heightmap
    TW_Iterate_1OP(T_normal_1D, T_height_gloss, it_height_rev);
    //TW_Save(T_normal_1D, fname, "debug-6", "normal-G-diff-H");

    // Compress
    fmt = D3DFMT_DXT5;
    ID3DTexture2D* T_normal_2C = TW_LoadTextureFromTexture(T_normal_1D, fmt, 0, dwWidth, dwHeight);
    //TW_Save(T_normal_2C, fname, "debug-7", "normal-G-diff-H-C");
    _RELEASE(T_normal_1U);
    _RELEASE(T_normal_1D);

    //
    string256 fnameB;
    strconcat(fnameB, "$user$", fname, "X");
    ref_texture t_temp = dxRenderDeviceRender::Instance().Resources->_CreateTexture(fnameB);
    t_temp->surface_set(T_normal_2C);
    RELEASE(T_normal_2C); // texture should keep reference to it by itself
#endif

    // release and return
    // T_normal_1C - normal.gloss, reversed
    // T_normal_2C - 2*error.height, non-reversed
    _RELEASE(T_height_gloss);
    _RELEASE(T_normal_1);
    return T_normal_1C;
}
*/
_BUMP_from_base:
{
    Msg("! auto-generated bump map: %s", fname);
//////////////////
#ifndef _EDITOR
    if (strstr(fname, "_bump#"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed" DELIMITER "ed_dummy_bump#", ".dds"), "ed_dummy_bump#");
        S = FS.r_open(fn);
        R_ASSERT2(S, fn);
        img_size = S->length();
        goto _DDS;
    }
    if (strstr(fname, "_bump"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed" DELIMITER "ed_dummy_bump", ".dds"), "ed_dummy_bump");
        S = FS.r_open(fn);

        R_ASSERT2(S, fn);

        img_size = S->length();
        goto _DDS;
    }
#endif
    //////////////////

    *strstr(fname, "_bump") = 0;
    R_ASSERT2(FS.exist(fn, "$game_textures$", fname, ".dds"), fname);

    // Load   SYS-MEM-surface, bound to device restrictions
    TexMetadata IMG;
    S = FS.r_open(fn);
    img_size = S->length();
    ScratchImage T_base;
    R_CHK2(LoadFromDDSMemory(S->pointer(), S->length(), DDS_FLAGS_NONE, &IMG, T_base), fn);
    FS.r_close(S);

    // Create HW-surface
    ScratchImage T_normal_1;
    R_CHK(ComputeNormalMap(T_base.GetImages(), T_base.GetImageCount(), T_base.GetMetadata(),
        CNMAP_COMPUTE_OCCLUSION | CNMAP_CHANNEL_LUMINANCE, _BUMPHEIGH, DXGI_FORMAT_B8G8R8A8_UNORM, T_normal_1));

    // Transfer gloss-map
    TW_Iterate_1OP(T_normal_1, T_base, it_gloss_rev_base);

    // Compress
    img_loaded_lod = get_texture_load_lod(fn);
    ScratchImage T_normal_1C;
    R_CHK(Compress(T_normal_1.GetImages(), T_normal_1.GetImageCount(), T_normal_1.GetMetadata(),
        DXGI_FORMAT_BC5_UNORM, TEX_COMPRESS_DEFAULT, 0.0f, T_normal_1C));
    mip_cnt = T_normal_1C.GetMetadata().mipLevels;

#if RENDER == R_R2
    // Decompress (back)
    ScratchImage T_normal_1U;
    R_CHK(Decompress(T_normal_1C.GetImages(), T_normal_1C.GetImageCount(), T_normal_1C.GetMetadata(),
        DXGI_FORMAT_B8G8R8A8_UNORM, T_normal_1U));

    // Calculate difference
    ScratchImage T_normal_1D;
    T_normal_1D.Initialize(T_normal_1U.GetMetadata());
    TW_Iterate_2OP(T_normal_1D, T_normal_1, T_normal_1U, it_difference);

    // Reverse channels back + transfer heightmap
    TW_Iterate_1OP(T_normal_1D, T_base, it_height_rev_base);

    // Compress
    ScratchImage T_normal_2C;
    R_CHK(Compress(T_normal_1D.GetImages(), T_normal_1D.GetImageCount(), T_normal_1D.GetMetadata(),
        DXGI_FORMAT_BC5_UNORM, TEX_COMPRESS_DEFAULT, 0.0f, T_normal_2C));
    ID3DTexture2D* pTexture2D = TW_LoadTextureFromTexture(T_normal_2C, D3DFMT_DXT5);
    T_normal_1U.Release();
    T_normal_1D.Release();

    //
    string256 fnameB;
    strconcat(sizeof(fnameB), fnameB, "$user$", fname, "_bumpX");
    ref_texture t_temp = Resources->_CreateTexture(fnameB);
    t_temp->surface_set(pTexture2D);
    _RELEASE(pTexture2D); // texture should keep reference to it by itself
#endif
    // T_normal_1C - normal.gloss, reversed
    // T_normal_2C - 2*error.height, non-reversed
    T_base.Release();
    T_normal_1.Release();
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return TW_LoadTextureFromTexture(T_normal_1C, D3DFMT_DXT5);
}
}
