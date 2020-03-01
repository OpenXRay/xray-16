////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_base_action.cpp
//	Created 	: 25.03.2004
//  Modified 	: 27.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker base action
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_base_action.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "stalker_animation_manager.h"
#include "stalker_planner.h"
#include "ai/stalker/ai_stalker_space.h"

using namespace StalkerSpace;

CStalkerActionBase::CStalkerActionBase(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name) {}
void CStalkerActionBase::initialize()
{
    inherited::initialize();
    object().animation().clear_script_animations();
    object().brain().affect_cover(false);
}

void CStalkerActionBase::execute() { inherited::execute(); }
void CStalkerActionBase::finalize()
{
    inherited::finalize();
    object().animation().clear_script_animations();
}
