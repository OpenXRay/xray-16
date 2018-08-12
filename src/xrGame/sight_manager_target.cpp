////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_manager_target.cpp
//	Created 	: 27.12.2003
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : sight manager target functions
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "sight_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"
#include "xrAICore/Navigation/level_graph.h"
#include "ai_space.h"
#include "ai/stalker/ai_stalker_space.h"
#include "detail_path_manager.h"

void CSightManager::SetPointLookAngles(
    const Fvector& tPosition, float& yaw, float& pitch, Fvector const& look_position, const CGameObject* object)
{
    Fvector my_position = look_position;
    Fvector target = tPosition;
    if (!aim_target(my_position, target, object))
    {
        target = tPosition;
        my_position = look_position;
    }

    target.sub(my_position);
    target.getHP(yaw, pitch);

    VERIFY(_valid(yaw));
    yaw *= -1;

    VERIFY(_valid(pitch));
    pitch *= -1;
}

#include "Actor.h"
void aim_target(shared_str const& aim_bone_id, Fvector& result, const CGameObject* object);

bool CSightManager::aim_target(Fvector& my_position, Fvector& aim_target, const CGameObject* object) const
{
    if (!object)
        return (false);

    if (m_object->aim_bone_id().size())
    {
        m_object->aim_target(aim_target, object);
        return (true);
    }

    extern CActor* g_actor;

    if (g_actor == object)
    {
        ::aim_target("bip01_head", aim_target, object);
        return (true);
    }

    if (CAI_Stalker const* stalker = smart_cast<CAI_Stalker const*>(object))
    {
        if (stalker->g_Alive())
        {
            ::aim_target("bip01_head", aim_target, object);
            return (true);
        }
    }

    if (!object->use_center_to_aim())
        return (false);

    m_object->Center(my_position);
#if 1
    //. hack is here, just because our actor model is animated with 20cm shift
    m_object->XFORM().transform_tiny(my_position, Fvector().set(.2f, my_position.y - m_object->Position().y, 0.f));
#else
    const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(object);
    if (!entity_alive || entity_alive->g_Alive())
    {
        aim_target.x = m_object->Position().x;
        aim_target.z = m_object->Position().z;
    }
#endif

    return (true);
}

void CSightManager::SetFirePointLookAngles(
    const Fvector& tPosition, float& yaw, float& pitch, Fvector const& look_position, const CGameObject* object)
{
    Fvector my_position = look_position;
    Fvector target = tPosition;
    if (!aim_target(my_position, target, object))
    {
        target = tPosition;
        my_position = look_position;
    }

    target.sub(my_position);
    if (fis_zero(target.square_magnitude()))
        target.set(0.f, 0.f, 1.f);

    target.getHP(yaw, pitch);
    VERIFY(_valid(yaw));
    VERIFY(_valid(pitch));
    yaw *= -1;
    pitch *= -1;
}

void CSightManager::SetDirectionLook()
{
    //	MonsterSpace::SBoneRotation				orientation = object().movement().m_head, body_orientation =
    // object().movement().body_orientation();
    //	orientation.target						= orientation.current;
    //	body_orientation.target					= body_orientation.current;
    if (GetDirectionAngles(object().movement().m_head.target.yaw, object().movement().m_head.target.pitch))
    {
        object().movement().m_head.target.yaw *= -1;
        object().movement().m_head.target.pitch *= 0; //-1;
    }
    else
        object().movement().m_head.target = object().movement().m_head.current;
    object().movement().m_body.target = object().movement().m_head.target;
}

void CSightManager::SetLessCoverLook(const CLevelGraph::CVertex* tpNode, bool bDifferenceLook)
{
    SetDirectionLook();

    if (m_object->movement().detail().path().empty())
        return;

    SetLessCoverLook(tpNode, MAX_HEAD_TURN_ANGLE, bDifferenceLook);
}

void CSightManager::SetLessCoverLook(const CLevelGraph::CVertex* tpNode, float fMaxHeadTurnAngle, bool bDifferenceLook)
{
    float fAngleOfView, range, fMaxSquare = -1.f, fBestAngle = object().movement().m_head.target.yaw;
    m_object->update_range_fov(range, fAngleOfView, m_object->eye_range, m_object->eye_fov);
    fAngleOfView = (fAngleOfView / 180.f * PI) / 2.f;

    CLevelGraph::CVertex* tpNextNode = 0;
    u32 node_id;
    bool bOk = false;
    if (bDifferenceLook && !m_object->movement().detail().path().empty() &&
        (m_object->movement().detail().path().size() - 1 > m_object->movement().detail().curr_travel_point_index()))
    {
        CLevelGraph::const_iterator i, e;
        ai().level_graph().begin(tpNode, i, e);
        for (; i != e; ++i)
        {
            node_id = ai().level_graph().value(tpNode, i);
            if (!ai().level_graph().valid_vertex_id(node_id))
                continue;
            tpNextNode = ai().level_graph().vertex(node_id);
            if (ai().level_graph().inside(
                    tpNextNode, m_object->movement()
                                    .detail()
                                    .path()[m_object->movement().detail().curr_travel_point_index() + 1]
                                    .position))
            {
                bOk = true;
                break;
            }
        }
    }

    if (!bDifferenceLook || !bOk)
        for (float fIncrement = object().movement().m_body.target.yaw - fMaxHeadTurnAngle;
             fIncrement <= object().movement().m_body.target.yaw + fMaxHeadTurnAngle;
             fIncrement += fMaxHeadTurnAngle / 18.f)
        {
            float fSquare = ai().level_graph().compute_high_square(-fIncrement, fAngleOfView, tpNode);
            if (fSquare > fMaxSquare)
            {
                fMaxSquare = fSquare;
                fBestAngle = fIncrement;
            }
        }
    else
    {
        float fMaxSquareSingle = -1.f, fSingleIncrement = object().movement().m_head.target.yaw;
        for (float fIncrement = object().movement().m_body.target.yaw - fMaxHeadTurnAngle;
             fIncrement <= object().movement().m_body.target.yaw + fMaxHeadTurnAngle;
             fIncrement += 2 * fMaxHeadTurnAngle / 60.f)
        {
            float fSquare0 = ai().level_graph().compute_high_square(-fIncrement, fAngleOfView, tpNode);
            float fSquare1 = ai().level_graph().compute_high_square(-fIncrement, fAngleOfView, tpNextNode);
            if ((fSquare1 - fSquare0 > fMaxSquare) ||
                (fsimilar(fSquare1 - fSquare0, fMaxSquare, EPS_L) &&
                    (_abs(fIncrement - object().movement().m_body.target.yaw) <
                        _abs(fBestAngle - object().movement().m_body.target.yaw))))
            {
                fMaxSquare = fSquare1 - fSquare0;
                fBestAngle = fIncrement;
            }

            if (fSquare0 > fMaxSquareSingle)
            {
                fMaxSquareSingle = fSquare0;
                fSingleIncrement = fIncrement;
            }
        }
        if (_sqrt(fMaxSquare) < 0 * PI_DIV_6)
            fBestAngle = fSingleIncrement;
    }

    object().movement().m_head.target.yaw = angle_normalize_signed(fBestAngle);
    object().movement().m_head.target.pitch = 0;
    VERIFY(_valid(object().movement().m_head.target.yaw));
}

bool CSightManager::GetDirectionAngles(float& yaw, float& pitch)
{
    if (!object().movement().path().empty() &&
        (m_object->movement().detail().curr_travel_point_index() + 1 < m_object->movement().detail().path().size()))
    {
        Fvector t;
        t.sub(object().movement().path()[m_object->movement().detail().curr_travel_point_index() + 1].position,
            object().movement().path()[m_object->movement().detail().curr_travel_point_index()].position);
        t.getHP(yaw, pitch);
        return (true);
    }
    return (GetDirectionAnglesByPrevPositions(yaw, pitch));
};

bool CSightManager::GetDirectionAnglesByPrevPositions(float& yaw, float& pitch)
{
    Fvector tDirection;
    int i = m_object->ps_Size();

    if (i < 2)
        return (false);

    GameObjectSavedPosition tPreviousPosition = m_object->ps_Element(i - 2),
                            tCurrentPosition = m_object->ps_Element(i - 1);
    VERIFY(_valid(tPreviousPosition.vPosition));
    VERIFY(_valid(tCurrentPosition.vPosition));
    tDirection.sub(tCurrentPosition.vPosition, tPreviousPosition.vPosition);
    if (tDirection.magnitude() < EPS_L)
        return (false);
    tDirection.getHP(yaw, pitch);
    VERIFY(_valid(yaw));
    VERIFY(_valid(pitch));

    return (true);
}
