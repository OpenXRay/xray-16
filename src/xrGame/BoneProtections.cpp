#include "stdafx.h"
#include "BoneProtections.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#include "Level.h"

float SBoneProtections::getBoneProtection(s16 bone_id)
{
    storage_it it = m_bones_koeff.find(bone_id);
    if (it != m_bones_koeff.end())
        return it->second.koeff;
    else
        return m_default.koeff;
}

float SBoneProtections::getBoneArmor(s16 bone_id)
{
    storage_it it = m_bones_koeff.find(bone_id);
    if (it != m_bones_koeff.end())
        return it->second.armor;
    else
        return m_default.armor;
}

BOOL SBoneProtections::getBonePassBullet(s16 bone_id)
{
    storage_it it = m_bones_koeff.find(bone_id);
    if (it != m_bones_koeff.end())
        return it->second.BonePassBullet;
    else
        return m_default.BonePassBullet;
}

void SBoneProtections::reload(const shared_str& bone_sect, IKinematics* kinematics)
{
    VERIFY(kinematics);
    m_bones_koeff.clear();

    m_fHitFracNpc = READ_IF_EXISTS(pSettings, r_float, bone_sect, "hit_fraction_npc", 0.1f);

    m_default.koeff = 1.0f;
    m_default.armor = 0.0f;
    m_default.BonePassBullet = FALSE;

    CInifile::Sect& protections = pSettings->r_section(bone_sect);
    for (auto i = protections.Data.cbegin(); protections.Data.cend() != i; ++i)
    {
        if (strstr(i->first.c_str(), "hit_fraction"))
            continue;
        if (!xr_strcmp(i->first.c_str(), "ap_scale"))
            continue;

        string256 buffer;
        if (!xr_strcmp(i->first.c_str(), "default"))
        {
            m_default.koeff = (float)atof(_GetItem(i->second.c_str(), 0, buffer));
            m_default.armor = (float)atof(_GetItem(i->second.c_str(), 1, buffer));
            m_default.BonePassBullet = (BOOL)(atof(_GetItem(i->second.c_str(), 2, buffer)) > 0.5f);
        }
        else
        {
            u16 bone_id = kinematics->LL_BoneID(i->first);
            if (bone_id != BI_NONE)
            {
                BoneProtection BP;
                BP.koeff = (float)atof(_GetItem(i->second.c_str(), 0, buffer));
                BP.armor = (float)atof(_GetItem(i->second.c_str(), 1, buffer));
                BP.BonePassBullet = (BOOL)(atof(_GetItem(i->second.c_str(), 2, buffer)) > 0.5f);
                if (m_bones_koeff.find(bone_id) == m_bones_koeff.end())
                    m_bones_koeff.emplace(bone_id, BP);
                else
                    m_bones_koeff[bone_id] = BP;
            }
        }
    }
}

void SBoneProtections::add(const shared_str& bone_sect, IKinematics* kinematics)
{
    if (!IsGameTypeSingle())
        return;

    VERIFY(kinematics);
    m_fHitFracNpc += READ_IF_EXISTS(pSettings, r_float, bone_sect.c_str(), "hit_fraction_npc", 0.0f);

    CInifile::Sect& protections = pSettings->r_section(bone_sect);
    for (auto i = protections.Data.cbegin(); protections.Data.cend() != i; ++i)
    {
        if (strstr(i->first.c_str(), "hit_fraction"))
            continue;
        if (!xr_strcmp(i->first.c_str(), "ap_scale"))
            continue;

        string256 buffer;
        if (!xr_strcmp(i->first.c_str(), "default"))
        {
            m_default.koeff += (float)atof(_GetItem(i->second.c_str(), 0, buffer));
            m_default.armor += (float)atof(_GetItem(i->second.c_str(), 1, buffer));
        }
        else
        {
            u16 bone_id = kinematics->LL_BoneID(i->first);
            if (bone_id != BI_NONE)
            {
                BoneProtection& BP = m_bones_koeff[bone_id];
                BP.koeff += (float)atof(_GetItem(i->second.c_str(), 0, buffer));
                BP.armor += (float)atof(_GetItem(i->second.c_str(), 1, buffer));
            }
        }
    }
}
