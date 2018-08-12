////////////////////////////////////////////////////////////////////////////
//	Module 		: movement_manager_game.cpp
//	Created 	: 03.12.2003
//  Modified 	: 03.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Movement manager for game paths
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "movement_manager.h"
#include "alife_simulator.h"
#include "alife_graph_registry.h"
#include "alife_level_registry.h"
#include "xrEngine/profiler.h"
#include "game_location_selector.h"
#include "game_path_manager.h"
#include "level_location_selector.h"
#include "level_path_manager.h"
#include "detail_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "CustomMonster.h"
#include "level_path_builder.h"
#include "detail_path_builder.h"
#include "mt_config.h"

void CMovementManager::show_game_path_info()
{
    Msg("! Cannot build GAME path! (object %s)", *object().cName());
    Msg("! CURRENT LEVEL : %s", *Level().name());
    Fvector temp = ai().game_graph().vertex(object().ai_location().game_vertex_id())->level_point();
    Msg("! CURRENT game point position : [%f][%f][%f]", VPUSH(temp));
    const GameGraph::CVertex* vertex = ai().game_graph().vertex(game_dest_vertex_id());
    Msg("! TARGET LEVEL : %s", *ai().game_graph().header().level(vertex->level_id()).name());
    temp = vertex->level_point();
    Msg("! TARGET  game point position : [%f][%f][%f]", VPUSH(temp));
    const u8* target_vertex_type = ai().game_graph().vertex(game_dest_vertex_id())->vertex_type();
    Msg("! Target point mask [%d][%d][%d][%d]", target_vertex_type[0], target_vertex_type[1], target_vertex_type[2],
        target_vertex_type[3]);

    Msg("! Object masks (%d) :", m_location_manager->vertex_types().size());

    typedef GameGraph::TERRAIN_VECTOR::const_iterator const_iterator;
    const_iterator I = m_location_manager->vertex_types().begin();
    const_iterator E = m_location_manager->vertex_types().end();
    for (; I != E; ++I)
        Msg("!   [%d][%d][%d][%d]", (*I).tMask[0], (*I).tMask[1], (*I).tMask[2], (*I).tMask[3]);
}

void CMovementManager::process_game_path()
{
    START_PROFILE("Build Path/Process Game Path");

    if (m_path_state != ePathStateTeleport)
    {
        if (!level_path().actual() && (m_path_state > ePathStateBuildLevelPath))
            m_path_state = ePathStateBuildLevelPath;

        if (!game_path().actual() && (m_path_state > ePathStateBuildGamePath))
            m_path_state = ePathStateBuildGamePath;
    }

    switch (m_path_state)
    {
    case ePathStateSelectGameVertex:
    {
        u32 current = game_path().m_dest_vertex_id;
        game_selector().select_location(object().ai_location().game_vertex_id(), game_path().m_dest_vertex_id);
        if ((current == game_path().m_dest_vertex_id) && game_selector().used())
        {
            m_path_state = ePathStatePathCompleted;
            break;
        }

        if (game_selector().failed())
            break;

        m_path_state = ePathStateBuildGamePath;
    }
    case ePathStateBuildGamePath:
    {
        game_path().build_path(object().ai_location().game_vertex_id(), game_dest_vertex_id());

        if (game_path().failed())
        {
            show_game_path_info();
            break;
        }

        m_path_state = ePathStateContinueGamePath;
    }
    case ePathStateContinueGamePath:
    {
        game_path().select_intermediate_vertex();
        if (ai().game_graph().vertex(object().ai_location().game_vertex_id())->level_id() !=
            ai().game_graph().vertex(game_path().intermediate_vertex_id())->level_id())
        {
            m_path_state = ePathStateTeleport;
            VERIFY(ai().get_alife());
            VERIFY(ai().alife().graph().level().level_id() ==
                ai().game_graph().vertex(object().ai_location().game_vertex_id())->level_id());
            teleport(game_path().intermediate_vertex_id());
            break;
        }

        m_path_state = ePathStateBuildLevelPath;
    }
    case ePathStateBuildLevelPath:
    {
        VERIFY(ai().game_graph().vertex(object().ai_location().game_vertex_id())->level_id() ==
            ai().game_graph().vertex(game_path().intermediate_vertex_id())->level_id());

        u32 dest_level_vertex_id = ai().game_graph().vertex(game_path().intermediate_vertex_id())->level_vertex_id();

        if (!accessible(dest_level_vertex_id))
        {
            Fvector dest_pos;
            dest_level_vertex_id =
                restrictions().accessible_nearest(ai().level_graph().vertex_position(dest_level_vertex_id), dest_pos);
        }

        Fvector temp =
            ai().level_graph().vertex_position(dest_level_vertex_id /**level_path().intermediate_vertex_id()/**/);
        level_path_builder().setup(object().ai_location().level_vertex_id(), dest_level_vertex_id, true, &temp);

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
        VERIFY(!level_path().failed());

        level_path().select_intermediate_vertex();

        m_path_state = ePathStateBuildDetailPath;
    }
    case ePathStateBuildDetailPath:
    {
        detail().set_state_patrol_path(true);
        detail().set_start_position(object().Position());
        detail().set_start_direction(Fvector().setHP(-m_body.current.yaw, 0));
        detail().set_dest_position(ai().level_graph().vertex_position(level_path().intermediate_vertex_id()));

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
        if (!game_selector().actual(object().ai_location().game_vertex_id(), path_completed()))
            m_path_state = ePathStateSelectGameVertex;
        else if (!game_path().actual())
            m_path_state = ePathStateBuildGamePath;
        else if (!level_path().actual())
            m_path_state = ePathStateBuildLevelPath;
        else if (!detail().actual())
            m_path_state = ePathStateBuildLevelPath;
        else if (detail().completed(object().Position(), !detail().state_patrol_path()))
        {
            m_path_state = ePathStateContinueLevelPath;
            if (level_path().completed())
            {
                m_path_state = ePathStateContinueGamePath;
                if (game_path().completed())
                    m_path_state = ePathStatePathCompleted;
            }
        }
        break;
    }
    case ePathStatePathCompleted:
    {
        if (!game_selector().actual(object().ai_location().game_vertex_id(), path_completed()))
            m_path_state = ePathStateSelectGameVertex;
        break;
    }
    case ePathStateTeleport: { break;
    }
    default: NODEFAULT;
    }

    STOP_PROFILE
}
