#pragma once

#include "WeaponCustomPistol.h"
#include "xrEngine/xr_collide_form.h"
#include "xrCore/buffer_vector.h"

class CWeaponKnife : public CWeapon
{
private:
    typedef CWeapon inherited;

protected:
    virtual void switch2_Idle();
    virtual void switch2_Hiding();
    virtual void switch2_Hidden();
    virtual void switch2_Showing();
    void switch2_Attacking(u32 state);

    virtual void OnAnimationEnd(u32 state);
    virtual void OnMotionMark(u32 state, const motion_marks&);
    virtual void OnStateSwitch(u32 S, u32 oldState);

    void state_Attacking(float dt);

    virtual void KnifeStrike(const Fvector& pos, const Fvector& dir);

    float fWallmarkSize;
    u16 knife_material_idx;

protected:
    ALife::EHitType m_eHitType;

    ALife::EHitType m_eHitType_1;
    Fvector4 fvHitPower_1;
    Fvector4 fvHitPowerCritical_1;
    float fHitImpulse_1;

    ALife::EHitType m_eHitType_2;
    Fvector4 fvHitPower_2;
    Fvector4 fvHitPowerCritical_2;
    float fHitImpulse_2;

    float fCurrentHit;

    float fHitImpulse_cur;

protected:
    virtual void LoadFireParams(LPCSTR section);

public:
    CWeaponKnife();
    virtual ~CWeaponKnife();

    void Load(LPCSTR section);

    virtual bool IsZoomEnabled() const { return false; }
    void Fire2Start();
    virtual void FireStart();

    virtual bool Action(u16 cmd, u32 flags);

    virtual bool GetBriefInfo(II_BriefInfo& info);

#ifdef DEBUG
    virtual void OnRender();
#endif

private:
    typedef buffer_vector<Fvector> shot_targets_t;
#ifdef DEBUG
    struct dbg_draw_data
    {
        typedef xr_vector<std::pair<Fvector, float>> spheres_t;
        typedef xr_vector<Fobb> obbes_t;
        typedef xr_vector<std::pair<Fvector, Fvector>> lines_t;
        typedef xr_vector<Fvector> targets_t;

        spheres_t m_spheres;
        lines_t m_pick_vectors;
        targets_t m_targets_vectors;
        obbes_t m_target_boxes;
    };
    dbg_draw_data m_dbg_data;
#endif
    float m_Hit1Distance;
    float m_Hit2Distance;

    Fvector3 m_Hit1SpashDir;
    Fvector3 m_Hit2SpashDir;

    float m_Hit1SplashRadius;
    float m_Hit2SplashRadius;

    shared_str m_SplashHitBone;

    u32 m_Splash1HitsCount;
    u32 m_Splash1PerVictimsHCount;
    u32 m_Splash2HitsCount;
    float m_NextHitDivideFactor;

    float m_hit_dist;
    Fvector3 m_splash_dir;
    float m_splash_radius;
    u32 m_hits_count;
    u32 m_perv_hits_count;

    void MakeShot(Fvector const& pos, Fvector const& dir, float const k_hit = 1.0f);
    void GetVictimPos(CEntityAlive* victim, Fvector& pos_dest);
    u32 SelectHitsToShot(shot_targets_t& dst_dirs, Fvector const& f_pos);
    bool SelectBestHitVictim(
        Fvector const& f_pos, Fmatrix& parent_xform_dest, Fvector& fendpos_dest, Fsphere& query_sphere);
    IGameObject* TryPick(Fvector const& start_pos, Fvector const& dir, float const dist);

    static BOOL RayQueryCallback(collide::rq_result& result, LPVOID this_ptr);
    collide::rq_results m_ray_query_results;
    u16 m_except_id;
    IGameObject* m_last_picked_obj;

    typedef xr_vector<ISpatial*> spartial_base_t;
    typedef buffer_vector<CEntityAlive*> victims_list_t;
    struct victim_bone_data
    {
        CCF_Skeleton::SElement const* m_bone_element;
        u16 m_victim_id;
        u16 m_shots_count;
    }; // struct	victim_bone_data
    typedef AssociativeVector<u16, u16> victims_hits_count_t;
    typedef buffer_vector<std::pair<victim_bone_data, float>> victims_shapes_list_t;

    spartial_base_t m_spartial_query_res;
    victims_hits_count_t m_victims_hits_count;

    class victim_filter
    {
    public:
        victim_filter(u16 except_id, Fvector const& pos, float query_distance);
        victim_filter(victim_filter const& copy);
        bool operator()(spartial_base_t::value_type const& left) const;

    private:
        victim_filter& operator=(victim_filter const& copy) = delete;

        u16 m_except_id;
        CWeaponKnife* m_owner;
        Fvector m_start_pos;
        float m_query_distance;
    }; // class victim_filter
    class best_victim_selector
    {
    public:
        best_victim_selector(
            u16 except_id, Fvector const& pos, float query_distance, spartial_base_t::value_type& dest_result);

        best_victim_selector(best_victim_selector const& copy);
        void operator()(spartial_base_t::value_type const& left);

    private:
        best_victim_selector& operator=(best_victim_selector const& copy) = delete;

        Fvector m_start_pos;
        float m_min_dist;
        float m_query_distance;
        u16 m_except_id;
        spartial_base_t::value_type& m_dest_result;
    }; // struct best_victim_selector

    static bool shapes_compare_predicate(
        victims_shapes_list_t::value_type const& left, victims_shapes_list_t::value_type const& right)
    {
        return left.second < right.second;
    }

    static void create_victims_list(spartial_base_t spartial_result, victims_list_t& victims_dest);
    static u32 get_entity_bones_count(CEntityAlive const* entity);
    void fill_shapes_list(CEntityAlive const* entity, Fvector const& camera_endpos, victims_shapes_list_t& dest_shapes);
    void make_hit_sort_vectors(Fvector& basis_hit_specific, float& max_dist);
    void fill_shots_list(victims_shapes_list_t& victims_shapres, Fsphere const& query, shot_targets_t& dest_shots);
};
