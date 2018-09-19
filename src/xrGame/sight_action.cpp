////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_action.cpp
//	Created 	: 27.12.2003
//  Modified 	: 03.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Sight action
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "sight_action.h"
#include "ai/stalker/ai_stalker.h"
#include "sight_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"

//#define SIGHT_TEST

void CSightAction::initialize()
{
    VERIFY(!m_initialized);
    m_initialized = true;

    m_start_time = Device.dwTimeGlobal;
    m_state_fire_switch_time = Device.dwTimeGlobal;
    m_already_switched = false;
    m_internal_state = u32(-1);

    if (SightManager::eSightTypeCoverLookOver == m_sight_type)
        initialize_cover_look_over();

    if (SightManager::eSightTypeFireObject == m_sight_type)
        initialize_fire_object();
}

void CSightAction::finalize()
{
    VERIFY(m_initialized);
    m_initialized = false;
}

void CSightAction::execute()
{
    VERIFY(m_initialized);

    switch (m_sight_type)
    {
    case SightManager::eSightTypeCurrentDirection:
    {
        execute_current_direction();
        break;
    }
    case SightManager::eSightTypePathDirection:
    {
        execute_path_direction();
        break;
    }
    case SightManager::eSightTypeDirection:
    {
        execute_direction();
        break;
    }
    case SightManager::eSightTypePosition:
    {
        execute_position(m_object->eye_matrix.c);
        break;
    }
    case SightManager::eSightTypeObject:
    {
        execute_object();
        break;
    }
    case SightManager::eSightTypeCover:
    {
        execute_cover();
        break;
    }
    case SightManager::eSightTypeSearch:
    {
        execute_search();
        break;
    }
    case SightManager::eSightTypeCoverLookOver:
    {
        execute_cover_look_over();
        break;
    }
    case SightManager::eSightTypeFireObject:
    {
        execute_fire_object();
        break;
    }
    case SightManager::eSightTypeAnimationDirection:
    {
        execute_animation_direction();
        break;
    }
    default: NODEFAULT;
    }
}

void CSightAction::remove_links(IGameObject* object)
{
    if (!m_object_to_look)
        return;

    if (m_object_to_look->ID() != object->ID())
        return;

    //	execute				();

    m_object_to_look = 0;

    m_sight_type = SightManager::eSightTypeDirection;
    m_vector3d.setHP(-this->object().movement().m_head.target.yaw, this->object().movement().m_head.target.pitch);
}

bool CSightAction::target_reached()
{
    return (!!fsimilar(angle_normalize_signed(object().movement().m_head.target.yaw),
        angle_normalize_signed(object().movement().m_head.current.yaw)));
}

void CSightAction::execute_current_direction()
{
    object().movement().m_head.target = object().movement().m_head.current;
#ifdef SIGHT_TEST
    Msg("%6d eSightTypeCurrentDirection", Device.dwTimeGlobal);
#endif
}

void CSightAction::execute_path_direction()
{
    object().sight().SetDirectionLook();
#ifdef SIGHT_TEST
    Msg("%6d eSightTypePathDirection", Device.dwTimeGlobal);
#endif
}

void CSightAction::execute_direction()
{
    m_vector3d.getHP(object().movement().m_head.target.yaw, object().movement().m_head.target.pitch);
    object().movement().m_head.target.yaw *= -1;
    object().movement().m_head.target.pitch *= -1;
#ifdef SIGHT_TEST
    Msg("%6d eSightTypeDirection", Device.dwTimeGlobal);
#endif
}

void CSightAction::execute_position(Fvector const& look_position)
{
    if (m_torso_look)
        object().sight().SetFirePointLookAngles(
            m_vector3d, object().movement().m_head.target.yaw, object().movement().m_head.target.pitch, look_position);
    else
        object().sight().SetPointLookAngles(
            m_vector3d, object().movement().m_head.target.yaw, object().movement().m_head.target.pitch, look_position);

#ifdef SIGHT_TEST
    Msg("%6d %s", Device.dwTimeGlobal, m_torso_look ? "eSightTypeFirePosition" : "eSightTypePosition");
#endif
}

void CSightAction::execute_object()
{
    Fvector look_pos;
    m_object_to_look->Center(look_pos);

    Fvector my_position = m_object->eye_matrix.c;

    const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(m_object_to_look);
    if (!entity_alive || entity_alive->g_Alive())
    {
        look_pos.x = m_object_to_look->Position().x;
        look_pos.z = m_object_to_look->Position().z;

        m_object->Center(my_position);
        my_position.x = m_object->eye_matrix.c.x;
        my_position.z = m_object->eye_matrix.c.z;
    }

    if (m_torso_look)
        object().sight().SetFirePointLookAngles(
            look_pos, object().movement().m_head.target.yaw, object().movement().m_head.target.pitch, my_position);
    else
        object().sight().SetPointLookAngles(
            look_pos, object().movement().m_head.target.yaw, object().movement().m_head.target.pitch, my_position);

    //	Msg
    //("execute_object(%f)(%s)my_position[%f][%f][%f],object_position[%f][%f][%f]",object().movement().m_head.target.yaw,*m_object_to_look->cName(),VPUSH(m_object->eye_matrix.c),VPUSH(m_object_to_look->Position()));

    if (m_no_pitch)
        object().movement().m_head.target.pitch = 0.f;

#ifdef SIGHT_TEST
    Msg("%6d %s", Device.dwTimeGlobal, m_torso_look ? "eSightTypeFireObject" : "eSightTypeObject");
#endif
}

void CSightAction::execute_cover()
{
    if (m_torso_look)
        object().sight().SetLessCoverLook(m_object->ai_location().level_vertex(), PI, m_path);
    else
        object().sight().SetLessCoverLook(m_object->ai_location().level_vertex(), m_path);
#ifdef SIGHT_TEST
    Msg("%6d %s [%f] -> [%f]", Device.dwTimeGlobal, m_torso_look ? "eSightTypeFireCover" : "eSightTypeCover",
        object().movement().m_body.current.yaw, object().movement().m_body.target.yaw);
#endif
}

void CSightAction::execute_search()
{
    m_torso_look = false;
    if (m_torso_look)
        object().sight().SetLessCoverLook(m_object->ai_location().level_vertex(), PI, m_path);
    else
        object().sight().SetLessCoverLook(m_object->ai_location().level_vertex(), m_path);
    object().movement().m_head.target.pitch = PI_DIV_4;
#ifdef SIGHT_TEST
    Msg("%6d %s", Device.dwTimeGlobal, m_torso_look ? "eSightTypeFireSearch" : "eSightTypeSearch");
#endif
}

void CSightAction::initialize_cover_look_over()
{
    m_internal_state = 2;
    m_start_state_time = Device.dwTimeGlobal;
    m_stop_state_time = 3500;
    execute_cover();
    m_cover_yaw = object().movement().m_head.target.yaw;
}

void CSightAction::execute_cover_look_over()
{
    switch (m_internal_state)
    {
    case 0:
    case 2: {
#ifndef DEBUG
    fall_back:
#endif // #ifndef DEBUG
        if ((m_start_state_time + m_stop_state_time < Device.dwTimeGlobal) && target_reached())
        {
            m_start_state_time = Device.dwTimeGlobal;
            m_stop_state_time = 3500;
            m_internal_state = 1;
            object().movement().m_head.target.yaw = m_cover_yaw + ::Random.randF(-PI_DIV_8, PI_DIV_8);
        }
        break;
    }
    case 1:
    {
        if ((m_start_state_time + m_stop_state_time < Device.dwTimeGlobal) && target_reached())
        {
            execute_cover();
            m_internal_state = 0;
            m_start_state_time = Device.dwTimeGlobal;
        }
        break;
    }
    default: {
#ifdef DEBUG
        FATAL(make_string("m_internal_state = %d, object[0x%08x]", m_internal_state, this).c_str());
#else // #ifdef DEBUG
        m_internal_state = 0;
        goto fall_back;
#endif // #ifdef DEBUG
    }
    }
}

bool CSightAction::change_body_speed() const { return (false); }
float CSightAction::body_speed() const { return (object().movement().m_body.speed); }
bool CSightAction::change_head_speed() const
{
    return ((SightManager::eSightTypeCoverLookOver == m_sight_type) && (m_internal_state != 2));
}

float CSightAction::head_speed() const
{
    VERIFY(SightManager::eSightTypeCoverLookOver == m_sight_type);
    return (PI_DIV_8 * .5f);
}

void CSightAction::initialize_fire_object()
{
    m_holder_start_position = m_object->Position();
    m_object_start_position = m_object_to_look->Position();
    m_state_fire_object = 0;
}

void CSightAction::on_frame()
{
    switch (m_sight_type)
    {
    case SightManager::eSightTypeCurrentDirection: { break;
    }
    case SightManager::eSightTypePathDirection: { break;
    }
    case SightManager::eSightTypeDirection: { break;
    }
    case SightManager::eSightTypePosition: { break;
    }
    case SightManager::eSightTypeObject: { break;
    }
    case SightManager::eSightTypeCover: { break;
    }
    case SightManager::eSightTypeSearch: { break;
    }
    case SightManager::eSightTypeCoverLookOver:
    {
        execute_cover_look_over();
        break;
    }
    case SightManager::eSightTypeFireObject:
    {
        execute_fire_object();
        break;
    }
    case SightManager::eSightTypeAnimationDirection: { break;
    }
    default: NODEFAULT;
    }
}

void CSightAction::predict_object_position(bool use_exact_position)
{
    m_object->feel_vision_get(objects);
    if (std::find(objects.begin(), objects.end(), m_object_to_look) == objects.end())
    {
        m_vector3d = m_object->sight().object_position();
        return;
    }

    if (use_exact_position)
        m_vector3d = m_object->feel_vision_get_vispoint(const_cast<CGameObject*>(m_object_to_look));
    else
        m_vector3d = m_object->sight().object_position();

    u32 const count = m_object_to_look->ps_Size();
    if (count > 1)
    {
        GameObjectSavedPosition const current_position = m_object_to_look->ps_Element(count - 1);
        VERIFY(Device.dwTimeGlobal >= current_position.dwTime);

        GameObjectSavedPosition previous_position = m_object_to_look->ps_Element(count - 2);
        for (int i = 3; (current_position.dwTime == previous_position.dwTime) && (i <= (int)count); ++i)
            previous_position = m_object_to_look->ps_Element(count - i);

        if (Device.dwTimeGlobal - previous_position.dwTime < 300)
        {
            if (current_position.dwTime > previous_position.dwTime)
            {
                Fvector offset = Fvector().sub(current_position.vPosition, previous_position.vPosition);
                offset.y = 0.f;
                Fvector const velocity =
                    Fvector(offset).div(float(current_position.dwTime - previous_position.dwTime) / 1000.f);
                extern float g_aim_predict_time;
                float const predict_time = g_aim_predict_time; //*Device.fTimeDelta;
                m_vector3d.mad(velocity, predict_time);
            }
        }
    }

    VERIFY(_valid(m_vector3d));
    execute_position(m_object->eye_matrix.c);
}

void CSightAction::execute_fire_object()
{
    switch (m_state_fire_object)
    {
    case 0:
    {
        //			execute_object				();
        predict_object_position(false);

        if (!target_reached())
            break;

        if (!object().inventory().ActiveItem())
            break;

        if (m_object->can_kill_enemy() && !m_object->can_kill_member())
            break;

        if (m_object_to_look->Position().distance_to_sqr(m_object->Position()) < _sqr(5.f))
            break;

        //			Msg							("%6d switch to mode 1", Device.dwTimeGlobal);
        m_state_fire_object = 1;
        m_state_fire_switch_time = Device.dwTimeGlobal;
        m_object_start_position = m_object_to_look->Position();
        m_holder_start_position = m_object->Position();
        //			m_vector3d					= m_object->sight().object_position();
        break;
    }
    case 1:
    {
        if (Device.dwTimeGlobal >= m_state_fire_switch_time + 1500)
        {
            if (m_object_to_look->Position().distance_to_sqr(m_object->Position()) > _sqr(5.f))
            {
                if (!m_holder_start_position.similar(m_object->Position(), .05f))
                {
                    m_vector3d = m_object->sight().object_position();
                    m_already_switched = false;
                    //						Msg					("%6d switch to mode 0 (reson: holder position
                    //changed)",
                    // Device.dwTimeGlobal);
                    m_state_fire_object = 0;
                    break;
                }

                if (!m_object_start_position.similar(m_object_to_look->Position(), .05f))
                {
                    m_vector3d = m_object->sight().object_position();
                    //						Msg					("%6d switch to mode 0 (reson: object position
                    //changed)",
                    // Device.dwTimeGlobal);
                    m_already_switched = false;
                    m_state_fire_object = 0;
                    break;
                }
            }

            if (!m_already_switched)
            {
                m_vector3d = m_object->sight().object_position();
                //					Msg						("%6d switch to mode 0 (reson: time interval)",
                // Device.dwTimeGlobal);
                m_already_switched = true;
                m_state_fire_object = 0;
                break;
            }
        }

        predict_object_position(true);

        break;
    }
    default: NODEFAULT;
    }
}

void CSightAction::execute_animation_direction()
{
    if (object().animation_movement_controlled())
    {
        float h, p, b;
        object().XFORM().getHPB(h, p, b);
        object().movement().m_body.current.yaw = -h;
        object().movement().m_body.current.pitch = p;
        object().movement().m_body.current.roll = b;
        object().movement().m_body.target = object().movement().m_body.current;
    }

    object().movement().m_head.target = object().movement().m_body.current;
}
