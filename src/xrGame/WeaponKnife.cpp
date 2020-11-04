#include "StdAfx.h"

#include "WeaponKnife.h"
#include "Entity.h"
#include "Actor.h"
#include "Level.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "Include/xrRender/Kinematics.h"
#include "xrEngine/GameMtlLib.h"
#include "Level_Bullet_Manager.h"
#include "ai_sounds.h"
#include "game_cl_single.h"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "player_hud.h"
#include "ActorEffector.h"
#ifdef DEBUG
#include <iterator>
#endif

#define KNIFE_MATERIAL_NAME "objects" DELIMITER "knife"

#ifdef DEBUG
#include "debug_renderer.h"
extern BOOL g_bDrawBulletHit;
#endif //#ifdef DEBUG

CWeaponKnife::CWeaponKnife()
{
    SetState(eHidden);
    SetNextState(eHidden);
    knife_material_idx = (u16)-1;
    fHitImpulse_cur = 0.0f;

    m_Hit1Distance = 1.f;
    m_Hit2Distance = 1.f;

    m_Hit1SplashRadius = 1.0f;
    m_Hit2SplashRadius = 1.0f;

    m_Hit1SpashDir.set(0.f, -0.3, 1.f);
    m_Hit2SpashDir.set(0.f, 0.f, 1.f);

    m_Splash1HitsCount = 3;
    m_Splash1PerVictimsHCount = 1;
    m_Splash2HitsCount = 2;

    m_NextHitDivideFactor = 0.75f;

    oldStrikeMethod = false;
    attackMotionMarksAvailable = true;
}

enum class FieldTypes
{
    t_u32, t_float, t_fvector3
};

CWeaponKnife::~CWeaponKnife() {}
void CWeaponKnife::Load(LPCSTR section)
{
    // verify class
    inherited::Load(section);

    fWallmarkSize = pSettings->r_float(section, "wm_size");
    m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, SOUND_TYPE_WEAPON_SHOOTING);

    int successCount = 0;
    string1024 missingFields;

    // array of <name, fallbackName, type, variable>
    constexpr u32 elementsCount = 10;
    const xr_array<std::tuple<pcstr, pcstr, FieldTypes, void*>, elementsCount> fields =
    {{
        { "splash1_direction", nullptr, FieldTypes::t_fvector3, &m_Hit1SpashDir },
        { "splash2_direction", nullptr, FieldTypes::t_fvector3, &m_Hit2SpashDir },

        { "splash1_dist", "spash1_dist", FieldTypes::t_float, &m_Hit1Distance }, // We need those fallback names just because
        { "splash2_dist", "spash2_dist", FieldTypes::t_float, &m_Hit2Distance }, // GSC was too lazy to fix the typos!!!

        { "splash1_radius", "spash1_radius", FieldTypes::t_float, &m_Hit1SplashRadius },
        { "splash2_radius", "spash2_radius", FieldTypes::t_float, &m_Hit1SplashRadius },

        { "splash1_hits_count", nullptr, FieldTypes::t_u32, &m_Splash1HitsCount },
        { "splash1_pervictim_hcount", nullptr, FieldTypes::t_u32, &m_Splash1PerVictimsHCount },
        { "splash2_hits_count", nullptr, FieldTypes::t_u32, &m_Splash2HitsCount },
        { "splash_hit_divide_factor", nullptr, FieldTypes::t_float, &m_NextHitDivideFactor },
    }};

    const auto assertField = [&](pcstr name, pcstr /*fallback*/, FieldTypes /*type*/, void* outPtr)
    {
        R_ASSERT2(name && outPtr, "Some fields are missing or malformed");
    };

    for (const auto& field : fields)
    {
        std::apply(assertField, field);
    }

    const auto processField = [&](pcstr name, pcstr fallback, FieldTypes type, void* outPtr)
    {
        pcstr nameToRead = nullptr;
        if (pSettings->line_exist(section, name))
            nameToRead = name;
        else if (fallback && pSettings->line_exist(section, fallback))
            nameToRead = fallback;

        if (!nameToRead)
        {
            if (xr_strlen(missingFields))
                xr_strcat(missingFields, ", ");
            xr_strcat(missingFields, name);
            return;
        }

        switch (type)
        {
        case FieldTypes::t_u32:
        {
            u32* outValue = static_cast<u32*>(outPtr);
            *outValue = pSettings->r_u32(section, nameToRead);
            break;
        }
        case FieldTypes::t_float:
        {
            float* outValue = static_cast<float*>(outPtr);
            *outValue = pSettings->r_float(section, nameToRead);
            break;
        }
        case FieldTypes::t_fvector3:
        {
            Fvector3* outValue = static_cast<Fvector3*>(outPtr);
            *outValue = pSettings->r_fvector3(section, nameToRead);
            break;
        }
        }
        successCount++;
    };

    for (const auto& field : fields)
    {
        std::apply(processField, field);
    }

    R_ASSERT4(successCount == elementsCount || successCount == 0,
        "You need to provide all knife splash parameters or remove them all.",
        "Missing fields are:", missingFields);

    oldStrikeMethod = successCount == 0;

    knife_material_idx = GMLib.GetMaterialIdx(KNIFE_MATERIAL_NAME);

#ifdef DEBUG
    m_dbg_data.m_pick_vectors.reserve(std::max(m_Splash1HitsCount, m_Splash2HitsCount));
#endif
}

void CWeaponKnife::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    switch (S)
    {
    case eIdle: switch2_Idle(); break;
    case eShowing: switch2_Showing(); break;
    case eHiding:
    {
        if (oldState != eHiding)
            switch2_Hiding();
        break;
    }
    case eHidden: switch2_Hidden(); break;
    case eFire:
    {
        //-------------------------------------------
        m_eHitType = m_eHitType_1;
        // fHitPower		= fHitPower_1;
        if (ParentIsActor())
        {
            if (GameID() == eGameIDSingle)
            {
                fCurrentHit = fvHitPower_1[g_SingleGameDifficulty];
            }
            else
            {
                fCurrentHit = fvHitPower_1[egdMaster];
            }
        }
        else
        {
            fCurrentHit = fvHitPower_1[egdMaster];
        }
        fHitImpulse_cur = fHitImpulse_1;
        //-------------------------------------------
        switch2_Attacking(S);
    }
    break;
    case eFire2:
    {
        //-------------------------------------------
        m_eHitType = m_eHitType_2;
        // fHitPower		= fHitPower_2;
        if (ParentIsActor())
        {
            if (GameID() == eGameIDSingle)
            {
                fCurrentHit = fvHitPower_2[g_SingleGameDifficulty];
            }
            else
            {
                fCurrentHit = fvHitPower_2[egdMaster];
            }
        }
        else
        {
            fCurrentHit = fvHitPower_2[egdMaster];
        }
        fHitImpulse_cur = fHitImpulse_2;
        //-------------------------------------------
        switch2_Attacking(S);
    }
    break;
    }
}

void CWeaponKnife::KnifeStrike(const Fvector& pos, const Fvector& dir)
{
    if (oldStrikeMethod)
    {
        MakeShot(pos, dir);
        return;
    }

    IGameObject* real_victim = TryPick(pos, dir, m_hit_dist);
    if (real_victim)
    {
        float new_khit = m_eHitType == m_eHitType_1 ? float(m_Splash1PerVictimsHCount) : float(m_Splash2HitsCount);
        MakeShot(pos, dir, new_khit);
        return;
    }

    shot_targets_t dest_hits(xr_alloca(sizeof(Fvector) * m_hits_count), m_hits_count);

    if (SelectHitsToShot(dest_hits, pos))
    {
#ifdef DEBUG
        m_dbg_data.m_targets_vectors.clear();
        std::copy(dest_hits.begin(), dest_hits.end(),
            std::back_insert_iterator<dbg_draw_data::targets_t>(m_dbg_data.m_targets_vectors));
#endif
        float tmp_k_hit = 1.0f;
        for (shot_targets_t::const_iterator i = dest_hits.begin(), ie = dest_hits.end(); i != ie; ++i)
        {
            Fvector shot_dir;
            shot_dir.set(*i).sub(pos).normalize();
            MakeShot(pos, shot_dir, tmp_k_hit);
            tmp_k_hit *= m_NextHitDivideFactor;
        }
        return;
    }

    MakeShot(pos, dir);
}

void CWeaponKnife::MakeShot(Fvector const& pos, Fvector const& dir, float const k_hit)
{
    CCartridge cartridge;
    cartridge.param_s.buckShot = 1;
    cartridge.param_s.impair = 1.0f;
    cartridge.param_s.kDisp = 1.0f;
    cartridge.param_s.kHit = k_hit;
    //.	cartridge.param_s.kCritical		= 1.0f;
    cartridge.param_s.kImpulse = 1.0f;
    cartridge.param_s.kAP = EPS_L;
    cartridge.m_flags.set(CCartridge::cfTracer, FALSE);
    cartridge.m_flags.set(CCartridge::cfRicochet, FALSE);
    cartridge.param_s.fWallmarkSize = fWallmarkSize;
    cartridge.bullet_material_idx = knife_material_idx;

    while (m_magazine.size() < 2)
        m_magazine.push_back(cartridge);
    iAmmoElapsed = m_magazine.size();
    bool SendHit = SendHitAllowed(H_Parent());

    PlaySound("sndShot", pos);

    Level().BulletManager().AddBullet(pos, dir, m_fStartBulletSpeed, fCurrentHit, fHitImpulse_cur, H_Parent()->ID(),
        ID(), m_eHitType, fireDistance, cartridge, 1.f, SendHit);
}

void CWeaponKnife::OnKnifeStrike(u32 state)
{
    switch (state)
    {
    case eFire:
    {
        if (oldStrikeMethod)
            break;
        m_hit_dist = m_Hit1Distance;
        m_splash_dir = m_Hit1SpashDir;
        m_splash_radius = m_Hit1SplashRadius;
        m_hits_count = m_Splash1HitsCount;
        m_perv_hits_count = m_Splash1PerVictimsHCount;
        fireDistance = m_hit_dist + m_splash_radius;
        break;
    }
    case eFire2:
    {
        if (oldStrikeMethod)
            break;
        m_hit_dist = m_Hit2Distance;
        m_splash_dir = m_Hit2SpashDir;
        m_splash_radius = m_Hit2SplashRadius;
        m_hits_count = m_Splash2HitsCount;
        m_perv_hits_count = 0;
        fireDistance = m_hit_dist + m_splash_radius;
        break;
    }
    default:
        return;
    }

    if (H_Parent())
    {
        Fvector p1, d;
        p1.set(get_LastFP());
        d.set(get_LastFD());
        smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p1, d);
        KnifeStrike(p1, d);
    }
}

void CWeaponKnife::OnMotionMark(u32 state, const motion_marks& M)
{
    inherited::OnMotionMark(state, M);
    OnKnifeStrike(state);
}

void CWeaponKnife::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eHiding: SwitchState(eHidden); break;

    case eFire:
    case eFire2:
    {
        u32 time = 0;
        if (attackStarted)
        {
            attackStarted = false;

            if (state == eFire)
                time = PlayHUDMotion("anm_attack_end", "anim_shoot1_end", FALSE, this, state);
            else // eFire2
                time = PlayHUDMotion("anm_attack2_end", "anim_shoot2_end", FALSE, this, state);

            if (time != 0 && !attackMotionMarksAvailable)
                OnKnifeStrike(state);
        }
        if (time == 0)
        {
            SwitchState(eIdle);
        }
        break;
    }
    case eShowing:
    case eIdle: SwitchState(eIdle); break;

    default: inherited::OnAnimationEnd(state);
    }
}

void CWeaponKnife::state_Attacking(float) {}
void CWeaponKnife::switch2_Attacking(u32 state)
{
    if (IsPending())
        return;

    if (state == eFire)
        PlayHUDMotion("anm_attack", "anim_shoot1_start", FALSE, this, state);
    else // eFire2
        PlayHUDMotion("anm_attack2", "anim_shoot2_start", FALSE, this, state);

    // XXX: could check it once at initialization stage (could use something like CHudItem::isHUDAnimationExist())
    attackMotionMarksAvailable = !m_current_motion_def->marks.empty();
    attackStarted = true;
    SetPending(TRUE);
}

void CWeaponKnife::switch2_Idle()
{
    VERIFY(GetState() == eIdle);

    PlayAnimIdle();
    SetPending(FALSE);
}

void CWeaponKnife::switch2_Hiding()
{
    FireEnd();
    VERIFY(GetState() == eHiding);
    PlayHUDMotion("anm_hide", "anim_hide", TRUE, this, GetState());
}

void CWeaponKnife::switch2_Hidden()
{
    signal_HideComplete();
    SetPending(FALSE);
}

void CWeaponKnife::switch2_Showing()
{
    VERIFY(GetState() == eShowing);
    PlayHUDMotion("anm_show", "anim_draw", FALSE, this, GetState());
}

void CWeaponKnife::FireStart()
{
    inherited::FireStart();
    SwitchState(eFire);
}

void CWeaponKnife::Fire2Start() { SwitchState(eFire2); }
bool CWeaponKnife::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;
    switch (cmd)
    {
    case kWPN_ZOOM:
        if (flags & CMD_START)
            Fire2Start();

        return true;
    }
    return false;
}

void CWeaponKnife::LoadFireParams(LPCSTR section)
{
    inherited::LoadFireParams(section);

    string32 buffer;
    shared_str s_sHitPower_2;
    shared_str s_sHitPowerCritical_2;

    fvHitPower_1 = fvHitPower;
    fvHitPowerCritical_1 = fvHitPowerCritical;
    fHitImpulse_1 = fHitImpulse;
    m_eHitType_1 = ALife::g_tfString2HitType(pSettings->r_string(section, "hit_type"));

    // fHitPower_2			= pSettings->r_float	(section,strconcat(full_name, prefix, "hit_power_2"));
    s_sHitPower_2 = pSettings->r_string_wb(section, "hit_power_2");
    s_sHitPowerCritical_2 = READ_IF_EXISTS(pSettings, r_string_wb, section, "hit_power_critical_2", s_sHitPower_2);

    fvHitPower_2[egdMaster] =
        (float)atof(_GetItem(*s_sHitPower_2, 0, buffer)); //первый параметр - это хит для уровня игры мастер
    fvHitPowerCritical_2[egdMaster] =
        (float)atof(_GetItem(*s_sHitPowerCritical_2, 0, buffer)); //первый параметр - это хит для уровня игры мастер

    fvHitPower_2[egdNovice] = fvHitPower_2[egdStalker] = fvHitPower_2[egdVeteran] =
        fvHitPower_2[egdMaster]; //изначально параметры для других уровней сложности такие же
    fvHitPowerCritical_2[egdNovice] = fvHitPowerCritical_2[egdStalker] = fvHitPowerCritical_2[egdVeteran] =
        fvHitPowerCritical_2[egdMaster]; //изначально параметры для других уровней сложности такие же

    int num_game_diff_param = _GetItemCount(*s_sHitPower_2); //узнаём колличество параметров для хитов
    if (num_game_diff_param > 1) //если задан второй параметр хита
    {
        fvHitPower_2[egdVeteran] =
            (float)atof(_GetItem(*s_sHitPower_2, 1, buffer)); //то вычитываем его для уровня ветерана
    }
    if (num_game_diff_param > 2) //если задан третий параметр хита
    {
        fvHitPower_2[egdStalker] =
            (float)atof(_GetItem(*s_sHitPower_2, 2, buffer)); //то вычитываем его для уровня сталкера
    }
    if (num_game_diff_param > 3) //если задан четвёртый параметр хита
    {
        fvHitPower_2[egdNovice] =
            (float)atof(_GetItem(*s_sHitPower_2, 3, buffer)); //то вычитываем его для уровня новичка
    }

    num_game_diff_param = _GetItemCount(*s_sHitPowerCritical_2); //узнаём колличество параметров
    if (num_game_diff_param > 1) //если задан второй параметр хита
    {
        fvHitPowerCritical_2[egdVeteran] =
            (float)atof(_GetItem(*s_sHitPowerCritical_2, 1, buffer)); //то вычитываем его для уровня ветерана
    }
    if (num_game_diff_param > 2) //если задан третий параметр хита
    {
        fvHitPowerCritical_2[egdStalker] =
            (float)atof(_GetItem(*s_sHitPowerCritical_2, 2, buffer)); //то вычитываем его для уровня сталкера
    }
    if (num_game_diff_param > 3) //если задан четвёртый параметр хита
    {
        fvHitPowerCritical_2[egdNovice] =
            (float)atof(_GetItem(*s_sHitPowerCritical_2, 3, buffer)); //то вычитываем его для уровня новичка
    }

    fHitImpulse_2 = pSettings->r_float(section, "hit_impulse_2");
    m_eHitType_2 = ALife::g_tfString2HitType(pSettings->r_string(section, "hit_type_2"));
}

bool CWeaponKnife::GetBriefInfo(II_BriefInfo& info)
{
    info.clear();
    info.name._set(m_nameShort);
    info.icon._set(cNameSect());
    return true;
}

#ifdef DEBUG
void CWeaponKnife::OnRender()
{
    CDebugRenderer& renderer = Level().debug_renderer();
    if (g_bDrawBulletHit)
    {
        for (dbg_draw_data::spheres_t::const_iterator i = m_dbg_data.m_spheres.begin(), ie = m_dbg_data.m_spheres.end();
             i != ie; ++i)
        {
            float sc_r = i->second;
            Fmatrix sphere = Fmatrix().scale(sc_r, sc_r, sc_r);
            sphere.c = i->first;
            renderer.draw_ellipse(sphere, color_xrgb(100, 255, 0));
        }
        /*
        Fmatrix	sphere				= Fmatrix().scale(.05f, .05f, .05f);
        sphere.c					= m_dbg_data.m_pos;
        renderer.draw_ellipse		(sphere, color_xrgb(255, 0, 0));
        renderer.draw_line			(Fidentity, m_dbg_data.m_pos, m_dbg_data.m_endpos, color_xrgb(255, 255, 0));

        sphere.c					= m_dbg_data.m_endpos;
        renderer.draw_ellipse		(sphere, color_xrgb(100, 255, 0));*/
        // Fvector victim_end			(m_dbg_data.m_pos);
        // victim_end.add				(m_dbg_data.m_pick_vector);
        // renderer.draw_line			(Fidentity, m_dbg_data.m_pos, victim_end, color_xrgb(0, 255, 255));
    }
    float hit_power = 1.f;
    for (dbg_draw_data::targets_t::const_iterator i = m_dbg_data.m_targets_vectors.begin(),
                                                  ie = m_dbg_data.m_targets_vectors.end();
         i != ie; ++i)
    {
        Fmatrix sphere = Fmatrix().scale(0.05f, 0.05f, 0.05f);
        sphere.c = *i;
        u8 hit_color = u8(255 * hit_power);
        hit_power *= m_NextHitDivideFactor;
        renderer.draw_ellipse(sphere, color_xrgb(hit_color, 50, 0));
    }

    for (dbg_draw_data::obbes_t::const_iterator i = m_dbg_data.m_target_boxes.begin(),
                                                ie = m_dbg_data.m_target_boxes.end();
         i != ie; ++i)
    {
        Fmatrix tmp_matrix;
        tmp_matrix.set(i->m_rotate.i, i->m_rotate.j, i->m_rotate.k, i->m_translate);
        renderer.draw_obb(tmp_matrix, i->m_halfsize, color_xrgb(0, 255, 0));
    }
}
#endif

static bool intersect(Fsphere const& bone, Fsphere const& query)
{
    return bone.P.distance_to_sqr(query.P) < _sqr(bone.R + query.R);
}

static bool intersect(Fobb bone, Fsphere const& query)
{
    Fmatrix transform;
    bone.m_halfsize.add(Fvector().set(query.R, query.R, query.R));
    bone.xform_full(transform);
    transform.invert();

    Fvector new_position;
    transform.transform_tiny(new_position, query.P);

    return (new_position.x >= -1.f) && (new_position.y >= -1.f) && (new_position.z >= -1.f) &&
        (new_position.x <= 1.f) && (new_position.y <= 1.f) && (new_position.z <= 1.f);
}

static bool intersect(Fcylinder const& bone, Fsphere const& query)
{
    Fvector const bone2query = Fvector().sub(query.P, bone.m_center);
    float const axe_projection = bone2query.dotproduct(bone.m_direction);
    float const half_height = bone.m_height / 2.f;
    if (_abs(axe_projection) > half_height + query.R)
        return false;

    VERIFY(bone2query.square_magnitude() >= _sqr(axe_projection));
    float const axe_projection2_sqr = bone2query.square_magnitude() - _sqr(axe_projection);
    if (axe_projection2_sqr > _sqr(bone.m_radius + query.R))
        return false;

    if (_abs(axe_projection) <= half_height)
        return true;

    if (axe_projection2_sqr <= _sqr(bone.m_radius))
        return true;

    Fvector const center_direction = Fvector(bone.m_direction).mul(axe_projection >= 0.f ? 1.f : -1.f);
    Fvector const circle_center = Fvector(bone.m_center).mad(center_direction, half_height);
    Fvector const circle2sphere = Fvector().sub(query.P, circle_center);
    float const distance2plane = circle2sphere.dotproduct(center_direction);
    VERIFY(distance2plane > 0.f);
    VERIFY(_sqr(query.R) >= _sqr(distance2plane));
    float const circle_radius = _sqrt(_sqr(query.R) - _sqr(distance2plane));
    Fvector const sphere_circle_center = Fvector(query.P).mad(center_direction, -distance2plane);
    float const distance2center_sqr = circle_center.distance_to_sqr(sphere_circle_center);
    return distance2center_sqr <= _sqr(bone.m_radius + circle_radius);
}

void CWeaponKnife::GetVictimPos(CEntityAlive* victim, Fvector& pos_dest)
{
    /*VERIFY(victim);
    IKinematics*	tmp_kinem	= smart_cast<IKinematics*>(victim->Visual());
    u16 hit_bone_id				= tmp_kinem->LL_BoneID(m_SplashHitBone);
    if (hit_bone_id != BI_NONE)
    {
        Fmatrix			tmp_matrix;
        tmp_kinem->Bone_GetAnimPos	(tmp_matrix, hit_bone_id, u8(-1), true);
        pos_dest.set(tmp_matrix.c);
        Fmatrix	& tmp_xform			= victim->XFORM();
        tmp_xform.transform_tiny	(pos_dest);
    } else
    {
        Fbox const & tmp_box = tmp_kinem->GetBox();
        Fvector tmp_fake_vec;
        tmp_box.get_CD(pos_dest, tmp_fake_vec);
        pos_dest.add(victim->Position());
    }

    CBoneData& tmp_bone_data	= tmp_kinem->LL_GetData(hit_bone_id);
    Fmatrix	& tmp_xform			= victim->XFORM();
    CBoneInstance &bi			= tmp_kinem->LL_GetBoneInstance();

    switch (tmp_bone_data.shape.type)
    {
    case SBoneShape::stBox:
        {
            pos_dest = tmp_bone_data.shape.box.m_translate;
            break;
        };
    case SBoneShape::stSphere:
        {
            pos_dest = tmp_bone_data.shape.sphere.P;
        }break;
    case SBoneShape::stCylinder:
        {
            pos_dest = tmp_bone_data.shape.cylinder.m_center;
        }break;
    };//switch (tmp_bone_data.shape.type)
    tmp_xform.transform_tiny(pos_dest);
    bi.mTransform.transform_tiny(pos_dest);*/
}

u32 CWeaponKnife::get_entity_bones_count(CEntityAlive const* entity)
{
    VERIFY(entity);
    if (!entity)
        return 0;
    IKinematics* tmp_kinem = smart_cast<IKinematics*>(entity->Visual());
    if (!tmp_kinem)
        return 0;

    IKinematics::accel* tmp_accel = tmp_kinem->LL_Bones();
    if (!tmp_accel)
        return 0;

    return tmp_accel->size();
};

void CWeaponKnife::fill_shapes_list(
    CEntityAlive const* entity, Fvector const& camera_endpos, victims_shapes_list_t& dest_shapes)
{
    VERIFY(entity);
    if (!entity)
        return;

    CCF_Skeleton* tmp_skeleton = smart_cast<CCF_Skeleton*>(entity->GetCForm());
    if (!tmp_skeleton)
        return;

    Fvector basis_vector;
    float max_dist;
    make_hit_sort_vectors(basis_vector, max_dist);
    Fvector camendpos2;
    camendpos2.set(camera_endpos).mul(basis_vector);

    CCF_Skeleton::ElementVec const& elems_vec = tmp_skeleton->_GetElements();
    for (CCF_Skeleton::ElementVec::const_iterator i = elems_vec.begin(), ie = elems_vec.end(); i != ie; ++i)
    {
        Fvector tmp_pos;
        i->center(tmp_pos);
        tmp_pos.mul(basis_vector);
        // float basis_proj = tmp_pos.dotproduct(basis_vector);
        float bone_dist = tmp_pos.distance_to_sqr(camendpos2);
        if (bone_dist < max_dist)
        {
            victim_bone_data tmp_bone_data;
            tmp_bone_data.m_bone_element = &(*i);
            tmp_bone_data.m_victim_id = entity->ID();
            tmp_bone_data.m_shots_count = 0;
            dest_shapes.push_back(std::make_pair(tmp_bone_data, bone_dist));
        }
    }
}

void CWeaponKnife::fill_shots_list(
    victims_shapes_list_t& victims_shapres, Fsphere const& query, shot_targets_t& dest_shots)
{
    m_victims_hits_count.clear();
    for (victims_shapes_list_t::iterator i = victims_shapres.begin(), ie = victims_shapres.end(); i != ie; ++i)
    {
        if (dest_shots.capacity() <= dest_shots.size())
            return;

        bool intersect_res = false;
        Fvector target_pos;
#ifdef DEBUG
        m_dbg_data.m_target_boxes.clear();
#endif
        victim_bone_data& curr_bone = i->first;
        switch (curr_bone.m_bone_element->type)
        {
        case SBoneShape::stBox:
        {
            Fobb tmp_obb;
            Fmatrix tmp_xform;
            if (!tmp_xform.invert_b(curr_bone.m_bone_element->b_IM))
            {
                VERIFY2(false, "invalid bone xform");
                break;
            }
            tmp_obb.xform_set(tmp_xform);
            tmp_obb.m_halfsize = curr_bone.m_bone_element->b_hsize;
#ifdef DEBUG
            m_dbg_data.m_target_boxes.push_back(tmp_obb);
#endif
            intersect_res = intersect(tmp_obb, query);
            break;
        };
        case SBoneShape::stSphere:
        {
            intersect_res = intersect(curr_bone.m_bone_element->s_sphere, query);
            break;
        }
        break;
        case SBoneShape::stCylinder:
        {
            intersect_res = intersect(curr_bone.m_bone_element->c_cylinder, query);
            break;
        }
        break;
        }; // switch (tmp_bone_data.shape.type)*/
        if (intersect_res)
        {
            victims_hits_count_t::iterator tmp_vhits_iter = m_victims_hits_count.find(curr_bone.m_victim_id);
            if (m_perv_hits_count && (tmp_vhits_iter == m_victims_hits_count.end()))
            {
                m_victims_hits_count.insert(std::make_pair(curr_bone.m_victim_id, u16(1)));
            }
            else if (m_perv_hits_count && (tmp_vhits_iter->second < m_perv_hits_count))
            {
                ++tmp_vhits_iter->second;
            }
            else if (m_perv_hits_count)
            {
                continue;
            }
            curr_bone.m_bone_element->center(target_pos);
            dest_shots.push_back(target_pos);
        }
    }
}

void CWeaponKnife::create_victims_list(spartial_base_t spartial_result, victims_list_t& victims_dest)
{
    for (spartial_base_t::const_iterator i = spartial_result.begin(), ie = spartial_result.end(); i != ie; ++i)
    {
        IGameObject* tmp_obj = (*i)->dcast_GameObject();
        VERIFY(tmp_obj);
        if (!tmp_obj)
            continue;
        CEntityAlive* tmp_entity = smart_cast<CEntityAlive*>(tmp_obj);
        if (!tmp_entity)
            continue;
        VERIFY(victims_dest.capacity() > victims_dest.size());
        victims_dest.push_back(tmp_entity);
    }
}

void CWeaponKnife::make_hit_sort_vectors(Fvector& basis_hit_specific, float& max_dist)
{
    if (m_eHitType == m_eHitType_1)
    {
        // basis_hit_specific1.set(-1.f, 0.f, 0.f);
        basis_hit_specific.set(0.f, 1.f, 0.f);
        max_dist = 0.2;
    }
    else // if (m_eHitType == m_eHitType_2)
    {
        // basis_hit_specific1.set(0.f, -1.f, 0.f);
        basis_hit_specific.set(1.f, 0.f, 0.f);
        max_dist = 0.1;
    }
}

static float const spartial_prefetch_radius = 2.0f;

u32 CWeaponKnife::SelectHitsToShot(shot_targets_t& dst_dirs, Fvector const& f_pos)
{
    Fvector hit1_basis_vector;
    Fvector hit2_basis_vector;
    hit1_basis_vector.set(-1.f, 1.f, 0.f);
    hit2_basis_vector.set(0.f, -1.f, 0.f);

    Fmatrix parent_xform;
    Fvector fendpos;
    Fsphere query_sphere;

    dst_dirs.clear();
    if (!SelectBestHitVictim(f_pos, parent_xform, fendpos, query_sphere))
        return 0;

    victims_list_t tmp_victims_list(
        xr_alloca(m_spartial_query_res.size() * sizeof(CEntityAlive*)), m_spartial_query_res.size());

    create_victims_list(m_spartial_query_res, tmp_victims_list);

    u32 summ_shapes_count = 0;
    for (victims_list_t::const_iterator i = tmp_victims_list.begin(), ie = tmp_victims_list.end(); i != ie; ++i)
    {
        summ_shapes_count += get_entity_bones_count(*i);
    }
    victims_shapes_list_t tmp_shapes_list(
        xr_alloca(summ_shapes_count * sizeof(victims_shapes_list_t::value_type)), summ_shapes_count);

    Fvector basis_vector;
    if (m_eHitType == m_eHitType_1)
    {
        basis_vector.set(hit1_basis_vector);
    }
    else // if (m_eHitType == m_eHitType_2)
    {
        basis_vector.set(hit2_basis_vector);
    }

    parent_xform.transform_dir(basis_vector);
    basis_vector.normalize();

    for (victims_list_t::const_iterator i = tmp_victims_list.begin(), ie = tmp_victims_list.end(); i != ie; ++i)
    {
        fill_shapes_list(*i, fendpos, tmp_shapes_list);
    }
    std::sort(tmp_shapes_list.begin(), tmp_shapes_list.end(), shapes_compare_predicate);
    fill_shots_list(tmp_shapes_list, query_sphere, dst_dirs);

    return static_cast<u32>(dst_dirs.size());
}

bool CWeaponKnife::SelectBestHitVictim(
    Fvector const& f_pos, Fmatrix& parent_xform, Fvector& fendpos_dest, Fsphere& query_sphere)
{
    CActor* tmp_parent = smart_cast<CActor*>(H_Parent());
    VERIFY(tmp_parent);
    if (!tmp_parent)
        return false;

    if (GetHUDmode())
        tmp_parent->Cameras().hud_camera_Matrix(parent_xform);
    else
        return false;

    parent_xform.transform_dir(m_splash_dir);
    fendpos_dest.set(f_pos).mad(m_splash_dir, m_hit_dist);
    query_sphere.set(fendpos_dest, m_splash_radius);

#ifdef DEBUG
    m_dbg_data.m_spheres.push_back(std::make_pair(fendpos_dest, m_splash_radius));
#endif

    m_spartial_query_res.clear();
    g_SpatialSpace->q_sphere(m_spartial_query_res, 0, STYPE_COLLIDEABLE, fendpos_dest, m_splash_radius);

    if ((m_eHitType == m_eHitType_2) && (!m_spartial_query_res.empty()))
    {
        spartial_base_t::value_type tmp_best_victim = NULL;
        best_victim_selector tmp_selector(tmp_parent->ID(), fendpos_dest, spartial_prefetch_radius, tmp_best_victim);
        std::for_each(m_spartial_query_res.begin(), m_spartial_query_res.end(), tmp_selector);
        m_spartial_query_res.clear();
        if (tmp_best_victim)
            m_spartial_query_res.push_back(tmp_best_victim);
    }
    else
    {
        victim_filter tmp_filter(tmp_parent->ID(), fendpos_dest, spartial_prefetch_radius);
        m_spartial_query_res.erase(std::remove_if(m_spartial_query_res.begin(), m_spartial_query_res.end(), tmp_filter),
            m_spartial_query_res.end());
    }
    return !m_spartial_query_res.empty();
}

bool CWeaponKnife::RayQueryCallback(collide::rq_result& result, LPVOID this_ptr)
{
    CWeaponKnife* me = static_cast<CWeaponKnife*>(this_ptr);
    if (result.O && (result.O->ID() != me->m_except_id))
    {
        me->m_last_picked_obj = result.O;
        return FALSE; // first hit
    }
    return TRUE;
}

IGameObject* CWeaponKnife::TryPick(Fvector const& start_pos, Fvector const& dir, float const dist)
{
    collide::ray_defs tmp_rdefs(start_pos, dir, dist, CDB::OPT_FULL_TEST, collide::rqtObject);
    m_ray_query_results.r_clear();
    m_last_picked_obj = NULL;
    VERIFY(H_Parent());
    m_except_id = H_Parent()->ID();
    Level().ObjectSpace.RayQuery(
        m_ray_query_results, tmp_rdefs, &CWeaponKnife::RayQueryCallback, static_cast<LPVOID>(this), NULL, NULL);
    return m_last_picked_obj;
}

// predicates implementation

CWeaponKnife::victim_filter::victim_filter(u16 except_id, Fvector const& pos, float query_distance)
    : m_except_id(except_id), m_start_pos(pos), m_query_distance(query_distance)
{
}

CWeaponKnife::victim_filter::victim_filter(victim_filter const& copy)
    : m_except_id(copy.m_except_id), m_start_pos(copy.m_start_pos), m_query_distance(copy.m_query_distance)
{
}

bool CWeaponKnife::victim_filter::operator()(spartial_base_t::value_type const& left) const
{
    IGameObject* const tmp_obj = left->dcast_GameObject();
    VERIFY(tmp_obj);
    if (!tmp_obj)
        return true;

    if (tmp_obj->ID() == m_except_id)
        return true;

    CEntityAlive* const tmp_actor = smart_cast<CEntityAlive*>(tmp_obj);
    if (!tmp_actor)
        return true;

    Fvector obj_pos;
    tmp_actor->Center(obj_pos);

    Fvector tmp_dir = Fvector(obj_pos).sub(m_start_pos);
    float const tmp_dist = tmp_dir.magnitude();

    if (tmp_dist > m_query_distance)
        return true;

    return false;
}

CWeaponKnife::best_victim_selector::best_victim_selector(
    u16 except_id, Fvector const& pos, float const query_distance, spartial_base_t::value_type& dest_result)
    : m_except_id(except_id), m_start_pos(pos), m_query_distance(query_distance), m_dest_result(dest_result)
{
    m_dest_result = NULL;
}

CWeaponKnife::best_victim_selector::best_victim_selector(best_victim_selector const& copy)
    : m_min_dist(copy.m_min_dist), m_except_id(copy.m_except_id), m_start_pos(copy.m_start_pos),
      m_query_distance(copy.m_query_distance), m_dest_result(copy.m_dest_result)
{
}

void CWeaponKnife::best_victim_selector::operator()(spartial_base_t::value_type const& left)
{
    IGameObject* const tmp_obj = left->dcast_GameObject();
    VERIFY(tmp_obj);
    if (!tmp_obj)
        return;

    if (tmp_obj->ID() == m_except_id)
        return;

    CEntityAlive* const tmp_actor = smart_cast<CEntityAlive*>(tmp_obj);
    if (!tmp_actor)
        return;

    Fvector obj_pos;
    tmp_actor->Center(obj_pos);
    // m_owner->GetVictimPos	(tmp_actor, obj_pos);

    Fvector tmp_dir = Fvector(obj_pos).sub(m_start_pos);
    float const tmp_dist = tmp_dir.magnitude();
    tmp_dir.normalize();

    if (tmp_dist > m_query_distance)
        return;

    if (!m_dest_result || (m_min_dist > tmp_dist))
    {
        m_dest_result = left;
        m_min_dist = tmp_dist;
        return;
    }
}
