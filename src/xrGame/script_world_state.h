////////////////////////////////////////////////////////////////////////////
//	Module 		: script_world_state.h
//	Created 	: 19.03.2004
//  Modified 	: 19.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script world state
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "graph_engine_space.h"
#include "script_export_space.h"

typedef GraphEngineSpace::CWorldState CScriptWorldState;

class CScriptWorldStateWrapper {
public:
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptWorldStateWrapper)
#undef script_type_list
#define script_type_list save_type_list(CScriptWorldStateWrapper)
