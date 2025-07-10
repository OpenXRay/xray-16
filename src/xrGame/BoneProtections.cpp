#include "StdAfx.h"
#include "BoneProtections.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#include "game_type.h"

float SBoneProtections::getBoneProtection(s16 bone_id) const
{
    const auto it = m_bones_koeff.find(bone_id);
    if (it != m_bones_koeff.end())
        return it->second.koeff;

    return m_bones_koeff[BI_NONE].koeff;
}

float SBoneProtections::getBoneArmor(s16 bone_id) const
{
    const auto it = m_bones_koeff.find(bone_id);
    if (it != m_bones_koeff.end())
        return it->second.armor;

    return m_bones_koeff[BI_NONE].armor;
}

bool SBoneProtections::getBonePassBullet(s16 bone_id) const
{
    const auto it = m_bones_koeff.find(bone_id);
    if (it != m_bones_koeff.end())
        return it->second.BonePassBullet;

    return m_bones_koeff[BI_NONE].BonePassBullet;
}

void SBoneProtections::reload(const shared_str& bone_sect, IKinematics* kinematics)
{
    VERIFY(kinematics);

    if (m_hitFracType != HitFractionActorCS && m_hitFracType != HitFractionActorCOP)
    {
        if (pSettings->line_exist(bone_sect, HIT_FRACTION_NPC))
        {
            m_hitFracType = HitFractionNPC;
            m_fHitFrac = pSettings->r_float(bone_sect, HIT_FRACTION_NPC);
        }
        else if (pSettings->line_exist(bone_sect, HIT_FRACTION))
        {
            m_hitFracType = HitFraction;
            m_fHitFrac = pSettings->r_float(bone_sect, HIT_FRACTION);
        }
        else
        {
            const bool is_cop = GMLib.GetLibraryVersion() >= GAMEMTL_VERSION_COP;

            m_hitFracType     = is_cop ? HitFractionNPC : HitFraction;
            m_fHitFrac        = 0.1f;
        }
    }

    m_bones_koeff.clear();
    m_bones_koeff[BI_NONE] = BoneProtection{};

    CInifile::Sect& protections = pSettings->r_section(bone_sect);
    for (const auto& [name, value] : protections.Data)
    {
        if (xr_strcmp(name.c_str(), HIT_FRACTION) == 0)
            continue;
        if (xr_strcmp(name.c_str(), HIT_FRACTION_NPC) == 0)
            continue;

        string256 buffer;

        BoneProtection BP
        {
            .koeff = (float)atof(_GetItem(value.c_str(), 0, buffer)),
            .armor = (float)atof(_GetItem(value.c_str(), 1, buffer)),
            .BonePassBullet = (bool)(atof(_GetItem(value.c_str(), 2, buffer)) > 0.5f),
        };

        if (xr_strcmp(name.c_str(), "default") == 0)
        {
            m_bones_koeff[BI_NONE] = BP;
        }
        else
        {
            if (!kinematics)
                continue;

            s16 bone_id = kinematics->LL_BoneID(name);
            // TODO: fix that warning
            // warning: result of comparison of constant 65535 with expression of type 's16' (aka 'short') is always true
            R_ASSERT2(BI_NONE != bone_id, name.c_str());
            m_bones_koeff.emplace(bone_id, BP);
        }
    }
}

void SBoneProtections::add(const shared_str& bone_sect, IKinematics* kinematics)
{
    if (!IsGameTypeSingle())
        return;

    VERIFY(kinematics);

    if (m_hitFracType == HitFractionNPC)
        m_fHitFrac += pSettings->read_if_exists<float>(bone_sect, HIT_FRACTION_NPC, 0.0f);
    else if (m_hitFracType == HitFraction)
        m_fHitFrac += pSettings->read_if_exists<float>(bone_sect, HIT_FRACTION, 0.0f);

    CInifile::Sect& protections = pSettings->r_section(bone_sect);
    for (const auto& [name, value] : protections.Data)
    {
        if (xr_strcmp(name.c_str(), HIT_FRACTION) == 0)
            continue;
        if (xr_strcmp(name.c_str(), HIT_FRACTION_NPC) == 0)
            continue;

        string256 buffer;
        if (xr_strcmp(name.c_str(), "default") == 0)
        {
            BoneProtection& BP = m_bones_koeff[BI_NONE];
            BP.koeff += (float)atof(_GetItem(value.c_str(), 0, buffer));
            BP.armor += (float)atof(_GetItem(value.c_str(), 1, buffer));
        }
        else
        {
            if (!kinematics)
                continue;
            s16 bone_id = kinematics->LL_BoneID(name);
            // TODO: fix that warning
            // warning: result of comparison of constant 65535 with expression of type 's16' (aka 'short') is always true
            R_ASSERT2(BI_NONE != bone_id, name.c_str());
            BoneProtection& BP = m_bones_koeff[bone_id];
            BP.koeff += (float)atof(_GetItem(value.c_str(), 0, buffer));
            BP.armor += (float)atof(_GetItem(value.c_str(), 1, buffer));
        }
    }
}
