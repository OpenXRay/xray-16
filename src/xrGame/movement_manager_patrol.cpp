////////////////////////////////////////////////////////////////////////////
//	Module 		: movement_manager_patrol.cpp
//	Created 	: 03.12.2003
//  Modified 	: 03.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Movement manager for patrol paths
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "movement_manager.h"
#include "patrol_path_manager.h"
#include "level_path_manager.h"
#include "detail_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "CustomMonster.h"
#include "level_path_builder.h"
#include "detail_path_builder.h"
#include "mt_config.h"

void CMovementManager::process_patrol_path()
{
    if (!level_path().actual() && (m_path_state > ePathStateBuildLevelPath))
        m_path_state = ePathStateBuildLevelPath;

    if (!patrol().actual() && (m_path_state > ePathStateSelectPatrolPoint))
    {
        //		Msg					("[%6d][%s] actuality is false 2",Device.dwFrame,*object().cName());
        m_path_state = ePathStateSelectPatrolPoint;
    }

    switch (m_path_state)
    {
    case ePathStateSelectPatrolPoint:
    {
        patrol().select_point(object().Position(), level_path().m_dest_vertex_id);
        if (patrol().failed())
            break;

        if (patrol().completed())
        {
            m_path_state = ePathStatePathCompleted;
            break;
        }

        m_path_state = ePathStateBuildLevelPath;
    }
    case ePathStateBuildLevelPath:
    {
        level_path_builder().setup(object().ai_location().level_vertex_id(), level_dest_vertex_id(),
            patrol().extrapolate_path(), &patrol().destination_position());

        if (can_use_distributed_computations(mtLevelPath))
        {
            level_path_builder().register_to_process();
            break;
        }

        build_level_path();

        break;
    }
    case ePathStateContinueLevelPath:
    {
        level_path().select_intermediate_vertex();

        m_path_state = ePathStateBuildDetailPath;
    }
    case ePathStateBuildDetailPath:
    {
        detail().set_state_patrol_path(patrol().extrapolate_path());
        detail().set_start_position(object().Position());
        detail().set_start_direction(Fvector().setHP(-m_body.current.yaw, 0));
        detail().set_dest_position(patrol().destination_position());

        detail_path_builder().setup(level_path().path(), level_path().intermediate_index());

        if (can_use_distributed_computations(mtDetailPath))
        {
            detail_path_builder().register_to_process();
            break;
        }

        detail_path_builder().process();

        break;
    }
    case ePathStatePathVerification:
    {
        if (!patrol().actual())
        {
            //				Msg				("[%6d][%s] actuality is false 3",Device.dwFrame,*object().cName());
            m_path_state = ePathStateSelectPatrolPoint;
        }
        else if (!level_path().actual())
            m_path_state = ePathStateBuildLevelPath;
        else if (!detail().actual())
            m_path_state = ePathStateBuildLevelPath;
        else if (detail().completed(object().Position(), !detail().state_patrol_path()))
        {
            //							Msg				("[%6d][%s] detail path is
            // completed",Device.dwFrame,*object().cName());
            //							Msg				(
            //								"[path_size=%d][curr_id=%d][last_pp=%d] start_position=[%f][%f][%f]
            // stop_position=[%f][%f][%f] position=[%f][%f][%f]",
            //								detail().path().size(),
            //								detail().curr_travel_point_index(),
            //								detail().last_patrol_point(),
            //								VPUSH(detail().path().front().position),
            //								VPUSH(detail().curr_travel_point().position),
            //								VPUSH(object().Position())
            //							);
            m_path_state = ePathStateContinueLevelPath;
            if (level_path().completed())
            {
                m_path_state = ePathStateSelectPatrolPoint;
                if (patrol().completed())
                    m_path_state = ePathStatePathCompleted;
            }
        }
        break;
    }
    case ePathStatePathCompleted:
    {
        if (!patrol().actual())
        {
            //				Msg				("[%6d][%s] actuality is false 4",Device.dwFrame,*object().cName());
            m_path_state = ePathStateSelectPatrolPoint;
        }
        break;
    }
    default: NODEFAULT;
    }
}
