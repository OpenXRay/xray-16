#include "StdAfx.h"
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
        string256 buffer;

        BoneProtection BP;

        _GetItem(i->second.c_str(), 0, buffer);
        xr_from_chars(buffer, buffer + xr_strlen(buffer), BP.koeff);
        _GetItem(i->second.c_str(), 1, buffer);
        xr_from_chars(buffer, buffer + xr_strlen(buffer), BP.armor);
        _GetItem(i->second.c_str(), 2, buffer);
        {
            float tmp;
            xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp);
            BP.BonePassBullet = (tmp > 0.5f);
        }

        if (!xr_strcmp(i->first.c_str(), "default"))
        {
            m_default = BP;
        }
        else
        {
            if (!xr_strcmp(i->first.c_str(), "hit_fraction"))
                continue;

            s16 bone_id = kinematics->LL_BoneID(i->first);
            R_ASSERT2(BI_NONE != bone_id, i->first.c_str());
            m_bones_koeff.insert(std::make_pair(bone_id, BP));
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
        if (!xr_strcmp(i->first.c_str(), "hit_fraction"))
            continue;

        string256 buffer;
        if (!xr_strcmp(i->first.c_str(), "default"))
        {
            BoneProtection& BP = m_default;
            {
                float tmp;
                _GetItem(i->second.c_str(), 0, buffer);
                xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp);
                BP.koeff += tmp;
                _GetItem(i->second.c_str(), 1, buffer);
                xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp);
                BP.armor += tmp;
            }
        }
        else
        {
            s16 bone_id = kinematics->LL_BoneID(i->first);
            R_ASSERT2(BI_NONE != bone_id, i->first.c_str());
            BoneProtection& BP = m_bones_koeff[bone_id];
            {
                float tmp;
                _GetItem(i->second.c_str(), 0, buffer);
                xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp);
                BP.koeff += tmp;
                _GetItem(i->second.c_str(), 1, buffer);
                xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp);
                BP.armor += tmp;
            }
        }
    }
}
