////////////////////////////////////////////////////////////////////////////
//	Module 		: saved_game_wrapper.h
//	Created 	: 21.02.2006
//  Modified 	: 21.02.2006
//	Author		: Dmitriy Iassenev
//	Description : saved game wrapper class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "xrAICore/Navigation/game_graph_space.h"

class CSavedGameWrapper
{
public:
    typedef ALife::_TIME_ID _TIME_ID;
    typedef GameGraph::_LEVEL_ID _LEVEL_ID;

private:
    _TIME_ID m_game_time;
    _LEVEL_ID m_level_id;
    shared_str m_level_name;
    float m_actor_health;

public:
    CSavedGameWrapper(LPCSTR saved_game_name);
    static pcstr saved_game_full_name(pcstr saved_game_name, string_path& result, pcstr extension);
    static bool saved_game_exist(LPCSTR saved_game_name);
    static bool valid_saved_game(IReader& stream);
    static bool valid_saved_game(LPCSTR saved_game_name);
    inline const _TIME_ID& game_time() const;
    inline const _LEVEL_ID& level_id() const;
    inline LPCSTR level_name() const;
    inline const float& actor_health() const;
};

#include "saved_game_wrapper_inline.h"
