////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_manager.cpp
//	Created 	: 03.12.2003
//  Modified 	: 03.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Patrol path manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "GameObject.h"
#include "patrol_path_manager.h"
#include "script_game_object.h"
#include "restricted_object.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrAICore/Navigation/ai_object_location.h"
#include "script_entity_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"
#include "xrAICore/Navigation/level_graph.h"

#if 1 // def DEBUG
#include "space_restriction_manager.h"

static void show_restrictions(LPCSTR restrictions)
{
    string256 temp;
    for (int i = 0, n = _GetItemCount(restrictions); i < n; ++i)
        Msg("     %s", _GetItem(restrictions, i, temp));
}

bool show_restrictions(CRestrictedObject* object)
{
    Msg("DEFAULT OUT RESTRICTIONS :");
    show_restrictions(*Level().space_restriction_manager().default_out_restrictions() ?
            *Level().space_restriction_manager().default_out_restrictions() :
            "");

    Msg("DEFAULT IN RESTRICTIONS  :");
    show_restrictions(*Level().space_restriction_manager().default_in_restrictions() ?
            *Level().space_restriction_manager().default_in_restrictions() :
            "");

    Msg("OUT RESTRICTIONS         :");
    show_restrictions(*object->out_restrictions() ? *object->out_restrictions() : "");

    Msg("IN RESTRICTIONS          :");
    show_restrictions(*object->in_restrictions() ? *object->in_restrictions() : "");

    return (false);
}
#endif

CPatrolPathManager::~CPatrolPathManager() {}
bool CPatrolPathManager::extrapolate_path()
{
    VERIFY(m_path && m_path->vertex(m_curr_point_index));
    if (!m_extrapolate_callback)
        return (true);

    return (m_extrapolate_callback(m_curr_point_index));
}

void CPatrolPathManager::reinit()
{
    m_path = 0;
    m_actuality = true;
    m_failed = false;
    m_completed = true;
    m_extrapolate_callback.clear();

    reset();
}

IC bool CPatrolPathManager::accessible(const Fvector& position) const
{
    return (m_object ? object().accessible(position) : true);
}

IC bool CPatrolPathManager::accessible(u32 vertex_id) const
{
    return (m_object ? object().accessible(vertex_id) : true);
}

IC bool CPatrolPathManager::accessible(const CPatrolPath::CVertex* vertex) const
{
    return (vertex ? object().accessible(vertex->data().position()) : true);
}

struct CAccessabilityEvaluator
{
    const CPatrolPathManager* m_manager;

    IC CAccessabilityEvaluator(const CPatrolPathManager* manager) { m_manager = manager; }
    IC bool operator()(const Fvector& position) const { return (m_manager->accessible(position)); }
};

void CPatrolPathManager::select_point(const Fvector& position, u32& dest_vertex_id)
{
    VERIFY(m_path && !m_path->vertices().empty());
    const CPatrolPath::CVertex* vertex = 0;
    if (!actual() || !m_path->vertex(m_curr_point_index))
    {
        switch (m_start_type)
        {
        case ePatrolStartTypeFirst:
        {
            vertex = m_path->vertex(0);
            VERIFY3(accessible(vertex) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());
            break;
        }
        case ePatrolStartTypeLast:
        {
            vertex = m_path->vertex(m_path->vertices().size() - 1);
            VERIFY3(accessible(vertex) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());
            break;
        }
        case ePatrolStartTypeNearest:
        {
            vertex = m_path->point(position, CAccessabilityEvaluator(this));
            VERIFY3(accessible(vertex) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());
            break;
        }
        case ePatrolStartTypePoint:
        {
            VERIFY3(m_path->vertex(m_start_point_index), *m_path_name, *m_game_object->cName());
            vertex = m_path->vertex(m_start_point_index);
            VERIFY3(accessible(vertex) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());
            break;
        }
        case ePatrolStartTypeNext:
        {
            if (m_prev_point_index != u32(-1))
            {
                if ((m_prev_point_index + 1) < m_path->vertex_count())
                {
                    vertex = m_path->vertex(m_prev_point_index + 1);
                }
                else
                {
                    u32 next_point_id = get_next_point(m_prev_point_index);
                    vertex = m_path->vertex(next_point_id);
                }

                if (!accessible(vertex))
                    vertex = 0;
            }

            if (!vertex)
                vertex = m_path->point(position, CAccessabilityEvaluator(this));

            VERIFY3(accessible(vertex) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());
            break;
        }
        default: NODEFAULT;
        }
        if (!(vertex || show_restrictions(m_object)))
        {
            // ugly HACK, just because Plecha asked...
            VERIFY2(vertex || show_restrictions(m_object),
                make_string("any vertex in patrol path [%s] is inaccessible for object [%s]", *m_path_name,
                    *m_game_object->cName()));
            dest_vertex_id = m_game_object->ai_location().level_vertex_id();
            return;
        }

        R_ASSERT2(ai().level_graph().valid_vertex_id(vertex->data().level_vertex_id()),
            make_string("patrol path[%s], point on path [%s],object [%s]", *m_path_name, *vertex->data().name(),
                *m_game_object->cName()));

        if (!m_path->vertex(m_prev_point_index))
            m_prev_point_index = vertex->vertex_id();

        m_curr_point_index = vertex->vertex_id();

#if 0
		// если выбранная нода не соответствует текущей ноде - все ок
		// иначе выбрать следующую вершину патрульного пути
		if (vertex->data().level_vertex_id() != m_game_object->ai_location().level_vertex_id()) {
			dest_vertex_id		= vertex->data().level_vertex_id();
			m_dest_position		= vertex->data().position();
			VERIFY				(accessible(m_dest_position) || show_restrictions(m_object));
			m_actuality			= true;
			m_completed			= false;
			return;
		}
#else
        if (!m_game_object->Position().similar(vertex->data().position(), .1f))
        {
            dest_vertex_id = vertex->data().level_vertex_id();
            m_dest_position = vertex->data().position();
            VERIFY(accessible(m_dest_position) || show_restrictions(m_object));
            m_actuality = true;
            m_completed = false;
            return;
        }
#endif
    }
    VERIFY3(m_path->vertex(m_curr_point_index) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());

    m_game_object->callback(GameObject::ePatrolPathInPoint)(
        m_game_object->lua_game_object(), u32(ScriptEntity::eActionTypeMovement), m_curr_point_index);

    u32 count = 0; // количество разветвлений
    float sum = 0.f; // сумма весов разветвления
    vertex = m_path->vertex(m_curr_point_index);
    CPatrolPath::const_iterator I = vertex->edges().begin(), E = vertex->edges().end();
    u32 target = u32(-1);

    // вычислить количество разветвлений
    for (; I != E; ++I)
    {
        if ((*I).vertex_id() == m_prev_point_index)
            continue;

        if (!accessible(m_path->vertex((*I).vertex_id())))
            continue;

        if (count == 0)
            target = (*I).vertex_id();

        sum += (*I).weight();
        ++count;
    }

    if (count == 0)
    {
        switch (m_route_type)
        {
        case ePatrolRouteTypeStop:
        {
            m_completed = true;
            return;
        }
        case ePatrolRouteTypeContinue:
        {
            for (I = vertex->edges().begin(); I != E; ++I)
            {
                if (!accessible(m_path->vertex((*I).vertex_id())))
                    continue;

                target = (*I).vertex_id();
                break;
            }
            if (target != u32(-1))
                break;

            m_completed = true;
            return;
        }
        default: NODEFAULT;
        }
    }
    else
    {
        float fChoosed = 0.f;

        if (random() && (count > 1))
            fChoosed = ::Random.randF(sum);

        sum = 0.f;
        I = vertex->edges().begin();

        for (; I != E; ++I)
        {
            if ((*I).vertex_id() == m_prev_point_index)
                continue;

            if (!accessible(m_path->vertex((*I).vertex_id())))
                continue;

            sum += (*I).weight();

            if (sum >= fChoosed)
            {
                target = (*I).vertex_id();
                break;
            }
        }
    }

    VERIFY3(m_path->vertex(target) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());

    m_prev_point_index = m_curr_point_index;
    m_curr_point_index = target;
    dest_vertex_id = m_path->vertex(m_curr_point_index)->data().level_vertex_id();
    m_dest_position = m_path->vertex(m_curr_point_index)->data().position();
    VERIFY3(accessible(m_dest_position) || show_restrictions(m_object), *m_path_name, *m_game_object->cName());
    m_actuality = true;
    m_completed = false;
}

u32 CPatrolPathManager::get_next_point(u32 prev_point_index)
{
    u32 count = 0; // количество разветвлений
    float sum = 0.f; // сумма весов разветвления
    const CPatrolPath::CVertex* vertex = m_path->vertex(prev_point_index);

    CPatrolPath::const_iterator I = vertex->edges().begin(), E = vertex->edges().end();
    u32 target = u32(-1);

    // вычислить количество разветвлений
    for (; I != E; ++I)
    {
        if (!accessible(m_path->vertex((*I).vertex_id())))
            continue;

        sum += (*I).weight();
        ++count;
    }

    // проверить количество
    if (count != 0)
    {
        float fChoosed = 0.f;

        if (random() && (count > 1))
            fChoosed = ::Random.randF(sum);

        sum = 0.f;
        I = vertex->edges().begin();

        for (; I != E; ++I)
        {
            if (!accessible(m_path->vertex((*I).vertex_id())))
                continue;

            sum += (*I).weight();

            if (sum >= fChoosed)
            {
                target = (*I).vertex_id();
                break;
            }
        }
    }

    return target;
}

shared_str CPatrolPathManager::path_name() const
{
    if (!m_path)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Path not specified (object %s)!", *m_game_object->cName());
        return ("");
    }
    VERIFY(m_path);
    return (m_path_name);
}

void CPatrolPathManager::set_previous_point(int point_index)
{
    if (!m_path)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Path not specified (object %s)!", *m_game_object->cName());
        return;
    }

    if (!m_path->vertex(point_index))
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Start point violates path bounds %s (object %s)!",
            *m_path_name, *m_game_object->cName());
        return;
    }
    VERIFY(m_path);
    VERIFY(m_path->vertex(point_index));
    m_prev_point_index = point_index;
}

void CPatrolPathManager::set_start_point(int point_index)
{
    if (!m_path)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Path not specified (object %s)!", *m_game_object->cName());
        return;
    }
    if (!m_path->vertex(point_index))
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Start point violates path bounds %s (object %s)!",
            *m_path_name, *m_game_object->cName());
        return;
    }
    VERIFY(m_path);
    VERIFY(m_path->vertex(point_index));
    m_start_point_index = point_index;
}

void CPatrolPathManager::reset()
{
    m_curr_point_index = u32(-1);
    m_prev_point_index = u32(-1);
    m_start_point_index = u32(-1);

    m_start_type = ePatrolStartTypeDummy;
    m_route_type = ePatrolRouteTypeDummy;
}
