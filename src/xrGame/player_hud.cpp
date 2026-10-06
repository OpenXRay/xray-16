#include "StdAfx.h"
#include "player_hud.h"
#include "HudItem.h"
#include "xrUICore/ui_base.h"
#include "Actor.h"
#include "physic_item.h"
#include "static_cast_checked.hpp"
#include "ActorEffector.h"
#include "WeaponMagazinedWGrenade.h" // XXX: move somewhere
#include "GamePersistent.h"
#include "ik/HudIKController.h"

player_hud* g_player_hud = nullptr;
extern ENGINE_API shared_str current_player_hud_sect;
extern ENGINE_API float psHUD_FOV;

namespace
{
constexpr float CollisionMinCorrectionSq = 1e-10f;
}

// --#SM+# Begin--
constexpr float PITCH_OFFSET_R    = 0.0f;   // Насколько сильно ствол смещается вбок (влево) при вертикальных поворотах камеры
constexpr float PITCH_OFFSET_N    = 0.0f;   // Насколько сильно ствол поднимается\опускается при вертикальных поворотах камеры
constexpr float PITCH_OFFSET_D    = 0.02f;  // Насколько сильно ствол приближается\отдаляется при вертикальных поворотах камеры
constexpr float PITCH_LOW_LIMIT   = -PI;    // Минимальное значение pitch при использовании совместно с PITCH_OFFSET_N
constexpr float ORIGIN_OFFSET     = -0.05f; // Фактор влияния инерции на положение ствола (чем меньше, тем масштабней инерция)
constexpr float ORIGIN_OFFSET_AIM = -0.03f; // (Для прицеливания)
constexpr float TENDTO_SPEED      = 5.f;    // Скорость нормализации положения ствола
constexpr float TENDTO_SPEED_AIM  = 8.f;    // (Для прицеливания)
// --#SM+# End--

float CalcMotionSpeed(const shared_str& anim_name, const float anim_speed)
{
    // Apply custom animation speeds / configuration only for singleplayer games.
    // Fast reloading / showing / hiding animation does not seem fair.
    if (IsGameTypeSingle())
        return anim_speed;
    else
        return (anim_name == "anm_show" || anim_name == "anm_hide") ? 2.0f : 1.0f;
}

const player_hud_motion* player_hud_motion_container::find_motion(const shared_str& name) const
{
    const auto it = m_anims.find(name);
    return it != m_anims.end() ? &it->second : nullptr;
}

void player_hud_motion_container::load(IKinematicsAnimated* model, const shared_str& sect)
{
    const CInifile::Sect& _sect = pSettings->r_section(sect);

    for (const auto& [name, anm] : _sect.Data)
    {
        if (0 == strncmp(name.c_str(), "anm_",  sizeof("anm_")  - 1) ||
            0 == strncmp(name.c_str(), "anim_", sizeof("anim_") - 1))
        {
            player_hud_motion pm;

            if (_GetItemCount(anm.c_str()) == 1)
            {
                pm.m_base_name = anm;
                pm.m_additional_name = anm;
                pm.m_anim_speed = 1.f;
            }
            else
            {
                R_ASSERT2(_GetItemCount(anm.c_str()) <= 3, anm.c_str());
                string512 str_item;
                _GetItem(anm.c_str(), 0, str_item);
                pm.m_base_name = str_item;

                _GetItem(anm.c_str(), 1, str_item);
                pm.m_additional_name = xr_strlen(str_item) > 0 ? str_item : pm.m_base_name;

                _GetItem(anm.c_str(), 2, str_item);
                pm.m_anim_speed = xr_strlen(str_item) > 0 ? atof(str_item) : 1.f;
            }

            // and load all motions for it
            for (u32 i = 0; i <= 8; ++i)
            {
                string512 buff;
                if (i == 0)
                    xr_strcpy(buff, pm.m_base_name.c_str());
                else
                    xr_sprintf(buff, "%s%d", pm.m_base_name.c_str(), i);

                MotionID motion_ID = model->ID_Cycle_Safe(buff);
                if (motion_ID.valid())
                {
                    pm.m_animations.emplace_back(motion_descr{ std::move(motion_ID), buff });
#ifdef DEBUG
//					Msg(" alias=[%s] base=[%s] name=[%s]",pm.m_alias_name.c_str(), pm.m_base_name.c_str(), buff);
#endif // #ifdef DEBUG
                }
            }
            R_ASSERT2(!pm.m_animations.empty(), make_string("motion not found [%s]", pm.m_base_name.c_str()).c_str());

            m_anims.emplace(name, std::move(pm));
        }
    }
}

Fvector& attachable_hud_item::hands_attach_pos() { return m_measures.m_hands_attach[0]; }
Fvector& attachable_hud_item::hands_attach_rot() { return m_measures.m_hands_attach[1]; }

Fvector& attachable_hud_item::hands_offset_pos()
{
    const u8 idx = m_parent_hud_item->GetCurrentHudOffsetIdx();
    return m_measures.m_hands_offset[0][idx];
}

Fvector& attachable_hud_item::hands_offset_rot()
{
    u8 idx = m_parent_hud_item->GetCurrentHudOffsetIdx();
    return m_measures.m_hands_offset[1][idx];
}

void attachable_hud_item::set_bone_visible(const shared_str& bone_name, BOOL bVisibility, BOOL bSilent)
{
    const u16 bone_id = m_model->LL_BoneID(bone_name);
    if (bone_id == BI_NONE)
    {
        if (bSilent)
            return;
        R_ASSERT2(false, make_string("model [%s] has no bone [%s]", m_visual_name.c_str(), bone_name.c_str()).c_str());
    }
    const BOOL bVisibleNow = m_model->LL_GetBoneVisible(bone_id);
    if (bVisibleNow != bVisibility)
        m_model->LL_SetBoneVisible(bone_id, bVisibility, TRUE);
}

void attachable_hud_item::update(bool bForce)
{
    if (!bForce && m_upd_firedeps_frame == Device.dwFrame)
        return;

    refresh_measures();

    m_parent->calc_transform(m_attach_place_idx, m_attach_offset, m_item_transform);
    m_upd_firedeps_frame = Device.dwFrame;

    if (IKinematicsAnimated* ka = m_model->dcast_PKinematicsAnimated())
    {
        ka->UpdateTracks();
        ka->dcast_PKinematics()->CalculateBones_Invalidate();
        ka->dcast_PKinematics()->CalculateBones(TRUE);
    }
}

void attachable_hud_item::refresh_measures()
{
    const bool is_16x9 = UICore::is_widescreen();

    if (m_measures.m_prop_flags.test(hud_item_measures::e_16x9_mode_now) != is_16x9)
    {
        reload_measures();
    }

    if (GamePersistent().GetHudTuner().is_active())
        m_measures.update(m_attach_offset);
}

void attachable_hud_item::update_hud_additional(Fmatrix& trans) const
{
    if (m_parent_hud_item)
    {
        m_parent_hud_item->UpdateHudAdditonal(trans);
    }
}

void attachable_hud_item::setup_firedeps(firedeps& fd)
{
    update(false);
    // fire point&direction
    if (m_measures.m_prop_flags.test(hud_item_measures::e_fire_point))
    {
        Fmatrix& fire_mat = m_model->LL_GetTransform(m_measures.m_fire_bone);
        fire_mat.transform_tiny(fd.vLastFP, m_measures.m_fire_point_offset);
        m_item_transform.transform_tiny(fd.vLastFP);

        fd.vLastFD.set(0.f, 0.f, 1.f);
        if (m_hud_ik)
            m_hud_ik->TransformGunDirection(fd.vLastFD);
        m_item_transform.transform_dir(fd.vLastFD);
        VERIFY(_valid(fd.vLastFD));
        VERIFY(_valid(fd.vLastFD));

        fd.m_FireParticlesXForm.identity();
        fd.m_FireParticlesXForm.k.set(fd.vLastFD);
        Fvector::generate_orthonormal_basis_normalized(
            fd.m_FireParticlesXForm.k, fd.m_FireParticlesXForm.j, fd.m_FireParticlesXForm.i);
        VERIFY(_valid(fd.m_FireParticlesXForm));
    }

    if (m_measures.m_prop_flags.test(hud_item_measures::e_fire_point2))
    {
        Fmatrix& fire_mat = m_model->LL_GetTransform(m_measures.m_fire_bone2);
        fire_mat.transform_tiny(fd.vLastFP2, m_measures.m_fire_point2_offset);
        m_item_transform.transform_tiny(fd.vLastFP2);
        VERIFY(_valid(fd.vLastFP2));
        VERIFY(_valid(fd.vLastFP2));
    }

    if (m_measures.m_prop_flags.test(hud_item_measures::e_light_point))
    {
        Fmatrix& light_mat = m_model->LL_GetTransform(m_measures.m_light_bone);
        light_mat.transform_tiny(fd.vLastLP, m_measures.m_light_point_offset);
        m_item_transform.transform_tiny(fd.vLastLP);
        VERIFY(_valid(fd.vLastLP));
    }

    if (m_measures.m_prop_flags.test(hud_item_measures::e_shell_point))
    {
        Fmatrix& fire_mat = m_model->LL_GetTransform(m_measures.m_shell_bone);
        fire_mat.transform_tiny(fd.vLastSP, m_measures.m_shell_point_offset);
        m_item_transform.transform_tiny(fd.vLastSP);
        VERIFY(_valid(fd.vLastSP));
        VERIFY(_valid(fd.vLastSP));
    }
}

bool attachable_hud_item::need_renderable() const { return m_parent_hud_item->need_renderable(); }

void attachable_hud_item::render(u32 context_id, IRenderable* root)
{
    GEnv.Render->add_Visual(context_id, root, m_model->dcast_RenderVisual(), m_item_transform);
    m_parent_hud_item->render_hud_mode();
}

bool attachable_hud_item::render_item_ui_query() const { return m_parent_hud_item->render_item_3d_ui_query(); }
void attachable_hud_item::render_item_ui() const { m_parent_hud_item->render_item_3d_ui(); }

Fmatrix hud_item_measures::load(const shared_str& sect_name, IKinematics* K)
{
    const bool is_16x9 = UICore::is_widescreen();
    string64 _prefix;
    xr_sprintf(_prefix, "%s", is_16x9 ? "_16x9" : "");
    string128 val_name;

    strconcat(val_name, "hands_position", _prefix);
    m_hands_attach[0] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(val_name, "hands_orientation", _prefix);
    m_hands_attach[1] = pSettings->r_fvector3(sect_name, val_name);

    m_item_attach[0] = pSettings->r_fvector3(sect_name, "item_position");
    m_item_attach[1] = pSettings->r_fvector3(sect_name, "item_orientation");

    Fmatrix attach_offset;
    update(attach_offset);

    shared_str bone_name;
    m_prop_flags.set(e_fire_point, pSettings->line_exist(sect_name, "fire_bone"));
    if (m_prop_flags.test(e_fire_point))
    {
        bone_name = pSettings->r_string(sect_name, "fire_bone");
        m_fire_bone = K->LL_BoneID(bone_name);
        m_fire_point_offset = pSettings->r_fvector3(sect_name, "fire_point");
    }
    else
        m_fire_point_offset = {};

    m_prop_flags.set(e_fire_point2, pSettings->line_exist(sect_name, "fire_bone2"));
    if (m_prop_flags.test(e_fire_point2))
    {
        bone_name = pSettings->r_string(sect_name, "fire_bone2");
        m_fire_bone2 = K->LL_BoneID(bone_name);
        m_fire_point2_offset = pSettings->r_fvector3(sect_name, "fire_point2");
    }
    else
        m_fire_point2_offset = {};

    const bool explicit_light = pSettings->line_exist(sect_name, "light_bone");
    if (explicit_light)
    {
        m_light_bone = K->LL_BoneID(pSettings->r_string(sect_name, "light_bone"));
        m_light_point_offset = pSettings->read_if_exists<Fvector3>(sect_name, "light_point", m_fire_point_offset);
    }
    else if (m_prop_flags.test(e_fire_point))
    {
        m_light_bone = m_fire_bone;
        m_light_point_offset = pSettings->read_if_exists<Fvector3>(sect_name, "light_point", m_fire_point_offset);
    }
    else
        m_light_point_offset = {};
    m_prop_flags.set(e_light_point, explicit_light || m_prop_flags.test(e_fire_point));
    if (m_prop_flags.test(e_light_point) && !pSettings->line_exist(sect_name, "light_point"))
        m_light_point_offset.x = -0.05f;

    m_prop_flags.set(e_shell_point, pSettings->line_exist(sect_name, "shell_bone"));
    if (m_prop_flags.test(e_shell_point))
    {
        bone_name = pSettings->r_string(sect_name, "shell_bone");
        m_shell_bone = K->LL_BoneID(bone_name);
        m_shell_point_offset = pSettings->r_fvector3(sect_name, "shell_point");
    }
    else
        m_shell_point_offset = {};

    m_hands_offset[0][0] = {};
    m_hands_offset[1][0] = {};

    strconcat(val_name, "aim_hud_offset_pos", _prefix);
    m_hands_offset[0][1] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(val_name, "aim_hud_offset_rot", _prefix);
    m_hands_offset[1][1] = pSettings->r_fvector3(sect_name, val_name);

    strconcat(val_name, "gl_hud_offset_pos", _prefix);
    m_hands_offset[0][2] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(val_name, "gl_hud_offset_rot", _prefix);
    m_hands_offset[1][2] = pSettings->r_fvector3(sect_name, val_name);

    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point") == pSettings->line_exist(sect_name, "fire_bone"),
        sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point2") == pSettings->line_exist(sect_name, "fire_bone2"),
        sect_name.c_str());
    R_ASSERT2(!pSettings->line_exist(sect_name, "light_point") || m_prop_flags.test(e_light_point),
        sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "shell_point") == pSettings->line_exist(sect_name, "shell_bone"),
        sect_name.c_str());

    load_inertion_params(sect_name);
    m_prop_flags.set(e_16x9_mode_now, is_16x9);

    return attach_offset;
}

Fmatrix hud_item_measures::load_monolithic(const shared_str& sect_name, IKinematics* K, CHudItem* owner)
{
    m_item_attach[0] = pSettings->r_fvector3(sect_name, "position");
    m_item_attach[1] = pSettings->r_fvector3(sect_name, "orientation");

    Fmatrix attach_offset;
    update(attach_offset);

    // fire bone
    if (auto* wpn = smart_cast<CWeapon*>(owner))
    {
        cpcstr fire_bone = pSettings->r_string(sect_name, "fire_bone");
        m_fire_bone = K->LL_BoneID(fire_bone);
        if (m_fire_bone >= K->LL_BoneCount())
            xrDebug::Fatal(DEBUG_INFO, "There is no '%s' bone for weapon '%s'.", fire_bone, sect_name.c_str());
        m_fire_bone2 = m_fire_bone;
        m_shell_bone = m_fire_bone;

        m_fire_point_offset = pSettings->r_fvector3(sect_name, "fire_point");
        m_fire_point2_offset = pSettings->read_if_exists<Fvector3>(sect_name, "fire_point2", m_fire_point_offset);

        const bool explicit_light_bone = pSettings->line_exist(sect_name, "light_bone");
        m_light_bone = m_fire_bone;
        if (explicit_light_bone)
        {
            cpcstr light_bone = pSettings->r_string(sect_name, "light_bone");
            m_light_bone = K->LL_BoneID(light_bone);
            if (m_light_bone >= K->LL_BoneCount())
                xrDebug::Fatal(DEBUG_INFO, "There is no '%s' bone for weapon '%s'.", light_bone, sect_name.c_str());
        }
        m_light_point_offset = pSettings->read_if_exists<Fvector3>(sect_name, "light_point", m_fire_point_offset);
        if (!pSettings->line_exist(sect_name, "light_point"))
            m_light_point_offset.x = -0.05f;

        m_prop_flags.set(e_fire_point | e_fire_point2 | e_light_point | e_shell_point, true);

        if (pSettings->line_exist(owner->object().cNameSect(), "shell_particles"))
            m_shell_point_offset = pSettings->r_fvector3(sect_name, "shell_point");
        else
            m_shell_point_offset.set(0, 0, 0);

        m_hands_offset[0][0] = {};
        m_hands_offset[1][0] = {};

        if (wpn->IsZoomEnabled())
        {
            const auto load_zoom_offsets = [&](pcstr prefix, Fvector3& position, Fvector3& rotation)
            {
                string256 full_name;
                position = pSettings->r_fvector3(sect_name, strconcat(full_name, prefix, "zoom_offset"));
                rotation.x = pSettings->r_float(sect_name, strconcat(full_name, prefix, "zoom_rotate_x"));
                rotation.y = pSettings->r_float(sect_name, strconcat(full_name, prefix, "zoom_rotate_y"));
                rotation.z = pSettings->read_if_exists<float>(sect_name, strconcat(full_name, prefix, "zoom_rotate_z"), 0.f);
            };
            load_zoom_offsets("", m_hands_offset[0][1], m_hands_offset[1][1]);
            if (smart_cast<CWeaponMagazinedWGrenade*>(wpn))
            {
                load_zoom_offsets("grenade_", m_hands_offset[0][2], m_hands_offset[1][2]);
                if (wpn->GrenadeLauncherAttachable())
                    load_zoom_offsets("grenade_normal_", m_hands_offset[0][1], m_hands_offset[1][1]);
            }
        }
    }
    else
    {
        m_prop_flags.set(e_fire_point | e_fire_point2 | e_light_point | e_shell_point, false);

        m_fire_bone  = BI_NONE;
        m_fire_bone2 = BI_NONE;
        m_light_bone = BI_NONE;
        m_shell_bone = BI_NONE;

        m_fire_point_offset  = {};
        m_fire_point2_offset = {};
        m_light_point_offset = {};
        m_shell_point_offset = {};
    }

    load_inertion_params(sect_name);
    m_prop_flags.set(e_16x9_mode_now, UICore::is_widescreen());

    return attach_offset;
}

void hud_item_measures::load_inertion_params(const shared_str& sect_name)
{
    //Загрузка параметров инерции --#SM+# Begin--
    m_inertion_params.m_pitch_offset_r = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_right", PITCH_OFFSET_R);
    m_inertion_params.m_pitch_offset_n = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up", PITCH_OFFSET_N);
    m_inertion_params.m_pitch_offset_d = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_forward", PITCH_OFFSET_D);
    m_inertion_params.m_pitch_low_limit = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up_low_limit", PITCH_LOW_LIMIT);

    m_inertion_params.m_origin_offset = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_offset", ORIGIN_OFFSET);
    m_inertion_params.m_origin_offset_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_aim_offset", ORIGIN_OFFSET_AIM);
    m_inertion_params.m_tendto_speed = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_speed", TENDTO_SPEED);
    m_inertion_params.m_tendto_speed_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_aim_speed", TENDTO_SPEED_AIM);
    //--#SM+# End--
}

void hud_item_measures::update(Fmatrix& attach_offset)
{
    Fvector ypr = m_item_attach[1];
    ypr.mul(PI / 180.f);
    attach_offset.setHPB(ypr.x, ypr.y, ypr.z);
    attach_offset.translate_over(m_item_attach[0]);
}

attachable_hud_item::~attachable_hud_item()
{
    if (m_hud_ik)
    {
        m_hud_ik->Unbind();
        xr_delete(m_hud_ik);
    }
    IRenderVisual* v = m_model->dcast_RenderVisual();
    GEnv.Render->model_Delete(v);
}

attachable_hud_item::attachable_hud_item(player_hud* parent, const shared_str& sect_name, IKinematicsAnimated* hands_model)
    : m_parent(parent), m_sect_name(sect_name)
{
    // Visual
    if (pSettings->line_exist(m_sect_name, "item_visual"))
    {
        m_monolithic = false;
        m_visual_name = pSettings->r_string(m_sect_name, "item_visual");
    }
    else if (pSettings->line_exist(m_sect_name, "visual"))
    {
        m_monolithic = true;
        m_visual_name = pSettings->r_string(m_sect_name, "visual");
    }
    R_ASSERT3(!m_visual_name.empty(), "Missing 'item_visual' from weapon hud section.", m_sect_name.c_str());

    m_model = smart_cast<IKinematics*>(GEnv.Render->model_Create(m_visual_name.c_str()));
    if (m_monolithic && m_model)
    {
        m_hud_ik = xr_new<CHudIKController>();
        m_hud_ik->Bind(m_model);
        m_hud_ik->SetGunLeadCapable(true);
    }

    m_attach_place_idx = pSettings->read_if_exists<u16>(m_sect_name, "attach_place_idx", 0);

    IKinematicsAnimated* animatedHudItem;
    if (!m_monolithic && hands_model)
        animatedHudItem = hands_model;
    else
        animatedHudItem = smart_cast<IKinematicsAnimated*>(m_model);

    m_hand_motions.load(animatedHudItem, m_sect_name);
    reload_measures();
}

void attachable_hud_item::reload_measures()
{
    if (m_monolithic)
    {
        m_attach_offset = m_measures.load_monolithic(m_sect_name, m_model, m_parent_hud_item);
        if (m_hud_ik)
        {
            const bool has_fire = m_measures.m_prop_flags.test(hud_item_measures::e_fire_point);
            const bool has_light = m_measures.m_prop_flags.test(hud_item_measures::e_light_point);
            m_hud_ik->SetGunBones(has_fire ? m_measures.m_fire_bone : BI_NONE,
                has_light ? m_measures.m_light_bone : BI_NONE);
        }
    }
    else
        m_attach_offset = m_measures.load(m_sect_name, m_model);
}

void attachable_hud_item::reset_hud_ik()
{
    if (m_hud_ik)
        m_hud_ik->ResetAll();
}

u32 attachable_hud_item::anim_play(const shared_str& anm_name_b, BOOL bMixIn, const CMotionDef*& md, u8& rnd_idx)
{
    string256 anim_name_r;
    const bool is_16x9 = UICore::is_widescreen();
    xr_sprintf(anim_name_r, "%s%s", anm_name_b.c_str(), m_attach_place_idx == 1 && is_16x9 ? "_16x9" : "");

    const player_hud_motion* anm = m_hand_motions.find_motion(anim_name_r);
    R_ASSERT2(anm, make_string("model [%s] has no motion alias defined [%s]", m_sect_name.c_str(), anim_name_r).c_str());
    R_ASSERT2(anm->m_animations.size(), make_string("model [%s] has no motion defined in motion_alias [%s]",
                                            m_visual_name.c_str(), anim_name_r)
                                            .c_str());

    const float speed = CalcMotionSpeed(anm->m_base_name, anm->m_anim_speed);

    rnd_idx = (u8)Random.randI(anm->m_animations.size());
    const motion_descr& M = anm->m_animations[rnd_idx];

    IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(m_model);
    const u32 ret = m_parent->anim_play(m_attach_place_idx, M.mid, bMixIn, md, speed, m_monolithic ? ka : nullptr);

    if (ka)
    {
        shared_str item_anm_name;
        if (anm->m_base_name != anm->m_additional_name)
            item_anm_name = anm->m_additional_name;
        else
            item_anm_name = M.name;

        MotionID M2 = ka->ID_Cycle_Safe(item_anm_name);
        if (!M2.valid())
            M2 = ka->ID_Cycle_Safe("idle");
        else if (bDebug)
            Msg("playing item animation [%s]", item_anm_name.c_str());

        R_ASSERT3(M2.valid(), "model has no motion [idle] ", m_visual_name.c_str());

        if (!m_monolithic)
        {
            const u16 root_id = m_model->LL_GetBoneRoot();
            CBoneInstance& root_binst = m_model->LL_GetBoneInstance(root_id);
            root_binst.set_callback_overwrite(TRUE);
            root_binst.mTransform.identity();
        }

        const u16 pc = ka->partitions().count();
        for (u16 pid = 0; pid < pc; ++pid)
        {
            CBlend* B = ka->PlayCycle(pid, M2, bMixIn);
            R_ASSERT(B);
            B->speed *= speed;
        }

        m_model->CalculateBones_Invalidate();
    }

    R_ASSERT2(m_parent_hud_item, "parent hud item is NULL");
    CPhysicItem& parent_object = m_parent_hud_item->object();
    // R_ASSERT2		(parent_object, "object has no parent actor");
    // IGameObject*		parent_object = static_cast_checked<IGameObject*>(&m_parent_hud_item->object());

    if (IsGameTypeSingle() && parent_object.H_Parent() == Level().CurrentControlEntity())
    {
        CActor* current_actor = static_cast_checked<CActor*>(Level().CurrentControlEntity());
        VERIFY(current_actor);

        string_path ce_path;
        string_path anm_name;
        strconcat(anm_name, "camera_effects" DELIMITER "weapon" DELIMITER, M.name.c_str(), ".anm");
        if (FS.exist(ce_path, "$game_anims$", anm_name))
        {
            CEffectorCam* ec = current_actor->Cameras().GetCamEffector(eCEWeaponAction);
            if (ec)
                current_actor->Cameras().RemoveCamEffector(eCEWeaponAction);

            CAnimatorCamEffector* e = xr_new<CAnimatorCamEffector>();
            e->SetType(eCEWeaponAction);
            e->SetHudAffect(false);
            e->SetCyclic(false);
            e->Start(anm_name);
            current_actor->Cameras().AddCamEffector(e);
        }
    }
    return ret;
}

player_hud::~player_hud()
{
    if (m_hands_ik)
    {
        m_hands_ik->Unbind();
        xr_delete(m_hands_ik);
    }

    if (m_model)
    {
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GEnv.Render->model_Delete(v);
        m_model = nullptr;
    }

    for (auto& [name, item] : m_pool)
    {
        xr_delete(item);
    }
    m_pool.clear();
}

void player_hud::load(const shared_str& player_hud_sect)
{
    if (player_hud_sect == m_sect_name)
        return;

    m_sect_name = player_hud_sect;

    const bool b_reload = m_model != nullptr;

    if (!pSettings->section_exist(m_sect_name))
    {
        if (b_reload)
        {
            if (m_attached_items[1])
                m_attached_items[1]->m_parent_hud_item->on_a_hud_attach();

            if (m_attached_items[0])
                m_attached_items[0]->m_parent_hud_item->on_a_hud_attach();
        }

        return;
    }

    if (m_model)
    {
        if (m_hands_ik)
            m_hands_ik->Unbind();
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GEnv.Render->model_Delete(v);
        m_model = nullptr;
    }
    m_ancors.clear();

    const shared_str& model_name = pSettings->r_string(m_sect_name, "visual");
    m_model = smart_cast<IKinematicsAnimated*>(GEnv.Render->model_Create(model_name.c_str()));
    load_ancors();
    if (!m_hands_ik)
        m_hands_ik = xr_new<CHudIKController>();
    m_hands_ik->Bind(m_model->dcast_PKinematics());
    m_weapon_collision.Reset();
    // Msg("hands visual changed to [%s] [%s] [%s]", model_name.c_str(), b_reload ? "R" : "", m_attached_items[0] ? "Y" : "");

    if (!b_reload)
    {
        m_model->PlayCycle("hand_idle_doun");
    }
    else
    {
        if (m_attached_items[1])
            m_attached_items[1]->m_parent_hud_item->on_a_hud_attach();

        if (m_attached_items[0])
            m_attached_items[0]->m_parent_hud_item->on_a_hud_attach();
    }
    m_model->dcast_PKinematics()->CalculateBones_Invalidate();
    m_model->dcast_PKinematics()->CalculateBones(TRUE);
}

void player_hud::load_ancors()
{
    const CInifile::Sect& _sect = pSettings->r_section(m_sect_name);
    for (const auto& [name, bone] : _sect.Data)
    {
        if (0 == strncmp(name.c_str(), "ancor_", sizeof("ancor_") - 1))
        {
            m_ancors.emplace_back(m_model->dcast_PKinematics()->LL_BoneID(bone));
        }
    }
}

bool player_hud::render_item_ui_query() const
{
    bool res = false;
    if (m_attached_items[0])
        res |= m_attached_items[0]->render_item_ui_query();

    if (m_attached_items[1])
        res |= m_attached_items[1]->render_item_ui_query();

    return res;
}

void player_hud::render_item_ui() const
{
    if (m_attached_items[0])
        m_attached_items[0]->render_item_ui();

    if (m_attached_items[1])
        m_attached_items[1]->render_item_ui();
}

void player_hud::render_hud(u32 context_id, IRenderable* root)
{
    attachable_hud_item* item0 = m_attached_items[0];
    attachable_hud_item* item1 = m_attached_items[1];

    if (!item0 && !item1)
        return;

    const bool b_r0 = item0 && item0->need_renderable();
    const bool b_r1 = item1 && item1->need_renderable();

    if (!b_r0 && !b_r1)
        return;

    if (m_model)
        GEnv.Render->add_Visual(context_id, root, m_model->dcast_RenderVisual(), m_transform);

    if (item0)
        item0->render(context_id, root);

    if (item1)
        item1->render(context_id, root);
}

#include "xrCore/Animation/Motion.hpp"

u32 player_hud::motion_length(const shared_str& anim_name, const shared_str& hud_name, const CMotionDef*& md)
{
    const float speed = CalcMotionSpeed(anim_name, 1.0f);
    attachable_hud_item* pi = create_hud_item(hud_name);
    const player_hud_motion* pm = pi->m_hand_motions.find_motion(anim_name);

    if (!pm)
        return 100; // ms TEMPORARY
    R_ASSERT2(pm,
        make_string("hudItem model [%s] has no motion with alias [%s]", hud_name.c_str(), anim_name.c_str()).c_str());
    IKinematicsAnimated* model = pi->m_monolithic ? smart_cast<IKinematicsAnimated*>(pi->m_model) : nullptr;
    return motion_length(pm->m_animations[0].mid, md, speed, model);
}

u32 player_hud::motion_length(const MotionID& M, const CMotionDef*& md, float speed, IKinematicsAnimated* itemModel) const
{
    IKinematicsAnimated* model = itemModel ? itemModel : m_model;
    md = model->LL_GetMotionDef(M);
    VERIFY(md);
    if (md->flags & esmStopAtEnd)
    {
        return iFloor(0.5f + 1000.f * model->LL_MotionDuration(M) / (md->Speed() * speed));
    }
    return 0;
}

void player_hud::update(const Fmatrix& cam_trans)
{
    clear_collision_offsets();
    m_weapon_collision.Reset();

    Fmatrix trans = cam_trans;
    if (psHUD_Flags.test(HUD_LEFT_HANDED))
    {
        // faster than multiplication by flip matrix
        trans.m[0][0] = -trans.m[0][0];
        trans.m[0][1] = -trans.m[0][1];
        trans.m[0][2] = -trans.m[0][2];
        trans.m[0][3] = -trans.m[0][3];
    }

    update_inertion(trans);
    update_additional(trans);

    attachable_hud_item* item0 = m_attached_items[0];
    attachable_hud_item* item1 = m_attached_items[1];

    if (item0)
        item0->refresh_measures();

    if (item1)
        item1->refresh_measures();

    const bool monolithic = item0 && item0->m_monolithic || item1 && item1->m_monolithic;
    if (!m_model || monolithic)
    {
        m_transform = trans;
        configure_external_gun(nullptr);
        request_collision_snapshot(item0);
    }
    else
    {
        Fvector ypr{};
        if (item0)
            ypr = item0->hands_attach_rot();
        else if (item1)
            ypr = item1->hands_attach_rot();

        ypr.mul(PI / 180.f);
        m_attach_offset.setHPB(ypr.x, ypr.y, ypr.z);

        Fvector tmp{};
        if (item0)
            tmp = item0->hands_attach_pos();
        else if (item1)
            tmp = item1->hands_attach_pos();

        m_attach_offset.translate_over(tmp);
        m_transform.mul(trans, m_attach_offset);

        configure_external_gun(item0);
        request_collision_snapshot(item0);

        m_model->UpdateTracks();
        m_model->dcast_PKinematics()->CalculateBones_Invalidate();
        m_model->dcast_PKinematics()->CalculateBones(TRUE);
    }

    if (item0)
        item0->update(true);

    if (item1)
        item1->update(true);

    update_weapon_collision(item0);
}

u32 player_hud::anim_play(u16 part, const MotionID& M, BOOL bMixIn, const CMotionDef*& md, float speed, IKinematicsAnimated* itemModel)
{
    if (!itemModel && m_model)
    {
        u16 part_id = u16(-1);
        if (attached_item(0) && attached_item(1))
            part_id = m_model->partitions().part_id((part == 0) ? "right_hand" : "left_hand");

        const u16 pc = m_model->partitions().count();
        for (u16 pid = 0; pid < pc; ++pid)
        {
            if (pid == 0 || pid == part_id || part_id == u16(-1))
            {
                CBlend* B = m_model->PlayCycle(pid, M, bMixIn);
                R_ASSERT(B);
                B->speed *= speed;
            }
        }
        m_model->dcast_PKinematics()->CalculateBones_Invalidate();
    }

    return motion_length(M, md, speed, itemModel);
}

void player_hud::update_additional(Fmatrix& trans) const
{
    if (m_attached_items[0])
        m_attached_items[0]->update_hud_additional(trans);

    if (m_attached_items[1])
        m_attached_items[1]->update_hud_additional(trans);
}

void player_hud::update_inertion(Fmatrix& trans) const
{
    if (inertion_allowed())
    {
        attachable_hud_item* pMainHud = m_attached_items[0];

        Fmatrix xform;
        Fvector& origin = trans.c;
        xform = trans;

        static Fvector st_last_dir = {0, 0, 0};

        // load params
        hud_item_measures::inertion_params inertion_data;
        if (pMainHud != NULL)
        { // Загружаем параметры инерции из основного худа
            inertion_data.m_pitch_offset_r = pMainHud->m_measures.m_inertion_params.m_pitch_offset_r;
            inertion_data.m_pitch_offset_n = pMainHud->m_measures.m_inertion_params.m_pitch_offset_n;
            inertion_data.m_pitch_offset_d = pMainHud->m_measures.m_inertion_params.m_pitch_offset_d;
            inertion_data.m_pitch_low_limit = pMainHud->m_measures.m_inertion_params.m_pitch_low_limit;
            inertion_data.m_origin_offset = pMainHud->m_measures.m_inertion_params.m_origin_offset;
            inertion_data.m_origin_offset_aim = pMainHud->m_measures.m_inertion_params.m_origin_offset_aim;
            inertion_data.m_tendto_speed = pMainHud->m_measures.m_inertion_params.m_tendto_speed;
            inertion_data.m_tendto_speed_aim = pMainHud->m_measures.m_inertion_params.m_tendto_speed_aim;
        }
        else
        { // Загружаем дефолтные параметры инерции
            inertion_data.m_pitch_offset_r = PITCH_OFFSET_R;
            inertion_data.m_pitch_offset_n = PITCH_OFFSET_N;
            inertion_data.m_pitch_offset_d = PITCH_OFFSET_D;
            inertion_data.m_pitch_low_limit = PITCH_LOW_LIMIT;
            inertion_data.m_origin_offset = ORIGIN_OFFSET;
            inertion_data.m_origin_offset_aim = ORIGIN_OFFSET_AIM;
            inertion_data.m_tendto_speed = TENDTO_SPEED;
            inertion_data.m_tendto_speed_aim = TENDTO_SPEED_AIM;
        }

        // calc difference
        Fvector diff_dir;
        diff_dir.sub(xform.k, st_last_dir);

        // clamp by PI_DIV_2
        Fvector last;
        last.normalize_safe(st_last_dir);
        float dot = last.dotproduct(xform.k);
        if (dot < EPS)
        {
            Fvector v0;
            v0.crossproduct(st_last_dir, xform.k);
            st_last_dir.crossproduct(xform.k, v0);
            diff_dir.sub(xform.k, st_last_dir);
        }

        // tend to forward
        float _tendto_speed, _origin_offset;
        if (pMainHud != NULL && pMainHud->m_parent_hud_item->GetCurrentHudOffsetIdx() > 0)
        { // Худ в режиме "Прицеливание"
            float factor = pMainHud->m_parent_hud_item->GetInertionFactor();
            _tendto_speed = inertion_data.m_tendto_speed_aim - (inertion_data.m_tendto_speed_aim - inertion_data.m_tendto_speed) * factor;
            _origin_offset =
                inertion_data.m_origin_offset_aim - (inertion_data.m_origin_offset_aim - inertion_data.m_origin_offset) * factor;
        }
        else
        { // Худ в режиме "От бедра"
            _tendto_speed = inertion_data.m_tendto_speed;
            _origin_offset = inertion_data.m_origin_offset;
        }

        // Фактор силы инерции
        if (pMainHud != NULL)
        {
            float power_factor = pMainHud->m_parent_hud_item->GetInertionPowerFactor();
            _tendto_speed *= power_factor;
            _origin_offset *= power_factor;
        }

        st_last_dir.mad(diff_dir, _tendto_speed * Device.fTimeDelta);
        origin.mad(diff_dir, _origin_offset);

        // pitch compensation
        float pitch = angle_normalize_signed(xform.k.getP());

        if (pMainHud != NULL)
            pitch *= pMainHud->m_parent_hud_item->GetInertionFactor();

        // Отдаление\приближение
        origin.mad(xform.k, -pitch * inertion_data.m_pitch_offset_d);

        // Сдвиг в противоположную часть экрана
        origin.mad(xform.i, -pitch * inertion_data.m_pitch_offset_r);

        // Подьём\опускание
        clamp(pitch, inertion_data.m_pitch_low_limit, PI);
        origin.mad(xform.j, -pitch * inertion_data.m_pitch_offset_n);
    }
}

attachable_hud_item* player_hud::create_hud_item(const shared_str& sect)
{
    current_player_hud_sect = sect;
    auto& item = m_pool[sect];

    if (!item)
        item = xr_new<attachable_hud_item>(this, sect, m_model);

    return item;
}

bool player_hud::allow_activation(CHudItem* item) const
{
    if (m_attached_items[1])
        return m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);
    else
        return true;
}

void player_hud::attach_item(CHudItem* item)
{
    attachable_hud_item* pi = create_hud_item(item->HudSection());
    const int item_idx = pi->m_attach_place_idx;

    if (m_attached_items[item_idx] != pi || pi->m_parent_hud_item != item)
    {
        if (m_hands_ik)
            m_hands_ik->ResetAll();
        if (item_idx == 0)
            m_weapon_collision.Reset();
        if (m_attached_items[item_idx])
        {
            m_attached_items[item_idx]->reset_hud_ik();
            m_attached_items[item_idx]->m_parent_hud_item->on_b_hud_detach();
        }

        m_attached_items[item_idx] = pi;
        pi->reset_hud_ik();
        pi->m_parent_hud_item = item;
        pi->reload_measures();

        if (item_idx == 0 && m_attached_items[1])
            m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);

        item->on_a_hud_attach();
    }
    pi->m_parent_hud_item = item;
}

void player_hud::detach_item_idx(u16 idx)
{
    if (nullptr == attached_item(idx))
        return;

    if (m_hands_ik)
        m_hands_ik->ResetAll();
    if (idx == 0)
        m_weapon_collision.Reset();

    m_attached_items[idx]->reset_hud_ik();
    m_attached_items[idx]->m_parent_hud_item->on_b_hud_detach();

    m_attached_items[idx]->m_parent_hud_item = nullptr;
    m_attached_items[idx] = nullptr;

    if (idx == 1 && attached_item(0))
    {
        u16 part_idR = m_model->partitions().part_id("right_hand");
        u32 bc = m_model->LL_PartBlendsCount(part_idR);
        for (u32 bidx = 0; bidx < bc; ++bidx)
        {
            CBlend* BR = m_model->LL_PartBlend(part_idR, bidx);
            if (!BR)
                continue;

            MotionID M = BR->motionID;

            u16 pc = m_model->partitions().count();
            for (u16 pid = 0; pid < pc; ++pid)
            {
                if (pid != part_idR)
                {
                    CBlend* B = m_model->PlayCycle(pid, M, TRUE); // this can destroy BR calling UpdateTracks !
                    if (BR->blend_state() != CBlend::eFREE_SLOT)
                    {
                        u16 bop = B->bone_or_part;
                        *B = *BR;
                        B->bone_or_part = bop;
                    }
                }
            }
        }
    }
    else if (idx == 0 && attached_item(1))
    {
        OnMovementChanged(mcAnyMove);
    }
}

void player_hud::detach_item(CHudItem* item)
{
    if (nullptr == item->HudItemData())
        return;

    const u16 item_idx = item->HudItemData()->m_attach_place_idx;

    if (m_attached_items[item_idx] == item->HudItemData())
    {
        detach_item_idx(item_idx);
    }
}

void player_hud::detach_all_items()
{
    if (m_hands_ik)
        m_hands_ik->ResetAll();
    m_weapon_collision.Reset();

    for (attachable_hud_item*& item : m_attached_items)
    {
        if (item)
            item->reset_hud_ik();
        item = nullptr;
    }
}

void player_hud::configure_external_gun(const attachable_hud_item* primary)
{
    if (!m_hands_ik)
        return;

    IKinematics* hands = m_model ? m_model->dcast_PKinematics() : nullptr;
    if (primary && !primary->m_monolithic && primary->m_attach_place_idx == 0 && hands && hands == m_hands_ik->Skeleton() &&
        !m_ancors.empty())
    {
        const u16 anchor = m_ancors[0];
        if (anchor != BI_NONE && anchor < hands->LL_BoneCount())
        {
            m_hands_ik->SetExternalGun(anchor, primary->m_attach_offset);
            return;
        }
    }
    m_hands_ik->ClearExternalGun();
}

CHudIKController* player_hud::hud_ik(IKinematicsAnimated* model)
{
    if (!model)
        return nullptr;

    if (model == m_model)
        return m_hands_ik && m_hands_ik->Skeleton() ? m_hands_ik : nullptr;

    for (attachable_hud_item* item : m_attached_items)
    {
        if (!item || !item->m_model || (item->m_monolithic && !item->m_hud_ik))
            continue;

        if (item->m_model->dcast_PKinematicsAnimated() != model)
            continue;

        if (item->m_monolithic)
            return item->m_hud_ik;

        if (item == m_attached_items[0] && m_hands_ik && m_hands_ik->Skeleton())
            return m_hands_ik;
    }
    return nullptr;
}

CHudWeaponCollision& player_hud::weapon_collision()
{
    return m_weapon_collision;
}

void player_hud::clear_collision_offsets()
{
    if (m_hands_ik)
        m_hands_ik->ClearCollisionOffset();

    for (attachable_hud_item* item : m_attached_items)
    {
        if (item && item->m_hud_ik)
            item->m_hud_ik->ClearCollisionOffset();
    }
}

CHudIKController* player_hud::collision_controller(attachable_hud_item* primary, bool& external) const
{
    external = false;
    if (!primary || !primary->m_parent_hud_item || !primary->m_model)
        return nullptr;

    const hud_item_measures& measures = primary->m_measures;
    if (!measures.m_prop_flags.test(hud_item_measures::e_fire_point))
        return nullptr;

    if (measures.m_fire_bone >= primary->m_model->LL_BoneCount())
        return nullptr;

    if (primary->m_monolithic)
    {
        CHudIKController* controller = primary->m_hud_ik;
        return controller && controller->Skeleton() == primary->m_model ? controller : nullptr;
    }

    const attachable_hud_item* secondary = m_attached_items[1];
    if (secondary && secondary->m_monolithic)
        return nullptr;

    IKinematics* hands = m_model ? m_model->dcast_PKinematics() : nullptr;
    if (!hands || !m_hands_ik || m_hands_ik->Skeleton() != hands || !m_hands_ik->HasExternalGun())
        return nullptr;

    external = true;
    return m_hands_ik;
}

void player_hud::request_collision_snapshot(attachable_hud_item* primary)
{
    bool external = false;
    if (CHudIKController* controller = collision_controller(primary, external))
        controller->RequestSnapshot();
}

bool player_hud::collision_muzzle(attachable_hud_item* primary, CHudIKController* controller, bool external, bool raw,
    CHudWeaponCollision::Muzzle& muzzle) const
{
    muzzle = CHudWeaponCollision::Muzzle();
    if (!primary || !controller || !primary->m_parent_hud_item || !primary->m_model)
        return false;

    const hud_item_measures& measures = primary->m_measures;
    const u16 fire_bone = measures.m_fire_bone;
    if (fire_bone >= primary->m_model->LL_BoneCount())
        return false;

    Fvector point{};
    Fvector direction{};
    direction.set(0.f, 0.f, 1.f);
    Fmatrix root;

    if (external)
    {
        if (raw)
        {
            const CHudIKController::GunState& state = controller->GetGunState();
            if (!state.valid || state.frame != Device.dwFrame)
                return false;
            root.mul(m_transform, state.animated);
        }
        else
        {
            root = primary->m_item_transform;
        }
        primary->m_model->LL_GetTransform(fire_bone).transform_tiny(point, measures.m_fire_point_offset);
    }
    else
    {
        Fmatrix bone;
        if (raw)
        {
            if (!controller->GetRawBoneTransform(fire_bone, bone))
                return false;
        }
        else
        {
            bone = primary->m_model->LL_GetTransform(fire_bone);
            controller->TransformGunDirection(direction);
        }
        bone.transform_tiny(point, measures.m_fire_point_offset);
        root = primary->m_item_transform;
    }

    if (!_valid(root) || !_valid(point) || !_valid(direction))
        return false;

    root.transform_tiny(point);
    root.transform_dir(direction);
    if (!_valid(point) || !_valid(direction))
        return false;

    CHudItem* owner = primary->m_parent_hud_item;
    owner->TransformPosFromWorldToHud(point);
    owner->TransformDirFromWorldToHud(direction);

    const float length_sq = direction.square_magnitude();
    if (!_valid(point) || !_valid(direction) || !_valid(length_sq) || length_sq <= EPS * EPS)
        return false;

    direction.normalize();
    muzzle.valid = true;
    muzzle.firePoint = point;
    muzzle.direction = direction;
    return true;
}

bool player_hud::collision_to_controller(const Fvector& correction, const Fmatrix& controller_to_hud, Fvector& offset) const
{
    if (!_valid(correction) || !_valid(Device.mView) || !_valid(controller_to_hud))
        return false;

    Fvector view{};
    Device.mView.transform_dir(view, correction);
    view.x *= psHUD_FOV;
    view.y *= psHUD_FOV;

    Fmatrix inverse_view;
    if (!inverse_view.invert_b(Device.mView))
        return false;

    Fvector hud{};
    inverse_view.transform_dir(hud, view);

    Fmatrix inverse_root;
    if (!inverse_root.invert_b(controller_to_hud))
        return false;

    inverse_root.transform_dir(offset, hud);
    return _valid(offset);
}

void player_hud::recalculate_collision_pose(attachable_hud_item* primary, bool external)
{
    if (external)
    {
        IKinematics* hands = m_model ? m_model->dcast_PKinematics() : nullptr;
        if (!hands)
            return;

        hands->CalculateBones_Invalidate();
        hands->CalculateBones(TRUE);
        for (attachable_hud_item* item : m_attached_items)
        {
            if (item && !item->m_monolithic)
                calc_transform(item->m_attach_place_idx, item->m_attach_offset, item->m_item_transform);
        }
        return;
    }

    primary->m_model->CalculateBones_Invalidate();
    primary->m_model->CalculateBones(TRUE);
}

void player_hud::update_weapon_collision(attachable_hud_item* primary)
{
    bool external = false;
    CHudIKController* controller = collision_controller(primary, external);
    if (!controller)
        return;

    const Fvector camera = Device.vCameraPosition;
    const CHudWeaponCollision::Muzzle none{};
    if (!_valid(psHUD_FOV) || psHUD_FOV <= EPS)
    {
        m_weapon_collision.Solve(camera, none, none, false);
        return;
    }

    CHudWeaponCollision::Muzzle requested;
    const bool requested_valid = collision_muzzle(primary, controller, external, false, requested);

    const bool gun_enabled = controller->GetGun().enabled;
    const CHudIKController::GunState& first_state = controller->GetGunState();
    const bool snapshot = first_state.valid && first_state.frame == Device.dwFrame;

    CHudWeaponCollision::Muzzle original;
    bool original_valid = requested_valid && collision_muzzle(primary, controller, external, true, original);

    const bool can_apply = requested_valid && original_valid && gun_enabled && snapshot;
    m_weapon_collision.Solve(camera, original_valid ? original : none, requested_valid ? requested : none, can_apply);

    CHudWeaponCollision::Muzzle resolved = requested;
    const CHudWeaponCollision::State& result = m_weapon_collision.GetState();
    if (can_apply && result.frame == Device.dwFrame && _valid(result.correction) &&
        result.correction.square_magnitude() > CollisionMinCorrectionSq)
    {
        const Fmatrix& controller_to_hud = external ? m_transform : primary->m_item_transform;
        Fvector offset{};
        if (collision_to_controller(result.correction, controller_to_hud, offset) &&
            offset.square_magnitude() > CollisionMinCorrectionSq)
        {
            controller->SetCollisionOffset(offset);
            recalculate_collision_pose(primary, external);

            const CHudIKController::GunState& second_state = controller->GetGunState();
            if (!second_state.valid || !second_state.applied || second_state.frame != Device.dwFrame ||
                second_state.collision.square_magnitude() <= CollisionMinCorrectionSq)
            {
                controller->ClearCollisionOffset();
            }

            CHudWeaponCollision::Muzzle actual;
            resolved = collision_muzzle(primary, controller, external, false, actual) ? actual : none;
        }
    }
    m_weapon_collision.SetResolved(resolved);
}

void player_hud::calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result) const
{
    const attachable_hud_item* item = m_attached_items[attach_slot_idx];
    if (item && !item->m_monolithic)
    {
        Fmatrix external_pose;
        if (attach_slot_idx == 0 && m_hands_ik && m_hands_ik->GetExternalGunTransform(external_pose))
        {
            result.mul(m_transform, external_pose);
            return;
        }

        IKinematics* k = smart_cast<IKinematics*>(m_model);
        const Fmatrix ancor_m = k->LL_GetTransform(m_ancors[attach_slot_idx]);
        result.mul(m_transform, ancor_m);
        result.mulB_43(offset);
    }
    else
    {
        result.mul(m_transform, offset);
        VERIFY(!fis_zero(DET(result)));
    }
}

bool player_hud::inertion_allowed() const
{
    if (const attachable_hud_item* hi = m_attached_items[0])
    {
        return hi->m_parent_hud_item->HudInertionEnabled() && hi->m_parent_hud_item->HudInertionAllowed();
    }
    return true;
}

void player_hud::OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd) const
{
    CHudItem* hudItem0 = m_attached_items[0] ? m_attached_items[0]->m_parent_hud_item : nullptr;
    CHudItem* hudItem1 = m_attached_items[1] ? m_attached_items[1]->m_parent_hud_item : nullptr;

    if (cmd == 0)
    {
        if (hudItem0 && hudItem0->GetState() == CHUDState::eIdle)
            hudItem0->PlayAnimIdle();

        if (hudItem1 && hudItem1->GetState() == CHUDState::eIdle)
            hudItem1->PlayAnimIdle();
    }
    else
    {
        if (hudItem0)
            hudItem0->OnMovementChanged(cmd);

        if (hudItem1)
            hudItem1->OnMovementChanged(cmd);
    }
}
