#include "StdAfx.h"
#include "ai/monsters/control_path_builder_base.h"
#include "basemonster/base_monster.h"
#include "PHMovementControl.h"
#include "cover_evaluators.h"
#include "level_path_manager.h"
#include "detail_path_manager.h"
#include "level_location_selector.h"
#include "xrAICore/Navigation/ai_object_location.h"

const u32 pmt_global_failed_duration = 3000;

//////////////////////////////////////////////////////////////////////////
// Construction / Destruction
//////////////////////////////////////////////////////////////////////////

CControlPathBuilderBase::CControlPathBuilderBase() { m_cover_approach = 0; }
CControlPathBuilderBase::~CControlPathBuilderBase() { xr_delete(m_cover_approach); }
void CControlPathBuilderBase::reinit()
{
    inherited::reinit();

    if (!m_cover_approach)
        m_cover_approach = new CCoverEvaluatorCloseToEnemy(&m_man->path_builder().restrictions());

    reset();

    m_man->capture(this, ControlCom::eControlPath);
}

void CControlPathBuilderBase::reset()
{
    m_try_min_time = false;
    m_use_dest_orient = false;
    m_enable = false;
    m_path_type = MovementManager::ePathTypeLevelPath;
    m_dest_dir.set(0.f, 0.f, 0.f);
    m_extrapolate = false;
    m_velocity_mask = u32(-1);
    m_desirable_mask = u32(-1);
    m_last_time_dir_set = 0;
    m_last_time_target_set = 0;
    m_reset_actuality = false;
    m_game_graph_target_vertex = u32(-1);

    prepare_builder();

    m_path_end = false;
}

void CControlPathBuilderBase::on_event(ControlCom::EEventType type, ControlCom::IEventData* data)
{
    switch (type)
    {
    case ControlCom::eventPathBuilt: on_path_built(); break;
    case ControlCom::eventPathUpdated: on_path_updated(); break;
    case ControlCom::eventTravelPointChange: travel_point_changed(); break;
    }
}

void CControlPathBuilderBase::on_start_control(ControlCom::EControlType type)
{
    switch (type)
    {
    case ControlCom::eControlPath:
        m_man->subscribe(this, ControlCom::eventPathBuilt);
        m_man->subscribe(this, ControlCom::eventTravelPointChange);
        m_man->subscribe(this, ControlCom::eventPathUpdated);
        break;
    }
}

void CControlPathBuilderBase::on_stop_control(ControlCom::EControlType type)
{
    switch (type)
    {
    case ControlCom::eControlPath:
        m_man->unsubscribe(this, ControlCom::eventPathBuilt);
        m_man->unsubscribe(this, ControlCom::eventTravelPointChange);
        m_man->unsubscribe(this, ControlCom::eventPathUpdated);
        break;
    }
}

void CControlPathBuilderBase::detour_graph_points(u32 game_graph_vertex_id)
{
    m_game_graph_target_vertex = game_graph_vertex_id;
    set_game_path_type();
}

void CControlPathBuilderBase::set_dest_direction(const Fvector& dir)
{
    if (m_last_time_dir_set + m_time > time())
        return;
    m_dest_dir.set(dir);
    m_last_time_dir_set = time();
}

void CControlPathBuilderBase::set_target_accessible(STarget& target, const Fvector& position)
{
    if (!m_man->path_builder().accessible(position))
    {
        Fvector new_position;
        target.set_node(m_man->path_builder().restrictions().accessible_nearest(position, new_position));
        target.set_position(new_position);
    }
    else
    {
        target.set_node(u32(-1));
        target.set_position(position);
    }
}

// обновит	ь информацию о построенном пути (m_failed)
void CControlPathBuilderBase::on_path_built()
{
    // проверка на конец пути
    if (!m_man->path_builder().detail().path().empty() &&
        (m_man->path_builder().detail().curr_travel_point_index() < m_man->path_builder().detail().path().size() - 1))
        m_path_end = false;
}

void CControlPathBuilderBase::on_path_updated()
{
    // если level_path_manager failed
    if (m_man->path_builder().level_path().failed())
    {
        m_failed = true;
        m_man->path_builder().level_path().reset();
        VERIFY(!m_man->path_builder().level_path().failed());
    }

    // если level_path_manager failed
    if (m_man->path_builder().detail().failed())
        m_failed = true;

    // проверка на конец пути, если этот путь не конечный
    if ((m_man->path_builder().detail().path().empty() || (m_man->path_builder().detail().curr_travel_point_index() >=
                                                              m_man->path_builder().detail().path().size() - 1)) &&
        m_man->path_builder().detail().actual() && m_man->path_builder().enabled() &&
        // конечный путь?
        m_target_set.node() != m_object->ai_location().level_vertex_id() && m_target_actual)
    {
        m_failed = true;
    }

    m_time_path_updated_external = time();
}

void CControlPathBuilderBase::on_path_end() { m_path_end = true; }
void CControlPathBuilderBase::travel_point_changed()
{
    if (m_man->path_builder().detail().curr_travel_point_index() >= m_man->path_builder().detail().path().size() - 1)
    {
        on_path_end();
    }
}

bool CControlPathBuilderBase::global_failed()
{
    return (m_time_global_failed_started + pmt_global_failed_duration > time());
}
