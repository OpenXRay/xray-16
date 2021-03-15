#include "stdafx.h"

#include "TextureDescrManager.h"
#include "ETextureParams.h"

#include "xrCore/Threading/ParallelForEach.hpp"

// eye-params
float r__dtex_range = 50;
class cl_dt_scaler : public R_constant_setup
{
public:
    float scale;

    cl_dt_scaler(float s) : scale(s) {}
    void setup(R_constant* C) override { RCache.set_c(C, scale, scale, scale, 1 / r__dtex_range); }
};

void fix_texture_thm_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".thm") || !xr_stricmp(_ext, ".dds") ||
       !xr_stricmp(_ext, ".bmp") || !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

void CTextureDescrMngr::LoadLTX(pcstr initial, bool listTHM)
{
    string_path fname;
    FS.update_path(fname, initial, "textures.ltx");

    if (!FS.exist(fname))
        return;

#ifndef MASTER_GOLD
    Msg("Processing textures.ltx in [%s]", initial);
#endif

    CInifile ini(fname);

    Lock lock;
    if (ini.section_exist("association"))
    {
        CInifile::Sect& data = ini.r_section("association");
#ifndef MASTER_GOLD
        Msg("\tsection [%s] has %d lines", data.Name.c_str(), data.Data.size());
#endif
        m_texture_details.reserve(m_texture_details.size() + data.Data.size());
        m_detail_scalers.reserve(m_detail_scalers.size() + data.Data.size());

        const auto processAssociation = [&](const CInifile::Item& item)
        {
            if (listTHM)
                Msg("\t\t%s = %s", item.first.c_str(), item.second.c_str());

            lock.Enter();
            texture_desc& desc = m_texture_details[item.first];
            cl_dt_scaler*& dts = m_detail_scalers[item.first];
            lock.Leave();

            if (desc.m_assoc)
                xr_delete(desc.m_assoc);

            desc.m_assoc = xr_new<texture_assoc>();

            string_path T;
            float s;

            const int res = sscanf(*item.second, "%[^,],%f", T, &s);
            R_ASSERT4(res == 2, "Bad texture association", item.first.c_str(), fname);
            desc.m_assoc->detail_name = T;
            if (dts)
                dts->scale = s;
            else
                dts = xr_new<cl_dt_scaler>(s);

            if (strstr(item.second.c_str(), "usage[diffuse_or_bump]"))
                desc.m_assoc->usage.set(texture_assoc::flDiffuseDetail | texture_assoc::flBumpDetail);
            else if (strstr(item.second.c_str(), "usage[bump]"))
                desc.m_assoc->usage.set(texture_assoc::flBumpDetail);
            else if (strstr(item.second.c_str(), "usage[diffuse]"))
                desc.m_assoc->usage.set(texture_assoc::flDiffuseDetail);
        };
        xr_parallel_for_each(data.Data, processAssociation);
    } // "association"

    if (ini.section_exist("specification"))
    {
        CInifile::Sect& data = ini.r_section("specification");
#ifndef MASTER_GOLD
        Msg("\tsection [%s] has %d lines", data.Name.c_str(), data.Data.size());
#endif
        m_texture_details.reserve(m_texture_details.size() + data.Data.size());

        const auto processSpecification = [&](const CInifile::Item& item)
        {
            if (listTHM)
                Msg("\t\t%s = %s", item.first.c_str(), item.second.c_str());

            lock.Enter();
            texture_desc& desc = m_texture_details[item.first];
            lock.Leave();

            if (desc.m_spec)
                xr_delete(desc.m_spec);

            desc.m_spec = xr_new<texture_spec>();

            string_path bmode;
            const int res =
                    sscanf(item.second.c_str(), "bump_mode[%[^]]], material[%f]", bmode, &desc.m_spec->m_material);
            R_ASSERT4(res == 2, "Bad texture specification", item.first.c_str(), fname);
            if ((bmode[0] == 'u') && (bmode[1] == 's') && (bmode[2] == 'e') && (bmode[3] == ':'))
            {
                // bump-map specified
                desc.m_spec->m_bump_name = bmode + 4;
            }
        };
        xr_parallel_for_each(data.Data, processSpecification);
    } // "specification"
}

void CTextureDescrMngr::LoadTHM(LPCSTR initial, bool listTHM)
{
    FS_FileSet flist;
    FS.file_list(flist, initial, FS_ListFiles, "*.thm");

    if (flist.empty())
        return;

#ifndef MASTER_GOLD
    Msg("Processing %d .thm files in [%s]", flist.size(), initial);
#endif


    m_texture_details.reserve(m_texture_details.size() + flist.size());
    m_detail_scalers.reserve(m_detail_scalers.size() + flist.size());

    Lock lock;
    const auto processFile = [&](const FS_File& it)
    {
        // Alundaio: Print list of *.thm to find bad .thms!
        if (listTHM)
            Log("\t", it.name.c_str());

        string_path fn;
        FS.update_path(fn, initial, it.name.c_str());
        IReader* F = FS.r_open(fn);

#ifdef XR_PLATFORM_LINUX
        if (nullptr == F)
            FATAL_F("Failed to open file (upper register?): %s", it.name.c_str());
#endif

        xr_strcpy(fn, it.name.c_str());
        fix_texture_thm_name(fn);

        R_ASSERT3(F->find_chunk(THM_CHUNK_TYPE), "Cannot find THM chunk in file", fn);
        F->r_u32();
        STextureParams tp;
        tp.Load(*F);
        FS.r_close(F);
        if (STextureParams::ttImage == tp.type || STextureParams::ttTerrain == tp.type ||
            STextureParams::ttNormalMap == tp.type)
        {
            lock.Enter();
            texture_desc& desc = m_texture_details[fn];
            cl_dt_scaler*& dts = m_detail_scalers[fn];
            lock.Leave();

            if (tp.detail_name.size() &&
                tp.flags.is_any(STextureParams::flDiffuseDetail | STextureParams::flBumpDetail))
            {
                if (desc.m_assoc)
                    xr_delete(desc.m_assoc);

                desc.m_assoc = xr_new<texture_assoc>();
                desc.m_assoc->detail_name = tp.detail_name;
                if (dts)
                    dts->scale = tp.detail_scale;
                else
                    dts = xr_new<cl_dt_scaler>(tp.detail_scale);

                if (tp.flags.is(STextureParams::flDiffuseDetail))
                    desc.m_assoc->usage.set(texture_assoc::flDiffuseDetail);

                if (tp.flags.is(STextureParams::flBumpDetail))
                    desc.m_assoc->usage.set(texture_assoc::flBumpDetail);
            }
            if (desc.m_spec)
                xr_delete(desc.m_spec);

            desc.m_spec = xr_new<texture_spec>();
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
    xr_parallel_for_each(flist, processFile);
}

void CTextureDescrMngr::Load()
{
#ifndef MASTER_GOLD
    CTimer timer;
    timer.Start();
#endif // #ifdef DEBUG

    const bool listTHM = strstr(Core.Params, "-list_thm");

    LoadLTX("$game_textures$", listTHM);
    LoadLTX("$level$", listTHM);

    LoadTHM("$game_textures$", listTHM);
    LoadTHM("$level$", listTHM);

#ifndef MASTER_GOLD
    Msg("%s, texture descriptions loaded for %d ms", __FUNCTION__, timer.GetElapsed_ms());
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
            auto& usage = I->second.m_assoc->usage;
            bDiffuse = usage.test(texture_assoc::flDiffuseDetail);
            bBump = usage.test(texture_assoc::flBumpDetail);
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
            CS = It2 == m_detail_scalers.end() ? 0 : It2->second;
            return TRUE;
        }
    }
    return FALSE;
}
