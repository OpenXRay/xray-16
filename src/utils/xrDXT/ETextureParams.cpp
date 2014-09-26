#include "stdafx.h"
#pragma hdrstop          

#include "ETextureParams.h"

xr_token tparam_token[] =
{
    { "Advanced", STextureParams::kMIPFilterAdvanced },
    { "Point", STextureParams::kMIPFilterPoint },
    { "Box", STextureParams::kMIPFilterBox },
    { "Triangle", STextureParams::kMIPFilterTriangle },
    { "Quadratic", STextureParams::kMIPFilterQuadratic },
    { "Cubic", STextureParams::kMIPFilterCubic },
    { "Catrom", STextureParams::kMIPFilterCatrom },
    { "Mitchell", STextureParams::kMIPFilterMitchell },
    { "Gaussian", STextureParams::kMIPFilterGaussian },
    { "Sinc", STextureParams::kMIPFilterSinc },
    { "Bessel", STextureParams::kMIPFilterBessel },
    { "Hanning", STextureParams::kMIPFilterHanning },
    { "Hamming", STextureParams::kMIPFilterHamming },
    { "Blackman", STextureParams::kMIPFilterBlackman },
    { "Kaiser", STextureParams::kMIPFilterKaiser },
    { 0, 0 }
};

xr_token ttype_token[] =
{
    { "2D Texture", STextureParams::ttImage },
    { "Cube Map", STextureParams::ttCubeMap },
    { "Bump Map", STextureParams::ttBumpMap },
    { "Normal Map", STextureParams::ttNormalMap },
    { "Terrain", STextureParams::ttTerrain },
    { 0, 0 }
};

xr_token tfmt_token[] =
{
    { "DXT1", STextureParams::tfDXT1 },
    { "DXT1 Alpha", STextureParams::tfADXT1 },
    { "DXT3", STextureParams::tfDXT3 },
    { "DXT5", STextureParams::tfDXT5 },
    { "16 bit (1:5:5:5)", STextureParams::tf1555 },
    { "16 bit (5:6:5)", STextureParams::tf565 },
    { "32 bit (8:8:8:8)", STextureParams::tfRGBA },
    { "8 bit (alpha)", STextureParams::tfA8 },
    { "8 bit (luminance)", STextureParams::tfL8 },
    { "16 bit (alpha:luminance)", STextureParams::tfA8L8 },
    { 0, 0 }
};

xr_token tmtl_token[] =
{
    { "OrenNayar <-> Blin", STextureParams::tmOrenNayar_Blin },
    { "Blin <-> Phong", STextureParams::tmBlin_Phong },
    { "Phong <-> Metal", STextureParams::tmPhong_Metal },
    { "Metal <-> OrenNayar", STextureParams::tmMetal_OrenNayar },
    { 0, 0 }
};

xr_token tbmode_token[] =
{
    { "None", STextureParams::tbmNone },
    { "Use", STextureParams::tbmUse },
    { 0, 0 }
};

void STextureParams::Load(IReader& F)
{
    R_ASSERT(F.find_chunk(THM_CHUNK_TEXTUREPARAM));
    F.r(&fmt, sizeof(ETFormat));
    flags.assign(F.r_u32());
    border_color = F.r_u32();
    fade_color = F.r_u32();
    fade_amount = F.r_u32();
    mip_filter = F.r_u32();
    width = F.r_u32();
    height = F.r_u32();
    if (F.find_chunk(THM_CHUNK_TEXTURE_TYPE))
    {
        type = (ETType)F.r_u32();
    }
    if (F.find_chunk(THM_CHUNK_DETAIL_EXT))
    {
        F.r_stringZ(detail_name);
        detail_scale = F.r_float();
    }
    if (F.find_chunk(THM_CHUNK_MATERIAL))
    {
        material = (ETMaterial)F.r_u32();
        material_weight = F.r_float();
    }
    if (F.find_chunk(THM_CHUNK_BUMP))
    {
        bump_virtual_height = F.r_float();
        bump_mode = (ETBumpMode)F.r_u32();
        if (bump_mode < STextureParams::tbmNone)
        {
            //.. временно (до полного убирания Autogen)
            bump_mode = STextureParams::tbmNone;
        }
        F.r_stringZ(bump_name);
    }
    if (F.find_chunk(THM_CHUNK_EXT_NORMALMAP))
        F.r_stringZ(ext_normal_map_name);
    if (F.find_chunk(THM_CHUNK_FADE_DELAY))
        fade_delay = F.r_u8();
}


void STextureParams::Save(IWriter& F)
{
    F.open_chunk(THM_CHUNK_TEXTUREPARAM);
    F.w(&fmt, sizeof(ETFormat));
    F.w_u32(flags.get());
    F.w_u32(border_color);
    F.w_u32(fade_color);
    F.w_u32(fade_amount);
    F.w_u32(mip_filter);
    F.w_u32(width);
    F.w_u32(height);
    F.close_chunk();
    F.open_chunk(THM_CHUNK_TEXTURE_TYPE);
    F.w_u32(type);
    F.close_chunk();
    F.open_chunk(THM_CHUNK_DETAIL_EXT);
    F.w_stringZ(detail_name);
    F.w_float(detail_scale);
    F.close_chunk();
    F.open_chunk(THM_CHUNK_MATERIAL);
    F.w_u32(material);
    F.w_float(material_weight);
    F.close_chunk();
    F.open_chunk(THM_CHUNK_BUMP);
    F.w_float(bump_virtual_height);
    F.w_u32(bump_mode);
    F.w_stringZ(bump_name);
    F.close_chunk();
    F.open_chunk(THM_CHUNK_EXT_NORMALMAP);
    F.w_stringZ(ext_normal_map_name);
    F.close_chunk();
    F.open_chunk(THM_CHUNK_FADE_DELAY);
    F.w_u8(fade_delay);
    F.close_chunk();
}

#ifdef _EDITOR

#include "PropertiesListHelper.h"

void STextureParams::OnTypeChange(PropValue* prop)
{
    switch (type)
    {
    case ttImage:
    case ttCubeMap:
        break;
    case ttBumpMap:
        flags.set(flGenerateMipMaps, FALSE);
        break;
    case ttNormalMap:
        flags.set(flImplicitLighted |
            flBinaryAlpha |
            flAlphaBorder |
            flColorBorder |
            flFadeToColor |
            flFadeToAlpha |
            flDitherColor |
            flDitherEachMIPLevel |
            flBumpDetail, FALSE);
        flags.set(flGenerateMipMaps, TRUE);
        mip_filter = kMIPFilterKaiser;
        fmt = tfRGBA;
        break;
    case ttTerrain:
        flags.set(flGenerateMipMaps |
            flBinaryAlpha |
            flAlphaBorder |
            flColorBorder |
            flFadeToColor |
            flFadeToAlpha |
            flDitherColor |
            flDitherEachMIPLevel |
            flBumpDetail, FALSE);
        flags.set(flImplicitLighted, TRUE);
        fmt = tfDXT1;
        break;
    }
    if (!OnTypeChangeEvent.empty())
        OnTypeChangeEvent(prop);
}

void STextureParams::FillProp(LPCSTR base_name, PropItemVec& items, PropValue::TOnChange on_type_change)
{
    OnTypeChangeEvent = on_type_change;
    PropValue* P = PHelper().CreateToken32(items, "Type", (u32*)&type, ttype_token);
    P->OnChangeEvent.bind(this, &STextureParams::OnTypeChange);
    PHelper().CreateCaption(items, "Source\\Width", shared_str().sprintf("%d", width));
    PHelper().CreateCaption(items, "Source\\Height", shared_str().sprintf("%d", height));
    PHelper().CreateCaption(items, "Source\\Alpha", HasAlpha() ? "present" : "absent");
    switch (type)
    {
    case ttImage:
    case ttCubeMap:
        PHelper().CreateToken32(items, "Format", (u32*)&fmt, tfmt_token);
        PHelper().CreateFlag32(items, "MipMaps\\Enabled", &flags, flGenerateMipMaps);
        PHelper().CreateToken32(items, "MipMaps\\Filter", (u32*)&mip_filter, tparam_token);
        P = PHelper().CreateToken32(items, "Bump\\Mode", (u32*)&bump_mode, tbmode_token);
        P->OnChangeEvent.bind(this, &STextureParams::OnTypeChange);
        if (tbmUse == bump_mode)
        {
            AnsiString path;
            path = base_name;
            PHelper().CreateChoose(items, "Bump\\Texture", &bump_name, smTexture, path.c_str());
        }
        PHelper().CreateFlag32(items, "Details\\Use As Diffuse", &flags, flDiffuseDetail);
        PHelper().CreateFlag32(items, "Details\\Use As Bump (R2)", &flags, flBumpDetail);
        PHelper().CreateChoose(items, "Details\\Texture", &detail_name, smTexture);
        PHelper().CreateFloat(items, "Details\\Scale", &detail_scale, 0.1f, 10000.f, 0.1f, 2);
        PHelper().CreateToken32(items, "Material\\Base", (u32*)&material, tmtl_token);
        PHelper().CreateFloat(items, "Material\\Weight", &material_weight);
        if (false)
        {
            PHelper().CreateFlag32(items, "Flags\\Binary Alpha", &flags, flBinaryAlpha);
        }
        PHelper().CreateFlag32(items, "Flags\\Dither", &flags, flDitherColor);
        PHelper().CreateFlag32(items, "Flags\\Dither Each MIP", &flags, flDitherEachMIPLevel);
        PHelper().CreateFlag32(items, "Flags\\Implicit Lighted", &flags, flImplicitLighted);
        PHelper().CreateFlag32(items, "Fade\\Enable Color", &flags, flFadeToColor);
        PHelper().CreateFlag32(items, "Fade\\Enabled Alpha", &flags, flFadeToAlpha);
        PHelper().CreateU8(items, "Fade\\Delay 'n' MIP", &fade_delay, 0, 255);
        PHelper().CreateU32(items, "Fade\\% of color to fade in", &fade_amount, 0, 100, 0);
        PHelper().CreateColor(items, "Fade\\Color", &fade_color);
        PHelper().CreateU8(items, "Fade\\Alpha", ((u8*)&fade_color) + 3, 0, 255);
        PHelper().CreateFlag32(items, "Border\\Enabled Color", &flags, flColorBorder);
        PHelper().CreateFlag32(items, "Border\\Enabled Alpha", &flags, flAlphaBorder);
        PHelper().CreateColor(items, "Border\\Color", &border_color);
        break;
    case ttBumpMap:
        PHelper().CreateChoose(items, "Bump\\Special NormalMap", &ext_normal_map_name, smTexture, base_name);
        PHelper().CreateFloat(items, "Bump\\Virtual Height (m)", &bump_virtual_height, 0.f, 0.1f, 0.001f, 3);
        break;
    case ttNormalMap:
        P = PHelper().CreateToken32(items, "Format", (u32*)&fmt, tfmt_token);
        P->Owner()->Enable(false);
        PHelper().CreateFlag32(items, "MipMaps\\Enabled", &flags, flGenerateMipMaps);
        PHelper().CreateToken32(items, "MipMaps\\Filter", (u32*)&mip_filter, tparam_token);
        break;
    case ttTerrain:
        P = PHelper().CreateToken32(items, "Format", (u32*)&fmt, tfmt_token);
        P->Owner()->Enable(false);
        PHelper().CreateFlag32(items, "Details\\Use As Diffuse", &flags, flDiffuseDetail);
        PHelper().CreateFlag32(items, "Details\\Use As Bump (R2)", &flags, flBumpDetail);
        PHelper().CreateChoose(items, "Details\\Texture", &detail_name, smTexture);
        PHelper().CreateFloat(items, "Details\\Scale", &detail_scale, 0.1f, 10000.f, 0.1f, 2);
        PHelper().CreateToken32(items, "Material\\Base", (u32*)&material, tmtl_token);
        PHelper().CreateFloat(items, "Material\\Weight", &material_weight);
        P = PHelper().CreateFlag32(items, "Flags\\Implicit Lighted", &flags, flImplicitLighted); 
        P->Owner()->Enable(false);
        break;
    }
}

LPCSTR STextureParams::FormatString()
{
    return get_token_name(tfmt_token, fmt);
}

u32 STextureParams::MemoryUsage(LPCSTR base_name)
{
    u32 mem_usage = width*height * 4;
    if (flags.test(flGenerateMipMaps))
    {
        mem_usage *= 3ul;
        mem_usage /= 2ul;
    }
    switch (fmt)
    {
    case STextureParams::tfDXT1:
    case STextureParams::tfADXT1:   mem_usage /= 6; break;
    case STextureParams::tfDXT3:
    case STextureParams::tfDXT5:    mem_usage /= 4; break;
    case STextureParams::tf4444:
    case STextureParams::tf1555:
    case STextureParams::tf565:     mem_usage /= 2; break;
    case STextureParams::tfRGBA:	break;
    }
    xr_string fn;
    FS.update_path(fn, _game_textures_, EFS.ChangeFileExt(base_name, ".seq").c_str());
    if (FS.exist(fn.c_str()))
    {
        string128 buffer;
        IReader* F = FS.r_open(0, fn.c_str());
        F->r_string(buffer, sizeof(buffer));
        int cnt = 0;
        while (!F->eof())
        {
            F->r_string(buffer, sizeof(buffer));
            cnt++;
        }
        FS.r_close(F);
        mem_usage *= cnt ? cnt : 1;
    }
    return mem_usage;
}

#endif
