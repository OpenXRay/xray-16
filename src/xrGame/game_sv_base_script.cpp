////////////////////////////////////////////////////////////////////////////
//	Module 		: game_sv_base_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Base server game script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "game_sv_base.h"
#include "xrMessages.h"

using namespace luabind;
using namespace luabind::policy;

template<typename T>
class enum_exporter {};

SCRIPT_EXPORT(game_sv_GameState, (game_GameState),
{
    module(luaState, "game")
    [
        class_<game_sv_GameState, game_GameState>("game_sv_GameState")
            .def("get_eid", &game_sv_GameState::get_eid)
            .def("get_id", &game_sv_GameState::get_id)
            //.def("get_it",				&game_sv_GameState::get_it)
            //.def("get_it_2_id",			&game_sv_GameState::get_it_2_id)
            //.def("get_name_it",			&game_sv_GameState::get_name_it)
            .def("get_name_id", &game_sv_GameState::get_name_id)
            .def("get_player_name_id", &game_sv_GameState::get_player_name_id)
        
            .def("get_players_count", &game_sv_GameState::get_players_count)
            .def("get_id_2_eid", &game_sv_GameState::get_id_2_eid)
        
            .def("get_option_i", &game_sv_GameState::get_option_i)
            .def("get_option_s", &game_sv_GameState::get_option_s)
            .def("u_EventSend", &game_sv_GameState::u_EventSend)
        
            .def("GenerateGameMessage", &game_sv_GameState::GenerateGameMessage)
            .def("getRP", &game_sv_GameState::getRP)
            .def("getRPcount", &game_sv_GameState::getRPcount)
    ];
});

SCRIPT_EXPORT(EGameEnums, (),
{
    module (luaState)
    [
        class_<enum_exporter<EGamePlayerFlags>>("game_player_flags")
        .enum_("flags")
        [
            value("GAME_PLAYER_FLAG_LOCAL", int(GAME_PLAYER_FLAG_LOCAL)),
            value("GAME_PLAYER_FLAG_READY", int(GAME_PLAYER_FLAG_READY)),
            value("GAME_PLAYER_FLAG_VERY_VERY_DEAD", int(GAME_PLAYER_FLAG_VERY_VERY_DEAD)),
            value("GAME_PLAYER_FLAG_SPECTATOR", int(GAME_PLAYER_FLAG_SPECTATOR)),
            value("GAME_PLAYER_FLAG_SCRIPT_BEGINS_FROM", int(GAME_PLAYER_FLAG_SCRIPT_BEGINS_FROM))
        ],

        class_<enum_exporter<EGamePhases>>("game_phases")
        .enum_("phases")
        [
            value("GAME_PHASE_NONE", int(GAME_PHASE_NONE)),
            value("GAME_PHASE_INPROGRESS", int(GAME_PHASE_INPROGRESS)),
            value("GAME_PHASE_PENDING", int(GAME_PHASE_PENDING)),
            value("GAME_PHASE_TEAM1_SCORES", int(GAME_PHASE_TEAM1_SCORES)),
            value("GAME_PHASE_TEAM2_SCORES", int(GAME_PHASE_TEAM2_SCORES)),
            value("GAME_PHASE_TEAMS_IN_A_DRAW", int(GAME_PHASE_TEAMS_IN_A_DRAW)),
            value("GAME_PHASE_SCRIPT_BEGINS_FROM", int(GAME_PHASE_SCRIPT_BEGINS_FROM))
        ],

        class_<enum_exporter<EGameMessages>>("game_messages")
        .enum_("messages")
        [
            value("GAME_EVENT_PLAYER_READY", int(GAME_EVENT_PLAYER_READY)),
            value("GAME_EVENT_PLAYER_CHANGE_TEAM", int(GAME_EVENT_PLAYER_GAME_MENU)),
            value("GAME_EVENT_PLAYER_KILL", int(GAME_EVENT_PLAYER_KILL)),
            value("GAME_EVENT_PLAYER_BUY_FINISHED", int(GAME_EVENT_PLAYER_BUY_FINISHED)),
            value("GAME_EVENT_PLAYER_CHANGE_SKIN", int(GAME_EVENT_PLAYER_GAME_MENU)),
            value("GAME_EVENT_PLAYER_CONNECTED", int(GAME_EVENT_PLAYER_CONNECTED)),
            value("GAME_EVENT_PLAYER_DISCONNECTED", int(GAME_EVENT_PLAYER_DISCONNECTED)),
            value("GAME_EVENT_PLAYER_KILLED", int(GAME_EVENT_PLAYER_KILLED)),
            value("GAME_EVENT_PLAYER_JOIN_TEAM", int(GAME_EVENT_PLAYER_JOIN_TEAM)),
            value("GAME_EVENT_ROUND_STARTED", int(GAME_EVENT_ROUND_STARTED)),
            value("GAME_EVENT_ROUND_END", int(GAME_EVENT_ROUND_END)),
            value("GAME_EVENT_ARTEFACT_SPAWNED", int(GAME_EVENT_ARTEFACT_SPAWNED)),
            value("GAME_EVENT_ARTEFACT_DESTROYED", int(GAME_EVENT_ARTEFACT_DESTROYED)),
            value("GAME_EVENT_ARTEFACT_TAKEN", int(GAME_EVENT_ARTEFACT_TAKEN)),
            value("GAME_EVENT_ARTEFACT_DROPPED", int(GAME_EVENT_ARTEFACT_DROPPED)),
            value("GAME_EVENT_ARTEFACT_ONBASE", int(GAME_EVENT_ARTEFACT_ONBASE)),
            value("GAME_EVENT_PLAYER_ENTER_TEAM_BASE", int(GAME_EVENT_PLAYER_ENTER_TEAM_BASE)),
            value("GAME_EVENT_PLAYER_LEAVE_TEAM_BASE", int(GAME_EVENT_PLAYER_LEAVE_TEAM_BASE)),
            value("GAME_EVENT_BUY_MENU_CLOSED", int(GAME_EVENT_BUY_MENU_CLOSED)),
            value("GAME_EVENT_TEAM_MENU_CLOSED", int(GAME_EVENT_TEAM_MENU_CLOSED)),
            value("GAME_EVENT_SKIN_MENU_CLOSED", int(GAME_EVENT_SKIN_MENU_CLOSED)),
            value("GAME_EVENT_SCRIPT_BEGINS_FROM", int(GAME_EVENT_SCRIPT_BEGINS_FROM))
        ]
    ];
});
