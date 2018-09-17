#include "StdAfx.h"
#include "ai/monsters/control_path_builder_base.h"
#include "basemonster/base_monster.h"
#include "xrAICore/Navigation/level_graph.h"
#include "ai_space.h"

//////////////////////////////////////////////////////////////////////////
// Method: prepare_builder
// Desc: reset path builder params to defaults
//////////////////////////////////////////////////////////////////////////
void CControlPathBuilderBase::prepare_builder()
{
    m_time = 0;
    m_distance_to_path_end = 1.f;
    m_failed = false;
    m_cover_info.use_covers = false;

    m_target_actual = false;

    m_target_set.init();

    set_target_accessible(m_target_found, m_object->Position());

    m_last_time_target_set = 0;

    m_time_global_failed_started = 0;
    m_time_path_updated_external = 0;

    m_game_graph_target_vertex = u32(-1);
}

//////////////////////////////////////////////////////////////////////////
// Method: set_target_point
// Desc: just set desirable position and update actuality
// all checkings will be made on update stage
//////////////////////////////////////////////////////////////////////////
void CControlPathBuilderBase::set_target_point(const Fvector& position, u32 node)
{
    // обновить актуальность
    m_target_actual = m_target_actual && (m_target_set.position().similar(position) && (m_target_set.node() == node));

    // установить позицию
    m_target_set.set(position, node);

    // установить глобальные параметры передвижения
    m_target_type = eMoveToTarget;

    set_level_path_type();
}

void CControlPathBuilderBase::set_target_point(u32 node)
{
    set_target_point(ai().level_graph().vertex_position(node), node);
}

void CControlPathBuilderBase::set_retreat_from_point(const Fvector& position)
{
    // обновить актуальность
    m_target_actual = m_target_actual && (m_target_set.position().similar(position));

    // установить позицию
    m_target_set.set(position, u32(-1));

    // установить глобальные параметры передвижения
    m_target_type = eRetreatFromTarget;

    set_level_path_type();
}
