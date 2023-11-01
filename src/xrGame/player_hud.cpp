#include "StdAfx.h"
#include "player_hud.h"
#include "HudItem.h"
#include "xrUICore/ui_base.h"
#include "Actor.h"
#include "physic_item.h"
#include "static_cast_checked.hpp"
#include "ActorEffector.h"
#include "WeaponMagazinedWGrenade.h" // XXX: move somewhere

extern u32 hud_adj_mode;
player_hud* g_player_hud = nullptr;
extern ENGINE_API shared_str current_player_hud_sect;
// clang-format off
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
// clang-format on

float CalcMotionSpeed(const shared_str& anim_name)
{
    if (!IsGameTypeSingle() && (anim_name == "anm_show" || anim_name == "anm_hide"))
        return 2.0f;
    else
        return 1.0f;
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
            }
            else
            {
                R_ASSERT2(_GetItemCount(anm.c_str()) == 2, anm.c_str());
                string512 str_item;
                _GetItem(anm.c_str(), 0, str_item);
                pm.m_base_name = str_item;

                _GetItem(anm.c_str(), 1, str_item);
                pm.m_additional_name = str_item;
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

    const bool is_16x9 = UICore::is_widescreen();

    if (m_measures.m_prop_flags.test(hud_item_measures::e_16x9_mode_now) != is_16x9)
    {
        reload_measures();
    }

    if (hud_adj_mode > 0)
        m_measures.update(m_attach_offset);

    m_parent->calc_transform(m_attach_place_idx, m_attach_offset, m_item_transform);
    m_upd_firedeps_frame = Device.dwFrame;

    if (IKinematicsAnimated* ka = m_model->dcast_PKinematicsAnimated())
    {
        ka->UpdateTracks();
        ka->dcast_PKinematics()->CalculateBones_Invalidate();
        ka->dcast_PKinematics()->CalculateBones(TRUE);
    }
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
    debug_draw_firedeps();
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

    strconcat(sizeof(val_name), val_name, "hands_position", _prefix);
    m_hands_attach[0] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(sizeof(val_name), val_name, "hands_orientation", _prefix);
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
        m_fire_point_offset.set(0, 0, 0);

    m_prop_flags.set(e_fire_point2, pSettings->line_exist(sect_name, "fire_bone2"));
    if (m_prop_flags.test(e_fire_point2))
    {
        bone_name = pSettings->r_string(sect_name, "fire_bone2");
        m_fire_bone2 = K->LL_BoneID(bone_name);
        m_fire_point2_offset = pSettings->r_fvector3(sect_name, "fire_point2");
    }
    else
        m_fire_point2_offset.set(0, 0, 0);

    m_prop_flags.set(e_shell_point, pSettings->line_exist(sect_name, "shell_bone"));
    if (m_prop_flags.test(e_shell_point))
    {
        bone_name = pSettings->r_string(sect_name, "shell_bone");
        m_shell_bone = K->LL_BoneID(bone_name);
        m_shell_point_offset = pSettings->r_fvector3(sect_name, "shell_point");
    }
    else
        m_shell_point_offset.set(0, 0, 0);

    m_hands_offset[0][0].set(0, 0, 0);
    m_hands_offset[1][0].set(0, 0, 0);

    strconcat(sizeof(val_name), val_name, "aim_hud_offset_pos", _prefix);
    m_hands_offset[0][1] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(sizeof(val_name), val_name, "aim_hud_offset_rot", _prefix);
    m_hands_offset[1][1] = pSettings->r_fvector3(sect_name, val_name);

    strconcat(sizeof(val_name), val_name, "gl_hud_offset_pos", _prefix);
    m_hands_offset[0][2] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(sizeof(val_name), val_name, "gl_hud_offset_rot", _prefix);
    m_hands_offset[1][2] = pSettings->r_fvector3(sect_name, val_name);

    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point") == pSettings->line_exist(sect_name, "fire_bone"),
        sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point2") == pSettings->line_exist(sect_name, "fire_bone2"),
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

        if (pSettings->line_exist(owner->object().cNameSect(), "shell_particles"))
            m_shell_point_offset = pSettings->r_fvector3(sect_name, "shell_point");
        else
            m_shell_point_offset.set(0, 0, 0);

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
            load_zoom_offsets("", m_hands_offset[0][0], m_hands_offset[1][0]);
            if (smart_cast<CWeaponMagazinedWGrenade*>(wpn))
            {
                load_zoom_offsets("grenade_", m_hands_offset[0][1], m_hands_offset[1][1]);
                if (wpn->GrenadeLauncherAttachable())
                    load_zoom_offsets("grenade_normal_", m_hands_offset[0][2], m_hands_offset[1][2]);
            }
        }
    }
    else
    {
        m_fire_bone = BI_NONE;
        m_fire_bone2 = BI_NONE;
        m_shell_bone = BI_NONE;

        m_fire_point_offset.set(0, 0, 0);
        m_fire_point2_offset.set(0, 0, 0);
        m_shell_point_offset.set(0, 0, 0);
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
    GEnv.Render->hud_loading = true;
    m_model = smart_cast<IKinematics*>(GEnv.Render->model_Create(m_visual_name.c_str()));
    GEnv.Render->hud_loading = false;
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
        m_attach_offset = m_measures.load_monolithic(m_sect_name, m_model, m_parent_hud_item);
    else
        m_attach_offset = m_measures.load(m_sect_name, m_model);
}

u32 attachable_hud_item::anim_play(const shared_str& anm_name_b, BOOL bMixIn, const CMotionDef*& md, u8& rnd_idx)
{
    const float speed = CalcMotionSpeed(anm_name_b);

    string256 anim_name_r;
    const bool is_16x9 = UICore::is_widescreen();
    xr_sprintf(anim_name_r, "%s%s", anm_name_b.c_str(), m_attach_place_idx == 1 && is_16x9 ? "_16x9" : "");

    const player_hud_motion* anm = m_hand_motions.find_motion(anim_name_r);
    R_ASSERT2(anm, make_string("model [%s] has no motion alias defined [%s]", m_sect_name.c_str(), anim_name_r).c_str());
    R_ASSERT2(anm->m_animations.size(), make_string("model [%s] has no motion defined in motion_alias [%s]",
                                            m_visual_name.c_str(), anim_name_r)
                                            .c_str());

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
        strconcat(sizeof(anm_name), anm_name, "camera_effects" DELIMITER "weapon" DELIMITER, M.name.c_str(), ".anm");
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
    if (m_model)
    {
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GEnv.Render->model_Delete(v);
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
    if (m_model)
    {
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GEnv.Render->model_Delete(v);
    }

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

    const shared_str& model_name = pSettings->r_string(m_sect_name, "visual");
    GEnv.Render->hud_loading = true;
    m_model = smart_cast<IKinematicsAnimated*>(GEnv.Render->model_Create(model_name.c_str()));
    GEnv.Render->hud_loading = false;
    load_ancors();
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
    const float speed = CalcMotionSpeed(anim_name);
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
        CMotion* motion = model->LL_GetRootMotion(M);
        return iFloor(0.5f + 1000.f * motion->GetLength() / (md->Dequantize(md->speed) * speed));
    }
    return 0;
}

void player_hud::update(const Fmatrix& cam_trans)
{
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

    const bool monolithic = item0 && item0->m_monolithic || item1 && item1->m_monolithic;
    if (!m_model || monolithic)
        m_transform = trans;
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

        m_model->UpdateTracks();
        m_model->dcast_PKinematics()->CalculateBones_Invalidate();
        m_model->dcast_PKinematics()->CalculateBones(TRUE);
    }

    if (item0)
        item0->update(true);

    if (item1)
        item1->update(true);
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
        if (m_attached_items[item_idx])
            m_attached_items[item_idx]->m_parent_hud_item->on_b_hud_detach();

        m_attached_items[item_idx] = pi;
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

void player_hud::calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result) const
{
    const attachable_hud_item* item = m_attached_items[attach_slot_idx];
    if (item && !item->m_monolithic)
    {
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
