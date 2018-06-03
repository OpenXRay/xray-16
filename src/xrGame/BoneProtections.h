#pragma once

class IKinematics;

struct SBoneProtections
{
    struct BoneProtection
    {
        float koeff;
        float armor;
        BOOL BonePassBullet;
    };
    float m_fHitFracNpc;
    float m_fHitFracActor;
    typedef xr_map<s16, BoneProtection> storage_type;
    typedef storage_type::iterator storage_it;
    SBoneProtections() : m_fHitFracNpc(0)
    {
        m_default.koeff = 1.0f;
        m_default.armor = 0;
        m_fHitFracActor = 0.1f;
    }

    BoneProtection m_default;
    storage_type m_bones_koeff;
    void reload(const shared_str& outfit_section, IKinematics* kinematics);
    void add(const shared_str& outfit_section, IKinematics* kinematics);
    float getBoneProtection(s16 bone_id);
    float getBoneArmor(s16 bone_id);
    BOOL getBonePassBullet(s16 bone_id);
};
