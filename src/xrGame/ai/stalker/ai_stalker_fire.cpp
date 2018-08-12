////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_fire.cpp
//	Created 	: 25.02.2003
//  Modified 	: 25.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Fire and enemy parameters for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "ai_stalker_impl.h"
#include "script_entity_action.h"
#include "Inventory.h"
#include "ef_storage.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "CustomZone.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "agent_manager.h"
#include "stalker_animation_manager.h"
#include "stalker_planner.h"
#include "ef_pattern.h"
#include "memory_manager.h"
#include "hit_memory_manager.h"
#include "enemy_manager.h"
#include "item_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "EntityCondition.h"
#include "sound_player.h"
#include "cover_point.h"
#include "agent_member_manager.h"
#include "agent_location_manager.h"
#include "danger_cover_location.h"
#include "object_handler_planner.h"
#include "object_handler_space.h"
#include "visual_memory_manager.h"
#include "Weapon.h"
#include "ai_stalker_space.h"
#include "EffectorShot.h"
#include "BoneProtections.h"
#include "RadioactiveZone.h"
#include "restricted_object.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Missile.h"
#include "xrPhysics/IPHWorld.h"
#include "stalker_animation_names.h"
#include "agent_corpse_manager.h"
#include "CharacterPhysicsSupport.h"
#include "stalker_planner.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "Inventory.h"
#include "trajectories.h"

using namespace StalkerSpace;

static float const DANGER_DISTANCE = 3.f;
static u32 const DANGER_INTERVAL = 120000;

static float const PRECISE_DISTANCE = 2.5f;
static float const FLOOR_DISTANCE = 2.f;
static float const NEAR_DISTANCE = 2.5f;
static u32 const FIRE_MAKE_SENSE_INTERVAL = 10000;

static float const min_throw_distance = 10.f;

float CAI_Stalker::GetWeaponAccuracy() const
{
    float base = PI / 180.f;

    //влияние ранга на меткость
    base *= m_fRankDisperison;

    if (!movement().path_completed())
    {
        if (movement().movement_type() == eMovementTypeWalk)
            if (movement().body_state() == eBodyStateStand)
                return (base * m_disp_walk_stand);
            else
                return (base * m_disp_walk_crouch);
        else if (movement().movement_type() == eMovementTypeRun)
            if (movement().body_state() == eBodyStateStand)
                return (base * m_disp_run_stand);
            else
                return (base * m_disp_run_crouch);
    }

    if (movement().body_state() == eBodyStateStand)
        if (zoom_state())
            return (base * m_disp_stand_stand);
        else
            return (base * m_disp_stand_stand_zoom);
    else if (zoom_state())
        return (base * m_disp_stand_crouch);
    else
        return (base * m_disp_stand_crouch_zoom);
}

void CAI_Stalker::g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D)
{
    //.	VERIFY				(inventory().ActiveItem());
    if (!inventory().ActiveItem())
    {
#ifdef DEBUG
        Msg("! CAI_Stalker::g_fireParams() : VERIFY(inventory().ActiveItem())");
#endif // DEBUG
        P = Position();
        D = Fvector().set(0.f, 0.f, 1.f);
        return;
    }

    CWeapon* weapon = smart_cast<CWeapon*>(inventory().ActiveItem());
    if (!weapon)
    {
        CMissile* missile = smart_cast<CMissile*>(inventory().ActiveItem());
        if (missile)
        {
            update_throw_params();
            P = m_throw_position;
            D = Fvector().set(m_throw_velocity).normalize();
            VERIFY(!fis_zero(D.square_magnitude()));
            return;
        }

        P = eye_matrix.c;
        D = eye_matrix.k;

        if (weapon_shot_effector().IsActive())
            D = weapon_shot_effector_direction(D);

        VERIFY(!fis_zero(D.square_magnitude()));
        return;
    }

    if (!g_Alive())
    {
        P = weapon->get_LastFP();
        D = weapon->get_LastFD();
        VERIFY(!fis_zero(D.square_magnitude()));
        return;
    }

    if (!animation().script_animations().empty() || animation().global_selector())
    {
        P = weapon->get_LastFP();

        if (sniper_fire_mode())
            D.setHP(-movement().m_head.target.yaw, -movement().m_head.target.pitch);
        else
            D = weapon->get_LastFD();
        VERIFY(!fis_zero(D.square_magnitude()));

        return;
    }

    switch (movement().body_state())
    {
    case eBodyStateStand:
    {
        if (movement().movement_type() == eMovementTypeStand)
        {
            P = eye_matrix.c;
            D = eye_matrix.k;
            if (weapon_shot_effector().IsActive())
                D = weapon_shot_effector_direction(D);
            VERIFY(!fis_zero(D.square_magnitude()));
        }
        else
        {
            D.setHP(-movement().m_head.current.yaw, -movement().m_head.current.pitch);
            if (weapon_shot_effector().IsActive())
                D = weapon_shot_effector_direction(D);
            Center(P);
            P.mad(D, .5f);
            P.y += .50f;
            VERIFY(!fis_zero(D.square_magnitude()));
        }

        if (sniper_fire_mode())
            D.setHP(-movement().m_head.target.yaw, -movement().m_head.target.pitch);

        return;
    }
    case eBodyStateCrouch:
    {
        P = eye_matrix.c;
        D = eye_matrix.k;
        if (weapon_shot_effector().IsActive())
            D = weapon_shot_effector_direction(D);

        VERIFY(!fis_zero(D.square_magnitude()));

        if (sniper_fire_mode())
            D.setHP(-movement().m_head.target.yaw, -movement().m_head.target.pitch);

        return;
    }
    default: NODEFAULT;
    }

#ifdef DEBUG
    P = weapon->get_LastFP();
    D = weapon->get_LastFD();
    VERIFY(!fis_zero(D.square_magnitude()));
#endif
}

void CAI_Stalker::g_WeaponBones(int& L, int& R1, int& R2)
{
    int r_hand, r_finger2, l_finger1;
    CObjectHandler::weapon_bones(r_hand, r_finger2, l_finger1);
    R1 = r_hand;
    R2 = r_finger2;
    if (!animation().script_animations().empty() && animation().script_animations().front().hand_usage())
        L = R2;
    else
        L = l_finger1;
}

void CAI_Stalker::Hit(SHit* pHDS)
{
    //хит может меняться в зависимости от ранга (новички получают больше хита, чем ветераны)
    SHit HDS = *pHDS;
    HDS.add_wound = true;

    float hit_power = HDS.power * m_fRankImmunity;

    if (m_boneHitProtection && HDS.hit_type == ALife::eHitTypeFireWound)
    {
        float BoneArmor = m_boneHitProtection->getBoneArmor(HDS.bone());
        float ap = HDS.armor_piercing;
        if (!fis_zero(BoneArmor, EPS))
        {
            if (ap > BoneArmor)
            {
                float d_hit_power = (ap - BoneArmor) / ap;
                if (d_hit_power < m_boneHitProtection->m_fHitFracNpc)
                    d_hit_power = m_boneHitProtection->m_fHitFracNpc;

                hit_power *= d_hit_power;
                VERIFY(hit_power >= 0.0f);
            }
            else
            {
                hit_power *= m_boneHitProtection->m_fHitFracNpc;
                HDS.add_wound = false;
            }
        }

        if (wounded()) //уже лежит => добивание
        {
            hit_power = 1000.f;
        }
    }
    HDS.power = hit_power;

    if (g_Alive())
    {
        bool already_critically_wounded = critically_wounded();

        if (!already_critically_wounded)
        {
            const CCoverPoint* cover = agent_manager().member().member(this).cover();
            if (!invulnerable() && cover && HDS.initiator() && (HDS.initiator()->ID() != ID()) &&
                !fis_zero(HDS.damage()) && brain().affect_cover())
            {
                agent_manager().location().add(
                    new CDangerCoverLocation(cover, Device.dwTimeGlobal, DANGER_INTERVAL, DANGER_DISTANCE));
            }
        }

        const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(HDS.initiator());
        if (entity_alive && !wounded())
        {
            if (is_relation_enemy(entity_alive))
                sound().play(eStalkerSoundInjuring);
            //			else
            //				sound().play		(eStalkerSoundInjuringByFriend);
        }

        int weapon_type = -1;
        if (best_weapon())
            weapon_type = best_weapon()->object().ef_weapon_type();

        if (!wounded() && !already_critically_wounded)
        {
            bool became_critically_wounded = update_critical_wounded(HDS.boneID, HDS.power);
            if (!became_critically_wounded && animation().script_animations().empty() && (HDS.bone() != BI_NONE))
            {
                Fvector D;
                float yaw, pitch;
                D.getHP(yaw, pitch);

#pragma todo("Dima to Dima : forward-back bone impulse direction has been determined incorrectly!")
                float power_factor = m_power_fx_factor * HDS.damage() / 100.f;
                clamp(power_factor, 0.f, 1.f);

                // IKinematicsAnimated		*tpKinematics = smart_cast<IKinematicsAnimated*>(Visual());
                IKinematics* tpKinematics = smart_cast<IKinematics*>(Visual());
#ifdef DEBUG
                tpKinematics->LL_GetBoneInstance(HDS.bone());
                if (HDS.bone() >= tpKinematics->LL_BoneCount())
                {
                    Msg("tpKinematics has no bone_id %d", HDS.bone());
                    HDS._dump();
                }
#endif
                //				int						fx_index =
                //iFloor(tpKinematics->LL_GetBoneInstance(HDS.bone()).get_param(1)
                //+
                //(angle_difference(movement().m_body.current.yaw,-yaw) <= PI_DIV_2 ? 0 : 1));
                //				if (fx_index != -1)
                //					animation().play_fx	(power_factor,fx_index);
            }
            else
            {
                if (!already_critically_wounded && became_critically_wounded)
                {
                    if (HDS.who)
                    {
                        CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(HDS.who);
                        if (stalker && stalker->g_Alive())
                            stalker->on_critical_wound_initiator(this);
                    }
                }
            }
        }
    }

    if (g_Alive() && (!m_hit_callback || m_hit_callback(&HDS)))
    {
        float const damage_factor = invulnerable() ? 0.f : 100.f;
        memory().hit().add(damage_factor * HDS.damage(), HDS.direction(), HDS.who, HDS.boneID);
    }

    // conditions().health()			= 1.f;

    inherited::Hit(&HDS);
}

void CAI_Stalker::HitSignal(float amount, Fvector& vLocalDir, IGameObject* who, s16 element) {}
void CAI_Stalker::OnItemTake(CInventoryItem* inventory_item)
{
    CObjectHandler::OnItemTake(inventory_item);
    m_item_actuality = false;
    m_sell_info_actuality = false;
}

void CAI_Stalker::OnItemDrop(CInventoryItem* inventory_item, bool just_before_destroy)
{
    CObjectHandler::OnItemDrop(inventory_item, just_before_destroy);

    m_item_actuality = false;
    m_sell_info_actuality = false;

    if (!g_Alive())
        return;

    if (!critically_wounded())
        return;

    //	VERIFY						(inventory().ActiveItem());

    if (inventory().ActiveItem() && (inventory().ActiveItem() != inventory_item))
        return;

    brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, false);
}

void CAI_Stalker::update_best_item_info()
{
    if (m_bTrading || !m_can_select_weapon)
        return;

    update_best_item_info_impl();
}

void CAI_Stalker::update_best_item_info_impl()
{
    ai().ef_storage().alife_evaluation(false);
    if (m_item_actuality && m_best_item_to_kill && m_best_item_to_kill->can_kill())
    {
        if (!memory().enemy().selected())
            return;

        ai().ef_storage().non_alife().member() = this;
        ai().ef_storage().non_alife().enemy() = memory().enemy().selected() ? memory().enemy().selected() : this;
        ai().ef_storage().non_alife().member_item() = &m_best_item_to_kill->object();
        float value;
        value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
        if (fsimilar(value, m_best_item_value))
            return;
    }

    // initialize parameters
    m_item_actuality = true;
    ai().ef_storage().non_alife().member() = this;
    ai().ef_storage().non_alife().enemy() = memory().enemy().selected() ? memory().enemy().selected() : this;
    m_best_item_to_kill = 0;
    m_best_ammo = 0;
    m_best_found_item_to_kill = 0;
    m_best_found_ammo = 0;
    m_best_item_value = 0.f;

    // try to find the best item which can kill
    {
        TIItemContainer::iterator I = inventory().m_all.begin();
        TIItemContainer::iterator E = inventory().m_all.end();
        for (; I != E; ++I)
        {
            if ((*I)->can_kill())
            {
                ai().ef_storage().non_alife().member_item() = &(*I)->object();
                float value;
                if (memory().enemy().selected())
                    value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
                else
                    value = (float)(*I)->Cost();

                if (!fsimilar(value, m_best_item_value) && (value < m_best_item_value))
                    continue;

                if (!fsimilar(value, m_best_item_value) && (value > m_best_item_value))
                {
                    m_best_item_value = value;
                    m_best_item_to_kill = *I;
                    continue;
                }

                VERIFY(fsimilar(value, m_best_item_value));
                if (m_best_item_to_kill && ((*I)->Cost() <= m_best_item_to_kill->Cost()))
                    continue;

                m_best_item_value = value;
                m_best_item_to_kill = *I;
            }
        }
    }

    // check if we found
    if (m_best_item_to_kill)
    {
        m_best_ammo = m_best_item_to_kill;
        return;
    }

    // so we do not have such an item
    // check if we remember we saw item which can kill
    // or items which can make my item killing
    {
        xr_vector<const CGameObject*>::const_iterator I = memory().item().objects().begin();
        xr_vector<const CGameObject*>::const_iterator E = memory().item().objects().end();
        for (; I != E; ++I)
        {
            const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
            if (!inventory_item || !memory().item().useful(&inventory_item->object()))
                continue;
            CInventoryItem* item = inventory_item->can_kill(&inventory());
            if (item)
            {
                ai().ef_storage().non_alife().member_item() = &inventory_item->object();
                float value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
                if (value > m_best_item_value)
                {
                    m_best_item_value = value;
                    m_best_found_item_to_kill = inventory_item;
                    m_best_found_ammo = 0;
                    m_best_ammo = item;
                }
            }
            else
            {
                item = inventory_item->can_make_killing(&inventory());
                if (!item)
                    continue;

                ai().ef_storage().non_alife().member_item() = &item->object();
                float value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
                if (value > m_best_item_value)
                {
                    m_best_item_value = value;
                    m_best_item_to_kill = item;
                    m_best_found_item_to_kill = 0;
                    m_best_found_ammo = inventory_item;
                }
            }
        }
    }

    // check if we found such an item
    if (m_best_found_item_to_kill || m_best_found_ammo)
        return;

    // check if we remember we saw item to kill
    // and item which can make this item killing
    xr_vector<const CGameObject*>::const_iterator I = memory().item().objects().begin();
    xr_vector<const CGameObject*>::const_iterator E = memory().item().objects().end();
    for (; I != E; ++I)
    {
        const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
        if (!inventory_item || !memory().item().useful(&inventory_item->object()))
            continue;
        const CInventoryItem* item = inventory_item->can_kill(memory().item().objects());
        if (item)
        {
            ai().ef_storage().non_alife().member_item() = &inventory_item->object();
            float value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
            if (value > m_best_item_value)
            {
                m_best_item_value = value;
                m_best_found_item_to_kill = inventory_item;
                m_best_found_ammo = item;
            }
        }
    }
}

bool CAI_Stalker::item_to_kill()
{
    update_best_item_info();
    return (!!m_best_item_to_kill);
}

bool CAI_Stalker::item_can_kill()
{
    update_best_item_info();
    return (!!m_best_ammo);
}

bool CAI_Stalker::remember_item_to_kill()
{
    update_best_item_info();
    return (!!m_best_found_item_to_kill);
}

bool CAI_Stalker::remember_ammo()
{
    update_best_item_info();
    return (!!m_best_found_ammo);
}

bool CAI_Stalker::ready_to_kill()
{
    return (m_best_item_to_kill && inventory().ActiveItem() &&
        (inventory().ActiveItem()->object().ID() == m_best_item_to_kill->object().ID()) &&
        m_best_item_to_kill->ready_to_kill());
}

bool CAI_Stalker::ready_to_detour()
{
    if (!ready_to_kill())
        return (false);

    CWeapon* weapon = smart_cast<CWeapon*>(m_best_item_to_kill);
    if (!weapon)
        return (false);

    return (weapon->GetAmmoElapsed() > weapon->GetAmmoMagSize() / 2);
}

class ray_query_param
{
public:
    CAI_Stalker* m_holder;
    float m_power;
    float m_power_threshold;
    bool m_can_kill_enemy;
    bool m_can_kill_member;
    float m_pick_distance;

public:
    IC ray_query_param(const CAI_Stalker* holder, float power_threshold, float distance)
    {
        m_holder = const_cast<CAI_Stalker*>(holder);
        m_power_threshold = power_threshold;
        m_power = 1.f;
        m_can_kill_enemy = false;
        m_can_kill_member = false;
        m_pick_distance = distance;
    }
};

IC BOOL ray_query_callback(collide::rq_result& result, LPVOID params)
{
    ray_query_param* param = (ray_query_param*)params;
    float power = param->m_holder->feel_vision_mtl_transp(result.O, result.element);
    param->m_power *= power;

    //	if (power >= .05f) {
    //		param->m_pick_distance			= result.range;
    //		return							(true);
    //	}

    if (!result.O)
    {
        if (param->m_power > param->m_power_threshold)
            return (true);

        param->m_pick_distance = result.range;
        return (false);
    }

    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(result.O);
    if (!entity_alive)
    {
        if (param->m_power > param->m_power_threshold)
            return (true);

        param->m_pick_distance = result.range;
        return (false);
    }

    if (param->m_holder->is_relation_enemy(entity_alive))
        param->m_can_kill_enemy = true;
    else
        param->m_can_kill_member = true;

    param->m_pick_distance = result.range;
    return (false);
}

void CAI_Stalker::can_kill_entity(
    const Fvector& position, const Fvector& direction, float distance, collide::rq_results& rq_storage)
{
    VERIFY(!fis_zero(direction.square_magnitude()));

    collide::ray_defs ray_defs(position, direction, distance, CDB::OPT_CULL, collide::rqtBoth);
    VERIFY(!fis_zero(ray_defs.dir.square_magnitude()));

    ray_query_param params(this, memory().visual().transparency_threshold(), distance);

    Level().ObjectSpace.RayQuery(rq_storage, ray_defs, ray_query_callback, &params, NULL, this);
    m_can_kill_enemy = m_can_kill_enemy || params.m_can_kill_enemy;
    m_can_kill_member = m_can_kill_member || params.m_can_kill_member;
    m_pick_distance = _max(m_pick_distance, params.m_pick_distance);
}

void CAI_Stalker::can_kill_entity_from(const Fvector& position, Fvector direction, float distance)
{
    m_pick_distance = 0.f;
    rq_storage.r_clear();
    can_kill_entity(position, direction, distance, rq_storage);
    if (m_can_kill_member && m_can_kill_enemy)
        return;

    float yaw, pitch, safety_fire_angle = PI_DIV_8 * .125f;
    direction.getHP(yaw, pitch);

    direction.setHP(yaw, pitch - safety_fire_angle);
    can_kill_entity(position, direction, distance, rq_storage);
    if (m_can_kill_member && m_can_kill_enemy)
        return;

    direction.setHP(yaw, pitch + safety_fire_angle);
    can_kill_entity(position, direction, distance, rq_storage);
    if (m_can_kill_member)
        return;

    bool can_kill_enemy = m_can_kill_enemy;

    direction.setHP(yaw - safety_fire_angle, pitch);
    can_kill_entity(position, direction, distance, rq_storage);
    m_can_kill_enemy = can_kill_enemy;
    if (m_can_kill_member)
        return;

    direction.setHP(yaw + safety_fire_angle, pitch);
    can_kill_entity(position, direction, distance, rq_storage);
    m_can_kill_enemy = can_kill_enemy;
}

IC float CAI_Stalker::start_pick_distance() const
{
    float result = 50.f;
    if (!memory().enemy().selected())
        return (result);

    return (_max(result, memory().enemy().selected()->Position().distance_to(Position()) + 1.f));
}

float CAI_Stalker::pick_distance()
{
    if (!inventory().ActiveItem())
        return (start_pick_distance());

    update_can_kill_info();
    return (m_pick_distance);
}

void CAI_Stalker::update_can_kill_info()
{
    if (m_pick_frame_id == Device.dwFrame)
        return;

    m_pick_frame_id = Device.dwFrame;
    m_can_kill_member = false;
    m_can_kill_enemy = false;

    Fvector position, direction;
    VERIFY(inventory().ActiveItem());
    g_fireParams(0, position, direction);
    can_kill_entity_from(position, direction, start_pick_distance());
}

bool CAI_Stalker::undetected_anomaly()
{
    return (
        inside_anomaly() || brain().CStalkerPlanner::m_storage.property(StalkerDecisionSpace::eWorldPropertyAnomaly));
}

bool CAI_Stalker::inside_anomaly()
{
    xr_vector<IGameObject*>::const_iterator I = feel_touch.begin();
    xr_vector<IGameObject*>::const_iterator E = feel_touch.end();
    for (; I != E; ++I)
    {
        CCustomZone* zone = smart_cast<CCustomZone*>(*I);
        if (zone && (zone->restrictor_type() != RestrictionSpace::eRestrictorTypeNone))
        {
            if (smart_cast<CRadioactiveZone*>(zone))
                continue;

            return (true);
        }
    }
    return (false);
}

bool CAI_Stalker::zoom_state() const
{
    if (!inventory().ActiveItem())
        return (false);

    if ((movement().movement_type() != eMovementTypeStand) && (movement().body_state() != eBodyStateCrouch) &&
        !movement().path_completed())
        return (false);

    switch (CObjectHandler::planner().current_action_state_id())
    {
    case ObjectHandlerSpace::eWorldOperatorAim1:
    case ObjectHandlerSpace::eWorldOperatorAim2:
    case ObjectHandlerSpace::eWorldOperatorAimingReady1:
    case ObjectHandlerSpace::eWorldOperatorAimingReady2:
    case ObjectHandlerSpace::eWorldOperatorQueueWait1:
    case ObjectHandlerSpace::eWorldOperatorQueueWait2:
    case ObjectHandlerSpace::eWorldOperatorFire1:
    // we need this 2 operators to prevent fov/range switching
    case ObjectHandlerSpace::eWorldOperatorReload1:
    case ObjectHandlerSpace::eWorldOperatorReload2:
    case ObjectHandlerSpace::eWorldOperatorForceReload1:
    case ObjectHandlerSpace::eWorldOperatorForceReload2: return (true);
    }

    return (false);
}

void CAI_Stalker::update_range_fov(float& new_range, float& new_fov, float start_range, float start_fov)
{
    float range = start_range, fov = start_fov;

    if (zoom_state())
        inventory().ActiveItem()->modify_holder_params(range, fov);

    VERIFY2(start_fov < 180.f, make_string("[%s] %f", cName().c_str(), start_fov));
    VERIFY2(fov < 180.f, make_string("fix addon multiplier for weapon or scope %s (fov=%f)",
                             inventory().ActiveItem()->object().cName().c_str(), fov));

    return (inherited::update_range_fov(new_range, new_fov, range, fov));
}

bool CAI_Stalker::fire_make_sense()
{
    // if we do not have an enemy
    const CEntityAlive* enemy = memory().enemy().selected();
    if (!enemy)
        return (false);

    if ((pick_distance() + PRECISE_DISTANCE) < Position().distance_to(enemy->Position()))
        return (false);

    if (_abs(Position().y - enemy->Position().y) > FLOOR_DISTANCE)
        return (false);

    if (pick_distance() < NEAR_DISTANCE)
        return (false);

    if (memory().visual().visible_right_now(enemy))
        return (true);

    u32 last_time_seen = memory().visual().visible_object_time_last_seen(enemy);
    if (last_time_seen == u32(-1))
        return (false);

    if (Device.dwTimeGlobal > last_time_seen + FIRE_MAKE_SENSE_INTERVAL)
        return (false);

    // if we do not have a weapon
    if (!best_weapon())
        return (false);

    // if we do not have automatic weapon
    switch (best_weapon()->object().ef_weapon_type())
    {
    case 6:
    case 7:
    case 8:
    case 10: break;
    default: return (false);
    }

    return (true);
}

// shot effector stuff
void CAI_Stalker::on_weapon_shot_start(CWeapon* weapon)
{
    weapon_shot_effector().SetRndSeed(m_weapon_shot_random_seed);
    weapon_shot_effector().Shot(weapon);
}

void CAI_Stalker::on_weapon_shot_update() {}
void CAI_Stalker::on_weapon_shot_stop() {}
void CAI_Stalker::on_weapon_shot_remove(CWeapon* weapon) {}
void CAI_Stalker::on_weapon_hide(CWeapon* weapon) {}
void CAI_Stalker::notify_on_wounded_or_killed(IGameObject* object)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(object);
    if (!stalker)
        return;

    if (!stalker->g_Alive())
        return;

    stalker->on_enemy_wounded_or_killed(this);

    typedef CAgentCorpseManager::MEMBER_CORPSES MEMBER_CORPSES;

    const MEMBER_CORPSES& corpses = agent_manager().corpse().corpses();
    if (std::find(corpses.begin(), corpses.end(), this) != corpses.end())
        return;

    agent_manager().corpse().register_corpse(this);
}

void CAI_Stalker::notify_on_wounded_or_killed()
{
    ALife::_OBJECT_ID last_hit_object_id = memory().hit().last_hit_object_id();
    if (last_hit_object_id == ALife::_OBJECT_ID(-1))
        return;

    IGameObject* object = Level().Objects.net_Find(last_hit_object_id);
    if (!object)
        return;

    notify_on_wounded_or_killed(object);
}

void CAI_Stalker::wounded(bool value)
{
    if (m_wounded == value)
        return;

    if (!g_Alive())
        return;

    if (value)
        notify_on_wounded_or_killed();

    m_wounded = value;

    if (!m_wounded && g_Alive())
        character_physics_support()->CreateCharacterSafe();

    if (!m_wounded)
        return;

    character_physics_support()->movement()->DestroyCharacter();

    if (!agent_manager().member().registered_in_combat(this))
        return;

    agent_manager().member().unregister_in_combat(this);
}

bool CAI_Stalker::wounded(const CRestrictedObject* object) const
{
    if (!wounded())
        return (false);

    VERIFY(object);
    if (!object->accessible(Position()))
        return (false);

    if (!object->accessible(ai_location().level_vertex_id()))
        return (false);

    return (true);
}

bool CAI_Stalker::use_default_throw_force() { return (false); }
bool CAI_Stalker::use_throw_randomness() { return (false); }
float CAI_Stalker::missile_throw_force()
{
    update_throw_params();
    return (m_throw_velocity.magnitude());
}

void CAI_Stalker::compute_throw_miss(u32 const vertex_id)
{
    float const throw_miss_radius = ::Random.randF(2.f, 5.f);
    u32 const try_count = 6;

    CLevelGraph const& level_graph = ai().level_graph();
    for (u32 i = 0; i < try_count; ++i)
    {
        Fvector const direction = Fvector().random_dir();
        Fvector const check_position = Fvector().mad(m_throw_target_position, direction, throw_miss_radius);
        u32 const check_vertex_id =
            level_graph.check_position_in_direction(vertex_id, m_throw_target_position, check_position);
        if (!level_graph.valid_vertex_id(check_vertex_id))
            continue;

        m_throw_target_position = check_position;
        return;
    }
}

void CAI_Stalker::throw_target_impl(const Fvector& position, IGameObject* throw_ignore_object)
{
    float distance_to_sqr = position.distance_to_sqr(m_throw_target_position);
    m_throw_actual = m_throw_actual && (distance_to_sqr < _sqr(.1f));
    m_throw_target_position = position;
    m_throw_ignore_object = throw_ignore_object;
}

void CAI_Stalker::throw_target(const Fvector& position, IGameObject* throw_ignore_object)
{
    throw_target_impl(position, throw_ignore_object);
}

void CAI_Stalker::throw_target(const Fvector& position, u32 const vertex_id, IGameObject* throw_ignore_object)
{
    throw_target_impl(position, throw_ignore_object);
    compute_throw_miss(vertex_id);
}

// delete this function!!!!
bool CAI_Stalker::throw_check_error(
    float low, float high, const Fvector& position, const Fvector& velocity, const Fvector& gravity)
{
    return true;
}

void CAI_Stalker::check_throw_trajectory(const float& throw_time)
{
    m_throw_enabled = false;

    xr_vector<trajectory_pick>* trajectory_picks = NULL;
    xr_vector<Fvector>* collide_tris = NULL;
#ifdef DEBUG
    trajectory_picks = &m_throw_picks;
    collide_tris = &m_throw_collide_tris;
#endif // #ifdef DEBUG

    Fvector box_size = {0.f, 0.f, 0.f};
    // Fvector box_size				=	{ 0.1f, 0.1f , 0.1f };

    if (!trajectory_intersects_geometry(throw_time, m_throw_position, m_throw_target_position, m_throw_velocity,
            m_throw_collide_position, this, m_throw_ignore_object, rq_storage, trajectory_picks, collide_tris,
            box_size))
    {
        m_throw_collide_position = Fvector().set(flt_max, flt_max, flt_max);
        m_throw_enabled = true;
    }
}

void CAI_Stalker::update_throw_params()
{
    if (m_throw_actual)
    {
        if (m_computed_object_position.similar(Position()))
        {
            if (m_computed_object_direction.similar(Direction()))
            {
                VERIFY(_valid(m_throw_velocity));
                return;
            }
        }
    }

    m_throw_actual = true;
    m_computed_object_position = Position();
    m_computed_object_direction = Direction();

#if 0
	m_throw_position		= eye_matrix.c;
#else
    m_throw_position = Position();

    CMissile* const pMissile = dynamic_cast<CMissile*>(inventory().ActiveItem());
    if (pMissile)
    {
        static const LPCSTR third_person_offset_id = "third_person_throw_point_offset";

        if (!pSettings->line_exist(pMissile->cNameSect(), third_person_offset_id))
        {
            m_throw_position.y += 2.f;

            if (pMissile)
            {
                Fvector offset;
                XFORM().transform_dir(offset, pMissile->throw_point_offset());
                m_throw_position.add(offset);
            }
        }
        else
            m_throw_position.add(pSettings->r_fvector3(pMissile->cNameSect(), third_person_offset_id));
    }
#endif

    static float const distances[] = {30.f, 40.f, 50.f, 60.f};
    VERIFY(g_SingleGameDifficulty < sizeof(distances) / sizeof(distances[0]));
    float const max_distance = distances[g_SingleGameDifficulty];

    // computing velocity with minimum magnitude
    m_throw_velocity.sub(m_throw_target_position, m_throw_position);

    if (m_throw_velocity.magnitude() > max_distance)
    {
        m_throw_enabled = false;

#ifdef DEBUG
        m_throw_picks.clear();
#endif // DEBUG
        return;
    }

    float time = ThrowMinVelTime(m_throw_velocity, physics_world()->Gravity());
    TransferenceToThrowVel(m_throw_velocity, time, physics_world()->Gravity());

    check_throw_trajectory(time);

    m_throw_velocity.mul(::Random.randF(.99f, 1.01f));
}

void CAI_Stalker::on_throw_completed()
{
    agent_manager().member().on_throw_completed();
    m_last_throw_time = Device.dwTimeGlobal;
}

bool CAI_Stalker::critically_wounded()
{
    if (critical_wound_type() == critical_wound_type_dummy)
        return (false);

    if (!brain().CStalkerPlanner::m_storage.property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded))
    {
        critical_wounded_state_stop();
        return (false);
    }

    return (true);
}

bool CAI_Stalker::critical_wound_external_conditions_suitable()
{
    if (movement().body_state() != eBodyStateStand)
        return (false);

    if (animation().non_script_need_update())
        return (false);

    CWeapon* active_weapon = smart_cast<CWeapon*>(inventory().ActiveItem());
    if (!active_weapon)
        return (false);

    if (movement().in_smart_cover())
        return (false);

    switch (active_weapon->animation_slot())
    {
    case 1: // pistols
    case 2: // automatic weapon
    case 3: // rifles
        break;
    default: return (false);
    }

    if (!agent_manager().member().registered_in_combat(this))
        return (false);

    //	Msg								("%6d executing critical hit",Device.dwTimeGlobal);
    animation().global().make_inactual();
    return (true);
}

void CAI_Stalker::remove_critical_hit()
{
    brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, false);

    animation().global().remove_callback(CStalkerAnimationPair::CALLBACK_ID(this, &CAI_Stalker::remove_critical_hit));
}

void CAI_Stalker::critical_wounded_state_start()
{
    brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, true);

    animation().global().add_callback(CStalkerAnimationPair::CALLBACK_ID(this, &CAI_Stalker::remove_critical_hit));
}

bool CAI_Stalker::can_cry_enemy_is_wounded() const
{
    if (!brain().initialized())
        return (false);

    if (brain().current_action_id() != StalkerDecisionSpace::eWorldOperatorCombatPlanner)
        return (false);

    if (!agent_manager().member().group_behaviour())
        return (false);

    typedef CActionPlannerActionScript<CAI_Stalker> planner_type;
    planner_type* planner = smart_cast<planner_type*>(&brain().current_action());
    VERIFY(planner);

    switch (planner->current_action_id())
    {
    case StalkerDecisionSpace::eWorldOperatorGetReadyToKill:
    case StalkerDecisionSpace::eWorldOperatorGetReadyToDetour:
    case StalkerDecisionSpace::eWorldOperatorKillEnemy:
    case StalkerDecisionSpace::eWorldOperatorTakeCover:
    case StalkerDecisionSpace::eWorldOperatorLookOut:
    case StalkerDecisionSpace::eWorldOperatorHoldPosition:
    case StalkerDecisionSpace::eWorldOperatorDetourEnemy:
    case StalkerDecisionSpace::eWorldOperatorSearchEnemy:
    case StalkerDecisionSpace::eWorldOperatorKillEnemyIfNotVisible:
    case StalkerDecisionSpace::eWorldOperatorKillEnemyIfCriticallyWounded:
    case StalkerDecisionSpace::eWorldOperatorThrowGrenade:
    case StalkerDecisionSpace::eWorldOperatorGetDistance:
    case StalkerDecisionSpace::eWorldOperatorLowCover:
    case StalkerDecisionSpace::eWorldOperatorInSmartCover: return (true);
    case StalkerDecisionSpace::eWorldOperatorGetItemToKill:
    case StalkerDecisionSpace::eWorldOperatorRetreatFromEnemy:
    case StalkerDecisionSpace::eWorldOperatorPostCombatWait:
    case StalkerDecisionSpace::eWorldOperatorHideFromGrenade:
    case StalkerDecisionSpace::eWorldOperatorSuddenAttack:
    case StalkerDecisionSpace::eWorldOperatorKillEnemyIfPlayerOnThePath:
    case StalkerDecisionSpace::eWorldOperatorKillWoundedEnemy:
    case StalkerDecisionSpace::eWorldOperatorCriticallyWounded: return (false);
    default: NODEFAULT;
    }
#ifdef DEBUG
    return (false);
#endif // DEBUG
}

void CAI_Stalker::on_critical_wound_initiator(const CAI_Stalker* critically_wounded)
{
    if (!g_Alive())
        return;

    if (!can_cry_enemy_is_wounded())
        return;

    sound().play(eStalkerSoundEnemyCriticallyWounded);
}

void CAI_Stalker::on_enemy_wounded_or_killed(const CAI_Stalker* wounded_or_killed)
{
    if (!g_Alive())
        return;

    if (!can_cry_enemy_is_wounded())
        return;

    sound().play(eStalkerSoundEnemyKilledOrWounded);
}

bool CAI_Stalker::can_kill_member()
{
    if (!animation().script_animations().empty())
        return (false);

    update_can_kill_info();
    return (m_can_kill_member);
}

bool CAI_Stalker::can_kill_enemy()
{
    VERIFY(inventory().ActiveItem());
    update_can_kill_info();
    return (m_can_kill_enemy);
}

bool CAI_Stalker::too_far_to_kill_enemy(const Fvector& position)
{
#if 1
    return (false);
#else
    VERIFY(memory().enemy().selected());
    VERIFY(best_weapon());

    int weapon_type = best_weapon()->object().ef_weapon_type();
    float distance = position.distance_to(Position());
    switch (weapon_type)
    {
    // pistols
    case 5:
        return (distance > 10.f);
    // shotguns
    case 9:
        return (distance > 5.f);
    // sniper rifles
    case 11:
    case 12: return (distance > 70.f);
    default: return (distance > 5.f);
    }
#endif
}
