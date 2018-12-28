#include "stdafx.h"

#include <tbb/parallel_for_each.h>

#include "TextureDescrManager.h"
#include "ETextureParams.h"

//#define SINGLETHREADED_TEXTURES_DESCR

// eye-params
float r__dtex_range = 50;
class cl_dt_scaler : public R_constant_setup
{
public:
    float scale;

    cl_dt_scaler(float s) : scale(s){};
    virtual void setup(R_constant* C) { RCache.set_c(C, scale, scale, scale, 1 / r__dtex_range); }
};

void fix_texture_thm_name(LPSTR fn)
{
    LPSTR _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".thm") || !xr_stricmp(_ext, ".dds") ||
       !xr_stricmp(_ext, ".bmp") || !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

#ifdef SINGLETHREADED_TEXTURES_DESCR
#define LOCK_DESCR(lock)
#define UNLOCK_DESCR(lock)
#else
#define LOCK_DESCR(lock) lock.Enter()
#define UNLOCK_DESCR(lock) lock.Leave()
#endif

void CTextureDescrMngr::LoadTHM(LPCSTR initial)
{
    FS_FileSet flist;
    FS.file_list(flist, initial, FS_ListFiles, "*.thm");
#ifndef MASTER_GOLD
    Msg("%s, count of .thm files: %d", __FUNCTION__, flist.size());
#endif

#ifndef SINGLETHREADED_TEXTURES_DESCR
    Lock lock;
#endif

    const bool listTHM = strstr(Core.Params, "-list_thm");

    const auto processFile = [&](const FS_File& it)
    {
        // Alundaio: Print list of *.thm to find bad .thms!
        if (listTHM)
            Log(it.name.c_str());

        string_path fn;
        FS.update_path(fn, initial, it.name.c_str());
        IReader* F = FS.r_open(fn);
        xr_strcpy(fn, it.name.c_str());
        fix_texture_thm_name(fn);

        R_ASSERT(F->find_chunk(THM_CHUNK_TYPE));
        F->r_u32();
        STextureParams tp;
        tp.Load(*F);
        FS.r_close(F);
        if (STextureParams::ttImage == tp.type || STextureParams::ttTerrain == tp.type ||
            STextureParams::ttNormalMap == tp.type)
        {
            LOCK_DESCR(lock);
            texture_desc& desc = m_texture_details[fn];
            cl_dt_scaler*& dts = m_detail_scalers[fn];
            UNLOCK_DESCR(lock);

            if (tp.detail_name.size() &&
                tp.flags.is_any(STextureParams::flDiffuseDetail | STextureParams::flBumpDetail))
            {
                if (desc.m_assoc)
                    xr_delete(desc.m_assoc);

                desc.m_assoc = new texture_assoc();
                desc.m_assoc->detail_name = tp.detail_name;
                if (dts)
                    dts->scale = tp.detail_scale;
                else
                    /*desc.m_assoc->cs*/ dts = new cl_dt_scaler(tp.detail_scale);

                desc.m_assoc->usage = 0;

                if (tp.flags.is(STextureParams::flDiffuseDetail))
                    desc.m_assoc->usage |= (1 << 0);

                if (tp.flags.is(STextureParams::flBumpDetail))
                    desc.m_assoc->usage |= (1 << 1);
            }
            if (desc.m_spec)
                xr_delete(desc.m_spec);

            desc.m_spec = new texture_spec();
            desc.m_spec->m_material = tp.material + tp.material_weight;
            desc.m_spec->m_use_steep_parallax = false;

            if (tp.bump_mode == STextureParams::tbmUse)
            {
                desc.m_spec->m_bump_name = tp.bump_name;
            }
            else if (tp.bump_mode == STextureParams::tbmUseParallax)
            {
                desc.m_spec->m_bump_name = tp.bump_name;
                desc.m_spec->m_use_steep_parallax = true;
            }
        }
    };

#ifdef SINGLETHREADED_TEXTURES_DESCR
    for (auto& it : flist)
        processFile(it);
#else
    tbb::parallel_for_each(flist, processFile);
#endif
}

void CTextureDescrMngr::Load()
{
#ifndef MASTER_GOLD
    CTimer timer;
    timer.Start();
#endif // #ifdef DEBUG

    LoadTHM("$game_textures$");
    LoadTHM("$level$");

#ifndef MASTER_GOLD
    Msg("%s, .thm loading time: %d ms", __FUNCTION__, timer.GetElapsed_ms());
#endif
}

void CTextureDescrMngr::UnLoad()
{
    for (auto& it : m_texture_details)
    {
        xr_delete(it.second.m_assoc);
        xr_delete(it.second.m_spec);
    }
    m_texture_details.clear();
}

CTextureDescrMngr::~CTextureDescrMngr()
{
    for (auto& it : m_detail_scalers)
        xr_delete(it.second);

    m_detail_scalers.clear();
}

shared_str CTextureDescrMngr::GetBumpName(const shared_str& tex_name) const
{
    map_TD::const_iterator I = m_texture_details.find(tex_name);
    if (I != m_texture_details.end())
    {
        if (I->second.m_spec)
        {
            return I->second.m_spec->m_bump_name;
        }
    }
    return "";
}

BOOL CTextureDescrMngr::UseSteepParallax(const shared_str& tex_name) const
{
    map_TD::const_iterator I = m_texture_details.find(tex_name);
    if (I != m_texture_details.end())
    {
        if (I->second.m_spec)
        {
            return I->second.m_spec->m_use_steep_parallax;
        }
    }
    return FALSE;
}

float CTextureDescrMngr::GetMaterial(const shared_str& tex_name) const
{
    map_TD::const_iterator I = m_texture_details.find(tex_name);
    if (I != m_texture_details.end())
    {
        if (I->second.m_spec)
        {
            return I->second.m_spec->m_material;
        }
    }
    return 1.0f;
}

void CTextureDescrMngr::GetTextureUsage(const shared_str& tex_name, bool& bDiffuse, bool& bBump) const
{
    map_TD::const_iterator I = m_texture_details.find(tex_name);
    if (I != m_texture_details.end())
    {
        if (I->second.m_assoc)
        {
            u8 usage = I->second.m_assoc->usage;
            bDiffuse = !!(usage & (1 << 0));
            bBump = !!(usage & (1 << 1));
        }
    }
}

BOOL CTextureDescrMngr::GetDetailTexture(const shared_str& tex_name, LPCSTR& res, R_constant_setup*& CS) const
{
    map_TD::const_iterator I = m_texture_details.find(tex_name);
    if (I != m_texture_details.end())
    {
        if (I->second.m_assoc)
        {
            texture_assoc* TA = I->second.m_assoc;
            res = TA->detail_name.c_str();
            map_CS::const_iterator It2 = m_detail_scalers.find(tex_name);
            CS = It2 == m_detail_scalers.end() ? 0 : It2->second; // TA->cs;
            return TRUE;
        }
    }
    return FALSE;
}
