#pragma once

class IKinematics;

struct SBoneProtections
{
    static constexpr cpcstr HIT_FRACTION     = "hit_fraction";
    static constexpr cpcstr HIT_FRACTION_NPC = "hit_fraction_npc";

    enum HitFractionType : u8
    {
        // Exists in SOC and CS
        HitFraction,

        // Introduced in CS, externally assigned
        // Used in outfits instead of HitFraction
        HitFractionActorCS,

        // Introduced in COP
        // Used for NPCs to distinguish from hit_frac_monster,
        // which is non-relevant to this struct
        HitFractionNPC,

        // Hit formula changed in COP, externally assigned
        HitFractionActorCOP,
    };

    struct BoneProtection
    {
        float koeff{ 1.0f };
        float armor{ 0.0f };
        bool BonePassBullet{};
    };

    mutable xr_map<s16, BoneProtection> m_bones_koeff;

    float m_fHitFrac{ 0.1f };
    HitFractionType m_hitFracType{ HitFractionNPC };

    [[nodiscard]] float getBoneProtection(s16 bone_id) const;
    [[nodiscard]] float getBoneArmor(s16 bone_id) const;
    [[nodiscard]] bool  getBonePassBullet(s16 bone_id) const;

    void reload(const shared_str& outfit_section, IKinematics* kinematics);
    void add(const shared_str& outfit_section, IKinematics* kinematics);
};
