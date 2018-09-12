#include "StdAfx.h"
#include "monster_home.h"
#include "basemonster/base_monster.h"
#include "ai_space.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "xrAICore/Navigation/level_graph.h"
#include "cover_point.h"
#include "monster_cover_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "restricted_object.h"
#include "xrAICore/Navigation/game_graph.h"

#ifdef _DEBUG

void check_path(const CBaseMonster* monster, const CPatrolPath* path)
{
    VERIFY2(ai().game_graph().vertex(path->vertices().begin()->second->data().game_vertex_id())->level_id() ==
            ai().level_graph().level_id(),
        make_string("invalid patrol path [%s] as home specified for monster [%s]\nmonster is on level %s\npatrol path "
                    "is on level %s",
            *path->m_name, *monster->cName(),
            *ai().game_graph()
                 .header()
                 .level(ai().game_graph().vertex(monster->ai_location().game_vertex_id())->level_id())
                 .name(),
            *ai().game_graph()
                 .header()
                 .level(ai().game_graph().vertex(path->vertices().begin()->second->data().game_vertex_id())->level_id())
                 .name()));
}
#else // DEBUG
#define check_path(a, b)
#endif // DEBUG

void CMonsterHome::load(LPCSTR line)
{
    m_path = 0;
    m_level_vertex_id = u32(-1);
    m_radius_min = 20.f;
    m_radius_middle = 30.f;
    m_radius_max = 40.f;
    min_move_dist = u32(7);
    max_move_dist = u32(10);

    if (m_object->spawn_ini() && m_object->spawn_ini()->section_exist(line))
    {
        m_path = ai().patrol_paths().path(m_object->spawn_ini()->r_string(line, "path"));
        check_path(m_object, m_path);
        if (m_object->spawn_ini()->line_exist(line, "radius_min"))
            m_radius_min = m_object->spawn_ini()->r_float(line, "radius_min");
        if (m_object->spawn_ini()->line_exist(line, "radius_max"))
            m_radius_max = m_object->spawn_ini()->r_float(line, "radius_max");

        VERIFY3(
            m_radius_max > m_radius_min, "Error: Wrong home point radius specified for monster ", *m_object->cName());

        if (m_object->spawn_ini()->line_exist(line, "radius_middle"))
        {
            m_radius_middle = m_object->spawn_ini()->r_float(line, "radius_middle");
            if (m_radius_middle > m_radius_max || m_radius_middle < m_radius_min)
            {
                m_radius_middle = m_radius_min + (m_radius_max - m_radius_min) / 2;
            }
        }
        else
        {
            m_radius_middle = m_radius_min + (m_radius_max - m_radius_min) / 2;
        }
        if (m_object->spawn_ini()->line_exist(line, "min_move_dist"))
        {
            min_move_dist = m_object->spawn_ini()->r_u32(line, "min_move_dist");
            ;
        }
        if (m_object->spawn_ini()->line_exist(line, "max_move_dist"))
        {
            max_move_dist = m_object->spawn_ini()->r_u32(line, "max_move_dist");
            ;
        }
        if (min_move_dist >= max_move_dist)
        {
            min_move_dist = u32(7);
            max_move_dist = u32(10);
        }
    }

    m_aggressive = false;
}

void CMonsterHome::setup(LPCSTR path_name, float min_radius, float max_radius, bool aggressive, float middle_radius)
{
    m_path = ai().patrol_paths().path(path_name);
    check_path(m_object, m_path);
    m_radius_min = min_radius;
    m_radius_max = max_radius;
    if (middle_radius > max_radius || middle_radius < min_radius)
    {
        m_radius_middle = m_radius_min + (m_radius_max - m_radius_min) / 2;
    }
    else
    {
        m_radius_middle = middle_radius;
    }

    m_aggressive = aggressive;
}
void CMonsterHome::setup(u32 lv_ID, float min_radius, float max_radius, bool aggressive, float middle_radius)
{
    m_path = 0;
    m_level_vertex_id = lv_ID;
    m_radius_min = min_radius;
    m_radius_max = max_radius;
    if (middle_radius > max_radius || middle_radius < min_radius)
    {
        m_radius_middle = m_radius_min + (m_radius_max - m_radius_min) / 2;
    }
    else
    {
        m_radius_middle = middle_radius;
    }

    m_aggressive = aggressive;
}

u32 CMonsterHome::get_place_in_min_home()
{
    u32 result = u32(-1);
    u32 input_node;

    if (!m_path)
    {
        if (!ai().level_graph().valid_vertex_id(m_level_vertex_id))
            input_node = m_object->ai_location().level_vertex_id();
        else
            input_node = m_level_vertex_id;
    }
    else
    {
        const CPatrolPath::CVertex* vertex = m_path->vertex(Random.randI(m_path->vertex_count()));
        input_node = vertex->data().level_vertex_id();
    }
    m_object->control().path_builder().get_node_in_radius(input_node, 1, m_radius_min, 5, result);

    if (result == u32(-1))
    {
        // TODO: find more acceptable decision, than return its level_vertex_id, if !accessible
        if (!m_object->control().path_builder().accessible(input_node))
            result = m_object->ai_location().level_vertex_id();
        else
            result = input_node;
    }

    return result;
}

u32 CMonsterHome::get_place_in_mid_home()
{
    u32 result = u32(-1);
    u32 input_node;
    const CPatrolPath::CVertex* vertex;

    int m_move_dist = min_move_dist + Random.randI(max_move_dist - min_move_dist);
    float m_res_dist = 0.f + m_move_dist;
    if (!at_mid_home(m_object->Position()) || at_min_home(m_object->Position()))
    {
        if (!m_path)
        {
            if (!ai().level_graph().valid_vertex_id(m_level_vertex_id))
                input_node = m_object->ai_location().level_vertex_id();
            else
                input_node = m_level_vertex_id;
        }
        else
        {
            vertex = m_path->vertex(Random.randI(m_path->vertex_count()));
            input_node = vertex->data().level_vertex_id();
        }
        m_object->control().path_builder().get_node_in_radius(input_node, m_radius_min, m_radius_middle, 5, result);
    }
    else
    {
        int i = 0;
        Fvector m_dest_direction;
        do
        {
            i++;
            float m_heading, m_pitch;
            m_object->Direction().getHP(m_heading, m_pitch);
            float mAngle;
            if (i > 5)
            {
                if (Random.randI(2) == 1)
                {
                    mAngle = Random.randF(-PI / 3, -PI / 4);
                }
                else
                {
                    mAngle = Random.randF(PI / 4, PI / 3);
                }
            }
            else
            {
                mAngle = Random.randF(-PI / 4, PI / 4);
            }
            m_heading = angle_normalize(m_heading + mAngle);
            m_dest_direction.setHP(m_heading, m_pitch);
            m_dest_direction.mul(m_res_dist);
            m_dest_direction.add(m_object->Position(), m_dest_direction);
        } while (!ai().level_graph().valid_vertex_position(m_dest_direction) && i <= 10);
        if (ai().level_graph().valid_vertex_position(m_dest_direction))
        {
            result = ai().level_graph().vertex_id(m_dest_direction);
            input_node = result;
        }
        else
        {
            input_node = m_object->ai_location().level_vertex_id();
            m_object->control().path_builder().get_node_in_radius(input_node, m_res_dist - 1, m_res_dist, 5, result);
        }
    }

    if (result == u32(-1))
    {
        // TODO: find more acceptable decision, than return its level_vertex_id, if !accessible
        if (ai().level_graph().valid_vertex_id(input_node) && m_object->control().path_builder().accessible(input_node))
            result = input_node;
        else
            result = m_object->ai_location().level_vertex_id();
    }

    if (!ai().level_graph().valid_vertex_id(result) || !at_mid_home(ai().level_graph().vertex_position(result)))
        return get_place_in_min_home();

    return result;
}

u32 CMonsterHome::get_place_in_max_home_to_direction(Fvector to_direction)
{
    Fvector m_home_point = get_home_point();
    u32 input_node = u32(-1);
    u32 result = u32(-1);
    int i = 0;
    float mAngle;
    Fvector m_dest_direction;
    float m_res_dist = m_radius_middle + (m_radius_max - m_radius_middle) / 2 - (m_radius_max - m_radius_middle) / 10;
    float m_heading, m_pitch;
    do
    {
        i++;
        to_direction.getHP(m_heading, m_pitch);
        if (i > 5)
        {
            if (Random.randI(2) == 1)
            {
                mAngle = Random.randF(-PI / 4, -PI / 5);
            }
            else
            {
                mAngle = Random.randF(PI / 5, PI / 4);
            }
        }
        else
        {
            mAngle = Random.randF(-PI / 5, PI / 5);
        }
        m_heading = angle_normalize(m_heading + mAngle);
        m_dest_direction.setHP(m_heading, m_pitch);
        m_dest_direction.x = m_home_point.x + m_res_dist * m_dest_direction.x;
        m_dest_direction.y = m_home_point.y + m_res_dist * m_dest_direction.y;
        m_dest_direction.z = m_home_point.z + m_res_dist * m_dest_direction.z;

    } while (!ai().level_graph().valid_vertex_position(m_dest_direction) && i <= 10);

    if (ai().level_graph().valid_vertex_position(m_dest_direction))
        input_node = ai().level_graph().vertex_id(m_dest_direction);

    if (input_node != u32(-1))
        m_object->control().path_builder().get_node_in_radius(
            input_node, 1, (m_radius_max - m_radius_middle) / 2, 5, result);

    if (result == u32(-1))
    {
        if (ai().level_graph().valid_vertex_id(input_node) && m_object->control().path_builder().accessible(input_node))
        {
            result = input_node;
        }
        else
        {
            m_res_dist = m_radius_min + (m_radius_middle - m_radius_min) / 2 - (m_radius_middle - m_radius_min) / 10;
            i = 0;
            input_node = u32(-1);
            do
            {
                i++;
                to_direction.getHP(m_heading, m_pitch);
                if (i > 5)
                {
                    if (Random.randI(2) == 1)
                    {
                        mAngle = Random.randF(-PI / 3, -PI / 5);
                    }
                    else
                    {
                        mAngle = Random.randF(PI / 5, PI / 3);
                    }
                }
                else
                {
                    mAngle = Random.randF(-PI / 3, PI / 3);
                }
                m_heading = angle_normalize(m_heading + mAngle);
                m_dest_direction.setHP(m_heading, m_pitch);
                m_dest_direction.x = m_home_point.x + m_res_dist * m_dest_direction.x;
                m_dest_direction.y = m_home_point.y + m_res_dist * m_dest_direction.y;
                m_dest_direction.z = m_home_point.z + m_res_dist * m_dest_direction.z;
            } while (!ai().level_graph().valid_vertex_position(m_dest_direction) && i <= 10);

            if (ai().level_graph().valid_vertex_position(m_dest_direction))
                input_node = ai().level_graph().vertex_id(m_dest_direction);

            if (input_node != u32(-1))
                m_object->control().path_builder().get_node_in_radius(
                    input_node, 1, (m_radius_max - m_radius_middle) / 2, 5, result);

            if (result == u32(-1))
            {
                if (ai().level_graph().valid_vertex_id(input_node) &&
                    m_object->control().path_builder().accessible(input_node))
                {
                    result = input_node;
                }
                else
                {
                    result = get_place_in_min_home();
                }
            }
        }
    }

    if (result == u32(-1))
        result = get_place_in_max_home();

    return (result);
}

u32 CMonsterHome::get_place_in_max_home()
{
    u32 result = u32(-1);
    u32 input_node;

    if (!m_path)
    {
        if (!ai().level_graph().valid_vertex_id(m_level_vertex_id))
            input_node = m_object->ai_location().level_vertex_id();
        else
            input_node = m_level_vertex_id;
    }
    else
    {
        const CPatrolPath::CVertex* vertex = m_path->vertex(Random.randI(m_path->vertex_count()));
        input_node = vertex->data().level_vertex_id();
    }
    m_object->control().path_builder().get_node_in_radius(input_node, m_radius_min, m_radius_max, 5, result);

    if (result == u32(-1))
    {
        // TODO: find more acceptable decision, than return its level_vertex_id, if !accessible
        if (!m_object->control().path_builder().accessible(input_node))
            result = m_object->ai_location().level_vertex_id();
        else
            result = input_node;
    }

    return result;
}

u32 CMonsterHome::get_place()
{
    u32 result = u32(-1);
    if (!m_path)
    {
        if (ai().level_graph().valid_vertex_id(m_level_vertex_id))
            m_object->control().path_builder().get_node_in_radius(
                m_level_vertex_id, m_radius_min, m_radius_min + (m_radius_max - m_radius_min) / 2, 5, result);

        if (result == u32(-1))
        {
            m_object->control().path_builder().get_node_in_radius(
                m_object->ai_location().level_vertex_id(), 5, 15, 5, result);
            if (result == u32(-1))
            {
                m_object->control().path_builder().get_node_in_radius(
                    m_object->ai_location().level_vertex_id(), 2, 3, 10, result);
                if (result == u32(-1))
                    return m_object->ai_location().level_vertex_id();
            }
        }
    }
    else
    {
        VERIFY(m_path);

        // get_random_point
        const CPatrolPath::CVertex* vertex = m_path->vertex(Random.randI(m_path->vertex_count()));

        // get_random node
        m_object->control().path_builder().get_node_in_radius(vertex->data().level_vertex_id(), m_radius_min,
            m_radius_min + (m_radius_max - m_radius_min) / 2, 5, result);
        if (result == u32(-1))
        {
            // TODO: find more acceptable decision, than return its level_vertex_id, if !accessible
            if (m_object->control().path_builder().accessible(vertex->data().level_vertex_id()))
                return vertex->data().level_vertex_id();
            else
                return m_object->ai_location().level_vertex_id();
        }
    }

    // if node was not found - return vertex selected

    return result;
}

u32 CMonsterHome::get_place_in_cover()
{
    if (!m_path)
    {
        if (ai().level_graph().valid_vertex_id(m_level_vertex_id))
        {
            const CCoverPoint* point =
                m_object->CoverMan->find_cover(ai().level_graph().vertex_position(m_level_vertex_id),
                    ai().level_graph().vertex_position(m_level_vertex_id), m_radius_min,
                    m_radius_min + (m_radius_max - m_radius_min) / 2);
            if (point)
                return point->level_vertex_id();
        }
    }
    else
    {
        VERIFY(m_path);

        // get_random_point
        const CPatrolPath::CVertex* vertex = m_path->vertex(Random.randI(m_path->vertex_count()));

        // find cover
        const CCoverPoint* point = m_object->CoverMan->find_cover(vertex->data().position(), vertex->data().position(),
            m_radius_min, m_radius_min + (m_radius_max - m_radius_min) / 2);
        if (point)
            return point->level_vertex_id();
    }
    return u32(-1);
}

bool CMonsterHome::at_home() { return at_home(m_object->Position()); }
bool CMonsterHome::at_home(const Fvector& pos) { return at_home(pos, m_radius_max); }
bool CMonsterHome::at_min_home(const Fvector& pos) { return at_home(pos, m_radius_min); }
bool CMonsterHome::at_mid_home(const Fvector& pos) { return at_home(pos, m_radius_middle); }
bool CMonsterHome::at_home(const Fvector& pos, float const radius)
{
    if (!m_path)
    {
        if (!ai().level_graph().valid_vertex_id(m_level_vertex_id))
            return true;

        float dist = pos.distance_to(ai().level_graph().vertex_position(m_level_vertex_id));

        if (dist < radius)
            return true;
    }
    else
    {
        // check every point and distance to it
        for (u32 i = 0; i < m_path->vertex_count(); i++)
        {
            const CPatrolPath::CVertex* vertex = m_path->vertex(i);

            u32 const level_vertex = vertex->data().level_vertex_id();
            if (!ai().level_graph().valid_vertex_id(level_vertex))
                continue;

            float dist = pos.distance_to(ai().level_graph().vertex_position(level_vertex));
            if (dist < radius)
                return true;
        }
    }
    return false;
}

void CMonsterHome::remove_home()
{
    m_path = 0;
    m_level_vertex_id = u32(-1);
    m_aggressive = false;
}

void CMonsterHome::set_move_dists(u32 min_dist, u32 max_dist)
{
    if (max_dist > min_dist)
    {
        min_move_dist = min_dist;
        max_move_dist = max_dist;
    }
}

Fvector CMonsterHome::get_home_point()
{
    if (!m_path)
    {
        if (ai().level_graph().valid_vertex_id(m_level_vertex_id))
            return ai().level_graph().vertex_position(m_level_vertex_id);
        else
            return (m_object->Position());
    }

    typedef CPatrolPath::CVertex CVertex;
    if (!m_path->vertices().empty())
        return (ai().level_graph().vertex_position(m_path->vertex(0)->data().level_vertex_id()));

    return (m_object->Position());
}
