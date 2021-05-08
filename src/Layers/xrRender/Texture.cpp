// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

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
IC u32 GetPowerOf2Plus1(u32 v)
{
    u32 cnt = 0;
    while (v)
    {
        v >>= 1;
        cnt++;
    };
    return cnt;
}
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

void TW_Save(ID3DTexture2D* T, LPCSTR name, LPCSTR prefix, LPCSTR postfix)
{
    string256 fn;
    strconcat(sizeof(fn), fn, name, "_", prefix, "-", postfix);
    for (int it = 0; it < int(xr_strlen(fn)); it++)
        if (_DELIMITER == fn[it])
            fn[it] = '_';
    string256 fn2;
    strconcat(sizeof(fn2), fn2, "debug" DELIMITER, fn, ".dds");
    Log("* debug texture save: ", fn2);
    R_CHK(D3DXSaveTextureToFile(fn2, D3DXIFF_DDS, T, nullptr));
}

ID3DTexture2D* TW_LoadTextureFromTexture(
    ID3DTexture2D* t_from, D3DFORMAT& t_dest_fmt, int levels_2_skip, u32& w, u32& h)
{
    // Calculate levels & dimensions
    ID3DTexture2D* t_dest = nullptr;
    D3DSURFACE_DESC t_from_desc0;
    R_CHK(t_from->GetLevelDesc(0, &t_from_desc0));
    int levels_exist = t_from->GetLevelCount();
    int top_width = t_from_desc0.Width;
    int top_height = t_from_desc0.Height;
    Reduce(top_width, top_height, levels_exist, levels_2_skip);

    // Create HW-surface
    if (D3DX_DEFAULT == t_dest_fmt)
        t_dest_fmt = t_from_desc0.Format;
    R_CHK(D3DXCreateTexture(HW.pDevice, top_width, top_height, levels_exist, 0, t_dest_fmt,
        (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED),
    &t_dest));

    // Copy surfaces & destroy temporary
    ID3DTexture2D* T_src = t_from;
    ID3DTexture2D* T_dst = t_dest;

    int L_src = T_src->GetLevelCount() - 1;
    int L_dst = T_dst->GetLevelCount() - 1;
    for (; L_dst >= 0; L_src--, L_dst--)
    {
        // Get surfaces
        IDirect3DSurface9 *S_src, *S_dst;
        R_CHK(T_src->GetSurfaceLevel(L_src, &S_src));
        R_CHK(T_dst->GetSurfaceLevel(L_dst, &S_dst));

        // Copy
        R_CHK(D3DXLoadSurfaceFromSurface(S_dst, NULL, NULL, S_src, NULL, NULL, D3DX_FILTER_NONE, 0));

        // Release surfaces
        _RELEASE(S_src);
        _RELEASE(S_dst);
    }

    // OK
    w = top_width;
    h = top_height;
    return t_dest;
}

template <class _It>
void TW_Iterate_1OP(ID3DTexture2D* t_dst, ID3DTexture2D* t_src, const _It pred)
{
    u32 mips = t_dst->GetLevelCount();
    R_ASSERT(mips == t_src->GetLevelCount());
    for (u32 i = 0; i < mips; i++)
    {
        D3DLOCKED_RECT Rsrc, Rdst;
        D3DSURFACE_DESC desc, descS;

        t_dst->GetLevelDesc(i, &desc);
        t_src->GetLevelDesc(i, &descS);
        VERIFY(desc.Format == descS.Format);
        VERIFY(desc.Format == D3DFMT_A8R8G8B8);
        t_src->LockRect(i, &Rsrc, nullptr, 0);
        t_dst->LockRect(i, &Rdst, nullptr, 0);
        for (u32 y = 0; y < desc.Height; y++)
        {
            for (u32 x = 0; x < desc.Width; x++)
            {
                u32& pSrc = *(((u32*)((u8*)Rsrc.pBits + (y * Rsrc.Pitch))) + x);
                u32& pDst = *(((u32*)((u8*)Rdst.pBits + (y * Rdst.Pitch))) + x);
                pDst = pred(pDst, pSrc);
            }
        }
        t_dst->UnlockRect(i);
        t_src->UnlockRect(i);
    }
}
template <class _It>
void TW_Iterate_2OP(ID3DTexture2D* t_dst, ID3DTexture2D* t_src0, ID3DTexture2D* t_src1, const _It pred)
{
    u32 mips = t_dst->GetLevelCount();
    R_ASSERT(mips == t_src0->GetLevelCount());
    R_ASSERT(mips == t_src1->GetLevelCount());
    for (u32 i = 0; i < mips; i++)
    {
        D3DLOCKED_RECT Rsrc0, Rsrc1, Rdst;
        D3DSURFACE_DESC desc, descS0, descS1;

        t_dst->GetLevelDesc(i, &desc);
        t_src0->GetLevelDesc(i, &descS0);
        t_src1->GetLevelDesc(i, &descS1);
        VERIFY(desc.Format == descS0.Format);
        VERIFY(desc.Format == descS1.Format);
        VERIFY(desc.Format == D3DFMT_A8R8G8B8);
        t_src0->LockRect(i, &Rsrc0, nullptr, 0);
        t_src1->LockRect(i, &Rsrc1, nullptr, 0);
        t_dst->LockRect(i, &Rdst, nullptr, 0);
        for (u32 y = 0; y < desc.Height; y++)
        {
            for (u32 x = 0; x < desc.Width; x++)
            {
                u32& pSrc0 = *(((u32*)((u8*)Rsrc0.pBits + (y * Rsrc0.Pitch))) + x);
                u32& pSrc1 = *(((u32*)((u8*)Rsrc1.pBits + (y * Rsrc1.Pitch))) + x);
                u32& pDst = *(((u32*)((u8*)Rdst.pBits + (y * Rdst.Pitch))) + x);
                pDst = pred(pDst, pSrc0, pSrc1);
            }
        }
        t_dst->UnlockRect(i);
        t_src0->UnlockRect(i);
        t_src1->UnlockRect(i);
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
    ID3DTexture2D* pTexture2D = nullptr;
    IDirect3DCubeTexture9* pTextureCUBE = nullptr;
    string_path fn;
    u32 dwWidth, dwHeight;
    size_t img_size = 0;
    int img_loaded_lod = 0;
    D3DFORMAT fmt;
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
    D3DXIMAGE_INFO IMG;
    S = FS.r_open(fn);
#ifdef DEBUG
    Msg("* Loaded: %s[%d]", fn, S->length());
#endif // DEBUG
    img_size = S->length();
    R_ASSERT(S);
    result = D3DXGetImageInfoFromFileInMemory(S->pointer(), S->length(), &IMG);
    if (FAILED(result))
    {
        Msg("! Can't get image info for texture '%s'", fn);
        FS.r_close(S);
        string_path temp;
        R_ASSERT(FS.exist(temp, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds"));
        R_ASSERT(xr_strcmp(temp, fn));
        xr_strcpy(fn, temp);
        goto _DDS;
    }

    if (IMG.ResourceType == D3DRTYPE_CUBETEXTURE)
        goto _DDS_CUBE;
    else
        goto _DDS_2D;

_DDS_CUBE:
{
    result = D3DXCreateCubeTextureFromFileInMemoryEx(HW.pDevice, S->pointer(), S->length(), D3DX_DEFAULT,
        IMG.MipLevels, 0, IMG.Format,
       (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED),
        D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, nullptr, &pTextureCUBE);
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

    // OK
    dwWidth = IMG.Width;
    dwHeight = IMG.Height;
    fmt = IMG.Format;
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    mip_cnt = pTextureCUBE->GetLevelCount();
    return pTextureCUBE;
}
_DDS_2D:
{
    xr_strlwr(fn);
    // Load   SYS-MEM-surface, bound to device restrictions
    ID3DTexture2D* T_sysmem;
    HRESULT const result =
        D3DXCreateTextureFromFileInMemoryEx(HW.pDevice, S->pointer(), S->length(), D3DX_DEFAULT, D3DX_DEFAULT,
            IMG.MipLevels, 0, IMG.Format, D3DPOOL_SYSTEMMEM, D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, nullptr, &T_sysmem);
    FS.r_close(S);

    if (FAILED(result))
    {
        Msg("! Can't load texture '%s'", fn);
        string_path temp;
        R_ASSERT(FS.exist(temp, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds"));
        xr_strlwr(temp);
        R_ASSERT(xr_strcmp(temp, fn));
        xr_strcpy(fn, temp);
        goto _DDS;
    }

    img_loaded_lod = get_texture_load_lod(fn);
    pTexture2D = TW_LoadTextureFromTexture(T_sysmem, IMG.Format, img_loaded_lod, dwWidth, dwHeight);
    mip_cnt = pTexture2D->GetLevelCount();
    _RELEASE(T_sysmem);

    // OK
    fmt = IMG.Format;
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return pTexture2D;
}
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
        goto _DDS_2D;
    }
    if (strstr(fname, "_bump"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed" DELIMITER "ed_dummy_bump", ".dds"), "ed_dummy_bump");
        S = FS.r_open(fn);

        R_ASSERT2(S, fn);

        img_size = S->length();
        goto _DDS_2D;
    }
#endif
    //////////////////

    *strstr(fname, "_bump") = 0;
    R_ASSERT2(FS.exist(fn, "$game_textures$", fname, ".dds"), fname);

    // Load   SYS-MEM-surface, bound to device restrictions
    D3DXIMAGE_INFO IMG;
    S = FS.r_open(fn);
    img_size = S->length();
    ID3DTexture2D* T_base;
    R_CHK2(D3DXCreateTextureFromFileInMemoryEx(HW.pDevice, S->pointer(), S->length(), D3DX_DEFAULT, D3DX_DEFAULT,
        D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, nullptr, &T_base), fn);
    FS.r_close(S);

    // Create HW-surface
    ID3DTexture2D* T_normal_1 = nullptr;
    R_CHK(D3DXCreateTexture(
        HW.pDevice, IMG.Width, IMG.Height, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &T_normal_1));
    R_CHK(D3DXComputeNormalMap(
        T_normal_1, T_base, nullptr, D3DX_NORMALMAP_COMPUTE_OCCLUSION, D3DX_CHANNEL_LUMINANCE, _BUMPHEIGH));

    // Transfer gloss-map
    TW_Iterate_1OP(T_normal_1, T_base, it_gloss_rev_base);

    // Compress
    fmt = D3DFMT_DXT5;
    img_loaded_lod = get_texture_load_lod(fn);
    ID3DTexture2D* T_normal_1C = TW_LoadTextureFromTexture(T_normal_1, fmt, img_loaded_lod, dwWidth, dwHeight);
    mip_cnt = T_normal_1C->GetLevelCount();

#if RENDER == R_R2
    // Decompress (back)
    fmt = D3DFMT_A8R8G8B8;
    ID3DTexture2D* T_normal_1U = TW_LoadTextureFromTexture(T_normal_1C, fmt, 0, dwWidth, dwHeight);

    // Calculate difference
    ID3DTexture2D* T_normal_1D = 0;
    R_CHK(D3DXCreateTexture(HW.pDevice, dwWidth, dwHeight, T_normal_1U->GetLevelCount(), 0, D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM, &T_normal_1D));
    TW_Iterate_2OP(T_normal_1D, T_normal_1, T_normal_1U, it_difference);

    // Reverse channels back + transfer heightmap
    TW_Iterate_1OP(T_normal_1D, T_base, it_height_rev_base);

    // Compress
    fmt = D3DFMT_DXT5;
    ID3DTexture2D* T_normal_2C = TW_LoadTextureFromTexture(T_normal_1D, fmt, 0, dwWidth, dwHeight);
    _RELEASE(T_normal_1U);
    _RELEASE(T_normal_1D);

    //
    string256 fnameB;
    strconcat(sizeof(fnameB), fnameB, "$user$", fname, "_bumpX");
    ref_texture t_temp = Resources->_CreateTexture(fnameB);
    t_temp->surface_set(T_normal_2C);
    _RELEASE(T_normal_2C); // texture should keep reference to it by itself
#endif
    // T_normal_1C - normal.gloss, reversed
    // T_normal_2C - 2*error.height, non-reversed
    _RELEASE(T_base);
    _RELEASE(T_normal_1);
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return T_normal_1C;
}
}
