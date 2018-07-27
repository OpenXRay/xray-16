#pragma once
#include "firedeps.h"

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "actor_defs.h"

class player_hud;
class CHudItem;
class CMotionDef;

struct motion_descr
{
    MotionID mid;
    shared_str name;
};

struct player_hud_motion
{
    shared_str m_alias_name;
    shared_str m_base_name;
    shared_str m_additional_name;
    xr_vector<motion_descr> m_animations;
};

struct player_hud_motion_container
{
    xr_vector<player_hud_motion> m_anims;
    player_hud_motion* find_motion(const shared_str& name);
    void load(IKinematicsAnimated* model, const shared_str& sect);
};

struct hud_item_measures
{
    enum
    {
        e_fire_point = (1 << 0),
        e_fire_point2 = (1 << 1),
        e_shell_point = (1 << 2),
        e_16x9_mode_now = (1 << 3)
    };
    Flags8 m_prop_flags;

    Fvector m_item_attach[2]; // pos,rot

    Fvector m_hands_offset[2][3]; // pos,rot/ normal,aim,GL

    u16 m_fire_bone;
    Fvector m_fire_point_offset;
    u16 m_fire_bone2;
    Fvector m_fire_point2_offset;
    u16 m_shell_bone;
    Fvector m_shell_point_offset;

    Fvector m_hands_attach[2]; // pos,rot

    void load(const shared_str& sect_name, IKinematics* K);
	
    struct inertion_params
    {
        float m_pitch_offset_r;
        float m_pitch_offset_n;
        float m_pitch_offset_d;
        float m_pitch_low_limit;
        float m_origin_offset;
        float m_origin_offset_aim;
        float m_tendto_speed;
        float m_tendto_speed_aim;
    };
    inertion_params m_inertion_params; //--#SM+#--	
};

struct attachable_hud_item
{
    player_hud* m_parent;
    CHudItem* m_parent_hud_item;
    shared_str m_sect_name;
    IKinematics* m_model;
    u16 m_attach_place_idx;
    hud_item_measures m_measures;

    // runtime positioning
    Fmatrix m_attach_offset;
    Fmatrix m_item_transform;

    player_hud_motion_container m_hand_motions;

    attachable_hud_item(player_hud* pparent) : m_parent(pparent), m_upd_firedeps_frame(u32(-1)),
                                               m_parent_hud_item(nullptr), m_model(nullptr),
                                               m_attach_place_idx(0) {}

    ~attachable_hud_item();
    void load(const shared_str& sect_name);
    void update(bool bForce);
    void update_hud_additional(Fmatrix& trans);
    void setup_firedeps(firedeps& fd);
    void render();
    void render_item_ui();
    bool render_item_ui_query();
    bool need_renderable();
    void set_bone_visible(const shared_str& bone_name, BOOL bVisibility, BOOL bSilent = FALSE);
    void debug_draw_firedeps();

    // hands bind position
    Fvector& hands_attach_pos();
    Fvector& hands_attach_rot();

    // hands runtime offset
    Fvector& hands_offset_pos();
    Fvector& hands_offset_rot();

    // props
    u32 m_upd_firedeps_frame;
    void tune(Ivector values);
    u32 anim_play(const shared_str& anim_name, BOOL bMixIn, const CMotionDef*& md, u8& rnd);
};

class player_hud
{
public:
    player_hud();
    ~player_hud();
    void load(const shared_str& model_name);
    void load_default() { load("actor_hud_05"); };
    void update(const Fmatrix& trans);
    void render_hud();
    void render_item_ui();
    bool render_item_ui_query();
    u32 anim_play(u16 part, const MotionID& M, BOOL bMixIn, const CMotionDef*& md, float speed);
    const shared_str& section_name() const { return m_sect_name; }
    attachable_hud_item* create_hud_item(const shared_str& sect);

    void attach_item(CHudItem* item);
    bool allow_activation(CHudItem* item);
    attachable_hud_item* attached_item(u16 item_idx) { return m_attached_items[item_idx]; };
    void detach_item_idx(u16 idx);
    void detach_item(CHudItem* item);
    void detach_all_items()
    {
        m_attached_items[0] = NULL;
        m_attached_items[1] = NULL;
    };

    void calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result);
    void tune(Ivector values);
    u32 motion_length(const MotionID& M, const CMotionDef*& md, float speed);
    u32 motion_length(const shared_str& anim_name, const shared_str& hud_name, const CMotionDef*& md);
    void OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd);

private:
    void update_inertion(Fmatrix& trans);
    void update_additional(Fmatrix& trans);
    bool inertion_allowed();

private:
    const Fvector& attach_rot() const;
    const Fvector& attach_pos() const;

    shared_str m_sect_name;

    Fmatrix m_attach_offset;

    Fmatrix m_transform;
    IKinematicsAnimated* m_model;
    xr_vector<u16> m_ancors;
    attachable_hud_item* m_attached_items[2];
    xr_vector<attachable_hud_item*> m_pool;
};

extern player_hud* g_player_hud;
